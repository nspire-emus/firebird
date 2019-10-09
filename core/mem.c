#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "emu.h"
#include "os/os.h"
#include "interrupt.h"
#include "misc.h"
#include "keypad.h"
#include "flash.h"
#include "link.h"
#include "sha256.h"
#include "des.h"
#include "lcd.h"
#include "usblink.h"
#include "usb.h"
#include "casplus.h"
#include "mem.h"
#include "debug.h"
#include "translate.h"
#include "usb_cx2.h"
#include "cx2.h"

uint8_t   (*read_byte_map[64])(uint32_t addr);
uint16_t  (*read_half_map[64])(uint32_t addr);
uint32_t  (*read_word_map[64])(uint32_t addr);
void (*write_byte_map[64])(uint32_t addr, uint8_t value);
void (*write_half_map[64])(uint32_t addr, uint16_t value);
void (*write_word_map[64])(uint32_t addr, uint32_t value);

/* For invalid/unknown physical addresses */
uint8_t bad_read_byte(uint32_t addr)               { warn("Bad read_byte: %08x", addr); return 0; }
uint16_t bad_read_half(uint32_t addr)              { warn("Bad read_half: %08x", addr); return 0; }
uint32_t bad_read_word(uint32_t addr)              { warn("Bad read_word: %08x", addr); return 0; }
void bad_write_byte(uint32_t addr, uint8_t value)  { warn("Bad write_byte: %08x %02x", addr, value); }
void bad_write_half(uint32_t addr, uint16_t value) { warn("Bad write_half: %08x %04x", addr, value); }
void bad_write_word(uint32_t addr, uint32_t value) { warn("Bad write_word: %08x %08x", addr, value); }

uint8_t *mem_and_flags = NULL;
struct mem_area_desc mem_areas[5];

void *phys_mem_ptr(uint32_t addr, uint32_t size) {
    unsigned int i;
    for (i = 0; i < sizeof(mem_areas)/sizeof(*mem_areas); i++) {
        uint32_t offset = addr - mem_areas[i].base;
        if (offset < mem_areas[i].size && size <= mem_areas[i].size - offset)
            return mem_areas[i].ptr + offset;
    }
    return NULL;
}

uint32_t phys_mem_addr(void *ptr) {
    int i;
    for (i = 0; i < 3; i++) {
        uint32_t offset = (uint8_t *)ptr - mem_areas[i].ptr;
        if (offset < mem_areas[i].size)
            return mem_areas[i].base + offset;
    }
    return -1; // should never happen
}

void read_action(void *ptr) {
    uint32_t addr = phys_mem_addr(ptr);
    if (!gdb_connected)
        emuprintf("Hit read breakpoint at %08x. Entering debugger.\n", addr);
    debugger(DBG_READ_BREAKPOINT, addr);
}

void write_action(void *ptr) {
    uint32_t addr = phys_mem_addr(ptr);
    uint32_t *flags = &RAM_FLAGS((size_t)ptr & ~3);
    if (*flags & RF_WRITE_BREAKPOINT) {
        if (!gdb_connected)
            emuprintf("Hit write breakpoint at %08x. Entering debugger.\n", addr);
        debugger(DBG_WRITE_BREAKPOINT, addr);
    }
#ifndef NO_TRANSLATION
    if (*flags & RF_CODE_TRANSLATED) {
        logprintf(LOG_CPU, "Wrote to translated code at %08x. Deleting translations.\n", addr);
        invalidate_translation(*flags >> RFS_TRANSLATION_INDEX);
    } else {
        *flags &= ~RF_CODE_NO_TRANSLATE;
    }
    *flags &= ~RF_CODE_EXECUTED;
#endif
}

