/* Declarations for memory.c */

#ifndef _H_MEM
#define _H_MEM

#include <stdint.h>

#include "emu.h"

#define MEM_MAXSIZE (65*1024*1024) // also defined as RAM_FLAGS in asmcode.S

// Must be allocated below 2GB (see comments for mmu.c)
extern uint8_t *mem_and_flags;
struct mem_area_desc {
    uint32_t base, size;
    uint8_t *ptr;
};
extern struct mem_area_desc mem_areas[4];
void *phys_mem_ptr(uint32_t addr, uint32_t size);

/* Each word of memory has a flag word associated with it. For fast access,
 * flags are located at a constant offset from the memory data itself.
 *
 * These can't be per-byte because a translation index wouldn't fit then.
 * This does mean byte/halfword accesses have to mask off the low bits to
 * check flags, but the alternative would be another 32MB of memory overhead. */
#define RAM_FLAGS(memptr) (*(uint32_t *)((uint8_t *)(memptr) + MEM_MAXSIZE))

#define RF_READ_BREAKPOINT   1
#define RF_WRITE_BREAKPOINT  2
#define RF_EXEC_BREAKPOINT   4
#define RF_EXEC_DEBUG_NEXT   8
#define RF_EXEC_HACK         16
#define RF_CODE_TRANSLATED   32
#define RF_CODE_NO_TRANSLATE 64
#define RF_READ_ONLY         128
#define RF_ARMLOADER_CB      256
#define RFS_TRANSLATION_INDEX 9

uint8_t bad_read_byte(uint32_t addr);
uint16_t bad_read_half(uint32_t addr);
uint32_t bad_read_word(uint32_t addr);
void bad_write_byte(uint32_t addr, uint8_t value);
void bad_write_half(uint32_t addr, uint16_t value);
void bad_write_word(uint32_t addr, uint32_t value);

void write_action(void *ptr) __asm__("write_action");
void read_action(void *ptr) __asm__("read_action");

uint32_t FASTCALL mmio_read_byte(uint32_t addr) __asm__("mmio_read_byte");
uint32_t FASTCALL mmio_read_half(uint32_t addr) __asm__("mmio_read_half");
uint32_t FASTCALL mmio_read_word(uint32_t addr) __asm__("mmio_read_word");
void FASTCALL mmio_write_byte(uint32_t addr, uint32_t value) __asm__("mmio_write_byte");
void FASTCALL mmio_write_half(uint32_t addr, uint32_t value) __asm__("mmio_write_half");
void FASTCALL mmio_write_word(uint32_t addr, uint32_t value) __asm__("mmio_write_word");

bool memory_initialize(uint32_t sdram_size);
void memory_deinitialize();

#endif
