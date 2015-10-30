/* Declarations for sha256.c */

#ifndef _H_SHA256
#define _H_SHA256

#ifdef __cplusplus
extern "C" {
#endif

typedef struct sha256_state {
    uint32_t hash_state[8], hash_block[16];
} sha256_state;

void sha256_reset(void);
typedef struct emu_snapshot emu_snapshot;
bool sha256_suspend(emu_snapshot *snapshot);
bool sha256_resume(const emu_snapshot *snapshot);
uint32_t sha256_read_word(uint32_t addr);
void sha256_write_word(uint32_t addr, uint32_t value);

#ifdef __cplusplus
}
#endif

#endif
