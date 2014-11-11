#include "asmcode.h"
#include "debug.h"
#include "mmu.h"
#include "mem.h"

#define likely(x) (__builtin_expect(x, 1))
#define unlikely(x) (__builtin_expect(x, 0))

//TODO: Read and write breakpoints

void *translation_next, *translation_next_bx, *arm_shift_proc;

uint32_t FASTCALL read_word_ldr(uint32_t addr)
{
    uint32_t entry = *(uint32_t*)(addr_cache + ((addr >> 10) << 1));
    uintptr_t located_addr = entry + addr;

    //If the sum doesn't contain the address directly
    if(unlikely(located_addr & (AC_NOT_PTR | 0b11)))
    {
        if(entry & (1 << 22)) //Invalid entry
        {
            addr_cache_miss(addr, false, data_abort);
            return read_word_ldr(addr);
        }
        else //Physical address
        {
            entry <<= 10;
            located_addr = addr + entry;
            return mmio_read_word(located_addr);
        }
    }

    return *(uint32_t*)located_addr;
}

uint32_t FASTCALL read_byte(uint32_t addr)
{
    uint32_t entry = *(uint32_t*)(addr_cache + ((addr >> 10) << 1));
    uintptr_t located_addr = entry + addr;

    //If the sum doesn't contain the address directly
    if(located_addr & AC_NOT_PTR)
    {
        if(entry & (1 << 22)) //Invalid entry
        {
            addr_cache_miss(addr, false, data_abort);
            return read_byte(addr);
        }
        else //Physical address
        {
            entry <<= 10;
            located_addr = addr + entry;
            return mmio_read_byte(located_addr);
        }
    }

    return *(uint8_t*)located_addr;
}

uint32_t FASTCALL read_half(uint32_t addr)
{
    uint32_t entry = *(uint32_t*)(addr_cache + ((addr >> 10) << 1));
    uintptr_t located_addr = entry + addr;

    //If the sum doesn't contain the address directly
    if(located_addr & (AC_NOT_PTR | 0b01))
    {
        if(entry & (1 << 22)) //Invalid entry
        {
            addr_cache_miss(addr, false, data_abort);
            return read_half(addr);
        }
        else //Physical address
        {
            entry <<= 10;
            located_addr = addr + entry;
            return mmio_read_half(located_addr);
        }
    }

    return *(uint16_t*)located_addr;
}

uint32_t FASTCALL read_word(uint32_t addr)
{
    return read_word_ldr(addr);
}

void FASTCALL write_byte(uint32_t addr, uint32_t value)
{
    uint32_t entry = *(uint32_t*)(addr_cache + ((addr >> 10) << 1));
    uintptr_t located_addr = entry + addr;

    //If the sum doesn't contain the address directly
    if(located_addr & AC_NOT_PTR)
    {
        if(entry & (1 << 22)) //Invalid entry
        {
            addr_cache_miss(addr, false, data_abort);
            return write_byte(addr, value);
        }
        else //Physical address
        {
            entry <<= 10;
            located_addr = addr + entry;
            return mmio_write_byte(located_addr, value);
        }
    }

    *(uint8_t*)located_addr = value;
}

void FASTCALL write_half(uint32_t addr, uint32_t value)
{
    uint32_t entry = *(uint32_t*)(addr_cache + ((addr >> 10) << 1));
    uintptr_t located_addr = entry + addr;

    //If the sum doesn't contain the address directly
    if(located_addr & (AC_NOT_PTR | 0b01))
    {
        if(entry & (1 << 22)) //Invalid entry
        {
            addr_cache_miss(addr, false, data_abort);
            return write_half(addr, value);
        }
        else //Physical address
        {
            entry <<= 10;
            located_addr = addr + entry;
            return mmio_write_half(located_addr, value);
        }
    }

    *(uint16_t*)located_addr = value;
}

void FASTCALL write_word(uint32_t addr, uint32_t value)
{
    uint32_t entry = *(uint32_t*)(addr_cache + ((addr >> 10) << 1));
    uintptr_t located_addr = entry + addr;

    //If the sum doesn't contain the address directly
    if(located_addr & (AC_NOT_PTR | 0b11))
    {
        if(entry & (1 << 22)) //Invalid entry
        {
            addr_cache_miss(addr, false, data_abort);
            return write_word(addr, value);
        }
        else //Physical address
        {
            entry <<= 10;
            located_addr = addr + entry;
            return mmio_write_word(located_addr, value);
        }
    }

    *(uint32_t*)located_addr = value;
}
