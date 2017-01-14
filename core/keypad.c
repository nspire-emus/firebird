#include "emu.h"
#include <string.h>
#include "misc.h"
#include "keypad.h"
#include "schedule.h"
#include "interrupt.h"
#include "mem.h"

/* 900E0000: Keypad controller */
keypad_state keypad;

void keypad_int_check() {
    if(keypad.touchpad_contact != keypad.touchpad_last_contact)
        keypad.touchpad_irq_state |= 0x4;
    if(keypad.touchpad_down != keypad.touchpad_last_down)
        keypad.touchpad_irq_state |= 0x8;

    keypad.touchpad_last_contact = keypad.touchpad_contact;
    keypad.touchpad_last_down = keypad.touchpad_down;

    int_set(INT_KEYPAD, (keypad.kpc.int_enable & keypad.kpc.int_active)
                      | (keypad.kpc.gpio_int_enable & keypad.kpc.gpio_int_active));
}

void keypad_on_pressed() {
    if(pmu.on_irq_enabled)
        int_set(INT_POWER, true);
}

uint32_t keypad_read(uint32_t addr) {
    switch (addr & 0x7F) {
        case 0x00:
            cycle_count_delta += 1000; // avoid slowdown with polling loops
            return keypad.kpc.control;
        case 0x04: return keypad.kpc.size;
        case 0x08: return keypad.kpc.int_active;
        case 0x0C: return keypad.kpc.int_enable;
        case 0x10: case 0x14: case 0x18: case 0x1C:
        case 0x20: case 0x24: case 0x28: case 0x2C:
            return *(uint32_t *)((uint8_t *)keypad.kpc.data + ((addr - 0x10) & 31));
        case 0x30: return 0; // GPIO direction?
        case 0x34: return 0; // GPIO output?
        case 0x38: return 0; // GPIO input?
        case 0x3C: return 0;
        case 0x40: return keypad.kpc.gpio_int_enable;
        case 0x44: return keypad.kpc.gpio_int_active;
        case 0x48: return 0;
    }
    return bad_read_word(addr);
}
void keypad_write(uint32_t addr, uint32_t value) {
    switch (addr & 0x7F) {
        case 0x00:
            keypad.kpc.control = value;
            if (keypad.kpc.control & 2) {
                if (!(keypad.kpc.control >> 2 & 0x3FFF))
                    error("keypad time between rows = 0");
                event_set(SCHED_KEYPAD, (keypad.kpc.control >> 16) + (keypad.kpc.control >> 2 & 0x3FFF));
            } else {
                event_clear(SCHED_KEYPAD);
            }
            return;
        case 0x04: keypad.kpc.size = value; return;
        case 0x08: keypad.kpc.int_active &= ~value; keypad_int_check(); return;
        case 0x0C: keypad.kpc.int_enable = value & 7; keypad_int_check(); return;

        case 0x30: return;
        case 0x34: return;
        case 0x3C: return;
        case 0x40: keypad.kpc.gpio_int_enable = value; keypad_int_check(); return;
        case 0x44: keypad.kpc.gpio_int_active &= ~value; keypad_int_check(); return;
        case 0x48: return;
    }
    bad_write_word(addr, value);
}
// Scan next row of keypad, if scanning is enabled
static void keypad_scan_event(int index) {
    if (keypad.kpc.current_row >= 16)
        error("too many keypad rows");

    uint16_t row = ~keypad.key_map[keypad.kpc.current_row];
    row &= ~(0x80000 >> keypad.kpc.current_row); // Emulate weird diagonal glitch
    row |= ~0u << (keypad.kpc.size >> 8 & 0xFF);  // Unused columns read as 1
    if (emulate_cx)
        row = ~row;

    if (keypad.kpc.data[keypad.kpc.current_row] != row) {
        keypad.kpc.data[keypad.kpc.current_row] = row;
        keypad.kpc.int_active |= 2;
    }

    keypad.kpc.current_row++;
    if (keypad.kpc.current_row < (keypad.kpc.size & 0xFF)) {
        event_repeat(index, keypad.kpc.control >> 2 & 0x3FFF);
    } else {
        keypad.kpc.current_row = 0;
        keypad.kpc.int_active |= 1;
        if (keypad.kpc.control & 1) {
            event_repeat(index, (keypad.kpc.control >> 16) + (keypad.kpc.control >> 2 & 0x3FFF));
        } else {
            // If in single scan mode, go to idle mode
            keypad.kpc.control &= ~3;
        }
    }
    keypad_int_check();
}
void keypad_reset() {
    memset(&keypad.kpc, 0, sizeof keypad.kpc);
    keypad.touchpad_page = 0x04;
    sched.items[SCHED_KEYPAD].clock = CLOCK_APB;
    sched.items[SCHED_KEYPAD].second = -1;
    sched.items[SCHED_KEYPAD].proc = keypad_scan_event;
}

