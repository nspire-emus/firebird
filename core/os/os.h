#ifndef OS_H
#define OS_H

#include <stdlib.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void *os_reserve(size_t size);
void os_free(void *ptr, size_t size);
void *os_commit(void *addr, size_t size);
void *os_sparse_commit(void *page, size_t size);
void os_sparse_decommit(void *page, size_t size);
void *os_alloc_executable(size_t size);

typedef struct { void *prev, *function; } os_exception_frame_t;
void addr_cache_init(os_exception_frame_t *frame);
void addr_cache_deinit();

/*
Useful links for this section:
http://blog.nervus.org/managing-virtual-address-spaces-with-mmap/
http://msdn.microsoft.com/en-us/library/windows/desktop/aa366887%28v=vs.85%29.aspx
*/

#ifdef __cplusplus
}
#endif

#endif
