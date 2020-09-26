/* Declarations for keypad.c */

#ifndef _H_KEYPAD
#define _H_KEYPAD

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define NUM_KEYPAD_TYPES 5
struct keypad_controller_state {
	uint32_t control;
	uint32_t size;
	uint8_t  current_row;
	uint8_t  int_active;
	uint8_t  int_enable;
	uint16_t data[16];
	uint32_t gpio_int_enable;
	uint32_t gpio_int_active;
};

struct touchpad_gpio_state {
    uint8_t prev_clock;
    uint8_t prev_data;
    uint8_t state;
    uint8_t byte;
    uint8_t bitcount;
    uint8_t port;
};

struct touchpad_cx_state {
    int state;
    int reading;
    uint8_t port;
};

struct touchpad_captivate_state {
    uint8_t current_cmd; // 0 means waiting for command
    uint8_t byte_offset; // How many bytes were read/written for this cmd
};

typedef struct keypad_state {
    uint16_t key_map[16];
    uint16_t touchpad_x, touchpad_y;
    uint8_t unused, touchpad_irq_state, touchpad_last_down, touchpad_last_contact;
    uint8_t touchpad_page;
    int8_t touchpad_rel_x, touchpad_rel_y;
    bool touchpad_down, touchpad_contact;
    struct keypad_controller_state kpc;
    struct touchpad_gpio_state touchpad_gpio;
    struct touchpad_cx_state touchpad_cx;
    struct touchpad_captivate_state tpad_captivate;
} keypad_state;

extern keypad_state keypad;

void keypad_reset();
typedef struct emu_snapshot emu_snapshot;
bool keypad_suspend(emu_snapshot *snapshot);
bool keypad_resume(const emu_snapshot *snapshot);
#define KEYPAD_ROWS 8
#define KEYPAD_COLS 11
void keypad_set_key(int row, int col, bool state);
void keypad_int_check();
void keypad_on_pressed();
uint32_t keypad_read(uint32_t addr);
void keypad_write(uint32_t addr, uint32_t value);
void touchpad_cx_reset(void);
uint32_t touchpad_cx_read(uint32_t addr);
void touchpad_cx_write(uint32_t addr, uint32_t value);
void touchpad_set_state(float x, float y, bool contact, bool down);

#define TOUCHPAD_X_MAX 0x0918
#define TOUCHPAD_Y_MAX 0x069B
void touchpad_gpio_reset(void);
void touchpad_gpio_change();

#ifdef __cplusplus
}
#endif

#endif