void touchpad_write(uint8_t addr, uint8_t value) {
    //printf("touchpad write: %02x %02x %02x\n", keypad.touchpad_page, addr, value);
    if (addr == 0xFF)
        keypad.touchpad_page = value;
}
uint8_t touchpad_read(uint8_t addr) {
    //printf("touchpad read:  %02x\n", addr);
    if (addr == 0xFF)
        return keypad.touchpad_page;

    if (keypad.touchpad_page == 0x10) {
        switch (addr) {
            case 0x04: return TOUCHPAD_X_MAX >> 8;
            case 0x05: return TOUCHPAD_X_MAX & 0xFF;
            case 0x06: return TOUCHPAD_Y_MAX >> 8;
            case 0x07: return TOUCHPAD_Y_MAX & 0xFF;
            default: gui_debug_printf("FIXME: TPAD read 10%02x\n", addr);
        }
    } else if (keypad.touchpad_page == 0x04) {
        switch (addr) {
            case 0x00: return keypad.touchpad_down || keypad.touchpad_contact; // contact
            case 0x01: return keypad.touchpad_down ? 100 : keypad.touchpad_contact ? 0x2F : 0; // proximity
            case 0x02: return keypad.touchpad_x >> 8;
            case 0x03: return keypad.touchpad_x & 0xFF;
            case 0x04: return keypad.touchpad_y >> 8;
            case 0x05: return keypad.touchpad_y & 0xFF;
            case 0x06: // relative x
            {
                uint8_t a = keypad.touchpad_rel_x;
                keypad.touchpad_rel_x = 0;
                return a;
            }
            case 0x07: // relative y
            {
                uint8_t a = keypad.touchpad_rel_y;
                keypad.touchpad_rel_y = 0;
                return a;
            }
            case 0x08: return 0x0; // ?
            case 0x09: return 0x0; // ?
            case 0x0A: return keypad.touchpad_down; // down
            case 0x0B: // IRQ status
            {
                uint8_t ret = keypad.touchpad_irq_state;
                keypad.touchpad_irq_state = 0x53; // Reading resets IRQs, those few bits are always active
                return ret;
            }
            case 0xE4: return 1; // firmware version
            case 0xE5: return 6; // firmware version
            case 0xE6: return 0; // firmware version
            case 0xE7: return 0; // firmware version
            default: gui_debug_printf("FIXME: TPAD read 04%02x\n", addr);
        }
    }
    return 0;
}

