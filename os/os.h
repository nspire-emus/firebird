#include <stdlib.h>

int os_kbhit();
int os_getch();

void *os_reserve(size_t size);
void *os_commit(void *addr, size_t size);
void *os_sparse_commit(void *page, size_t size);
void os_sparse_decommit(void *page, size_t size);
void *os_alloc_executable(size_t size);

#if defined(_WIN32) || defined(_WIN64) || defined(__WIN32__) || defined(__TOS_WIN__) || defined(__WINDOWS__)
#include <windows.h>
typedef LARGE_INTEGER os_time_t;
typedef LARGE_INTEGER os_frequency_t;
#else
typedef struct timeval os_time_t;
typedef long long os_frequency_t;
#endif

void os_query_time(os_time_t *t);
double os_time_diff(os_time_t x, os_time_t y);
long long os_frequency_hz(os_frequency_t f);
void os_query_frequency(os_frequency_t *f);

void throttle_timer_on();
void throttle_timer_wait();
void throttle_timer_off();

typedef struct { void *prev, *function; } os_exception_frame_t;
void addr_cache_init(os_exception_frame_t *frame);

/*
Useful links for this section:
http://blog.nervus.org/managing-virtual-address-spaces-with-mmap/
http://msdn.microsoft.com/en-us/library/windows/desktop/aa366887%28v=vs.85%29.aspx

#define os_free_executable(ptr) VirtualFree(ptr, 0, MEM_RELEASE)

*/
