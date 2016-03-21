#include <errno.h>

#include <emscripten.h>

#include "core/mmu.h"
#include "core/debug.h"
#include "core/emu.h"

void gui_do_stuff(bool wait)
{
}

void do_stuff(int i)
{
}

void gui_debug_printf(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);

    gui_debug_vprintf(fmt, ap);

    va_end(ap);
}

void gui_debug_vprintf(const char *fmt, va_list ap)
{
    vprintf(fmt, ap);
}

void gui_status_printf(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);

    gui_debug_vprintf(fmt, ap);

    va_end(ap);
}

void gui_perror(const char *msg)
{
    gui_debug_printf("%s: %s\n", msg, strerror(errno));
}

void gui_debugger_entered_or_left(bool entered) {}

void gui_debugger_request_input(debug_input_cb callback)
{
    if(!callback) return;
    char debug_in[20];
    fgets(debug_in, 20, stdin);
    callback(debug_in);
}

void gui_putchar(char c) { putc(c, stdout); }
int gui_getchar() { return -1; }
void gui_set_busy(bool busy) {}
void gui_show_speed(double d) {}
void gui_usblink_changed(bool state) {}
void throttle_timer_off() {}
void throttle_timer_on() {}
void throttle_timer_wait() {}

void step()
{
    int i = 1000;
    while(i--)
    {
        sched_process_pending_events();
        while (!exiting && cycle_count_delta < 0)
        {
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

void emscripten_loop(bool reset)
{
    if(reset)
    {
    reset:
        memset(mem_areas[1].ptr, 0, mem_areas[1].size);

        memset(&arm, 0, sizeof arm);
        arm.control = 0x00050078;
        arm.cpsr_low28 = MODE_SVC | 0xC0;
        cpu_events &= EVENT_DEBUG_STEP;

        sched_reset();
        sched.items[SCHED_THROTTLE].clock = CLOCK_27M;
        sched.items[SCHED_THROTTLE].proc = do_stuff;

        memory_reset();
    }

    addr_cache_flush();

    sched_update_next_event(0);

    exiting = false;

// clang segfaults with that, for an iOS build :(
#ifndef NO_SETJMP
    // Workaround for LLVM bug #18974
    while(__builtin_setjmp(restart_after_exception)){};
#endif

    emscripten_set_main_loop(step, 60, 1);
    return;
}

int main(int argc, char **argv)
{
    path_boot1 = argv[1];
    path_flash = argv[2];
    bool ret = emu_start(0, 0, NULL);
    if(!ret)
        return -1;
    turbo_mode = true;
    emscripten_loop(true);
}
