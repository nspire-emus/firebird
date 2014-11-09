/* Declarations for asmcode.S */

#ifndef _H_ASMCODE
#define _H_ASMCODE

void translation_enter() __asm__("translation_enter");

/*
 * Fastcall attribute is needed because theese assembler functions use registers to read the parameters...
 * TODO: Change them...
 */
uint32_t __attribute__((fastcall))  read_byte(uint32_t addr) __asm__("read_byte");
uint32_t __attribute__((fastcall))  read_half(uint32_t addr) __asm__("read_half");
uint32_t __attribute__((fastcall))  read_word(uint32_t addr) __asm__("read_word");
uint32_t __attribute__((fastcall))  read_word_ldr(uint32_t addr) __asm__("read_word_ldr");
void __attribute__((fastcall))  write_byte(uint32_t addr, uint32_t value) __asm__("write_byte");
void __attribute__((fastcall))  write_half(uint32_t addr, uint32_t value) __asm__("write_half");
void __attribute__((fastcall))  write_word(uint32_t addr, uint32_t value) __asm__("write_word");

#endif
