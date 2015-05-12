/* Declarations for sha256.c */

#ifndef _H_SHA256
#define _H_SHA256

#ifdef __cplusplus
extern "C" {
#endif

void sha256_reset(void);
uint32_t sha256_read_word(uint32_t addr);
void sha256_write_word(uint32_t addr, uint32_t value);

#ifdef __cplusplus
}
#endif

#endif
