#define _GNU_SOURCE
#define _XOPEN_SOURCE

#include <sys/mman.h>
#include <stdio.h>
#include <sys/time.h>
#include <unistd.h>
#ifdef __APPLE__
#include <mach/clock.h>
#include <mach/mach.h>
#endif

#include "os.h"
#include "../debug.h"
#include "../mmu.h"

void *os_reserve(size_t size)
{
#ifdef __i386__
    // Has to have bit 31 zero
    void *ptr = mmap((void*)0x70000000, size, PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANON, -1, 0);
#else
    void *ptr = mmap((void*)0, size, PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANON, -1, 0);
#endif

    if((intptr_t)ptr == -1)
        return NULL;

    msync(ptr, size, MS_SYNC|MS_INVALIDATE);
    return ptr;
}

void os_free(void *ptr, size_t size)
{
    if(ptr)
        munmap(ptr, size);
}

void *os_commit(void *addr, size_t size)
{
    void *ptr = mmap(addr, size, PROT_READ|PROT_WRITE, MAP_FIXED|MAP_SHARED|MAP_ANON, -1, 0);
    msync(addr, size, MS_SYNC|MS_INVALIDATE);
    return ptr;
}

void *os_sparse_commit(void *page, size_t size)
{
    void *ptr = mmap(page, size, PROT_READ|PROT_WRITE, MAP_FIXED|MAP_SHARED|MAP_ANON, -1, 0);
    msync(page, size, MS_SYNC|MS_INVALIDATE);
    return ptr;
}

void os_sparse_decommit(void *page, size_t size)
{
    // instead of unmapping the address, we're just gonna trick
    // the TLB to mark this as a new mapped area which, due to
    // demand paging, will not be committed until used.
    mmap(page, size, PROT_NONE, MAP_FIXED|MAP_PRIVATE|MAP_ANON, -1, 0);
    msync(page, size, MS_SYNC|MS_INVALIDATE);
}

void *os_alloc_executable(size_t size)
{
#if defined(__i386__) || defined(__x86_64__)
    // Has to be in 32-bit space for the JIT
    void *ptr = mmap((void*)0x30000000, size, PROT_READ|PROT_WRITE|PROT_EXEC, MAP_SHARED|MAP_ANON, -1, 0);
#else
    void *ptr = mmap((void*)0x0, size, PROT_READ|PROT_WRITE|PROT_EXEC, MAP_SHARED|MAP_ANON, -1, 0);
#endif

    if((intptr_t)ptr == -1)
        return NULL;

    msync(ptr, size, MS_SYNC|MS_INVALIDATE);
    return ptr;
}

void make_writable(void *addr)
{
    uintptr_t ps = sysconf(_SC_PAGE_SIZE);
    void *prev = (void*)((uintptr_t)(addr) & (~(ps - 1)));
    if(mprotect(prev, ps, PROT_READ | PROT_WRITE | PROT_EXEC) != 0)
        emuprintf("mprotect failed.\n");
}

void addr_cache_init(os_exception_frame_t *frame)
{
    (void) frame;

    // Only run this if not already initialized
    if(addr_cache)
        return;

    addr_cache = mmap((void*)0, AC_NUM_ENTRIES * sizeof(ac_entry), PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANON, -1, 0);
    if(addr_cache == MAP_FAILED)
    {
        addr_cache = NULL;
        fprintf(stderr, "Failed to mmap addr_cache.\n");
        exit(1);
    }

    setbuf(stdout, NULL);

    unsigned int i;
    for(i = 0; i < AC_NUM_ENTRIES; ++i)
    {
        AC_SET_ENTRY_INVALID(addr_cache[i], (i >> 1) << 10)
    }

#if defined(__i386__) && !defined(NO_TRANSLATION)
    // Relocate the assembly code that wants addr_cache at a fixed address
    extern uint32_t *ac_reloc_start[] __asm__("ac_reloc_start"), *ac_reloc_end[] __asm__("ac_reloc_end");
    uint32_t **reloc;
    for(reloc = ac_reloc_start; reloc != ac_reloc_end; reloc++)
    {
        make_writable(*reloc);
        **reloc += (uintptr_t)addr_cache;
    }
#endif
}

void addr_cache_deinit()
{
    if(addr_cache)
        munmap(addr_cache, AC_NUM_ENTRIES * sizeof(ac_entry));

    addr_cache = NULL;
}
