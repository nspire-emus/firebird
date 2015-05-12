/* Declarations for des.c */

#ifndef _DES_H
#define _DES_H

#ifdef __cplusplus
extern "C" {
#endif

void des_initialize();
void des_reset(void);
uint32_t des_read_word(uint32_t addr);
void des_write_word(uint32_t addr, uint32_t value);

#ifdef __cplusplus
}
#endif

#endif
