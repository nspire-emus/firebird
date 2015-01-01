/* Declarations for translate.c */

#ifndef _H_TRANSLATE
#define _H_TRANSLATE

struct translation {
    uintptr_t unused;
    void** jump_table;
    uint32_t *start_ptr;
    uint32_t *end_ptr;
} __attribute__((packed));
extern struct translation translation_table[] __asm__("translation_table");
#define INSN_BUFFER_SIZE 10000000
extern uint8_t *insn_buffer;
extern uint8_t *insn_bufptr;

int translate(uint32_t start_pc, uint32_t *insnp);
void flush_translations();
void invalidate_translation(int index);
void fix_pc_for_fault();
int range_translated(uint32_t range_start, uint32_t range_end);

#endif
