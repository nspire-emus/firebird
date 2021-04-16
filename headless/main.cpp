#include <errno.h>

#include "core/debug.h"
#include "core/emu.h"
#include "core/mem.h"
#include "core/mmu.h"
#include "core/usblink_queue.h"

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

    putchar('\n');

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
void throttle_timer_wait(unsigned int usec) {}

int main(int argc, char *argv[])
{
	const char *boot1 = nullptr, *flash = nullptr, *snapshot = nullptr, *rampayload = nullptr;
	uint32_t rampayload_base = 0x10000000;

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
		else if(strcmp(argv[argi], "--rampayload-address") == 0)
			rampayload_base = strtol(argv[++argi], nullptr, 0);
		else if(strcmp(argv[argi], "--debug-on-start") == 0)
			debug_on_start = true;
		else if(strcmp(argv[argi], "--debug-on-warn") == 0)
			debug_on_warn = true;
		else if(strcmp(argv[argi], "--print-on-warn") == 0)
			print_on_warn = true;
		else if(strcmp(argv[argi], "--diags") == 0)
			boot_order = ORDER_DIAGS;
		else
		{
			fprintf(stderr, "Unknown argument '%s'.\n", argv[argi]);
			return 1;
		}
	}

	if(!boot1 || !flash)
	{
		fprintf(stderr, "You need to specify at least Boot1 and Flash images.\n");
		return 2;
	}

	path_boot1 = boot1;
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

		fseek(f, 0, SEEK_END);
		size_t size = ftell(f);
		rewind(f);

		void *target = phys_mem_ptr(rampayload_base, size);
		if(!target)
		{
			fprintf(stderr, "RAM payload too big");
			return 5;
		}

		if(fread(target, size, 1, f) != 1)
		{
			perror("Could not read RAM payload");
			return 4;
		}

		fclose(f);

		// Jump to payload
		arm.reg[15] = rampayload_base;
	}

	turbo_mode = true;
	emu_loop(false);

	return 0;
}
