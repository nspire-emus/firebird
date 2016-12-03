/* Declarations for translate.c */

#ifndef _H_TRANSLATE
#define _H_TRANSLATE

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

struct translation {
    uintptr_t unused;
    void** jump_table;
    uint32_t *start_ptr;
    uint32_t *end_ptr;
} __attribute__((packed));
extern struct translation translation_table[] __asm__("translation_table");
#define INSN_BUFFER_SIZE 0x1000000

bool translate_init();
void translate_deinit();
void translate(uint32_t start_pc, uint32_t *insnp);
void flush_translations();
void invalidate_translation(int index);
void translate_fix_pc();

#ifdef __cplusplus
}
#endif

#endif
