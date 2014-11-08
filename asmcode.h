/* Declarations for asmcode.S */

#ifndef _H_ASMCODE
#define _H_ASMCODE

void translation_enter() __asm__("translation_enter");

/*
 * Fastcall attribute is needed because theese assembler functions use registers to read the parameters...
 * TODO: Change them...
 */
u32 __attribute__((fastcall))  read_byte(u32 addr) __asm__("read_byte");
u32 __attribute__((fastcall))  read_half(u32 addr) __asm__("read_half");
u32 __attribute__((fastcall))  read_word(u32 addr) __asm__("read_word");
u32 __attribute__((fastcall))  read_word_ldr(u32 addr) __asm__("read_word_ldr");
void __attribute__((fastcall))  write_byte(u32 addr, u32 value) __asm__("write_byte");
void __attribute__((fastcall))  write_half(u32 addr, u32 value) __asm__("write_half");
void __attribute__((fastcall))  write_word(u32 addr, u32 value) __asm__("write_word");

#endif
