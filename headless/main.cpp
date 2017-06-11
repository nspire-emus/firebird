#include <errno.h>

#include "core/debug.h"
#include "core/emu.h"
#include "core/mem.h"
#include "core/mmu.h"

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
    static char debug_in[40];
    fgets(debug_in, sizeof(debug_in), stdin);
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
	const char *boot1 = nullptr, *flash = nullptr, *snapshot = nullptr, *rampayload = nullptr;

	for(int argi = 1; argi < argc; ++argi)
	{
		if(strcmp(argv[argi], "--boot1") == 0)
			boot1 = argv[++argi];
		else if(strcmp(argv[argi], "--flash") == 0)
			flash = argv[++argi];
		else if(strcmp(argv[argi], "--snapshot") == 0)
			snapshot = argv[++argi];
		else if(strcmp(argv[argi], "--rampayload") == 0)
			rampayload = argv[++argi];
		else if(strcmp(argv[argi], "--debug-on-start") == 0)
			debug_on_start = true;
		else if(strcmp(argv[argi], "--debug-on-warn") == 0)
			debug_on_warn = true;
		else if(strcmp(argv[argi], "--diags") == 0)
			boot_order = ORDER_DIAGS;
		else if(strcmp(argv[argi], "--product") == 0)
			product = strtoul(argv[++argi], nullptr, 0);
		else if(strcmp(argv[argi], "--features") == 0)
			features = strtoul(argv[++argi], nullptr, 0);
		else
		{
			fprintf(stderr, "Unknown argument '%s'.\n", argv[argi]);
			return 1;
		}
	}

	if(boot1)
		path_boot1 = boot1;

	if(flash)
		path_flash = flash;

	if(!emu_start(0, 0, snapshot))
		return 1;

	if(rampayload)
	{
		FILE *f = fopen(rampayload, "rb");
		if(!f)
		{
			perror("Could not open RAM payload");
			return 3;
		}

		if(fread(mem_areas[1].ptr, 1, mem_areas[1].size, f) < 0)
		{
			perror("Could not read RAM payload");
			return 4;
		}

		fclose(f);

		// Jump to payload
		arm.reg[15] = mem_areas[1].base;
	}

	turbo_mode = true;
	emu_loop(false);

	return 0;
}