/* 00000000, 10000000, A4000000: ROM and RAM */
uint8_t memory_read_byte(uint32_t addr) {
    uint8_t *ptr = phys_mem_ptr(addr, 1);
    if (!ptr) return bad_read_byte(addr);
    if (RAM_FLAGS((size_t)ptr & ~3) & DO_READ_ACTION) read_action(ptr);
    return *ptr;
}
uint16_t memory_read_half(uint32_t addr) {
    uint16_t *ptr = phys_mem_ptr(addr, 2);
    if (!ptr) return bad_read_half(addr);
    if (RAM_FLAGS((size_t)ptr & ~3) & DO_READ_ACTION) read_action(ptr);
    return *ptr;
}
uint32_t memory_read_word(uint32_t addr) {
    uint32_t *ptr = phys_mem_ptr(addr, 4);
    if (!ptr) return bad_read_word(addr);
    if (RAM_FLAGS(ptr) & DO_READ_ACTION) read_action(ptr);
    return *ptr;
}
void memory_write_byte(uint32_t addr, uint8_t value) {
    uint8_t *ptr = phys_mem_ptr(addr, 1);
    if (!ptr) { bad_write_byte(addr, value); return; }
    uint32_t flags = RAM_FLAGS((size_t)ptr & ~3);
    if (flags & RF_READ_ONLY) { bad_write_byte(addr, value); return; }
    if (flags & DO_WRITE_ACTION) write_action(ptr);
    *ptr = value;
}
void memory_write_half(uint32_t addr, uint16_t value) {
    uint16_t *ptr = phys_mem_ptr(addr, 2);
    if (!ptr) { bad_write_half(addr, value); return; }
    uint32_t flags = RAM_FLAGS((size_t)ptr & ~3);
    if (flags & RF_READ_ONLY) { bad_write_half(addr, value); return; }
    if (flags & DO_WRITE_ACTION) write_action(ptr);
    *ptr = value;
}
void memory_write_word(uint32_t addr, uint32_t value) {
    uint32_t *ptr = phys_mem_ptr(addr, 4);
    if (!ptr) { bad_write_word(addr, value); return; }
    uint32_t flags = RAM_FLAGS(ptr);
    if (flags & RF_READ_ONLY) { bad_write_word(addr, value); return; }
    if (flags & DO_WRITE_ACTION) write_action(ptr);
    *ptr = value;
}

/* The APB (Advanced Peripheral Bus) hosts peripherals that do not require
 * high bandwidth. The bridge to the APB is accessed via addresses 90xxxxxx. */
/* The AMBA specification does not mention anything about transfer sizes in APB,
 * so probably all reads/writes are effectively 32 bit. */
struct apb_map_entry {
    uint32_t (*read)(uint32_t addr);
    void (*write)(uint32_t addr, uint32_t value);
} apb_map[0x16];
void apb_set_map(int entry, uint32_t (*read)(uint32_t addr), void (*write)(uint32_t addr, uint32_t value)) {
    apb_map[entry].read = read;
    apb_map[entry].write = write;
}
uint8_t apb_read_byte(uint32_t addr) {
    if (addr >= 0x90150000) return bad_read_byte(addr);
    return apb_map[addr >> 16 & 31].read(addr & ~3) >> ((addr & 3) << 3);
}
uint16_t apb_read_half(uint32_t addr) {
    if (addr >= 0x90150000) return bad_read_half(addr);
    return apb_map[addr >> 16 & 31].read(addr & ~2) >> ((addr & 2) << 3);
}
uint32_t apb_read_word(uint32_t addr) {
    if (addr >= 0x90150000) return bad_read_word(addr);
    return apb_map[addr >> 16 & 31].read(addr);
}
void apb_write_byte(uint32_t addr, uint8_t value) {
    if (addr >= 0x90150000) { bad_write_byte(addr, value); return; }
    apb_map[addr >> 16 & 31].write(addr & ~3, value * 0x01010101u);
}
void apb_write_half(uint32_t addr, uint16_t value) {
    if (addr >= 0x90150000) { bad_write_half(addr, value); return; }
    apb_map[addr >> 16 & 31].write(addr & ~2, value * 0x00010001u);
}
void apb_write_word(uint32_t addr, uint32_t value) {
    if (addr >= 0x90150000) { bad_write_word(addr, value); return; }
    apb_map[addr >> 16 & 31].write(addr, value);
}

