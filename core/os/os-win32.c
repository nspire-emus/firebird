#include "os.h"

#define WIN32_LEAN_AND_MEAN
#include <conio.h>
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <share.h>

#include "../emu.h"
#include "../mmu.h"

void *os_reserve(size_t size)
{
    return VirtualAlloc(NULL, size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
}

void os_free(void *ptr, size_t size)
{
    (void) size;
    VirtualFree(ptr, 0, MEM_RELEASE);
}

void *os_commit(void *addr, size_t size)
{
    return VirtualAlloc(addr, size, MEM_COMMIT, PAGE_READWRITE);
}

void *os_sparse_commit(void *page, size_t size)
{
    return VirtualAlloc(page, size, MEM_COMMIT, PAGE_READWRITE);
}

void os_sparse_decommit(void *page, size_t size)
{
    VirtualFree(page, size, MEM_DECOMMIT);
    return;
}

void *os_alloc_executable(size_t size)
{
    return VirtualAlloc(NULL, size, MEM_RESERVE | MEM_COMMIT, PAGE_EXECUTE_READWRITE);
}

void *os_map_cow(const char *filename, size_t size)
{
    (void)size;

    wchar_t filename_w[MAX_PATH];
    MultiByteToWideChar(CP_UTF8, 0, filename, -1, filename_w, MAX_PATH);

    flash_fd = _wsopen(filename_w, _O_RDONLY, SH_DENYNO);
    if(flash_fd == -1)
        return NULL;

    HANDLE flash_fhandle = (HANDLE)_get_osfhandle(flash_fd);

    flash_mapping = CreateFileMapping(flash_fhandle, NULL, PAGE_WRITECOPY, 0, 0, NULL);
    if(flash_mapping == NULL)
    {
        _close(flash_fd);
        return NULL;
    }

    void *map = MapViewOfFile(flash_mapping, FILE_MAP_COPY, 0, 0, 0);
    if (!map)
    {
        CloseHandle(flash_mapping);
        _close(flash_fd);
        return NULL;
    }

    return map;
}

void os_unmap_cow(void *addr, size_t size)
{
    (void)size;
    UnmapViewOfFile(addr);
    CloseHandle(flash_mapping);
    _close(flash_fd);
}

static int addr_cache_exception(PEXCEPTION_RECORD er, void *x, void *y, void *z) {
    x = x; y = y; z = z; // unused parameters
    if (er->ExceptionCode == EXCEPTION_ACCESS_VIOLATION) {
        if (addr_cache_pagefault((void *)er->ExceptionInformation[1]))
            return 0; // Continue execution
    }
    return 1; // Continue search
}

void addr_cache_init(os_exception_frame_t *frame) {
    // Don't run more than once
    if(addr_cache)
        return;

    DWORD flags = MEM_RESERVE;

#ifndef NDEBUG
    // Commit memory to not trigger segfaults which make debugging a PITA
    flags |= MEM_COMMIT;
#endif

    addr_cache = VirtualAlloc(NULL, AC_NUM_ENTRIES * sizeof(ac_entry), flags, PAGE_READWRITE);

    frame->function = (void *)addr_cache_exception;
    asm ("movl %%fs:(%1), %0" : "=r" (frame->prev) : "r" (0));
    asm ("movl %0, %%fs:(%1)" : : "r" (frame), "r" (0));

#ifndef NDEBUG
    // Without segfaults we have to invalidate everything here
    unsigned int i;
    for(i = 0; i < AC_NUM_ENTRIES; ++i)
    {
        AC_SET_ENTRY_INVALID(addr_cache[i], (i >> 1) << 10)
    }
#endif

    // Relocate the assembly code that wants addr_cache at a fixed address
    extern DWORD *ac_reloc_start[] __asm__("ac_reloc_start"), *ac_reloc_end[] __asm__("ac_reloc_end");
    DWORD **reloc;
    for (reloc = ac_reloc_start; reloc != ac_reloc_end; reloc++) {
        DWORD prot;
        VirtualProtect(*reloc, 4, PAGE_EXECUTE_READWRITE, &prot);
        **reloc += (DWORD)addr_cache;
        VirtualProtect(*reloc, 4, prot, &prot);
    }
}

void addr_cache_deinit() {
    if(!addr_cache)
        return;

    // Undo the relocations
    extern DWORD *ac_reloc_start[] __asm__("ac_reloc_start"), *ac_reloc_end[] __asm__("ac_reloc_end");
    DWORD **reloc;
    for (reloc = ac_reloc_start; reloc != ac_reloc_end; reloc++)
        **reloc -= (DWORD)addr_cache;

    VirtualFree(addr_cache, 0, MEM_RELEASE);
    addr_cache = NULL;
}
