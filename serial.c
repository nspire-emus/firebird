#include <string.h>
#include <stdio.h>
#include "emu.h"
#include "interrupt.h"
#include "misc.h"
#include "mem.h"
#include "casplus.h"

FILE *xmodem_file;
uint8_t xmodem_buf[0x84];
int xmodem_buf_pos;

static void xmodem_next_char() {
    if (xmodem_file && xmodem_buf_pos < 0x84)
        serial_byte_in(xmodem_buf[xmodem_buf_pos++]);
}

static void xmodem_next_packet() {
    int len;
    int i, sum = 0;
    xmodem_buf[0] = 0x01;
    xmodem_buf[1]++; // sequence number
    xmodem_buf[2] = ~xmodem_buf[1]; // sequence number complement
    len = fread(&xmodem_buf[3], 1, 0x80, xmodem_file);
    if (len == 0) {
        serial_byte_in(0x04);
        fclose(xmodem_file);
        xmodem_file = NULL;
        return;
    }
    memset(&xmodem_buf[3 + len], 0x1A, 0x80 - len);
    for (i = 3; i < 0x83; i++) sum += xmodem_buf[i];
    xmodem_buf[0x83] = sum;
    xmodem_buf_pos = 0;
    xmodem_next_char();
}

void xmodem_send(char *filename) {
    if (xmodem_file)
        fclose(xmodem_file);
    xmodem_file = fopen(filename, "rb");
    if (!xmodem_file) {
        gui_perror(filename);
        return;
    }
    emuprintf("XMODEM: sending file %s...\n", filename);
    xmodem_buf[1] = 0;
    xmodem_next_packet();
}

void serial_byte_out(uint8_t byte) {
    if (xmodem_file) {
        if (byte == 6) {
            emuprintf("\r%ld bytes sent", ftell(xmodem_file));
            xmodem_next_packet();
        } else {
            emuprintf("xmodem got byte %02X\n", byte);
            fclose(xmodem_file);
            xmodem_file = NULL;
        }
    }
    gui_putchar(byte);
}

struct {
    uint8_t rx_char;
    uint8_t interrupts;
    uint8_t DLL;
    uint8_t DLM;
    uint8_t IER;
    uint8_t LCR;
} serial;
static void serial_int_check() {
    if (emulate_casplus)
        casplus_int_set(15, serial.interrupts & serial.IER);
    else
        int_set(INT_SERIAL, serial.interrupts & serial.IER);
}
void serial_reset() {
    if (xmodem_file) {
        fclose(xmodem_file);
        xmodem_file = NULL;
    }
    memset(&serial, 0, sizeof serial);
}
uint32_t serial_read(uint32_t addr) {
    switch (addr & 0x3F) {
        case 0x00:
            if (serial.LCR & 0x80)
                return serial.DLL; /* Divisor Latch LSB */
            if (!(serial.interrupts & 1))
                error("Attempted to read empty RBR");
            uint8_t byte = serial.rx_char;
            serial.interrupts &= ~1;
            serial_int_check();
            xmodem_next_char();
            return byte;
        case 0x04:
            if (serial.LCR & 0x80)
                return serial.DLM; /* Divisor Latch MSB */
            return serial.IER; /* Interrupt Enable Register */
        case 0x08: /* Interrupt Identification Register */
            if (serial.interrupts & serial.IER & 1) {
                return 4;
            } else if (serial.interrupts & serial.IER & 2) {
                serial.interrupts &= ~2;
                serial_int_check();
                return 2;
            } else {
                return 1;
            }
        case 0x0C: /* Line Control Register */
            return serial.LCR;
        case 0x14: /* Line Status Register */
            return 0x60 | (serial.interrupts & 1);
    }
    return bad_read_word(addr);
}
void serial_write(uint32_t addr, uint32_t value) {
    switch (addr & 0x3F) {
        case 0x00:
            if (serial.LCR & 0x80) {
                serial.DLL = value;
            } else {
                serial_byte_out(value);
                serial.interrupts |= 2;
                serial_int_check();
            }
            return;
        case 0x04:
            if (serial.LCR & 0x80) {
                serial.DLM = value;
            } else {
                serial.IER = value & 0x0F;
                serial_int_check();
            }
            return;
        case 0x0C: /* Line Control Register */
            serial.LCR = value;
            return;
        case 0x08: /* FIFO Control Register */
        case 0x10: /* Modem Control Register */
        case 0x20: /* unknown, used by DIAGS */
            return;
    }
    bad_write_word(addr, value);
}

struct {
    uint8_t rx_char;
    uint8_t rx;
    uint16_t int_status;
    uint16_t int_mask;
} serial_cx;
static inline void serial_cx_int_check() {
    int_set(INT_SERIAL, serial_cx.int_status & serial_cx.int_mask);
}
void serial_cx_reset() {
    if (xmodem_file) {
        fclose(xmodem_file);
        xmodem_file = NULL;
    }
    memset(&serial_cx, 0, sizeof serial_cx);
}
uint32_t serial_cx_read(uint32_t addr) {
    switch (addr & 0xFFFF) {
        case 0x000:
            if (!(serial_cx.rx & 1))
                error("Attempted to read empty RBR");
            uint8_t byte = serial_cx.rx_char;
            serial_cx.rx = 0;
            serial_cx.int_status &= ~0x10;
            serial_cx_int_check();
            xmodem_next_char();
            return byte;
        case 0x018: return 0x90 & ~(serial_cx.rx << 4);
        case 0x040: return serial_cx.int_status & serial_cx.int_mask;
        case 0xFE0: return 0x11;
        case 0xFE4: return 0x10;
        case 0xFE8: return 0x34;
        case 0xFEC: return 0x00;
    }
    return bad_read_word(addr);
}
void serial_cx_write(uint32_t addr, uint32_t value) {
    switch (addr & 0xFFFF) {
        case 0x000: serial_byte_out(value); return;
        case 0x004: return;
        case 0x024: return;
        case 0x028: return;
        case 0x02C: return;
        case 0x030: return;
        case 0x034: return;
        case 0x038:
            serial_cx.int_mask = value & 0x7FF;
            serial_cx_int_check();
            return;
        case 0x044:
            serial_cx.int_status &= ~value;
            serial_cx_int_check();
            return;
    }
    bad_write_word(addr, value);
}

void serial_byte_in(uint8_t byte) {
    if (!emulate_cx) {
        serial.rx_char = byte;
        serial.interrupts |= 1;
        serial_int_check();
    } else {
        serial_cx.rx_char = byte;
        serial_cx.rx = 1;
        serial_cx.int_status |= 0x10;
        serial_cx_int_check();
    }
}
