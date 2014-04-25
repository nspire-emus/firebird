/* Declarations for des.c */

void des_initialize();
void des_reset(void);
u32 des_read_word(u32 addr);
void des_write_word(u32 addr, u32 value);
void *des_save_state(size_t *size);
void des_reload_state(void *state);
