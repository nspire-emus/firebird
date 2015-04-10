/* Declarations for asmcode.S */

#ifndef _H_ASMCODE
#define _H_ASMCODE

#include "emu.h"

void translation_enter() __asm__("translation_enter");

void * FASTCALL ptr(uint32_t addr) __asm__("ptr");
uint32_t FASTCALL  read_byte(uint32_t addr) __asm__("read_byte");
uint32_t FASTCALL  read_half(uint32_t addr) __asm__("read_half");
uint32_t FASTCALL  read_word(uint32_t addr) __asm__("read_word");
uint32_t FASTCALL  read_word_ldr(uint32_t addr) __asm__("read_word_ldr");
void FASTCALL  write_byte(uint32_t addr, uint32_t value) __asm__("write_byte");
void FASTCALL  write_half(uint32_t addr, uint32_t value) __asm__("write_half");
void FASTCALL  write_word(uint32_t addr, uint32_t value) __asm__("write_word");

#endif