uint32_t FASTCALL mmio_read_byte(uint32_t addr) {
    return read_byte_map[addr >> 26](addr);
}
uint32_t FASTCALL mmio_read_half(uint32_t addr) {
    return read_half_map[addr >> 26](addr);
}
uint32_t FASTCALL mmio_read_word(uint32_t addr) {
    return read_word_map[addr >> 26](addr);
}
void FASTCALL mmio_write_byte(uint32_t addr, uint32_t value) {
    write_byte_map[addr >> 26](addr, value);
}
void FASTCALL mmio_write_half(uint32_t addr, uint32_t value) {
    write_half_map[addr >> 26](addr, value);
}
void FASTCALL mmio_write_word(uint32_t addr, uint32_t value) {
    write_word_map[addr >> 26](addr, value);
}

uint8_t FASTCALL nope_read_byte(uint32_t addr) {
    return 0;
}
uint16_t FASTCALL nope_read_half(uint32_t addr) {
    return 0;
}
uint32_t FASTCALL nope_read_word(uint32_t addr) {
    return 0;
}
void FASTCALL nope_write_byte(uint32_t addr, uint8_t value) {
}
void FASTCALL nope_write_half(uint32_t addr, uint16_t value) {
}
void FASTCALL nope_write_word(uint32_t addr, uint32_t value) {
}

void (*reset_procs[20])(void);
unsigned int reset_proc_count;

void add_reset_proc(void (*proc)(void))
{
    if (reset_proc_count == sizeof(reset_procs)/sizeof(*reset_procs))
        abort();
    reset_procs[reset_proc_count++] = proc;
}

static uint32_t current_product = 0;

