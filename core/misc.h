/* Declarations for misc.c */

#ifndef _H_MISC
#define _H_MISC

#ifdef __cplusplus
extern "C" {
#endif

extern bool on_irq;

// We can't include emu.h here, so only a "forward-typedef"
typedef struct emu_snapshot emu_snapshot;

void sdramctl_write_word(uint32_t addr, uint32_t value);

typedef struct memctl_cx_state {
    uint32_t status, config;
    uint32_t nandctl_ecc_memcfg;
} memctl_cx_state;

bool memctl_cx_suspend(emu_snapshot *snapshot);
bool memctl_cx_resume(const emu_snapshot *snapshot);
void memctl_cx_reset(void);
uint32_t memctl_cx_read_word(uint32_t addr);
void memctl_cx_write_word(uint32_t addr, uint32_t value);

union gpio_reg { uint64_t w; uint8_t b[8]; };
typedef struct gpio_state {
        union gpio_reg direction;
        union gpio_reg output;
        union gpio_reg input;
        union gpio_reg invert;
        union gpio_reg sticky;
        union gpio_reg unknown_24;
} gpio_state;
extern gpio_state gpio;

bool gpio_suspend(emu_snapshot *snapshot);
bool gpio_resume(const emu_snapshot *snapshot);
void gpio_reset();
uint32_t gpio_read(uint32_t addr);
void gpio_write(uint32_t addr, uint32_t value);

struct timerpair {
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
};

typedef struct timer_state {
    struct timerpair pairs[3];
} timer_state;

bool timer_suspend(emu_snapshot *snapshot);
bool timer_resume(const emu_snapshot *snapshot);
uint32_t timer_read(uint32_t addr);
void timer_write(uint32_t addr, uint32_t value);
void timer_reset(void);

void xmodem_send(const char *filename);

typedef struct serial_state {
    uint8_t rx_char;
    uint8_t interrupts;
    uint8_t DLL;
    uint8_t DLM;
    uint8_t IER;
    uint8_t LCR;
} serial_state;

bool serial_suspend(emu_snapshot *snapshot);
bool serial_resume(const emu_snapshot *snapshot);
void serial_reset(void);
uint32_t serial_read(uint32_t addr);
void serial_write(uint32_t addr, uint32_t value);

typedef struct serial_cx_state {
    uint8_t rx_char;
    uint8_t rx;
    uint32_t cr;
    uint16_t int_status;
    uint16_t int_mask;
} serial_cx_state;

bool serial_cx_suspend(emu_snapshot *snapshot);
bool serial_cx_resume(const emu_snapshot *snapshot);
void serial_cx_reset(void);
uint32_t serial_cx_read(uint32_t addr);
void serial_cx_write(uint32_t addr, uint32_t value);
void serial_byte_in(uint8_t byte);

uint32_t unknown_cx_read(uint32_t addr);
void unknown_cx_write(uint32_t addr, uint32_t value);

typedef struct watchdog_state {
    uint32_t load;
    uint32_t value;
    uint8_t control;
    uint8_t interrupt;
    uint8_t locked;
} watchdog_state;

bool watchdog_suspend(emu_snapshot *snapshot);
bool watchdog_resume(const emu_snapshot *snapshot);
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

typedef struct pmu_state {
        uint32_t clocks_load;
        uint32_t wake_mask;
        uint32_t disable;
        uint32_t disable2;
        uint32_t clocks;
        bool on_irq_enabled;
} pmu_state;
extern pmu_state pmu;

bool pmu_suspend(emu_snapshot *snapshot);
bool pmu_resume(const emu_snapshot *snapshot);
void pmu_reset(void);
uint32_t pmu_read(uint32_t addr);
void pmu_write(uint32_t addr, uint32_t value);

typedef struct timer_cx_state {
    struct cx_timer {
        uint32_t load;
        uint32_t value;
        uint8_t prescale;
        uint8_t control;
        uint8_t interrupt;
        uint8_t reload;
    } timer[3][2];
} timer_cx_state;

bool timer_cx_suspend(emu_snapshot *snapshot);
bool timer_cx_resume(const emu_snapshot *snapshot);
uint32_t timer_cx_read(uint32_t addr);
void timer_cx_write(uint32_t addr, uint32_t value);
void timer_cx_reset(void);

typedef struct hdq1w_state {
    uint8_t lcd_contrast;
} hdq1w_state;
extern hdq1w_state hdq1w;

bool hdq1w_suspend(emu_snapshot *snapshot);
bool hdq1w_resume(const emu_snapshot *snapshot);
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

typedef struct adc_state {
    uint32_t int_status;
    uint32_t int_mask;
    struct adc_channel {
        uint32_t unknown;
        uint32_t count;
        uint32_t address;
        uint16_t value;
        uint16_t speed;
    } channel[7];
} adc_state;

bool adc_suspend(emu_snapshot *snapshot);
bool adc_resume(const emu_snapshot *snapshot);
void adc_reset();
uint32_t adc_read_word(uint32_t addr);
void adc_write_word(uint32_t addr, uint32_t value);

#ifdef __cplusplus
}
#endif

#endif
