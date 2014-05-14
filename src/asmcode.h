/* Declarations for asmcode.S */

void translation_enter() __asm__("translation_enter");

u32 /* TODO: __attribute__((fastcall) ) */ read_byte(u32 addr) __asm__("read_byte");
u32 /* TODO: __attribute__((fastcall)) */ read_half(u32 addr) __asm__("read_half");
u32 /* TODO: __attribute__((fastcall)) */ read_word(u32 addr) __asm__("read_word");
u32 /* TODO: __attribute__((fastcall)) */ read_word_ldr(u32 addr) __asm__("read_word_ldr");
void /* TODO: __attribute__((fastcall)) */ write_byte(u32 addr, u32 value) __asm__("write_byte");
void /* TODO: __attribute__((fastcall)) */ write_half(u32 addr, u32 value) __asm__("write_half");
void /* TODO: __attribute__((fastcall)) */ write_word(u32 addr, u32 value) __asm__("write_word");
