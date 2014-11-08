/* Declarations for interrupt.c */

#ifndef _H_INTERRUPT
#define _H_INTERRUPT

#define INT_SERIAL   1
#define INT_WATCHDOG 3
#define INT_USB      8
#define INT_ADC      11
#define INT_POWER    15
#define INT_KEYPAD   16
#define INT_TIMER0   17
#define INT_TIMER1   18
#define INT_TIMER2   19
#define INT_LCD      21

extern struct interrupt_state {
	u32 active;
	u32 raw_status;         // .active ^ ~.noninverted
	u32 sticky_status;      // set on rising transition of .raw_status
	u32 status;             // +x04: mixture of bits from .raw_status and .sticky_status
	                        //       (determined by .sticky)
	u32 mask[2];            // +x08: enabled interrupts
	u8  prev_pri_limit[2];  // +x28: saved .priority_limit from reading +x24
	u8  priority_limit[2];  // +x2C: interrupts with priority >= this value are disabled
	u32 noninverted;        // +200: which interrupts not to invert in .raw_status
	u32 sticky;             // +204: which interrupts to use .sticky_status
	u8  priority[32];       // +3xx: priority per interrupt (0=max, 7=min)
} intr;

u32 int_read_word(u32 addr);
void int_write_word(u32 addr, u32 value);
u32 int_cx_read_word(u32 addr);
void int_cx_write_word(u32 addr, u32 value);
void int_set(u32 int_num, bool on);
void int_reset();

void *int_save_state(size_t *size);
void int_reload_state(void *state);

#endif
