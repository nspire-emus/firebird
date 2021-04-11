#ifndef CX2_H
#define CX2_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct emu_snapshot emu_snapshot;

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

typedef struct cx2_backlight_state {
    uint32_t pwm_period, pwm_value;
} cx2_backlight_state;

void cx2_backlight_write(uint32_t addr, uint32_t value);
void cx2_backlight_reset();

typedef struct cx2_lcd_spi_state {
	bool busy;
} cx2_lcd_spi_state;

uint32_t cx2_lcd_spi_read(uint32_t addr);
void cx2_lcd_spi_write(uint32_t addr, uint32_t value);

typedef struct dma_state {
	uint32_t csr; // 0x24
	struct {
		uint32_t control;
		uint32_t config;
		uint32_t src;
		uint32_t dest;
		uint32_t len;
	} channels[1]; // 0x100+
} dma_state;

void dma_cx2_reset();
uint32_t dma_cx2_read_word(uint32_t addr);
void dma_cx2_write_word(uint32_t addr, uint32_t value);

// The peripherals in cx2.cpp have trivial suspend/resume ops, so don't need
// separate functions each.
bool cx2_suspend(emu_snapshot *snapshot);
bool cx2_resume(const emu_snapshot *snapshot);

#ifdef __cplusplus
}
#endif

#endif // CX2_H
