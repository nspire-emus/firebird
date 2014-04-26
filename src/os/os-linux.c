#include <sys/mman.h>
#include "os.h"

void *os_reserve(size_t size)
{
	void * ptr = mmap((void*)0, size, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANON, -1, 0);
	msync(ptr, size, MS_SYNC|MS_INVALIDATE);
	return ptr;
}

void *os_commit(void *addr, size_t size)
{
	void * ptr = mmap(addr, size, PROT_READ|PROT_WRITE, MAP_FIXED|MAP_SHARED|MAP_ANON, -1, 0);
	msync(addr, size, MS_SYNC|MS_INVALIDATE);
	return ptr;
}
