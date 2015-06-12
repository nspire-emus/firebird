/* Declarations for gdbstub.c */

#ifndef _H_GDBSTUB
#define _H_GDBSTUB

#ifdef __cplusplus
extern "C" {
#endif

bool gdbstub_init(unsigned int port);
void gdbstub_quit();
void gdbstub_reset(void);
void gdbstub_recv(void);
void gdbstub_debugger(enum DBG_REASON reason, uint32_t addr);

#ifdef __cplusplus
}
#endif

#endif
