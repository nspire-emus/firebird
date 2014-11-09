/* Declarations for misc.c */

#ifndef _H_MISC
#define _H_MISC

void sdramctl_write_word(uint32_t addr, uint32_t value);

void memctl_cx_reset(void);
uint32_t memctl_cx_read_word(uint32_t addr);
void memctl_cx_write_word(uint32_t addr, uint32_t value);

union gpio_reg { uint32_t w; uint8_t b[4]; };
extern struct gpio_state {
        union gpio_reg direction;
        union gpio_reg output;
        union gpio_reg input;
        union gpio_reg invert;
        union gpio_reg sticky;
        union gpio_reg unknown_24;
} gpio;
void gpio_reset();
uint32_t gpio_read(uint32_t addr);
void gpio_write(uint32_t addr, uint32_t value);

extern struct timerpair {
        struct timer {
                uint16_t ticks;
                uint16_t start_value;     /* Write value of +00 */
                uint16_t value;           /* Read value of +00  */
                uint16_t divider;         /* Value of +04 */
                uint16_t control;         /* Value of +08 */
        } timers[2];
        uint16_t completion_value[6];
        uint8_t int_mask;
        uint8_t int_status;
} timerpairs[3];
uint32_t timer_read(uint32_t addr);
void timer_write(uint32_t addr, uint32_t value);
void timer_reset(void);

void xmodem_send(char *filename);
void serial_reset(void);
uint32_t serial_read(uint32_t addr);
void serial_write(uint32_t addr, uint32_t value);
void serial_cx_reset(void);
uint32_t serial_cx_read(uint32_t addr);
void serial_cx_write(uint32_t addr, uint32_t value);
void serial_byte_in(uint8_t byte);

uint32_t unknown_cx_read(uint32_t addr);
void unknown_cx_write(uint32_t addr, uint32_t value);

void watchdog_reset();
uint32_t watchdog_read(uint32_t addr);
void watchdog_write(uint32_t addr, uint32_t value);

void unknown_9008_write(uint32_t addr, uint32_t value);

uint32_t rtc_read(uint32_t addr);
void rtc_write(uint32_t addr, uint32_t value);
uint32_t rtc_cx_read(uint32_t addr);
void rtc_cx_write(uint32_t addr, uint32_t value);

uint32_t misc_read(uint32_t addr);
void misc_write(uint32_t addr, uint32_t value);

extern struct pmu_state {
        uint32_t clocks_load;
        uint32_t wake_mask;
        uint32_t disable;
        uint32_t disable2;
        uint32_t clocks;
} pmu;
void pmu_reset(void);
uint32_t pmu_read(uint32_t addr);
void pmu_write(uint32_t addr, uint32_t value);

uint32_t timer_cx_read(uint32_t addr);
void timer_cx_write(uint32_t addr, uint32_t value);
void timer_cx_reset(void);

void hdq1w_reset(void);
uint32_t hdq1w_read(uint32_t addr);
void hdq1w_write(uint32_t addr, uint32_t value);

uint32_t unknown_9011_read(uint32_t addr);
void unknown_9011_write(uint32_t addr, uint32_t value);

uint32_t spi_read_word(uint32_t addr);
void spi_write_word(uint32_t addr, uint32_t value);

uint8_t sdio_read_byte(uint32_t addr);
uint16_t sdio_read_half(uint32_t addr);
uint32_t sdio_read_word(uint32_t addr);
void sdio_write_byte(uint32_t addr, uint8_t value);
void sdio_write_half(uint32_t addr, uint16_t value);
void sdio_write_word(uint32_t addr, uint32_t value);

uint32_t sramctl_read_word(uint32_t addr);
void sramctl_write_word(uint32_t addr, uint32_t value);

uint32_t unknown_BC_read_word(uint32_t addr);

void adc_reset();
uint32_t adc_read_word(uint32_t addr);
void adc_write_word(uint32_t addr, uint32_t value);

#endif