bool memory_initialize(uint32_t sdram_size)
{
    // If the memory size or product differ, reinitialize
    if(mem_and_flags && (sdram_size != mem_areas[1].size || product != current_product))
        memory_deinitialize();

    if(mem_and_flags)
        return true;

    uint32_t total_mem = 0;
    int i;

    mem_and_flags = os_reserve(MEM_MAXSIZE * 2);
    if(!mem_and_flags)
    {
        emuprintf("os_reserve failed!\n");
        return false;
    }

    // Boot ROM
    mem_areas[0].base = 0x0;
    mem_areas[0].size = 0x80000;

    // SDRAM
    mem_areas[1].base = 0x10000000;
    mem_areas[1].size = sdram_size;

    if (emulate_casplus)
    {
        mem_areas[2].base = 0x20000000;
        mem_areas[2].size = 0x40000;
    }
    else if (emulate_cx2)
    {
        mem_areas[2].base = 0xA4000000;
        mem_areas[2].size = 0x40000; // Double of CX

        mem_areas[3].base = 0xA8000000;
        mem_areas[3].size = 320 * 240 * 2; // One RGB565 frame
    }
    else
    {
        // Classic and CX
        mem_areas[2].base = 0xA4000000;
        mem_areas[2].size = 0x20000;
    }

    for (i = 0; i != sizeof(mem_areas)/sizeof(*mem_areas); i++) {
        if (mem_areas[i].size) {
            mem_areas[i].ptr = mem_and_flags + total_mem;
            total_mem += mem_areas[i].size;
        }
    }

    assert (total_mem <= MEM_MAXSIZE);

    current_product = product;

    if (product == 0x0D0) {
        // Lab cradle OS reads calibration data from F007xxxx,
        // probably a mirror of ROM at 0007xxxx
        mem_areas[3].base = 0xF0000000;
        mem_areas[3].size = mem_areas[0].size;
        mem_areas[3].ptr = mem_areas[0].ptr;
    }

    if (emulate_cx2)
    {
        mem_areas[4].base = 0xA0000000;
        mem_areas[4].size = mem_areas[0].size;
        mem_areas[4].ptr = mem_areas[0].ptr;
    }

    for (int i = 0; i < 64; i++) {
        // will fallback to bad_* on non-memory addresses
        read_byte_map[i] = memory_read_byte;
        read_half_map[i] = memory_read_half;
        read_word_map[i] = memory_read_word;
        write_byte_map[i] = memory_write_byte;
        write_half_map[i] = memory_write_half;
        write_word_map[i] = memory_write_word;
    }

    if (emulate_casplus) {
        read_byte_map[0x08 >> 2] = casplus_nand_read_byte;
        read_half_map[0x08 >> 2] = casplus_nand_read_half;
        write_byte_map[0x08 >> 2] = casplus_nand_write_byte;
        write_half_map[0x08 >> 2] = casplus_nand_write_half;

        read_byte_map[0xFF >> 2] = omap_read_byte;
        read_half_map[0xFF >> 2] = omap_read_half;
        read_word_map[0xFF >> 2] = omap_read_word;
        write_byte_map[0xFF >> 2] = omap_write_byte;
        write_half_map[0xFF >> 2] = omap_write_half;
        write_word_map[0xFF >> 2] = omap_write_word;

        add_reset_proc(casplus_reset);
        return true;
    }

    read_byte_map[0x90 >> 2] = apb_read_byte;
    read_half_map[0x90 >> 2] = apb_read_half;
    read_word_map[0x90 >> 2] = apb_read_word;
    write_byte_map[0x90 >> 2] = apb_write_byte;
    write_half_map[0x90 >> 2] = apb_write_half;
    write_word_map[0x90 >> 2] = apb_write_word;
    for (size_t i = 0; i < sizeof(apb_map)/sizeof(*apb_map); i++) {
        apb_map[i].read = bad_read_word;
        apb_map[i].write = bad_write_word;
    }

    apb_set_map(0x00, gpio_read, gpio_write);
    add_reset_proc(gpio_reset);
    apb_set_map(0x06, watchdog_read, watchdog_write);
    add_reset_proc(watchdog_reset);
    apb_set_map(0x09, rtc_read, rtc_write);
    add_reset_proc(rtc_reset);
    apb_set_map(0x0A, misc_read, misc_write);
    apb_set_map(0x0E, keypad_read, keypad_write);
    add_reset_proc(keypad_reset);
    apb_set_map(0x0F, hdq1w_read, hdq1w_write);

    add_reset_proc(hdq1w_reset);
    apb_set_map(0x11, unknown_9011_read, unknown_9011_write);

    read_byte_map[0xAC >> 2] = sdio_read_byte;
    read_half_map[0xAC >> 2] = sdio_read_half;
    read_word_map[0xAC >> 2] = sdio_read_word;
    write_byte_map[0xAC >> 2] = sdio_write_byte;
    write_half_map[0xAC >> 2] = sdio_write_half;
    write_word_map[0xAC >> 2] = sdio_write_word;

    if(!emulate_cx2)
    {
        read_byte_map[0xB0 >> 2] = usb_read_byte;
        read_half_map[0xB0 >> 2] = usb_read_half;
        read_word_map[0xB0 >> 2] = usb_read_word;
        write_word_map[0xB0 >> 2] = usb_write_word;

        //TODO: It's a different controller, but for now we use the same state
        read_byte_map[0xB4 >> 2] = usb_read_byte;
        read_half_map[0xB4 >> 2] = usb_read_half;
        read_word_map[0xB4 >> 2] = usb_read_word;
        write_word_map[0xB4 >> 2] = usb_write_word;
    }
    else
    {
        read_byte_map[0xB0 >> 2] = usb_cx2_read_byte;
        read_half_map[0xB0 >> 2] = usb_cx2_read_half;
        read_word_map[0xB0 >> 2] = usb_cx2_read_word;
        write_word_map[0xB0 >> 2] = usb_cx2_write_word;

        read_byte_map[0xB4 >> 2] = nope_read_byte;
        read_half_map[0xB4 >> 2] = nope_read_half;
        read_word_map[0xB4 >> 2] = nope_read_word;
        write_word_map[0xB4 >> 2] = nope_write_word;
    }
    add_reset_proc(usb_reset);
    add_reset_proc(usblink_reset);

    read_word_map[0xBC >> 2] = unknown_BC_read_word;
    write_word_map[0xBC >> 2] = unknown_BC_write_word;

    read_word_map[0xC0 >> 2] = lcd_read_word;
    write_word_map[0xC0 >> 2] = lcd_write_word;
    add_reset_proc(lcd_reset);

    read_word_map[0xC4 >> 2] = adc_read_word;
    write_word_map[0xC4 >> 2] = adc_write_word;
    add_reset_proc(adc_reset);

    des_initialize();
    read_word_map[0xC8 >> 2] = des_read_word;
    write_word_map[0xC8 >> 2] = des_write_word;
    add_reset_proc(des_reset);

    read_word_map[0xCC >> 2] = sha256_read_word;
    write_word_map[0xCC >> 2] = sha256_write_word;
    add_reset_proc(sha256_reset);

    if (!emulate_cx) {
        read_byte_map[0x08 >> 2] = nand_phx_raw_read_byte;
        write_byte_map[0x08 >> 2] = nand_phx_raw_write_byte;

        write_word_map[0x8F >> 2] = sdramctl_write_word;

        apb_set_map(0x01, timer_read, timer_write);
        apb_set_map(0x0B, pmu_read, pmu_write);
        add_reset_proc(pmu_reset);
        apb_set_map(0x0C, timer_read, timer_write);
        apb_set_map(0x0D, timer_read, timer_write);
        add_reset_proc(timer_reset);
        apb_set_map(0x02, serial_read, serial_write);
        add_reset_proc(serial_reset);
        apb_set_map(0x08, bad_read_word, unknown_9008_write);
        apb_set_map(0x10, ti84_io_link_read, ti84_io_link_write);
        add_reset_proc(ti84_io_link_reset);

        read_word_map[0xA9 >> 2] = spi_read_word;
        write_word_map[0xA9 >> 2] = spi_write_word;

        read_word_map[0xB8 >> 2] = nand_phx_read_word;
        write_word_map[0xB8 >> 2] = nand_phx_write_word;
        add_reset_proc(nand_phx_reset);

        read_word_map[0xDC >> 2] = int_read_word;
        write_word_map[0xDC >> 2] = int_write_word;
        add_reset_proc(int_reset);
    } else {
        apb_set_map(0x01, timer_cx_read, timer_cx_write);
        apb_set_map(0x0C, timer_cx_read, timer_cx_write);
        apb_set_map(0x0D, timer_cx_read, timer_cx_write);
        add_reset_proc(timer_cx_reset);
        apb_set_map(0x02, serial_cx_read, serial_cx_write);
        add_reset_proc(serial_cx_reset);
        apb_set_map(0x03, fastboot_cx_read, fastboot_cx_write);
        apb_set_map(0x05, touchpad_cx_read, touchpad_cx_write);
        add_reset_proc(touchpad_cx_reset);

        if(emulate_cx2)
        {
            // Second serial port - for now use the same state
            apb_set_map(0x04, cx2_lcd_spi_read, cx2_lcd_spi_write);
            apb_set_map(0x07, serial_cx_read, serial_cx_write);
            apb_set_map(0x08, unknown_9008_read, unknown_9008_write);
            apb_set_map(0x0B, adc_cx2_read_word, adc_cx2_write_word);
            apb_set_map(0x12, memc_ddr_read, memc_ddr_write);
            apb_set_map(0x13, bad_read_word, cx2_backlight_write);
            apb_set_map(0x14, aladdin_pmu_read, aladdin_pmu_write);
            add_reset_proc(aladdin_pmu_reset);

            read_word_map[0xB8 >> 2] = spinand_cx2_read_word;
            read_byte_map[0xB8 >> 2] = spinand_cx2_read_byte;
            write_word_map[0xB8 >> 2] = spinand_cx2_write_word;
            write_byte_map[0xB8 >> 2] = spinand_cx2_write_byte;
            add_reset_proc(flash_spi_reset);
        }
        else
        {
            read_word_map[0x8F >> 2] = memctl_cx_read_word;
            write_word_map[0x8F >> 2] = memctl_cx_write_word;
            add_reset_proc(memctl_cx_reset);

            apb_set_map(0x04, spi_cx_read, spi_cx_write);
            apb_set_map(0x0B, pmu_read, pmu_write);
            add_reset_proc(pmu_reset);

            read_byte_map[0x80 >> 2] = nand_cx_read_byte;
            read_word_map[0x80 >> 2] = nand_cx_read_word;
            write_byte_map[0x80 >> 2] = nand_cx_write_byte;
            write_word_map[0x80 >> 2] = nand_cx_write_word;

            read_word_map[0xB8 >> 2] = sramctl_read_word;
            write_word_map[0xB8 >> 2] = sramctl_write_word;
        }

        read_word_map[0xDC >> 2] = int_cx_read_word;
        write_word_map[0xDC >> 2] = int_cx_write_word;
        add_reset_proc(int_reset);
    }

    return true;
}

