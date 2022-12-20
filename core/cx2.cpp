#include "emu.h"

#include "mem.h"
#include "cx2.h"
#include "misc.h"
#include "schedule.h"
#include "interrupt.h"
#include "keypad.h"

/* 90140000 */
struct aladdin_pmu_state aladdin_pmu;

void aladdin_pmu_reset(void) {
	memset(&aladdin_pmu, 0, sizeof(aladdin_pmu));
	aladdin_pmu.clocks = 0x21020303;
	aladdin_pmu.disable[0] = 0;
	aladdin_pmu.noidea[0] = 0x1A;
	aladdin_pmu.noidea[1] = 0x101;
	// Special cases for 0x808-0x810, see aladdin_pmu_read
	//aladdin_pmu.noidea[4] = 0x111;
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
		case 0x04: return 0; // No idea
		case 0x08: return 0x2000;
		case 0x20: return aladdin_pmu.disable[0];
		case 0x24: return aladdin_pmu.int_state;
		case 0x30: return aladdin_pmu.clocks;
		case 0x50: return aladdin_pmu.disable[1];
		case 0x60: return aladdin_pmu.disable[2];
		}
	}
	else if(offset == 0x808) // efuse
		return 0x0021DB19; // TODO: Taken from CX II CAS, same on other models?
	else if(offset == 0x80C) // also efuse
		return asic_user_flags << 20; // Has to match the 80E0 field in OS images
	else if(offset == 0x810)
		/* Bit 8 clear when ON key pressed */
		return 0x11 | ((keypad.key_map[0] & 1<<9) ? 0 : 0x100);
	else if(offset >= 0x800 && offset < 0x900)
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
		case 0x04: return; // No idea
		case 0x08: return;
		case 0x20:
			if(value & 2)
			{
				/* enter sleep, jump to 0 when On pressed. */
				cpu_events |= EVENT_SLEEP;
				// Without this, the clocks are wrong
				aladdin_pmu_reset();
			}
			else
				aladdin_pmu.disable[0] = value;

			return;
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
		case 0xC4: return;
		}
	}
	else if(offset == 0x80C || offset == 0x810)
		return bad_write_word(addr, value);
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
	case 0x28:
		return 0;
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
static struct cx2_backlight_state cx2_backlight;

void cx2_backlight_reset()
{
	cx2_backlight.pwm_value = cx2_backlight.pwm_period = 0;
}

void cx2_backlight_write(uint32_t addr, uint32_t value)
{
	auto offset = addr & 0xFFF;
	if(offset == 0x014)
		cx2_backlight.pwm_value = value;
	else if(offset == 0x018)
		cx2_backlight.pwm_period = value;
	else if(offset != 0x020)
		bad_write_word(addr, value);

	// Hack: Mirror the value in hdq1w
	if(cx2_backlight.pwm_period == 0)
		hdq1w.lcd_contrast = 0;
	else
		// Lower value -> brighter
		hdq1w.lcd_contrast = 255 - (cx2_backlight.pwm_value * 255) / cx2_backlight.pwm_period;
}

/* 90040000: FTSSP010 SPI controller connected to the LCD.
   Only the bare minimum to get the OS to boot is implemented. */
struct cx2_lcd_spi_state cx2_lcd_spi;

uint32_t cx2_lcd_spi_read(uint32_t addr)
{
	switch (addr & 0xFFF)
	{
	case 0x0C: // REG_SR
	{
		uint32_t ret = 0x10 | (cx2_lcd_spi.busy ? 0x04 : 0x02);
		cx2_lcd_spi.busy = false;
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
		cx2_lcd_spi.busy = true;
		break;
	default:
		// Ignored
		break;
	}
}

/* BC000000: An FTDMAC020 */
static dma_state dma;

void dma_cx2_reset()
{
	memset(&dma, 0, sizeof(dma));
}

enum class DMAMemDir {
	INC=0,
	DEC=1,
	FIX=2
};

static void dma_cx2_update()
{
	if(!(dma.csr & 1)) // Enabled?
		return;

	if(dma.csr & 0b110) // Big-endian?
		return;

	for(auto &channel : dma.channels)
	{
		if(!(channel.control & 1)) // Start?
			continue;

		if((channel.control & 0b110) != 0b110) // AHB1?
			error("Not implemented");

		if(channel.control & (1 << 15)) // Abort
			error("Not implemented");

		auto dstdir = DMAMemDir((channel.control >> 3) & 3),
			 srcdir = DMAMemDir((channel.control >> 5) & 3);

		if(srcdir != DMAMemDir::INC || dstdir != DMAMemDir::INC)
			error("Not implemented");

		auto dstwidth = (channel.control >> 8) & 7,
			 srcwidth = (channel.control >> 11) & 7;

		if(dstwidth != srcwidth || dstwidth > 2)
			error("Not implemented");

		// Convert to bytes
		srcwidth = 1 << srcwidth;

		size_t total_len = channel.len * srcwidth;

		void *srcp = phys_mem_ptr(channel.src, total_len),
			 *dstp = phys_mem_ptr(channel.dest, total_len);

		if(!srcp || !dstp)
			error("Invalid DMA transfer?");

		/* Doesn't trigger any read or write actions, but on HW
		 * special care has to be taken anyway regarding caches
		 * and so on, so should be fine. */
		memcpy(dstp, srcp, total_len);

		channel.control &= ~1; // Clear start bit
	}
}

uint32_t dma_cx2_read_word(uint32_t addr)
{
	switch (addr & 0x3FFFFFF) {
		case 0x00C: return 0;
		case 0x01C: return 0;
		case 0x024: return dma.csr;
		case 0x100: return dma.channels[0].control;
		case 0x104: return dma.channels[0].config;
	}
	return bad_read_word(addr);
}

void dma_cx2_write_word(uint32_t addr, uint32_t value)
{
	switch (addr & 0x3FFFFFF) {
		case 0x024: dma.csr = value; return;
		case 0x100:
			dma.channels[0].control = value;
			dma_cx2_update();
			return;
		case 0x104: dma.channels[0].config = value; return;
		case 0x108: dma.channels[0].src = value; return;
		case 0x10C: dma.channels[0].dest = value; return;
		case 0x114: dma.channels[0].len = value & 0x003fffff; return;
	}
	bad_write_word(addr, value);
}

bool cx2_suspend(emu_snapshot *snapshot)
{
    return snapshot_write(snapshot, &aladdin_pmu, sizeof(aladdin_pmu))
            && snapshot_write(snapshot, &cx2_backlight, sizeof(cx2_backlight))
            && snapshot_write(snapshot, &cx2_lcd_spi, sizeof(cx2_lcd_spi))
            && snapshot_write(snapshot, &dma, sizeof(dma));
}

bool cx2_resume(const emu_snapshot *snapshot)
{
    return snapshot_read(snapshot, &aladdin_pmu, sizeof(aladdin_pmu))
            && snapshot_read(snapshot, &cx2_backlight, sizeof(cx2_backlight))
            && snapshot_read(snapshot, &cx2_lcd_spi, sizeof(cx2_lcd_spi))
            && snapshot_read(snapshot, &dma, sizeof(dma));
}