void touchpad_gpio_reset() {
    keypad.touchpad_gpio.prev_clock = 1;
    keypad.touchpad_gpio.prev_data = 1;
    keypad.touchpad_gpio.state = 0;
    keypad.touchpad_gpio.byte = 0;
    keypad.touchpad_gpio.bitcount = 0;
    keypad.touchpad_gpio.port = 0;
}
void touchpad_gpio_change() {
    uint8_t value = gpio.input.b[0] & (gpio.output.b[0] | gpio.direction.b[0]) & 0xA;
    uint8_t clock = value >> 1 & 1;
    uint8_t data  = value >> 3 & 1;

    if (keypad.touchpad_gpio.prev_clock == 1 && clock == 1) {
        if (data < keypad.touchpad_gpio.prev_data) {
            //printf("I2C start\n");
            keypad.touchpad_gpio.bitcount = 0;
            keypad.touchpad_gpio.byte = 0xFF;
            keypad.touchpad_gpio.state = 2;
        }
        if (data > keypad.touchpad_gpio.prev_data) {
            //printf("I2C stop\n");
            keypad.touchpad_gpio.state = 0;
        }
    }

    if (clock != keypad.touchpad_gpio.prev_clock) {
        if (keypad.touchpad_gpio.state == 0) {
            // idle, do nothing
        } else if (keypad.touchpad_gpio.bitcount < 8) {
            if (!clock) {
                gpio.input.b[0] &= ~8;
                gpio.input.b[0] |= keypad.touchpad_gpio.byte >> 4 & 8;
            } else {
                // bit transferred, shift the register
                keypad.touchpad_gpio.byte = keypad.touchpad_gpio.byte << 1 | data;
                keypad.touchpad_gpio.bitcount++;
            }
        } else switch (keypad.touchpad_gpio.state | clock) {
            case 2: // C->T address
                if ((keypad.touchpad_gpio.byte >> 1) != 0x20) {
                    // Wrong address
                    keypad.touchpad_gpio.state = 0;
                    break;
                }
                gpio.input.b[0] &= ~8;
                break;
            case 3: // C->T address
                if (!(keypad.touchpad_gpio.byte & 1)) {
                    keypad.touchpad_gpio.bitcount = 0;
                    keypad.touchpad_gpio.byte = 0xFF;
                    keypad.touchpad_gpio.state = 4;
                    break;
                }
read_again:
                keypad.touchpad_gpio.bitcount = 0;
                keypad.touchpad_gpio.byte = touchpad_read(keypad.touchpad_gpio.port);
                if (keypad.touchpad_gpio.port != 0xFF)
                    keypad.touchpad_gpio.port++;
                keypad.touchpad_gpio.state = 8;
                break;
            case 4: // C->T port
                keypad.touchpad_gpio.port = keypad.touchpad_gpio.byte;
                gpio.input.b[0] &= ~8;
                break;
            case 5: // C->T port
            case 7: // C->T value
                keypad.touchpad_gpio.bitcount = 0;
                keypad.touchpad_gpio.byte = 0xFF;
                keypad.touchpad_gpio.state = 6;
                break;
            case 6: // C->T value
                touchpad_write(keypad.touchpad_gpio.port, keypad.touchpad_gpio.byte);
                if (keypad.touchpad_gpio.port != 0xFF)
                    keypad.touchpad_gpio.port++;
                gpio.input.b[0] &= ~8;
                break;
            case 8: // T->C value
                gpio.input.b[0] |= 8;
                break;
            case 9: // T->C value
                if (!data)
                    goto read_again;
                keypad.touchpad_gpio.state = 0;
                break;
        }
    }

    keypad.touchpad_gpio.prev_clock = clock;
    keypad.touchpad_gpio.prev_data  = data;
}

/* 90050000 */
void touchpad_cx_reset(void) {
    keypad.touchpad_cx.state = 0;
}
uint32_t touchpad_cx_read(uint32_t addr) {
    switch (addr & 0xFFFF) {
        case 0x0010:
            if (!keypad.touchpad_cx.reading)
                break;
            keypad.touchpad_cx.reading--;
            uint32_t val = touchpad_read(keypad.touchpad_cx.port++);
            //printf("Port %02x = %02x\n", touchpad_cx.port - 1, val);
            return val;
        case 0x0070:
            return keypad.touchpad_cx.reading ? 12 : 4;
        case 0x00FC:
            return 0x44570140;
        default:
            return 0;
    }
    return bad_read_word(addr);
}
void touchpad_cx_write(uint32_t addr, uint32_t value) {
    switch (addr & 0xFFFF) {
        case 0x0010:
            if (keypad.touchpad_cx.state == 0) {
                keypad.touchpad_cx.port = value;
                keypad.touchpad_cx.state = 1;
            } else {
                if (value & 0x100) {
                    keypad.touchpad_cx.reading++;
                } else {
                    touchpad_write(keypad.touchpad_cx.port++, value);
                }
            }
            return;
        case 0x0038:
            keypad.touchpad_cx.state = 0;
            keypad.touchpad_cx.reading = 0;
            return;
    }
    //bad_write_word(addr, value);
}

bool keypad_suspend(emu_snapshot *snapshot)
{
    snapshot->mem.keypad = keypad;
    return true;
}

bool keypad_resume(const emu_snapshot *snapshot)
{
    keypad = snapshot->mem.keypad;
    return true;
}