void memory_reset()
{
    for(unsigned int i = 0; i < reset_proc_count; i++)
        reset_procs[i]();
}

void memory_deinitialize()
{
    if(mem_and_flags)
    {
        // translation_table uses absolute addresses
        flush_translations();
        memset(mem_areas, 0, sizeof(mem_areas));
        os_free(mem_and_flags, MEM_MAXSIZE * 2);
        mem_and_flags = NULL;
    }

    reset_proc_count = 0;
}

bool memory_suspend(emu_snapshot *snapshot)
{
    assert(mem_and_flags);

    snapshot->mem.sdram_size = mem_areas[1].size;
    memcpy(snapshot->mem.mem_and_flags, mem_and_flags, MEM_MAXSIZE);

    return gpio_suspend(snapshot)
            && fastboot_cx_suspend(snapshot)
            && watchdog_suspend(snapshot)
            && rtc_suspend(snapshot)
            && pmu_suspend(snapshot)
            && keypad_suspend(snapshot)
            && hdq1w_suspend(snapshot)
            && usb_suspend(snapshot)
            && lcd_suspend(snapshot)
            && adc_suspend(snapshot)
            && des_suspend(snapshot)
            && sha256_suspend(snapshot)
            && timer_suspend(snapshot)
            && serial_suspend(snapshot)
            && interrupt_suspend(snapshot)
            && memctl_cx_suspend(snapshot)
            && serial_cx_suspend(snapshot)
            && timer_cx_suspend(snapshot);
}

bool memory_resume(const emu_snapshot *snapshot)
{
    if(!memory_initialize(snapshot->mem.sdram_size))
        return false;

    memory_reset();

    memcpy(mem_and_flags, snapshot->mem.mem_and_flags, MEM_MAXSIZE);
    memset(mem_and_flags + MEM_MAXSIZE, 0, MEM_MAXSIZE); // Set all flags to 0

    return gpio_resume(snapshot)
            && fastboot_cx_resume(snapshot)
            && watchdog_resume(snapshot)
            && rtc_resume(snapshot)
            && pmu_resume(snapshot)
            && keypad_resume(snapshot)
            && hdq1w_resume(snapshot)
            && usb_resume(snapshot)
            && lcd_resume(snapshot)
            && adc_resume(snapshot)
            && des_resume(snapshot)
            && sha256_resume(snapshot)
            && timer_resume(snapshot)
            && serial_resume(snapshot)
            && interrupt_resume(snapshot)
            && memctl_cx_resume(snapshot)
            && serial_cx_resume(snapshot)
            && timer_cx_resume(snapshot);
}
