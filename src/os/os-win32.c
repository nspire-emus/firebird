#include "os.h"

#if defined(_WIN32) || defined(_WIN64) || defined(__WIN32__) || defined(__TOS_WIN__) || defined(__WINDOWS__)

#include <conio.h>

int os_kbhit()
{
	return _kbhit();
}

int os_getch()
{
	return _getch();
}

void *os_reserve(size_t size)
{
	return VirtualAlloc(NULL, size, MEM_RESERVE, PAGE_READWRITE);
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
	return  VirtualAlloc(NULL, size, MEM_RESERVE | MEM_COMMIT, PAGE_EXECUTE_READWRITE);
}

void os_query_time(os_time_t *t)
{
	QueryPerformanceCounter(t);
}

double os_time_diff(os_time_t x, os_time_t y)
{
	return x.QuadPart - y.QuadPart;
}


long os_frequency_hz(os_frequency_t f)
{
	return f.QuadPart;
}

void os_query_frequency(os_frequency_t *f)
{
	QueryPerformanceFrequency(f);
}

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include "../emu.h"
#include "../mmu.h"

static int addr_cache_exception(PEXCEPTION_RECORD er, void *x, void *y, void *z) {
	x = x; y = y; z = z; // unused parameters
	if (er->ExceptionCode == EXCEPTION_ACCESS_VIOLATION) {
		if (addr_cache_pagefault((void *)er->ExceptionInformation[1]))
			return 0; // Continue execution
	}
	return 1; // Continue search
}

void addr_cache_init(os_exception_frame_t *frame) {
	addr_cache = VirtualAlloc(NULL, AC_NUM_ENTRIES * sizeof(ac_entry), MEM_RESERVE, PAGE_READWRITE);

	frame->function = (void *)addr_cache_exception;
	asm ("movl %%fs:(%1), %0" : "=r" (frame->prev) : "r" (0));
	asm ("movl %0, %%fs:(%1)" : : "r" (frame), "r" (0));

	// Relocate the assembly code that wants addr_cache at a fixed address
	extern DWORD *ac_reloc_start[], *ac_reloc_end[];
	DWORD **reloc;
	for (reloc = ac_reloc_start; reloc != ac_reloc_end; reloc++) {
		DWORD prot;
		VirtualProtect(*reloc, 4, PAGE_EXECUTE_READWRITE, &prot);
		**reloc += (DWORD)addr_cache;
		VirtualProtect(*reloc, 4, prot, &prot);
	}
}

HANDLE hTimerEvent;
UINT uTimerID;
void throttle_timer_on() {
	hTimerEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	uTimerID = timeSetEvent(throttle_delay, throttle_delay, (LPTIMECALLBACK)hTimerEvent, 0,
	                        TIME_PERIODIC | TIME_CALLBACK_EVENT_SET);
	if (uTimerID == 0) {
		printf("timeSetEvent failed\n");
		exit(1);
	}
}
void throttle_timer_wait() {
	WaitForSingleObject(hTimerEvent, INFINITE);
}
void throttle_timer_off() {
	if (uTimerID != 0) {
		timeKillEvent(uTimerID);
		uTimerID = 0;
		CloseHandle(hTimerEvent);
	}
}

#endif
