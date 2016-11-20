#include <errno.h>

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

int main(int argc, char *argv[])
{
    if(argc != 3)
    {
        fprintf(stderr, "Usage: %s boot1.img flash.img\n", argv[0]);
        return 1;
    }

    path_boot1 = argv[1];
    path_flash = argv[2];

    if(!emu_start(0, 0, NULL))
        return 1;

    turbo_mode = true;
    emu_loop(true);

    return 0;
}
