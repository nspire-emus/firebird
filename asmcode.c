#include "asmcode.h"
#include "debug.h"
#include "mmu.h"
#include "mem.h"

#define likely(x) (__builtin_expect(x, 1))
#define unlikely(x) (__builtin_expect(x, 0))

//TODO: Read and write breakpoints

#if (!defined(__i386__) && !defined(__x86_64__)) || defined(NO_TRANSLATION)
void flush_translations() {}
void fix_pc_for_fault() {}
bool range_translated(uintptr_t x, uintptr_t y) { (void) x; (void) y; return false; }
#endif

void * FASTCALL ptr(uint32_t addr)
{
    uintptr_t entry = *(uintptr_t*)(addr_cache + ((addr >> 10) << 1));

    if(entry & AC_FLAGS)
    {
        if(entry & AC_INVALID)
        {
            addr_cache_miss(addr, false, prefetch_abort);
            return ptr(addr);
        }
        else
            return 0;
    }

    entry += addr;
    return (void*)entry;
}

uint32_t FASTCALL read_word_ldr(uint32_t addr)
{
    uintptr_t entry = *(uintptr_t*)(addr_cache + ((addr >> 10) << 1));

    //If the sum doesn't contain the address directly
    if(entry & AC_FLAGS)
    {
        if(entry & AC_INVALID) //Invalid entry
        {
            addr_cache_miss(addr, false, data_abort);
            return read_word_ldr(addr);
        }
        else //Physical address
        {
            entry &= ~AC_FLAGS;
            entry += addr;
            return mmio_read_word(entry);
        }
    }

    entry += addr;

    return *(uint32_t*)entry;
}

uint32_t FASTCALL read_byte(uint32_t addr)
{
    uintptr_t entry = *(uintptr_t*)(addr_cache + ((addr >> 10) << 1));

    //If the sum doesn't contain the address directly
    if(entry & AC_FLAGS)
    {
        if(entry & AC_INVALID) //Invalid entry
        {
            addr_cache_miss(addr, false, data_abort);
            return read_byte(addr);
        }
        else //Physical address
        {
            entry &= ~AC_FLAGS;
            entry += addr;
            return mmio_read_byte(entry);
        }
    }

    entry += addr;

    return *(uint8_t*)entry;
}

uint32_t FASTCALL read_half(uint32_t addr)
{
    uintptr_t entry = *(uintptr_t*)(addr_cache + ((addr >> 10) << 1));

    //If the sum doesn't contain the address directly
    if(entry & AC_FLAGS)
    {
        if(entry & AC_INVALID) //Invalid entry
        {
            addr_cache_miss(addr, false, data_abort);
            return read_half(addr);
        }
        else //Physical address
        {
            entry &= ~AC_FLAGS;
            entry += addr;
            return mmio_read_half(entry);
        }
    }

    entry += addr;

    return *(uint16_t*)entry;
}

uint32_t FASTCALL read_word(uint32_t addr)
{
    return read_word_ldr(addr);
}

void FASTCALL write_byte(uint32_t addr, uint32_t value)
{
    uintptr_t entry = *(uintptr_t*)(addr_cache + ((addr >> 10) << 1) + 1);

    //If the sum doesn't contain the address directly
    if(entry & AC_NOT_PTR)
    {
        if(entry & AC_INVALID) //Invalid entry
        {
            addr_cache_miss(addr, true, data_abort);
            return write_byte(addr, value);
        }
        else //Physical address
        {
            entry &= ~AC_FLAGS;
            entry += addr;
            return mmio_write_byte(entry, value);
        }
    }

    entry += addr;

    *(uint8_t*)entry = value;
}

void FASTCALL write_half(uint32_t addr, uint32_t value)
{
    uintptr_t entry = *(uintptr_t*)(addr_cache + ((addr >> 10) << 1) + 1);

    //If the sum doesn't contain the address directly
    if(entry & AC_NOT_PTR)
    {
        if(entry & AC_INVALID) //Invalid entry
        {
            addr_cache_miss(addr, true, data_abort);
            return write_half(addr, value);
        }
        else //Physical address
        {
            entry &= ~AC_FLAGS;
            entry += addr;
            return mmio_write_half(entry, value);
        }
    }

    entry += addr;

    *(uint16_t*)entry = value;
}

void FASTCALL write_word(uint32_t addr, uint32_t value)
{
    uintptr_t entry = *(uintptr_t*)(addr_cache + ((addr >> 10) << 1) + 1);

    //If the sum doesn't contain the address directly
    if(entry & AC_NOT_PTR)
    {
        if(entry & AC_INVALID) //Invalid entry
        {
            addr_cache_miss(addr, true, data_abort);
            return write_word(addr, value);
        }
        else //Physical address
        {
            entry &= ~AC_FLAGS;
            entry += addr;
            return mmio_write_word(entry, value);
        }
    }

    entry += addr;

    *(uint32_t*)entry = value;
}
