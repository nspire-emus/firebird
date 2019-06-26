/* Declarations for debug.c */
#ifndef _H_DEBUG
#define _H_DEBUG

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>

#ifdef __cplusplus

#include <string>
extern std::string ln_target_folder;

extern "C" {
#endif

extern bool gdb_connected;
extern bool in_debugger;
extern int rdbg_port;

enum DBG_REASON {
    DBG_USER,
    DBG_EXCEPTION,
    DBG_EXEC_BREAKPOINT,
    DBG_READ_BREAKPOINT,
    DBG_WRITE_BREAKPOINT,
};

void *virt_mem_ptr(uint32_t addr, uint32_t size);
void backtrace(uint32_t fp);
int process_debug_cmd(char *cmdline);
void debugger(enum DBG_REASON reason, uint32_t addr);
void rdebug_recv(void);
bool rdebug_bind(unsigned int port);
void rdebug_quit();

#ifdef __cplusplus
}
#endif

#endif
