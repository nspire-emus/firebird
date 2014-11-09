/* Declarations for gdbstub.c */

#ifndef _H_GDBSTUB
#define _H_GDBSTUB

void gdbstub_init(int port);

void gdbstub_reset(void);

void gdbstub_recv(void);

void gdbstub_debugger(enum DBG_REASON reason, uint32_t addr);

void *gdbstub_save_state(size_t *size);

void gdbstub_reload_state(void *state);

#endif
