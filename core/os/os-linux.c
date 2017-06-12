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


#ifdef IS_IOS_BUILD

#include <unistd.h>
#include <sys/sysctl.h>

int iOS_is_debugger_attached()
{
    struct kinfo_proc info;
    size_t info_size = sizeof(info);
    int name[4];

    name[0] = CTL_KERN;
    name[1] = KERN_PROC;
    name[2] = KERN_PROC_PID;
    name[3] = getpid();

    info.kp_proc.p_flag = 0;

    sysctl(name, 4, &info, &info_size, NULL, 0);

    return (info.kp_proc.p_flag & P_TRACED) != 0;
}
#endif

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

void *os_alloc_executable(size_t size)
{
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
    if(arm.addr_cache)
        return;

    arm.addr_cache = mmap((void*)0, AC_NUM_ENTRIES * sizeof(ac_entry), PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANON, -1, 0);
    if(arm.addr_cache == MAP_FAILED)
    {
        arm.addr_cache = NULL;
        fprintf(stderr, "Failed to mmap addr_cache.\n");
        exit(1);
    }

    setbuf(stdout, NULL);

    #if !defined(AC_FLAGS)
        unsigned int i;
        for(unsigned int i = 0; i < AC_NUM_ENTRIES; ++i)
        {
            AC_SET_ENTRY_INVALID(arm.addr_cache[i], (i >> 1) << 10)
        }
    #else
        memset(arm.addr_cache, 0xFF, AC_NUM_ENTRIES * sizeof(ac_entry));
    #endif

    #if defined(__i386__) && !defined(NO_TRANSLATION)
        // Relocate the assembly code that wants addr_cache at a fixed address
        extern uint32_t *ac_reloc_start[] __asm__("ac_reloc_start"), *ac_reloc_end[] __asm__("ac_reloc_end");
        for(uint32_t **reloc = ac_reloc_start; reloc != ac_reloc_end; reloc++)
        {
            make_writable(*reloc);
            **reloc += (uintptr_t)arm.addr_cache;
        }
    #endif
}

void addr_cache_deinit()
{
    if(arm.addr_cache)
        munmap(arm.addr_cache, AC_NUM_ENTRIES * sizeof(ac_entry));

    arm.addr_cache = NULL;
}
