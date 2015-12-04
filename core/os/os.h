#ifndef OS_H
#define OS_H

#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Some really crappy APIs don't use UTF-8 in fopen. */
FILE *fopen_utf8(const char *filename, const char *mode);

void *os_reserve(size_t size);
void os_free(void *ptr, size_t size);
void *os_commit(void *addr, size_t size);
void *os_sparse_commit(void *page, size_t size);
void os_sparse_decommit(void *page, size_t size);
void *os_alloc_executable(size_t size);

void *os_map_cow(const char *filename, size_t size);
void os_unmap_cow(void *addr, size_t size);

typedef struct { void *prev, *function; } os_exception_frame_t;
void addr_cache_init(os_exception_frame_t *frame);
void addr_cache_deinit();

#ifdef __cplusplus
}
#endif

#endif
