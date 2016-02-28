/* Declarations for asmcode.S */

#ifndef _H_ASMCODE
#define _H_ASMCODE

#include "emu.h"

#ifdef __cplusplus
extern "C" {
#endif

void translation_enter() __asm__("translation_enter");
void translation_jmp() __asm__("translation_jmp");

void * FASTCALL read_instruction(uint32_t addr) __asm__("read_instruction");
uint32_t FASTCALL read_byte(uint32_t addr) __asm__("read_byte");
uint32_t FASTCALL read_half(uint32_t addr) __asm__("read_half");
uint32_t FASTCALL read_word(uint32_t addr) __asm__("read_word");
void FASTCALL write_byte(uint32_t addr, uint32_t value) __asm__("write_byte");
void FASTCALL write_half(uint32_t addr, uint32_t value) __asm__("write_half");
void FASTCALL write_word(uint32_t addr, uint32_t value) __asm__("write_word");

#if defined(__arm__) || defined(__x86_64__)
// For the ARM JIT there's a special, faster, calling convention (r10 = &arm)
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
