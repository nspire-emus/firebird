#ifndef CX2_H
#define CX2_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct aladdin_pmu_state {
	uint32_t clocks;
	uint32_t disable[3];
	uint32_t int_state; // Actual bit assignments not known
	uint32_t noidea[0x100 / sizeof(uint32_t)];
} aladdin_pmu_state;

void aladdin_pmu_write(uint32_t addr, uint32_t value);
uint32_t aladdin_pmu_read(uint32_t addr);
void aladdin_pmu_reset(void);

uint32_t memc_ddr_read(uint32_t addr);
void memc_ddr_write(uint32_t addr, uint32_t value);

void cx2_backlight_write(uint32_t addr, uint32_t value);

struct cx2_lcd_spi_state {
    bool busy;
};

uint32_t cx2_lcd_spi_read(uint32_t addr);
void cx2_lcd_spi_write(uint32_t addr, uint32_t value);

#ifdef __cplusplus
}
#endif

#endif // CX2_H
