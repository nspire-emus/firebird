/* Declarations for asmcode.S */

#ifndef _H_ASMCODE
#define _H_ASMCODE

#include "emu.h"

#ifdef __cplusplus
extern "C" {
#endif

#if defined(__arm__) || defined(__aarch64__)
// Supply the pointer to the instruction directly to avoid read_instruction
void translation_enter(void *ptr) __asm__("translation_enter");
#define TRANSLATION_ENTER_HAS_PTR 1
#else
void translation_enter() __asm__("translation_enter");
#define TRANSLATION_ENTER_HAS_PTR 0
#endif

// Jump to the translated code starting at ptr.
// Checks for cycle_count_delta and exits if necessary.
void translation_jmp(void *ptr) __asm__("translation_jmp");
// Checks whether code at ptr is now translated and if so, jumps to it
void translation_jmp_ptr(void *ptr) __asm__("translation_jmp_ptr");

void * FASTCALL read_instruction(uint32_t addr) __asm__("read_instruction");
uint32_t FASTCALL read_byte(uint32_t addr) __asm__("read_byte");
uint32_t FASTCALL read_half(uint32_t addr) __asm__("read_half");
uint32_t FASTCALL read_word(uint32_t addr) __asm__("read_word");
void FASTCALL write_byte(uint32_t addr, uint32_t value) __asm__("write_byte");
void FASTCALL write_half(uint32_t addr, uint32_t value) __asm__("write_half");
void FASTCALL write_word(uint32_t addr, uint32_t value) __asm__("write_word");

#if defined(__arm__) || defined(__aarch64__) || defined(__x86_64__)
// For some JITs there's in internal version called by JIT'ed code that can't be called
// from outside the JIT'ed code
uint32_t FASTCALL read_byte_asm(uint32_t addr) __asm__("read_byte_asm");
uint32_t FASTCALL read_half_asm(uint32_t addr) __asm__("read_half_asm");
uint32_t FASTCALL read_word_asm(uint32_t addr) __asm__("read_word_asm");
void FASTCALL write_byte_asm(uint32_t addr, uint32_t value) __asm__("write_byte_asm");
void FASTCALL write_half_asm(uint32_t addr, uint32_t value) __asm__("write_half_asm");
void FASTCALL write_word_asm(uint32_t addr, uint32_t value) __asm__("write_word_asm");
#endif

#ifdef __cplusplus
}
#endif

#endif
