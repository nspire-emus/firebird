#include <string.h>
#include <stdint.h>
#include "translate.h"
#include "emu.h"
#include "cpu.h"
#include "mmu.h"
#include "mem.h"
#include "os/os.h"

/* Copy of translation table in memory (hack to approximate effect of having a TLB) */
static uint32_t mmu_translation_table[0x1000];

void mmu_dump_tables(void) {
    if ((arm.control & 1) == 0) {
        gui_debug_printf("MMU disabled\n");
        return;
    }

    gui_debug_printf("MMU translations:\n");
    uint32_t *tt = (uint32_t*)phys_mem_ptr(arm.translation_table_base, 0x4000);
    if (!tt) {
        gui_debug_printf("TTB points to invalid memory, using TLB\n");
        tt = mmu_translation_table;
    }

    for (uint32_t i = 0; i < 0x1000; i++) {
        uint32_t tt_entry = tt[i];
        uint32_t virt_addr = i << 20;
        uint32_t virt_shift = 0;
        uint32_t *l1_table = NULL;
        uint32_t l1_table_size = 0;;
        uint32_t l1_entry;
        uint32_t j = 0;
        uint32_t page_size = 0;
        char *page_type = "?";
        if (!(tt_entry & 3))
            continue; // Invalid
        if ((tt_entry & 3) == 2) { // Section (1MB)
            page_size = 0x100000;
            page_type = "1mB";
            l1_entry = tt_entry;
            j = 0;
            goto section;
        } else if ((tt_entry & 3) == 1) { // Coarse page table
            l1_table = phys_mem_ptr(tt_entry & 0xFFFFFC00, 0x400);
            l1_table_size = 256;
            virt_shift = 12;
        } else if ((tt_entry & 3) == 3) { // Fine page table
            l1_table = phys_mem_ptr(tt_entry & 0xFFFFF000, 0x1000);
            l1_table_size = 1024;
            virt_shift = 10;
        }

        for (j = 0; j < l1_table_size; j++) {
            l1_entry = l1_table[j];
            if (!(l1_entry & 3))
                continue; // Invalid
            if ((l1_entry & 3) == 1) { // Large page (64kB)
                page_size = 0x10000;
                page_type = "64kB";
            }
            else if ((l1_entry & 3) == 2) { // Small page (4kB)
                page_size = 0x1000;
                page_type = "4kB";
            }
            else if ((l1_entry & 3) == 3) { // Tiny page (1kB)
                page_size = 0x400;
                page_type = "1kB";
            }
section:;
            gui_debug_printf("%08x -> %08x (%s) (0x%8x)\n", virt_addr + j * (1 << virt_shift), l1_entry & -page_size, page_type, l1_entry);
        }
    }
}

/* Translate a virtual address to a physical address */
uint32_t mmu_translate(uint32_t addr, bool writing, fault_proc *fault, uint8_t *s_status) {
    uint32_t page_size;
    if (!(arm.control & 1))
        return addr;

    uint32_t *table = mmu_translation_table;
    uint32_t entry = table[addr >> 20];
    uint32_t domain = entry >> 5 & 0x0F;
    uint32_t status = domain << 4;
    uint32_t ap;

    switch (entry & 3) {
        default: /* Invalid */
            if (s_status) *s_status = status + 0x5;
            if (fault) fault(addr, status + 0x5); /* Section translation fault */
            return 0xFFFFFFFF;
        case 1: /* Course page table (one entry per 4kB) */
            table = (uint32_t *)(intptr_t)phys_mem_ptr(entry & 0xFFFFFC00, 0x400);
            if (!table) {
                if (fault) error("Bad page table pointer");
                return 0xFFFFFFFF;
            }
            entry = table[addr >> 12 & 0xFF];
            break;
        case 2: /* Section (1MB) */
            page_size = 0x100000;
            ap = entry >> 6;
            goto section;
        case 3: /* Fine page table (one entry per 1kB) */
            table = (uint32_t *)(intptr_t)phys_mem_ptr(entry & 0xFFFFF000, 0x1000);
            if (!table) {
                if (fault) error("Bad page table pointer");
                return 0xFFFFFFFF;
            }
            entry = table[addr >> 10 & 0x3FF];
            break;
    }

    status += 2;
    switch (entry & 3) {
        default: /* Invalid */
            if (s_status) *s_status = status + 0x5;
            if (fault) fault(addr, status + 0x5); /* Page translation fault */
            return 0xFFFFFFFF;
        case 1: /* Large page (64kB) */
            page_size = 0x10000;
            ap = entry >> (addr >> 13 & 6);
            break;
        case 2: /* Small page (4kB) */
            page_size = 0x1000;
            ap = entry >> (addr >> 9 & 6);
            break;
        case 3: /* Tiny page (1kB) */
            page_size = 0x400;
            ap = entry;
            break;
    }
section:;

    uint32_t domain_access = arm.domain_access_control >> (domain << 1) & 3;
    if (domain_access != 3) {
        if (!(domain_access & 1)) {
            /* 0 (No access) or 2 (Reserved)
             * Testing shows they both raise domain fault */
            if (s_status) *s_status = status + 0x9;
            if (fault) fault(addr, status + 0x9); /* Domain fault */
            return 0xFFFFFFFF;
        }
        /* 1 (Client) - check access permission bits */
        switch (ap >> 4 & 3) {
            case 0: /* Controlled by S/R bits */
                switch (arm.control >> 8 & 3) {
                    case 0: /* No access */
                    case 3: /* Reserved - testing shows this behaves like 0 */
perm_fault:
                        if (s_status) *s_status = status + 0xD;
                        if (fault) fault(addr, status + 0xD); /* Permission fault */
                        return 0xFFFFFFFF;
                    case 1: /* System - read-only for privileged, no access for user */
                        if (USER_MODE() || writing)
                            goto perm_fault;
                        break;
                    case 2: /* ROM - read-only */
                        if (writing)
                            goto perm_fault;
                        break;
                }
                break;
            case 1: /* Read/write for privileged, no access for user */
                if (USER_MODE())
                    goto perm_fault;
                break;
            case 2: /* Read/write for privileged, read-only for user */
                if (writing && USER_MODE())
                    goto perm_fault;
                break;
            case 3: /* Read/write */
                break;
        }
    }

    return (entry & -page_size) | (addr & (page_size - 1));
}

