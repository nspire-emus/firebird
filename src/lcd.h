/* Declarations for lcd.c */



void lcd_draw_frame(uint8_t buffer[240][160]);

void lcd_cx_draw_frame(uint16_t buffer[240][320], uint32_t colormasks[3]);

void lcd_reset(void);

uint32_t lcd_read_word(uint32_t addr);

void lcd_write_word(uint32_t addr, uint32_t value);

void *lcd_save_state(size_t *size);

void lcd_reload_state(void *state);
