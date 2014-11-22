//#include <conio.h>
#include <ctype.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "emu.h"
#include "cpu.h"
#include "schedule.h"
#include "mem.h"
#include "keypad.h"
#include "translate.h"
#include "debug.h"
#include "mmu.h"
#include "gdbstub.h"
#include "flash.h"
#include "misc.h"
#include "os/os.h"

#include <stdint.h>

/* cycle_count_delta is a (usually negative) number telling what the time is relative
 * to the next scheduled event. See sched.c */
int cycle_count_delta = 0;

int throttle_delay = 10; /* in milliseconds */

uint32_t cpu_events;

bool do_translate = true;
int asic_user_flags;
bool turbo_mode;
bool is_halting;
bool show_speed;

bool exiting, debug_on_start, debug_on_warn, large_nand, large_sdram;
int product = 0x0E0;
uint32_t boot2_base;
const char *path_boot1 = NULL, *path_boot2 = NULL, *path_flash = NULL, *pre_boot2 = NULL, *pre_diags = NULL, *pre_os = NULL;

int gdb_port = 0;
int rdbg_port = 0;

void *restart_after_exception[32];

void (*reset_procs[20])(void);
int reset_proc_count;

const char log_type_tbl[] = LOG_TYPE_TBL;
int log_enabled[MAX_LOG];
FILE *log_file[MAX_LOG];
void logprintf(int type, char *str, ...) {
    if (log_enabled[type]) {
        va_list va;
        va_start(va, str);
        vfprintf(log_file[type], str, va);
        va_end(va);
    }
}

void emuprintf(char *format, ...) {
    va_list va;
    va_start(va, format);
    gui_debug_vprintf(format, va);
    va_end(va);
}

