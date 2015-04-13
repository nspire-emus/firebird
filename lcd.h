/* Declarations for lcd.c */

#ifndef _H_LCD
#define _H_LCD

#include "emu.h"

#ifdef __cplusplus
extern "C" {
#endif

void lcd_draw_frame(uint8_t buffer[240][160]);
void lcd_cx_draw_frame(uint16_t *buffer, uint32_t *colormasks);

void lcd_reset(void);
uint32_t lcd_read_word(uint32_t addr);
void lcd_write_word(uint32_t addr, uint32_t value);

#ifdef __cplusplus
}
#endif

#endif
