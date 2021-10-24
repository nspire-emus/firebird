#include <cassert>
#include <chrono>
#include <cstdint>
#include <cctype>

#include <fcntl.h>
#include <unistd.h>

#include <zlib.h>

#include "emu.h"
#include "translate.h"
#include "debug.h"
#include "mmu.h"
#include "gdbstub.h"
#include "usblink_queue.h"
#include "os/os.h"
#include "schedule.h"
#include "misc.h"
#include "mem.h"

/* cycle_count_delta is a (usually negative) number telling what the time is relative
 * to the next scheduled event. See sched.c */
int cycle_count_delta = 0;

uint32_t cpu_events;

bool do_translate = true;
uint32_t product = 0x0E0, features = 0, asic_user_flags = 0;
bool turbo_mode = false;

bool exiting, debug_on_start, debug_on_warn, print_on_warn;
BootOrder boot_order = ORDER_DEFAULT;
std::string path_boot1, path_flash;

#if defined(IS_IOS_BUILD) || defined(__EMSCRIPTEN__)
// on iOS, setjmp and longjmp are broken and builtins setfault clang
typedef bool emu_jmp_buf;
static inline void emu_setjmp(emu_jmp_buf) {}
__attribute__((noreturn)) static inline int emu_longjmp(emu_jmp_buf) { assert(false); }
#elif defined(_WIN32) || defined(WIN32)
// with MinGW, setjmp and longjmp are broken, but the builtins work
typedef void* emu_jmp_buf[32];
// When longjmp happens, the call frame used for setjmp has to be valid still,
// i.e. the caller of setjmp must not have returned. So use a define.
// The loop is needed as clang/llvm require saving again after the jump backwards.
#define emu_setjmp(jb) do {} while(__builtin_setjmp(jb))
__attribute__((noreturn)) static inline int emu_longjmp(emu_jmp_buf jb) { __builtin_longjmp(jb, 1); }
#else
#include <setjmp.h>
typedef jmp_buf emu_jmp_buf;
#define emu_setjmp(jb) do {setjmp(jb);} while(0)
__attribute__((noreturn)) static inline int emu_longjmp(emu_jmp_buf jb) { longjmp(jb, 1); };
#endif

static emu_jmp_buf restart_after_exception;

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
    return_to_loop();
}

void return_to_loop()
{
    emu_longjmp(restart_after_exception);
}

extern "C" void usblink_timer();

// Store the last time the throttle event happened
static auto last_throttle = std::chrono::steady_clock::now();
// Calculate speed by summing up the elapsed virtual and real time and taking the ratio
static std::chrono::microseconds real_time_elapsed_sum, virt_time_elapsed_sum;