void warn(char *fmt, ...) {
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
void error(char *fmt, ...) {
    va_list va;
    va_start(va, fmt);
    gui_debug_printf("Error (%08x): ", arm.reg[15]);
    gui_debug_vprintf(fmt, va);
    gui_debug_printf("\n");
    /*fprintf(stderr, "\n\tBacktrace:\n");
    backtrace(arm.reg[11]);*/
    va_end(va);
    debugger(DBG_EXCEPTION, 0);
    cpu_events |= EVENT_RESET;
    __builtin_longjmp(restart_after_exception, 1);
}

int exec_hack() {
    if (arm.reg[15] == 0x10040) {
        arm.reg[15] = arm.reg[14];
        warn("BOOT1 is required to run this version of BOOT2.");
        return 1;
    }
    return 0;
}

void prefetch_abort(uint32_t mva, uint8_t status) {
    logprintf(LOG_CPU, "Prefetch abort: address=%08x status=%02x\n", mva, status);
    arm.reg[15] += 4;
    // Fault address register not changed
    arm.instruction_fault_status = status;
    cpu_exception(EX_PREFETCH_ABORT);
    if (mva == arm.reg[15])
        error("Abort occurred with exception vectors unmapped");
    __builtin_longjmp(restart_after_exception, 1);
}

void data_abort(uint32_t mva, uint8_t status) {
    logprintf(LOG_CPU, "Data abort: address=%08x status=%02x\n", mva, status);
    fix_pc_for_fault();
    arm.reg[15] += 8;
    arm.fault_address = mva;
    arm.data_fault_status = status;
    cpu_exception(EX_DATA_ABORT);
    __builtin_longjmp(restart_after_exception, 1);
}

os_frequency_t perffreq;

void throttle_interval_event(int index) {
    event_repeat(index, 27000000 / 100);

    /* Throttle interval (defined arbitrarily as 100Hz) - used for
     * keeping the emulator speed down, and other miscellaneous stuff
     * that needs to be done periodically */
    static int intervals;
    intervals++;

    extern void usblink_timer();
    usblink_timer();

    int c = gui_getchar();
    if(c != -1) {
        char ch = c;
        serial_byte_in(ch);
    }

    gdbstub_recv();

    rdebug_recv();

    gui_do_stuff();

    os_time_t interval_end;
    os_query_time(&interval_end);

    // Show speed
    static int prev_intervals;
    static os_time_t prev;
    int64_t time = os_time_diff(interval_end, prev) / 4;
    if (time >= os_frequency_hz(perffreq)) {
        double speed = (double)os_frequency_hz(perffreq) * (intervals - prev_intervals) / time;
        gui_show_speed(speed);
        prev_intervals = intervals;
        prev = interval_end;
    }

	if (!turbo_mode || is_halting)
		throttle_timer_wait();
	if (is_halting)
		is_halting--;
}

void add_reset_proc(void (*proc)(void))
{
    if (reset_proc_count == sizeof(reset_procs)/sizeof(*reset_procs))
        abort();
    reset_procs[reset_proc_count++] = proc;
}

int emulate()
{
    const char *preload_filename[4] = {pre_boot2, pre_diags, pre_os};
    int i;

    // Enter debug mode?
    if(debug_on_start)
        cpu_events |= EVENT_DEBUG_STEP;

    if (path_flash) {
        if(!flash_open(path_flash))
            return 1;
    } else {
        if(!flash_create_new(large_nand, preload_filename, product, large_sdram))
            return 1;
    }

    uint32_t sdram_size;
    flash_read_settings(&sdram_size);

    if(!memory_initialize(sdram_size))
        return 1;

    uint8_t *rom = mem_areas[0].ptr;
    memset(rom, -1, 0x80000);
    for (i = 0x00000; i < 0x80000; i += 4) {
        RAM_FLAGS(&rom[i]) = RF_READ_ONLY;
    }
    if (path_boot1) {
        /* Load the ROM */
        FILE *f = fopen(path_boot1, "rb");
        if (!f) {
            gui_perror(path_boot1);
            return 1;
        }
        fread(rom, 1, 0x80000, f);
        fclose(f);
    }

#ifndef NO_TRANSLATION
    if(!insn_buffer)
    {
        insn_buffer = os_alloc_executable(INSN_BUFFER_SIZE);
        insn_bufptr = insn_buffer;
    }

    if(!insn_buffer)
        return false;
#endif

    os_exception_frame_t frame;
    addr_cache_init(&frame);

    os_query_frequency(&perffreq);

    throttle_timer_on();

    if(gdb_port)
        gdbstub_init(gdb_port);

    if(rdbg_port)
        rdebug_bind(rdbg_port);

reset:
    memset(&arm, 0, sizeof arm);
    arm.control = 0x00050078;
    arm.cpsr_low28 = MODE_SVC | 0xC0;
    cpu_events &= EVENT_DEBUG_STEP;

    gdbstub_reset();

    if (path_boot2) {
        FILE *boot2_file = fopen(path_boot2, "rb");
        if(!boot2_file)
        {
            gui_perror(path_boot2);
            return 1;
        }

        /* Start from BOOT2. (needs to be re-loaded on each reset since
         * it can get overwritten in memory) */
        fseek(boot2_file, 0, SEEK_END);
        uint32_t boot2_size = ftell(boot2_file);
        fseek(boot2_file, 0, SEEK_SET);
        uint8_t *boot2_ptr = phys_mem_ptr(boot2_base, boot2_size);
        if (!boot2_ptr) {
            emuprintf("Address %08X is not in RAM.\n", boot2_base);
            return 1;
        }
        fread(boot2_ptr, 1, boot2_size, boot2_file);
        arm.reg[15] = boot2_base;
        if (boot2_ptr[3] < 0xE0)
            emuprintf("%s does not appear to be an uncompressed BOOT2 image.\n", path_boot2);

        fclose(boot2_file);

        if (!emulate_casplus) {
            /* To enter maintenance mode (home+enter+P), address A4012ECC
             * must contain an array indicating those keys before BOOT2 starts */
            uint8_t *shared = phys_mem_ptr(0xA4012EB0, 0x200);
            memcpy(&shared[0x1C], (void *)key_map, 0x12);

            /* BOOT1 is expected to store the address of a function pointer table
             * to A4012EE8. OS 3.0 calls some of these functions... */
            static const struct {
                uint32_t ptrs[8];
                uint16_t code[16];
            } stuff = { {
                        0x10020+0x01, // function 0: return *r0
                        0x10020+0x05, // function 1: *r0 = r1
                        0x10020+0x09, // function 2: *r0 |= r1
                        0x10020+0x11, // function 3: *r0 &= ~r1
                        0x10020+0x19, // function 4: *r0 ^= r1
                        0x10020+0x20, // function 5: related to C801xxxx (not implemented)
                        0x10020+0x03, // function 6: related to 9011xxxx (not implemented)
                        0x10020+0x03, // function 7: related to 9011xxxx (not implemented)
                      }, {
                0x6800,0x4770,               // return *r0
                0x6001,0x4770,               // *r0 = r1
                0x6802,0x430A,0x6002,0x4770, // *r0 |= r1
                0x6802,0x438A,0x6002,0x4770, // *r0 &= ~r1
                0x6802,0x404A,0x6002,0x4770, // *r0 ^= r1
            } };
            memcpy(&rom[0x10000], &stuff, sizeof stuff);
            RAM_FLAGS(&rom[0x10040]) |= RF_EXEC_HACK;
            *(uint32_t *)&shared[0x38] = 0x10000;
        }
    }
    addr_cache_flush();
    flush_translations();

    sched_reset();

    for (i = 0; i < reset_proc_count; i++)
        reset_procs[i]();

    sched_items[SCHED_THROTTLE].clock = CLOCK_27M;
    sched_items[SCHED_THROTTLE].proc = throttle_interval_event;

    sched_update_next_event(0);

    exiting = false;

    __builtin_setjmp(restart_after_exception);

    while (!exiting) {
        sched_process_pending_events();
        while (cycle_count_delta < 0) {
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
    return 0;
}

void cleanup()
{
    if(debugger_input)
        fclose(debugger_input);

    memory_deinitialize();
    flash_close();

    gdbstub_quit();
    rdebug_quit();
}
