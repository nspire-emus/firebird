/* Declarations for asmcode.S */

void translation_enter();

u32 /* TODO: __attribute__((fastcall) ) */ read_byte(u32 addr);
u32 /* TODO: __attribute__((fastcall)) */ read_half(u32 addr);
u32 /* TODO: __attribute__((fastcall)) */ read_word(u32 addr);
u32 /* TODO: __attribute__((fastcall)) */ read_word_ldr(u32 addr);
void /* TODO: __attribute__((fastcall)) */ write_byte(u32 addr, u32 value);
void /* TODO: __attribute__((fastcall)) */ write_half(u32 addr, u32 value);
void /* TODO: __attribute__((fastcall)) */ write_word(u32 addr, u32 value);
