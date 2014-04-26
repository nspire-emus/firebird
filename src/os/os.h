#include <stdlib.h>

void *os_reserve(size_t size);
void *os_commit(void *addr, size_t size);

/*
Useful links for this section:
http://blog.nervus.org/managing-virtual-address-spaces-with-mmap/
http://msdn.microsoft.com/en-us/library/windows/desktop/aa366887%28v=vs.85%29.aspx

#define os_alloc_executable(size) VirtualAlloc(NULL, size, MEM_RESERVE | MEM_COMMIT, PAGE_EXECUTE_READWRITE);
#define os_free_executable(ptr) VirtualFree(ptr, 0, MEM_RELEASE)

#define os_sparse_decommit(page, size) VirtualFree(page, size, MEM_DECOMMIT)
#define os_sparse_commit(page, size) VirtualAlloc(page, size, MEM_COMMIT, PAGE_READWRITE)

//typedef LARGE_INTEGER os_frequency_t;
typedef long os_frequency_t;
#define os_query_frequency(p) QueryPerformanceFrequency(&p)
#define os_frequency_hz(p) (p.QuadPart)
//typedef LARGE_INTEGER os_time_t;
typedef long os_time_t;
#define os_query_time(p) QueryPerformanceCounter(&p)
#define os_time_diff(x, y) (x.QuadPart - y.QuadPart)

gui 
//extern HWND hwndMain, hwndGfx;
#define os_redraw_screen() InvalidateRect(hwndGfx, NULL, FALSE);
#define os_set_window_title(buf) SetWindowText(hwndMain, buf)

Address cache 
typedef struct { void *prev, *function; } os_exception_frame_t;
void addr_cache_init(os_exception_frame_t *frame);

void throttle_timer_on();
void throttle_timer_wait();
void throttle_timer_off();

*/
