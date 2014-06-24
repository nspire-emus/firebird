/* Declarations for casplus.c */

#ifndef _H_CASPLUS
#define _H_CASPLUS

#include <stdbool.h>
#include "types.h"

void casplus_lcd_draw_frame(u8 buffer[240][160]);
u8 casplus_nand_read_byte(u32 addr);
u16 casplus_nand_read_half(u32 addr);
void casplus_nand_write_byte(u32 addr, u8 value);
void casplus_nand_write_half(u32 addr, u16 value);

void casplus_int_set(u32 int_num, bool on);

u8 omap_read_byte(u32 addr);
u16 omap_read_half(u32 addr);
u32 omap_read_word(u32 addr);
void omap_write_byte(u32 addr, u8 value);
void omap_write_half(u32 addr, u16 value);
void omap_write_word(u32 addr, u32 value);

void casplus_reset(void);

#endif
