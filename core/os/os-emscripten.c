#include <assert.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/mman.h>
#include <sys/stat.h>

#include <emscripten.h>

#include "core/mmu.h"
#include "os.h"

FILE *fopen_utf8(const char *filename, const char *mode)
{
    return fopen(filename, mode);
}

void *os_reserve(size_t size)
{
    return malloc(size);
}

void os_free(void *ptr, size_t size)
{
    (void) size;
    free(ptr);
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
    (void) size;
    return NULL;
}

void *os_map_cow(const char *filename, size_t size)
{
    assert(strcmp(filename, "flash.img") == 0);
    int fd = open(filename, O_RDONLY);
    if(fd == -1)
        return NULL;

    void *ret = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0);

    close(fd);
    return ret == MAP_FAILED ? NULL : ret;
}

void os_unmap_cow(void *addr, size_t size)
{
    (void) size;
    free(addr);
}

void addr_cache_init(os_exception_frame_t *frame)
{
    (void) frame;

    // Only run this if not already initialized
    if(arm.addr_cache)
        return;

    arm.addr_cache = malloc(AC_NUM_ENTRIES * sizeof(ac_entry));
    if(!arm.addr_cache)
    {
        arm.addr_cache = NULL;
        fprintf(stderr, "Failed to mmap addr_cache.\n");
        exit(1);
    }

    unsigned int i;
    for(i = 0; i < AC_NUM_ENTRIES; ++i)
    {
        AC_SET_ENTRY_INVALID(arm.addr_cache[i], (i >> 1) << 10)
    }
}

void addr_cache_deinit()
{
    free(arm.addr_cache);
    arm.addr_cache = NULL;
}
