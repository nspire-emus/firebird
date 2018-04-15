#include <cassert>
#include <chrono>
#include <cstdint>
#include <cctype>
#include <csetjmp>

#include <fcntl.h>

#include <zlib.h>

#include "emu.h"
#include "translate.h"
#include "debug.h"
#include "mmu.h"
#include "gdbstub.h"
#include "usblink_queue.h"
#include "os/os.h"

/* cycle_count_delta is a (usually negative) number telling what the time is relative
 * to the next scheduled event. See sched.c */
int cycle_count_delta = 0;

int throttle_delay = 10; /* in milliseconds */

uint32_t cpu_events;

bool do_translate = true;
uint32_t product = 0x0E0, features = 0, asic_user_flags = 0;
bool turbo_mode = false;

bool exiting, debug_on_start, debug_on_warn, print_on_warn;
BootOrder boot_order = ORDER_DEFAULT;
uint32_t boot2_base;
std::string path_boot1, path_flash;

void *restart_after_exception[32];

int log_enabled[MAX_LOG];
FILE *log_file[MAX_LOG];
void logprintf(int type, const char *str, ...) {
    if (log_enabled[type]) {
        va_list va;
        va_start(va, str);
        vfprintf(log_file[type], str, va);
        va_end(va);
    }
}

void emuprintf(const char *format, ...) {
    va_list va;
    va_start(va, format);
    gui_debug_vprintf(format, va);
    va_end(va);
}

void warn(const char *fmt, ...) {
    if (!print_on_warn && !debug_on_warn)
        return;

    va_list va;
    va_start(va, fmt);
    gui_debug_printf("Warning (%08x): ", arm.reg[15]);
    gui_debug_vprintf(fmt, va);
    gui_debug_printf("\n");
    va_end(va);
    if (debug_on_warn)
        debugger(DBG_EXCEPTION, 0);
}

__attribute__((noreturn))
void error(const char *fmt, ...) {
    va_list va;
    va_start(va, fmt);
    gui_debug_printf("Error (%08x): ", arm.reg[15]);
    gui_debug_vprintf(fmt, va);
    gui_debug_printf("\n");
    va_end(va);
    debugger(DBG_EXCEPTION, 0);
    cpu_events |= EVENT_RESET;
    #ifndef NO_SETJMP
        __builtin_longjmp(restart_after_exception, 1);
    #else
        assert(false);
    #endif
}

int exec_hack() {
    if (arm.reg[15] == 0x10040) {
        arm.reg[15] = arm.reg[14];
        warn("BOOT1 is required to run this version of BOOT2.");
        return 1;
    }
    return 0;
}

extern "C" void usblink_timer();

void throttle_interval_event(int index)
{
    event_repeat(index, 27000000 / 100);

    /* Throttle interval (defined arbitrarily as 100Hz) - used for
     * keeping the emulator speed down, and other miscellaneous stuff
     * that needs to be done periodically */
    static int intervals = 0, prev_intervals = 0;
    intervals += 1;

    usblink_timer();

    usblink_queue_do();

    int c = gui_getchar();
    if(c != -1)
        serial_byte_in((char) c);

    gdbstub_recv();

    rdebug_recv();

    // Calculate speed
    auto interval_end = std::chrono::high_resolution_clock::now();
    static auto prev = interval_end;
    static double speed = 1.0;
    auto time = std::chrono::duration_cast<std::chrono::microseconds>(interval_end - prev).count();
    if (time >= 500000) {
        speed = (double)10000 * (intervals - prev_intervals) / time;
        gui_show_speed(speed);
        prev_intervals = intervals;
        prev = interval_end;
    }

    gui_do_stuff(true);

    if (!turbo_mode)
		throttle_timer_wait();
}

size_t gzip_filesize(const char *path)
{
    #if __BYTE_ORDER == __LITTLE_ENDIAN
        int fp = open(path, O_RDONLY);
        if(fp == -1)
            return false;

        // The last four bytes of a gzip file are the uncompressed size (% 2^32)
        lseek(fp, -4, SEEK_END);
        size_t ret = 0;
        if(read(fp, &ret, 4) != 4)
            ret = 0;

        close(fp);
        return ret;
    #else
        #error "Not implemented"
    #endif
}

struct gui_busy_raii {
    gui_busy_raii() { gui_set_busy(true); }
    ~gui_busy_raii() { gui_set_busy(false); }
};

static void emu_reset()
{
    memset(mem_areas[1].ptr, 0, mem_areas[1].size);

    memset(&arm, 0, sizeof arm);
    arm.control = 0x00050078;
    arm.cpsr_low28 = MODE_SVC | 0xC0;
    cpu_events &= EVENT_DEBUG_STEP;

    sched_reset();
    sched.items[SCHED_THROTTLE].clock = CLOCK_27M;
    sched.items[SCHED_THROTTLE].proc = throttle_interval_event;

    memory_reset();
}

