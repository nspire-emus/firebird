/* Declarations for translate.c */

#ifndef _H_TRANSLATE
#define _H_TRANSLATE

struct translation {
        u32 unused;
        u32 jump_table;
        u32 *start_ptr;
        u32 *end_ptr;
};
extern struct translation translation_table[] __asm__("translation_table");
#define INSN_BUFFER_SIZE 10000000
extern u8 *insn_buffer;
extern u8 *insn_bufptr;

int translate(u32 start_pc, u32 *insnp);
void flush_translations();
void invalidate_translation(int index);
void fix_pc_for_fault();
int range_translated(u32 range_start, u32 range_end);
void *translate_save_state(size_t *size);
void translate_reload_state(void *state);

#endif