void mmu_check_priv(uint32_t addr, bool writing)
{
    uint8_t status = 0;
    uint32_t saved_cpsr = arm.cpsr_low28;
    arm.cpsr_low28 &= ~3;
    mmu_translate(addr, writing, NULL, &status);
    arm.cpsr_low28 = saved_cpsr;
    if((status & 0xF) == 0xD)
        data_abort(addr, status);
}

ac_entry *addr_cache = NULL;

// Keep a list of valid entries so we can invalidate everything quickly
#define AC_VALID_MAX 256
static uint32_t ac_valid_index;
static uint32_t ac_valid_list[AC_VALID_MAX];

static void addr_cache_invalidate(int i) {
    AC_SET_ENTRY_INVALID(addr_cache[i], i >> 1 << 10)
}

#if OS_HAS_PAGEFAULT_HANDLER

/* Since only a small fraction of the virtual address space, and therefore
 * only a small fraction of the pages making up addr_cache, will be in use
 * at a time, we can keep only a few pages committed and thereby reduce
 * the memory used by a lot. */
#define AC_COMMIT_MAX 128
#define AC_PAGE_SIZE 4096

bool addr_cache_pagefault(void *addr) {
    static uint8_t ac_commit_map[AC_NUM_ENTRIES * sizeof(ac_entry) / AC_PAGE_SIZE];
    static ac_entry *ac_commit_list[AC_COMMIT_MAX];
    static uint32_t ac_commit_index;

    ac_entry *page = (ac_entry *)((uintptr_t)addr & -AC_PAGE_SIZE);
    uint32_t offset = page - addr_cache;
    if (offset >= AC_NUM_ENTRIES)
        return false;
    ac_entry *oldpage = ac_commit_list[ac_commit_index];
    if (oldpage) {
        //printf("Freeing %p, ", oldpage);
        os_sparse_decommit(oldpage, AC_PAGE_SIZE);
        ac_commit_map[offset / (AC_PAGE_SIZE / sizeof(ac_entry))] = 0;
    }
    //printf("Committing %p\n", page);
    if (!os_sparse_commit(page, AC_PAGE_SIZE))
        return false;
    ac_commit_map[offset / (AC_PAGE_SIZE / sizeof(ac_entry))] = 1;

    uint32_t i;
    for (i = 0; i < (AC_PAGE_SIZE / sizeof(ac_entry)); i++)
        addr_cache_invalidate(offset + i);

    ac_commit_list[ac_commit_index] = page;
    ac_commit_index = (ac_commit_index + 1) % AC_COMMIT_MAX;
    return true;
}

#endif

void *addr_cache_miss(uint32_t virt, bool writing, fault_proc *fault) {
    ac_entry entry;
    uintptr_t phys = mmu_translate(virt, writing, fault, NULL);
    uint8_t *ptr = phys_mem_ptr(phys, 1);
    if (ptr && !(writing && (RAM_FLAGS((size_t)ptr & ~3) & RF_READ_ONLY))) {
        AC_SET_ENTRY_PTR(entry, virt, ptr)
                //printf("addr_cache_miss VA=%08x ptr=%p entry=%p\n", virt, ptr, entry);
    } else {
        AC_SET_ENTRY_PHYS(entry, virt, phys)
                //printf("addr_cache_miss VA=%08x PA=%08x entry=%p\n", virt, phys, entry);
    }
    uint32_t oldoffset = ac_valid_list[ac_valid_index];
    uint32_t offset = (virt >> 10) * 2 + writing;
    //if (ac_commit_map[oldoffset / (AC_PAGE_SIZE / sizeof(ac_entry))])
    addr_cache_invalidate(oldoffset);
    addr_cache[offset] = entry;
    ac_valid_list[ac_valid_index] = offset;
    ac_valid_index = (ac_valid_index + 1) % AC_VALID_MAX;
    return ptr;
}

void addr_cache_flush() {
    uint32_t i;

    if (arm.control & 1) {
        void *table = phys_mem_ptr(arm.translation_table_base, 0x4000);
        if (!table)
            error("Bad translation table base register: %x", arm.translation_table_base);
        memcpy(mmu_translation_table, table, 0x4000);
    }

    for (i = 0; i < AC_VALID_MAX; i++) {
        uint32_t offset = ac_valid_list[i];
        //	if (ac_commit_map[offset / (AC_PAGE_SIZE / sizeof(ac_entry))])
        addr_cache_invalidate(offset);
    }

    flush_translations();
}
