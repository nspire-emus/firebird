#include <sys/mman.h>
#include <stdio.h>
#include <termios.h>
#include <sys/ioctl.h>
#include "os.h"

int os_kbhit()
{
	static const int STDIN = 0;
	static int initialized = 0;

	if (!initialized)
	{
		// Use termios to turn off line buffering
		struct termios term;
		tcgetattr(STDIN, &term);
		term.c_lflag &= ~ICANON;
		tcsetattr(STDIN, TCSANOW, &term);
		setbuf(stdin, NULL);
		initialized = 1;
	}

	int bytes_waiting;
	ioctl(STDIN, FIONREAD, &bytes_waiting);
	return bytes_waiting;
}

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

void *os_sparse_commit(void *page, size_t size)
{
	void * ptr = mmap(page, size, PROT_READ|PROT_WRITE, MAP_FIXED|MAP_SHARED|MAP_ANON, -1, 0);
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
