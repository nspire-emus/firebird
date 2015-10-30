/* Declarations for des.c */

#ifndef _DES_H
#define _DES_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct des_state {
    uint32_t block[2];
    uint32_t key[6];
    struct des_ks_entry { uint32_t odd, even; } key_schedule[3][16];
    bool key_schedule_valid;
} des_state;

void des_initialize();
void des_reset(void);
typedef struct emu_snapshot emu_snapshot;
bool des_suspend(emu_snapshot *snapshot);
bool des_resume(const emu_snapshot *snapshot);
uint32_t des_read_word(uint32_t addr);
void des_write_word(uint32_t addr, uint32_t value);

#ifdef __cplusplus
}
#endif

#endif
