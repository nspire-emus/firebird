/* Declarations for sha256.c */

#ifndef _H_SHA256
#define _H_SHA256

void sha256_reset(void);

uint32_t sha256_read_word(uint32_t addr);

void sha256_write_word(uint32_t addr, uint32_t value);

void *sha256_save_state(size_t *size);

void sha256_reload_state(void *state);

#endif
