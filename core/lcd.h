/* Declarations for lcd.c */

#ifndef _H_LCD
#define _H_LCD

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct lcd_state {
    uint32_t timing[4];
    uint32_t upbase; // Upper panel base
    uint32_t lpbase; // Lower panel base (not used)
    uint32_t framebuffer; // Value of upbase latched at beginning of frame
    uint32_t control;
    uint8_t int_mask;
    uint8_t int_status;
    uint16_t palette[256];
    uint8_t cursor_ram[64*64/4]; // 64x64 2bpp cursor
    uint8_t cursor_control;
    uint8_t cursor_config;
    uint32_t cursor_palette[2];
    uint32_t cursor_xy;
    uint16_t cursor_clip;
    uint8_t cursor_int_mask;
    uint8_t cursor_int_status;
} lcd_state;

void lcd_draw_frame(uint8_t *buffer);
void lcd_cx_draw_frame(uint16_t *buffer);

void lcd_reset(void);
typedef struct emu_snapshot emu_snapshot;
bool lcd_suspend(emu_snapshot *snapshot);
bool lcd_resume(const emu_snapshot *snapshot);
uint32_t lcd_read_word(uint32_t addr);
void lcd_write_word(uint32_t addr, uint32_t value);

#ifdef __cplusplus
}
#endif

#endif
