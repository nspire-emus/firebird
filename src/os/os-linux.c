#ifdef __unix__

#include <sys/mman.h>
#include <stdio.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include "os.h"
#include "../mmu.h"

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

int os_getch()
{
	return getchar();
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

void *os_alloc_executable(size_t size)
{
	void * ptr = mmap((void*)0, size, PROT_READ|PROT_WRITE|PROT_EXEC, MAP_FIXED|MAP_SHARED|MAP_ANON, -1, 0);
	msync(ptr, size, MS_SYNC|MS_INVALIDATE);
	return ptr;
}

void os_query_time(os_time_t *t)
{
	gettimeofday(t, NULL);
}

double os_time_diff(os_time_t x, os_time_t y)
{
	double t;
	t = (y.tv_sec - x.tv_sec) * 1000.0;	// sec to ms
	t += (y.tv_usec - x.tv_usec) / 1000.0;	// us to ms
	return t;
}

long os_frequency_hz(os_frequency_t f)
{
}

void os_query_frequency(os_frequency_t *f)
{
}

void throttle_timer_on()
{
	/*
	hTimerEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
        uTimerID = timeSetEvent(throttle_delay, throttle_delay, (LPTIMECALLBACK)hTimerEvent, 0,
                                TIME_PERIODIC | TIME_CALLBACK_EVENT_SET);
        if (uTimerID == 0) {
                printf("timeSetEvent failed\n");
                exit(1);
        }
	*/
}

void throttle_timer_wait()
{
	//WaitForSingleObject(hTimerEvent, INFINITE);
}

void throttle_timer_off()
{
	/*
        if (uTimerID != 0) {
                timeKillEvent(uTimerID);
                uTimerID = 0;
                CloseHandle(hTimerEvent);
        }
	*/
}

static int addr_cache_exception(/*PEXCEPTION_RECORD er, void *x, void *y, void *z*/)
{
	//if(er->ExceptionCode == EXCEPTION_ACCESS_VIOLATION)
	//{
	//	if(addr_cache_pagefault((void *)er->ExceptionInformation[1]))
	//		return 0; // Continue execution
	//}
	//return 1; // Continue search
}

void addr_cache_init(os_exception_frame_t *frame)
{
	addr_cache = mmap((void*)0, AC_NUM_ENTRIES * sizeof(ac_entry), PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANON, -1, 0);

	frame->function = (void *)addr_cache_exception;

	// TODO: Fix...
	// http://feepingcreature.github.io/handling.html
	//asm ("movl %%fs:(%1), %0" : "=r" (frame->prev) : "r" (0));
	//asm ("movl %0, %%fs:(%1)" : : "r" (frame), "r" (0));

	// Relocate the assembly code that wants addr_cache at a fixed address
	//extern long *ac_reloc_start[], *ac_reloc_end[];
	//long **reloc;
	//for (reloc = ac_reloc_start; reloc != ac_reloc_end; reloc++)
	//{
	//	long prot;
	//	mprotect(*reloc, 4, PROT_READ|PROT_WRITE|PROT_EXEC);
	//	**reloc += (long)addr_cache;
	//	mprotect(*reloc, 4, PROT_READ|PROT_WRITE|PROT_EXEC);
	//}
}

#endif
