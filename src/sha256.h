/* Declarations for sha256.c */



void sha256_reset(void);

u32 sha256_read_word(u32 addr);

void sha256_write_word(u32 addr, u32 value);

void *sha256_save_state(size_t *size);

void sha256_reload_state(void *state);
