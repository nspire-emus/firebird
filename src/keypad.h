/* Declarations for keypad.c */



#define NUM_KEYPAD_TYPES 5

extern volatile int keypad_type;

extern volatile u16 key_map[16];

extern volatile u8 touchpad_proximity;

extern volatile u16 touchpad_x;

extern volatile u16 touchpad_y;

extern volatile u8 touchpad_down;



extern struct keypad_controller_state {

	u32 control;

	u32 size;

	u8  current_row;

	u8  int_active;

	u8  int_enable;

	u16 data[16];

	u32 gpio_int_enable;

	u32 gpio_int_active;

} kpc;

void keypad_reset();

void keypad_int_check();

u32 keypad_read(u32 addr);

void keypad_write(u32 addr, u32 value);

void touchpad_cx_reset(void);

u32 touchpad_cx_read(u32 addr);

void touchpad_cx_write(u32 addr, u32 value);



#define TOUCHPAD_X_MAX 0x0918

#define TOUCHPAD_Y_MAX 0x069B

void touchpad_set(u8 proximity, u16 x, u16 y, u8 down);

void touchpad_gpio_reset(void);

void touchpad_gpio_change();

void *keypad_save_state(size_t *size);

void keypad_reload_state(void *state);
