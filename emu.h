#ifndef _H_EMU
#define _H_EMU

#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>

#include "flash.h"

#ifdef __cplusplus
extern "C" {
#endif

// Can also be set manually
#if !defined(__i386__) && !defined(__x86_64__) && !defined(__arm__)
#define NO_TRANSLATION
#endif

// Needed for the assembler calling convention
#ifdef __i386__
    #ifdef FASTCALL
        #undef FASTCALL
    #endif
    #define FASTCALL __attribute__((fastcall))
#else
    #define FASTCALL
#endif

// Helper for micro-optimization
#define unlikely(x) __builtin_expect(x, 0)
#define likely(x) __builtin_expect(x, 1)
static inline uint16_t BSWAP16(uint16_t x) { return x << 8 | x >> 8; }
#define BSWAP32(x) __builtin_bswap32(x)

extern int cycle_count_delta __asm__("cycle_count_delta");
extern int throttle_delay;
extern uint32_t cpu_events __asm__("cpu_events");
#define EVENT_IRQ 1
#define EVENT_FIQ 2
#define EVENT_RESET 4
#define EVENT_DEBUG_STEP 8
#define EVENT_WAITING 16

// Settings
extern volatile bool exiting, debug_on_start, debug_on_warn, large_nand, large_sdram;
extern BootOrder boot_order;
extern bool do_translate;
extern int product;
extern int asic_user_flags;
extern uint32_t boot2_base;
extern const char *path_boot1, *path_boot2, *path_flash, *pre_manuf, *pre_boot2, *pre_diags, *pre_os;

#define emulate_casplus (product == 0x0C0)
// 0C-0E (CAS, lab cradle, plain Nspire) use old ASIC
// 0F-12 (CX CAS, CX, CM CAS, CM) use new ASIC
#define emulate_cx (product >= 0x0F0)
extern bool turbo_mode;

enum { LOG_CPU, LOG_IO, LOG_FLASH, LOG_INTS, LOG_ICOUNT, LOG_USB, LOG_GDB, MAX_LOG };
#define LOG_TYPE_TBL "CIFQ#UG"
extern int log_enabled[MAX_LOG];
void logprintf(int type, const char *str, ...);
void emuprintf(const char *format, ...);

void warn(const char *fmt, ...);
__attribute__((noreturn)) void error(const char *fmt, ...);
void throttle_timer_on();
void throttle_timer_off();
void throttle_timer_wait();
int exec_hack();
void add_reset_proc(void (*proc)(void));

// Is actually a jmp_buf, but __builtin_*jmp is used instead
// as the MinGW variant is buggy
extern void *restart_after_exception[32];

// GUI callbacks
void gui_do_stuff();
int gui_getchar();
void gui_putchar(char c);
void gui_debug_printf(const char *fmt, ...);
void gui_debug_vprintf(const char *fmt, va_list ap);
void gui_perror(const char *msg);
char *gui_debug_prompt();
void gui_status_printf(const char *fmt, ...);
void gui_show_speed(double speed);
void gui_usblink_changed(bool state);

int emulate(unsigned int port_gdb, unsigned int port_rdbg);
void emu_cleanup();

#ifdef __cplusplus
}
#endif

#endif
