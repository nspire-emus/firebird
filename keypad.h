/* Declarations for keypad.c */

#ifndef _H_KEYPAD
#define _H_KEYPAD

#define NUM_KEYPAD_TYPES 5

extern volatile int keypad_type;

extern volatile uint16_t key_map[16];

extern volatile uint8_t touchpad_proximity;

extern volatile uint16_t touchpad_x;

extern volatile uint16_t touchpad_y;

extern volatile uint8_t touchpad_down;



extern struct keypad_controller_state {

	uint32_t control;

	uint32_t size;

	uint8_t  current_row;

	uint8_t  int_active;

	uint8_t  int_enable;

	uint16_t data[16];

	uint32_t gpio_int_enable;

	uint32_t gpio_int_active;

} kpc;

void keypad_reset();

void keypad_int_check();

uint32_t keypad_read(uint32_t addr);

void keypad_write(uint32_t addr, uint32_t value);

void touchpad_cx_reset(void);

uint32_t touchpad_cx_read(uint32_t addr);

void touchpad_cx_write(uint32_t addr, uint32_t value);



#define TOUCHPAD_X_MAX 0x0918

#define TOUCHPAD_Y_MAX 0x069B

void touchpad_set(uint8_t proximity, uint16_t x, uint16_t y, uint8_t down);

void touchpad_gpio_reset(void);

void touchpad_gpio_change();

void *keypad_save_state(size_t *size);

void keypad_reload_state(void *state);

#endif
