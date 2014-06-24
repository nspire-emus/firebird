/* Declarations for misc.c */

#ifndef _H_MISC
#define _H_MISC

void sdramctl_write_word(u32 addr, u32 value);

void memctl_cx_reset(void);
u32 memctl_cx_read_word(u32 addr);
void memctl_cx_write_word(u32 addr, u32 value);

union gpio_reg { u32 w; u8 b[4]; };
extern struct gpio_state {
        union gpio_reg direction;
        union gpio_reg output;
        union gpio_reg input;
        union gpio_reg invert;
        union gpio_reg sticky;
        union gpio_reg unknown_24;
} gpio;
void gpio_reset();
u32 gpio_read(u32 addr);
void gpio_write(u32 addr, u32 value);

extern struct timerpair {
        struct timer {
                u16 ticks;
                u16 start_value;     /* Write value of +00 */
                u16 value;           /* Read value of +00  */
                u16 divider;         /* Value of +04 */
                u16 control;         /* Value of +08 */
        } timers[2];
        u16 completion_value[6];
        u8 int_mask;
        u8 int_status;
} timerpairs[3];
u32 timer_read(u32 addr);
void timer_write(u32 addr, u32 value);
void timer_reset(void);

void xmodem_send(char *filename);
void serial_reset(void);
u32 serial_read(u32 addr);
void serial_write(u32 addr, u32 value);
void serial_cx_reset(void);
u32 serial_cx_read(u32 addr);
void serial_cx_write(u32 addr, u32 value);
void serial_byte_in(u8 byte);

u32 unknown_cx_read(u32 addr);
void unknown_cx_write(u32 addr, u32 value);

void watchdog_reset();
u32 watchdog_read(u32 addr);
void watchdog_write(u32 addr, u32 value);

void unknown_9008_write(u32 addr, u32 value);

u32 rtc_read(u32 addr);
void rtc_write(u32 addr, u32 value);
u32 rtc_cx_read(u32 addr);
void rtc_cx_write(u32 addr, u32 value);

u32 misc_read(u32 addr);
void misc_write(u32 addr, u32 value);

extern struct pmu_state {
        u32 clocks_load;
        u32 wake_mask;
        u32 disable;
        u32 disable2;
        u32 clocks;
} pmu;
void pmu_reset(void);
u32 pmu_read(u32 addr);
void pmu_write(u32 addr, u32 value);

u32 timer_cx_read(u32 addr);
void timer_cx_write(u32 addr, u32 value);
void timer_cx_reset(void);

void hdq1w_reset(void);
u32 hdq1w_read(u32 addr);
void hdq1w_write(u32 addr, u32 value);

u32 unknown_9011_read(u32 addr);
void unknown_9011_write(u32 addr, u32 value);

u32 spi_read_word(u32 addr);
void spi_write_word(u32 addr, u32 value);

u8 sdio_read_byte(u32 addr);
u16 sdio_read_half(u32 addr);
u32 sdio_read_word(u32 addr);
void sdio_write_byte(u32 addr, u8 value);
void sdio_write_half(u32 addr, u16 value);
void sdio_write_word(u32 addr, u32 value);

u32 sramctl_read_word(u32 addr);
void sramctl_write_word(u32 addr, u32 value);

u32 unknown_BC_read_word(u32 addr);

void adc_reset();
u32 adc_read_word(u32 addr);
void adc_write_word(u32 addr, u32 value);

#endif
