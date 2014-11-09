/* Declarations for casplus.c */

#ifndef _H_CASPLUS
#define _H_CASPLUS

#include <stdbool.h>

void casplus_lcd_draw_frame(uint8_t buffer[240][160]);
uint8_t casplus_nand_read_byte(uint32_t addr);
uint16_t casplus_nand_read_half(uint32_t addr);
void casplus_nand_write_byte(uint32_t addr, uint8_t value);
void casplus_nand_write_half(uint32_t addr, uint16_t value);

void casplus_int_set(uint32_t int_num, bool on);

uint8_t omap_read_byte(uint32_t addr);
uint16_t omap_read_half(uint32_t addr);
uint32_t omap_read_word(uint32_t addr);
void omap_write_byte(uint32_t addr, uint8_t value);
void omap_write_half(uint32_t addr, uint16_t value);
void omap_write_word(uint32_t addr, uint32_t value);

void casplus_reset(void);

#endif
