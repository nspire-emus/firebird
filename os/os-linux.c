#define _GNU_SOURCE
#define _XOPEN_SOURCE

#include <sys/mman.h>
#include <stdio.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <signal.h>
#include <ucontext.h>
#include <unistd.h>
#include "os.h"

#include "../debug.h"
#include "../mmu.h"

void *os_reserve(size_t size)
{
#ifndef __x86_64__
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
    void *ptr = mmap((void*)0x30000000, size, PROT_READ|PROT_WRITE|PROT_EXEC, MAP_SHARED|MAP_ANON, -1, 0);
    if((intptr_t)ptr == -1)
        return NULL;

    msync(ptr, size, MS_SYNC|MS_INVALIDATE);
    return ptr;
}

void os_query_time(os_time_t *t)
{
    gettimeofday(t, NULL);
}

double os_time_diff(os_time_t x, os_time_t y)
{
    return (double)(x.tv_sec - y.tv_sec) * 10000000.0 + (x.tv_usec - y.tv_usec);
}

long long os_frequency_hz(os_frequency_t f)
{
    return f;
}

void os_query_frequency(os_frequency_t *f)
{
    *f = 2*1000*1000*1000; // 2GHz, good guess
    FILE *freq = fopen("/sys/devices/system/cpu/cpu0/cpufreq/cpuinfo_max_freq", "r");
    if(!freq)
        return;

    fscanf(freq, "%llu", f);
    fclose(freq);
}

static void addr_cache_exception(int sig, siginfo_t *si, void *uctx)
{
    (void) sig;
    (void) uctx;

    ucontext_t *u = (ucontext_t*) uctx;
#ifdef __i386__
#ifdef __linux__
    emuprintf("Got SIGSEGV trying to access 0x%lx (EIP=0x%x)\n", (long) si->si_addr, u->uc_mcontext.gregs[REG_EIP]);
#else // Apple
    emuprintf("Got SIGSEGV trying to access 0x%lx (EIP=0x%x)\n", (long) si->si_addr, u->uc_mcontext->__ss.__eip);
#endif
#elif defined(__x86_64__)
#ifdef __linux__
    emuprintf("Got SIGSEGV trying to access 0x%lx (RIP=0x%x)\n", (long) si->si_addr, u->uc_mcontext.gregs[REG_RIP]);
#else // Apple
    emuprintf("Got SIGSEGV trying to access 0x%lx (RIP=0x%x)\n", (long) si->si_addr, u->uc_mcontext->__ss.__rip);
#endif
#elif defined(__arm__)
    emuprintf("Got SIGSEGV (PC=0x%x)\n", u->uc_mcontext.arm_pc);
#endif

    if(!addr_cache_pagefault((uint8_t*)si->si_addr))
        exit(1);
}

void make_writable(void *addr)
{
    uintptr_t ps = sysconf(_SC_PAGE_SIZE);
    void *prev = (void*)((uintptr_t)(addr) & (~(ps - 1)));
    if(mprotect(prev, ps, PROT_READ | PROT_WRITE | PROT_EXEC) != 0)
        emuprintf("mprotect failed.\n");
}

static struct sigaction old_sa;

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

    struct sigaction sa;

    sa.sa_flags = SA_SIGINFO;
    sigemptyset(&sa.sa_mask);
    sa.sa_sigaction = addr_cache_exception;
    if(sigaction(SIGSEGV, &sa, &old_sa) == -1)
    {
        fprintf(stderr, "Failed to initialize SEGV handler.\n");
        addr_cache_deinit();
        exit(1);
    }

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
    sigaction(SIGSEGV, &old_sa, NULL);
}