void throttle_interval_event(int index)
{
    /* Throttle interval (defined arbitrarily as 100Hz) - used for
     * keeping the emulator speed down, and other miscellaneous stuff
     * that needs to be done periodically */
    event_repeat(index, 27000000 / 100);
    // 100Hz -> called every virtual 10ms
    const auto virt_throttle_interval = std::chrono::milliseconds(10);

    usblink_timer();

    usblink_queue_do();

    int c = gui_getchar();
    if(c != -1)
        serial_byte_in((char) c);

    gdbstub_recv();

    rdebug_recv();

    // Compute how much time elapsed since last_throttle
    auto real_interval = std::chrono::steady_clock::now() - last_throttle;
    auto real_time_diff = virt_throttle_interval - real_interval;
    auto real_time_left_us = std::chrono::duration_cast<std::chrono::microseconds>(real_time_diff).count();
    // If less than the virtual throttle interval elapsed, wait
    if(real_time_left_us > 0 && !turbo_mode)
        throttle_timer_wait(real_time_left_us);

    // Use this as the new value for last_throttle
    auto new_last_throttle = std::chrono::steady_clock::now();

    // Add the elapsed times to (real/virt)_time_elapsed_sum
    auto real_time_elapsed = (new_last_throttle - last_throttle);
    auto real_time_elapsed_us = std::chrono::duration_cast<std::chrono::microseconds>(real_time_elapsed);
    real_time_elapsed_sum += real_time_elapsed_us;
    virt_time_elapsed_sum += virt_throttle_interval;
    // When half a (real) second passes, use the sums for speed computation and clear them
    if(real_time_elapsed_sum >= std::chrono::milliseconds(500))
    {
        gui_show_speed(double(virt_time_elapsed_sum.count()) / real_time_elapsed_sum.count());
        virt_time_elapsed_sum = real_time_elapsed_sum = {};
    }

    last_throttle = new_last_throttle;

    gui_do_stuff(true);
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

bool snapshot_read(const emu_snapshot *snapshot, void *dest, int size)
{
    return gzread((gzFile)snapshot->stream_handle, dest, size) == size;
}

bool snapshot_write(emu_snapshot *snapshot, const void *src, int size)
{
    return gzwrite((gzFile)snapshot->stream_handle, src, size) == size;
}

bool emu_start(unsigned int port_gdb, unsigned int port_rdbg, const char *snapshot_file)
{
    gui_busy_raii gui_busy;

    if(snapshot_file)
    {
        // Open snapshot
        FILE *fp = fopen_utf8(snapshot_file, "rb");
        if(!fp)
            return false;

        int dupfd = dup(fileno(fp));
        fclose(fp);
        fp = nullptr;

        // gzdopen takes ownership of the fd
        gzFile gzf = gzdopen(dupfd, "r");
        if(!gzf)
        {
            close(dupfd);
            return false;
        }

        emu_snapshot snapshot = {};
        snapshot.stream_handle = gzf;
        // Read the header
        if(!snapshot_read(&snapshot, &snapshot.header, sizeof(snapshot.header)))
        {
            gzclose(gzf);
            return false;
        }

        sched.items[SCHED_THROTTLE].clock = CLOCK_27M;
        sched.items[SCHED_THROTTLE].proc = throttle_interval_event;

        // TODO: Max length
        path_boot1 = std::string(snapshot.header.path_boot1);
        path_flash = std::string(snapshot.header.path_flash);

        // Resume components
        uint32_t sdram_size, dummy;
        if(snapshot.header.sig != SNAPSHOT_SIG
                || snapshot.header.version != SNAPSHOT_VER
                || !flash_resume(&snapshot)
                || !flash_read_settings(&sdram_size, &product, &features, &asic_user_flags)
                || !cpu_resume(&snapshot)
                || !memory_resume(&snapshot)
                || !sched_resume(&snapshot)
                // Verify that EOF is next
                || gzread(gzf, &dummy, sizeof(dummy)) != 0
                || !gzeof(gzf))
        {
            gzclose(gzf);
            emu_cleanup();
            return false;
        }

        if(gzclose(gzf) != Z_OK)
        {
            emu_cleanup();
            return false;
        }
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
    #if OS_HAS_PAGEFAULT_HANDLER
        os_exception_frame_t seh_frame = { NULL, NULL };
        os_faulthandler_arm(&seh_frame);
    #endif

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

    emu_setjmp(restart_after_exception);

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

    #if OS_HAS_PAGEFAULT_HANDLER
        os_faulthandler_unarm(&seh_frame);
    #endif
}

bool emu_suspend(const char *file)
{
    gui_busy_raii gui_busy;

    FILE *fp = fopen_utf8(file, "wb");
    if(!fp)
        return false;

    int dupfd = dup(fileno(fp));
    fclose(fp);
    fp = nullptr;

    // gzdopen takes ownership of the fd
    gzFile gzf = gzdopen(dupfd, "wb");
    if(!gzf)
    {
        close(dupfd);
        return false;
    }

    emu_snapshot snapshot = {};
    snapshot.stream_handle = gzf;

    snapshot.header.sig = SNAPSHOT_SIG;
    snapshot.header.version = SNAPSHOT_VER;
    // TODO: Max length
    strncpy(snapshot.header.path_boot1, path_boot1.c_str(), sizeof(snapshot.header.path_boot1) - 1);
    strncpy(snapshot.header.path_flash, path_flash.c_str(), sizeof(snapshot.header.path_flash) - 1);

    if(!snapshot_write(&snapshot, &snapshot.header, sizeof(snapshot.header))
            || !flash_suspend(&snapshot)
            || !cpu_suspend(&snapshot)
            || !memory_suspend(&snapshot)
            || !sched_suspend(&snapshot))
    {
        gzclose(gzf);
        return false;
    }

    return gzclose(gzf) == Z_OK;
}

void emu_cleanup()
{
    exiting = true;

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
