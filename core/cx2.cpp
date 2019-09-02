#include "emu.h"

#include "cx2.h"
#include "misc.h"

/* 90140000 */
struct aladdin_pmu_state aladdin_pmu;

void aladdin_pmu_reset(void) {
	memset(&aladdin_pmu, 0, sizeof(aladdin_pmu));
	aladdin_pmu.clocks = 0x21010001;
	aladdin_pmu.disable[0] = 1 << 20;
	aladdin_pmu.noidea[0] = 0x1A,
	aladdin_pmu.noidea[1] = 0x101,
	aladdin_pmu.noidea[2] = 0x0021DB19;
	aladdin_pmu.noidea[3] = 0x00100000;
	aladdin_pmu.noidea[4] = 0x111;
	aladdin_pmu.noidea[5] = 0x1;
	aladdin_pmu.noidea[6] = 0x100;
	aladdin_pmu.noidea[7] = 0x10;

	uint32_t cpu = 396000000;
	sched.clock_rates[CLOCK_CPU] = cpu;
	sched.clock_rates[CLOCK_AHB] = cpu / 2;
	sched.clock_rates[CLOCK_APB] = cpu / 4;
}

static void aladdin_pmu_update_int()
{
	int_set(INT_POWER, aladdin_pmu.int_state != 0);
}

uint32_t aladdin_pmu_read(uint32_t addr)
{
	uint32_t offset = addr & 0xFFFF;
	if(offset < 0x100)
	{
		switch (addr & 0xFF)
		{
		case 0x00: return 0x040000; // Wakeup reason
		case 0x08: return 0x2000;
		case 0x20: return aladdin_pmu.disable[0];
		case 0x24: return aladdin_pmu.int_state;
		case 0x30: return aladdin_pmu.clocks;
		case 0x50: return aladdin_pmu.disable[1];
		case 0x60: return aladdin_pmu.disable[2];
		}
	}
	if(offset >= 0x800 && offset < 0x900)
		return aladdin_pmu.noidea[(offset & 0xFF) >> 2];

	return bad_read_word(addr);
}

void aladdin_pmu_write(uint32_t addr, uint32_t value)
{
	uint32_t offset = addr & 0xFFFF;
	if(offset < 0x100)
	{
		switch (offset & 0xFF)
		{
		case 0x00: return;
		case 0x08: return;
		case 0x20: aladdin_pmu.disable[0] = value; return;
		case 0x24:
			aladdin_pmu.int_state &= ~value;
			aladdin_pmu_update_int();
			return;
		case 0x30:
			aladdin_pmu.clocks = value;
			aladdin_pmu.int_state |= 1;
			aladdin_pmu_update_int();
			return;
		case 0x50: aladdin_pmu.disable[1] = value; return;
		case 0x60: aladdin_pmu.disable[2] = value; return;
		}
	}
	else if(offset >= 0x800 && offset < 0x900)
	{
		aladdin_pmu.noidea[(offset & 0xFF) >> 2] = value;
		return;
	}

	bad_write_word(addr, value);
}

/* 90120000: FTDDR3030 */
uint32_t memc_ddr_read(uint32_t addr)
{
	switch(addr & 0xFFFF)
	{
	case 0x04:
		return 0x102;
	case 0x10:
		return 3; // Size
	case 0x74:
		return 0;
	}
	return bad_read_word(addr);
}

void memc_ddr_write(uint32_t addr, uint32_t value)
{
	uint16_t offset = addr;
	if(offset < 0x40)
		return; // Config data - don't care

	switch(addr & 0xFFFF)
	{
	case 0x074:
	case 0x0A8:
	case 0x0AC:
	case 0x138:
		return;
	}
	bad_write_word(addr, value);
}

/* 90130000: ??? for LCD backlight */
void cx2_backlight_write(uint32_t addr, uint32_t value)
{
	switch(addr & 0xFFF)
	{
	case 0x014:
		return;
	case 0x018:
		hdq1w.lcd_contrast = value;
		return;
	case 0x020:
		return;
	}

	bad_write_word(addr, value);
}

/* 90040000: FTSSP010 SPI controller connected to the LCD.
   Only the bare minimum to get the OS to boot is implemented. */
struct cx2_lcd_spi_state cx2_lcd_spi_state;

uint32_t cx2_lcd_spi_read(uint32_t addr)
{
	switch (addr & 0xFFF)
	{
	case 0x0C: // REG_SR
	{
		uint32_t ret = 0x10 | (cx2_lcd_spi_state.busy ? 0x04 : 0x02);
		cx2_lcd_spi_state.busy = false;
		return ret;
	}
	default:
		// Ignored.
		return 0;
	}
}

void cx2_lcd_spi_write(uint32_t addr, uint32_t value)
{
	(void) value;

	switch (addr & 0xFF)
	{
	case 0x18: // REG_DR
		cx2_lcd_spi_state.busy = true;
		break;
	default:
		// Ignored
		break;
	}
}
