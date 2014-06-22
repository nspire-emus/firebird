#ifndef _H_MMU
#define _H_MMU

#include "emu.h"

/* Declarations for mmu.c */

/* Translate a VA to a PA, using a page table lookup */
uint32_t mmu_translate(uint32_t addr, bool writing, fault_proc *fault);

/* Table for quickly accessing RAM and ROM by virtual addresses. This contains
 * two entries for each 1kB of virtual address space, one for reading and one
 * for writing, and each entry may contain one of three kinds of values:
 *
 * a) Pointer entry
 *    The result of adding the virtual address (VA) to the entry has bit 31
 *    clear, and that sum is a pointer to within mem_and_flags.
 *    It would be cleaner to just use bit 0 or 1 to distinguish this case, but
 *    this way we can simultaneously check both that this is a pointer entry,
 *    and that the address is aligned, with a single bit test instruction.
 * b) Physical address entry
 *    VA + entry has bit 31 set, and the entry (not the sum) has bit 22 clear.
 *    Bits 0-21 contain the difference between virtual and physical address.
 * c) Invalid entry
 *    VA + entry has bit 31 set, entry has bit 22 set. Entry is invalid and
 *    addr_cache_miss must be called.
 */

#define AC_NUM_ENTRIES (4194304*2)
typedef uint8_t *ac_entry;
extern ac_entry *addr_cache;
#define AC_SET_ENTRY_PTR(entry, va, ptr) \
        entry = (ptr) - (va);
#define AC_NOT_PTR 0x80000000
#define AC_SET_ENTRY_PHYS(entry, va, pa) \
        entry = (ac_entry)(((pa) - (va)) >> 10); \
        entry += (~(uint32_t)((va) + entry) & AC_NOT_PTR);
#define AC_SET_ENTRY_INVALID(entry, va) \
        entry = (ac_entry)(1 << 22); \
        entry += (~(uint32_t)((va) + entry) & AC_NOT_PTR);

bool addr_cache_pagefault(void *addr);
void *addr_cache_miss(uint32_t addr, bool writing, fault_proc *fault) __asm__("addr_cache_miss");
void addr_cache_flush();
void *mmu_save_state(size_t *size);
void mmu_reload_state(void *state);

#endif
