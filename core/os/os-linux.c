#define _GNU_SOURCE
#define _XOPEN_SOURCE

#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#ifdef __APPLE__
    #include <mach/clock.h>
    #include <mach/mach.h>
#endif

#ifndef MAP_32BIT
    #define MAP_32BIT 0
#endif

#include "os.h"
#include "../debug.h"
#include "../mmu.h"

FILE *fopen_utf8(const char *filename, const char *mode)
{
    return fopen(filename, mode);
}

void *os_reserve(size_t size)
{
#ifdef __i386__
    // Has to have bit 31 zero
    void *ptr = mmap((void*)0x70000000, size, PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANON|MAP_32BIT, -1, 0);
#else
    void *ptr = mmap((void*)0, size, PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANON, -1, 0);
#endif

    if(ptr == MAP_FAILED)
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
    (void) size;
    return addr;
}

void *os_sparse_commit(void *page, size_t size)
{
    (void) size;
    return page;
}

void os_sparse_decommit(void *page, size_t size)
{
    (void) page;
    (void) size;
}

#ifdef IS_IOS_BUILD
bool runtime_is64Bit()
{
    #include <sys/utsname.h>
    struct utsname ret;
    uname(&ret);
    return (strstr(ret.version, "ARM64") != NULL);
}
#endif

void *os_alloc_executable(size_t size)
{
#ifdef IS_IOS_BUILD
    // the access() will succeed on jailbroken only
    if (access("/bin/bash", F_OK) != 0 || runtime_is64Bit())
    {
        // Can't use JIT
        return NULL;
    }
#endif
#if defined(__i386__) || defined(__x86_64__)
    // Has to be in 32-bit space for the JIT
    void *ptr = mmap((void*)0x30000000, size, PROT_READ|PROT_WRITE|PROT_EXEC, MAP_SHARED|MAP_ANON|MAP_32BIT, -1, 0);
#else
    void *ptr = mmap((void*)0x0, size, PROT_READ|PROT_WRITE|PROT_EXEC, MAP_SHARED|MAP_ANON, -1, 0);
#endif

    if(ptr == MAP_FAILED)
        return NULL;

    msync(ptr, size, MS_SYNC|MS_INVALIDATE);
    return ptr;
}

void *os_map_cow(const char *filename, size_t size)
{
    int fd = open(filename, O_RDONLY);
    if(fd == -1)
        return NULL;

    void *ret = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0);

    close(fd);
    return ret == MAP_FAILED ? NULL : ret;
}

void os_unmap_cow(void *addr, size_t size)
{
    munmap(addr, size);
}

__attribute__((unused)) static void make_writable(void *addr)
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