bool emu_start(unsigned int port_gdb, unsigned int port_rdbg, const char *snapshot_file)
{
    gui_busy_raii gui_busy;

    if(snapshot_file)
    {
        // Open snapshot
        size_t snapshot_size = gzip_filesize(snapshot_file);
        if(snapshot_size < sizeof(emu_snapshot))
            return false;

        gzFile gzf = gzopen(snapshot_file, "r");
        if(!gzf)
            return false;

        auto snapshot = (struct emu_snapshot *) malloc(snapshot_size);
        if(!snapshot)
        {
            gzclose(gzf);
            return false;
        }

        if((size_t) gzread(gzf, snapshot, snapshot_size) != snapshot_size)
        {
            gzclose(gzf);
            free(snapshot);
            return false;
        }
        gzclose(gzf);

        //sched_reset();
        sched.items[SCHED_THROTTLE].clock = CLOCK_27M;
        sched.items[SCHED_THROTTLE].proc = throttle_interval_event;

        // TODO: Max length
        path_boot1 = std::string(snapshot->path_boot1);
        path_flash = std::string(snapshot->path_flash);

        // TODO: Pass snapshot_size to flash_resume to avoid reading after the buffer

        // Resume components
        uint32_t sdram_size;
        if(snapshot->sig != SNAPSHOT_SIG
                || snapshot->version != SNAPSHOT_VER
                || !flash_resume(snapshot)
                || !flash_read_settings(&sdram_size, &product, &features, &asic_user_flags)
                || !cpu_resume(snapshot)
                || !memory_resume(snapshot)
                || !sched_resume(snapshot))
        {
            emu_cleanup();
            free(snapshot);
            return false;
        }
        free(snapshot);
    }
    else
    {
        if (!flash_open(path_flash.c_str()))
                return false;

        uint32_t sdram_size;
        flash_read_settings(&sdram_size, &product, &features, &asic_user_flags);

        flash_set_bootorder(boot_order);

        if(!memory_initialize(sdram_size))
        {
            emu_cleanup();
            return false;
        }
    }

    if(debug_on_start)
        cpu_events |= EVENT_DEBUG_STEP;

    uint8_t *rom = mem_areas[0].ptr;
    memset(rom, -1, 0x80000);
    for (int i = 0x00000; i < 0x80000; i += 4)
        RAM_FLAGS(&rom[i]) = RF_READ_ONLY;

    /* Load the ROM */
    FILE *f = fopen_utf8(path_boot1.c_str(), "rb");
    if (!f) {
        gui_perror(path_boot1.c_str());
        emu_cleanup();
        return false;
    }
    (void)fread(rom, 1, 0x80000, f);
    fclose(f);

#ifndef NO_TRANSLATION
    if(!translate_init())
    {
        gui_debug_printf("Could not init JIT, disabling translation.\n");
        do_translate = false;
    }
#endif

    addr_cache_init();

    throttle_timer_on();

    if(port_gdb)
        gdbstub_init(port_gdb);

    if(port_rdbg)
        rdebug_bind(port_rdbg);

    usblink_queue_reset();

    if(!snapshot_file)
        emu_reset();

    return true;
}

void emu_loop(bool reset)
{
    if(reset)
    {
        reset:
        emu_reset();
    }

    gdbstub_reset();

    addr_cache_flush();
    flush_translations();

    sched_update_next_event(0);

    exiting = false;

// clang segfaults with that, for an iOS build :(
#ifndef NO_SETJMP
    // Workaround for LLVM bug #18974
    while(__builtin_setjmp(restart_after_exception)){};
#endif

    while (!exiting) {
        sched_process_pending_events();
        while (!exiting && cycle_count_delta < 0) {
            if (cpu_events & EVENT_RESET) {
                gui_status_printf("Reset");
                goto reset;
            }

            if (cpu_events & (EVENT_FIQ | EVENT_IRQ)) {
                // Align PC in case the interrupt occurred immediately after a jump
                if (arm.cpsr_low28 & 0x20)
                    arm.reg[15] &= ~1;
                else
                    arm.reg[15] &= ~3;

                if (cpu_events & EVENT_WAITING)
                    arm.reg[15] += 4; // Skip over wait instruction

                arm.reg[15] += 4;
                cpu_exception((cpu_events & EVENT_FIQ) ? EX_FIQ : EX_IRQ);
            }
            cpu_events &= ~EVENT_WAITING;

            if (arm.cpsr_low28 & 0x20)
                cpu_thumb_loop();
            else
                cpu_arm_loop();
        }
    }
}

bool emu_suspend(const char *file)
{
    gui_busy_raii gui_busy;

    gzFile gzf = gzopen(file, "wb");

    size_t size = sizeof(emu_snapshot) + flash_suspend_flexsize();
    auto snapshot = (struct emu_snapshot *) malloc(size);
    if(!snapshot)
    {
        gzclose(gzf);
        return false;
    }

    snapshot->product = product;
    snapshot->asic_user_flags = asic_user_flags;
    // TODO: Max length
    strncpy(snapshot->path_boot1, path_boot1.c_str(), sizeof(snapshot->path_boot1) - 1);
    strncpy(snapshot->path_flash, path_flash.c_str(), sizeof(snapshot->path_flash) - 1);

    if(!flash_suspend(snapshot)
            || !cpu_suspend(snapshot)
            || !sched_suspend(snapshot)
            || !memory_suspend(snapshot))
    {
        free(snapshot);
        gzclose(gzf);
        return false;
    }

    snapshot->sig = SNAPSHOT_SIG;
    snapshot->version = SNAPSHOT_VER;

    bool success = (size_t) gzwrite(gzf, snapshot, size) == size;

    free(snapshot);
    gzclose(gzf);
    return success;
}

void emu_cleanup()
{
    exiting = true;

    if(debugger_input)
        fclose(debugger_input);

    // addr_cache_init is rather expensive and needs to be called once only
    //addr_cache_deinit();

    #ifndef NO_TRANSLATION
        translate_deinit();
    #endif

    memory_reset();
    memory_deinitialize();
    flash_close();

    gdbstub_quit();
    rdebug_quit();
}
