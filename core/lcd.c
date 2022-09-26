#include <assert.h>
#include <string.h>
#include <stdint.h>

#include "emu.h"
#include "gif.h"
#include "interrupt.h"
#include "schedule.h"
#include "mem.h"
#include "lcd.h"

static lcd_state lcd;

/* Draw the current screen into a 4bpp bitmap. (SetDIBitsToDevice
 * supports either orientation, but some programs can't paste right-side-up bitmaps) */
void lcd_draw_frame(uint8_t *buffer) {
    uint32_t bpp = 1 << (lcd.control >> 1 & 7);
    uint32_t *in = (uint32_t *)(intptr_t)phys_mem_ptr(lcd.framebuffer, (320 * 240) / 8 * bpp);
    if (!in || bpp > 32) {
        memset(buffer, 0, 160 * 240);
        return;
    }
    int row;
    for (row = 0; row < 230; ++row) {
        uint32_t pal_shift = (lcd.control & (1 << 8)) ? 11 : 1;
        int words = (320 / 32) * bpp;
        uint8_t *out = buffer + (row * 160);
        if (bpp < 16) {
            uint32_t mask = (1 << bpp) - 1;
            uint32_t bi = (lcd.control & (1 << 9)) ? 0 : 24;
            if (!(lcd.control & (1 << 10)))
                bi ^= (8 - bpp);
            do {
                uint32_t word = *in++;
                int bitpos = 32;
                do {
                    int color1 = lcd.palette[word >> ((bitpos -= bpp) ^ bi) & mask] >> pal_shift & 15;
                    int color2 = lcd.palette[word >> ((bitpos -= bpp) ^ bi) & mask] >> pal_shift & 15;
                    *out++ = color1 << 4 | color2;
                    color1 = lcd.palette[word >> ((bitpos -= bpp) ^ bi) & mask] >> pal_shift & 15;
                    color2 = lcd.palette[word >> ((bitpos -= bpp) ^ bi) & mask] >> pal_shift & 15;
                    *out++ = color1 << 4 | color2;
                } while (bitpos != 0);
            } while (--words != 0);
        } else if (bpp == 16) {
            uint32_t shift1 = pal_shift | ((lcd.control & (1 << 9)) ? 16 : 0);
            uint32_t shift2 = shift1 ^ 16;
            do {
                uint32_t word = *in++;
                *out++ = (word >> shift1 & 15) << 4 | (word >> shift2 & 15);
            } while (--words != 0);
        } else {
            uint32_t shift = (lcd.control & (1 << 8)) ? 20 : 4;
            do {
                int color1 = *in++ >> shift;
                int color2 = *in++ >> shift & 15;
                *out++ = color1 << 4 | color2;
            } while ((words -= 2) != 0);
        }
    }
}

void lcd_cx_w_draw_frame(uint16_t *buffer)
{
    uint32_t mode = lcd.control >> 1 & 7;
    uint32_t bpp;
    if (mode <= 5)
        bpp = 1 << mode;
    else
        bpp = 16;

    uint16_t *in = (uint16_t *)phys_mem_ptr(lcd.framebuffer, (320 * 240) / 8 * bpp);
    if(!in || !lcd.framebuffer)
    {
        memset(buffer, 0, 320 * 240 * 2);
        return;
    }

    if(mode == 6)
    {
        for (int col = 0; col < 320; ++col)
        {
            uint16_t *out = buffer + col;
            for(int row = 0; row < 240; ++row, out += 320)
                *out = *in++;
        }
    }
    else if(mode == 4)
    {
        for (int col = 0; col < 320; ++col)
        {
            uint16_t *out = buffer + col;
            for(int row = 0; row < 240; ++row, out += 320)
            {
                uint16_t color = *in++;
                uint8_t b = color & 0x1F,
                        g = (color >> 5) & 0x1F,
                        r = (color >> 10) & 0x1F;

                *out = (r << 11) | (g << 6) | b | (color >> 10 & 0x20);
            }
        }
    }
    else if(mode < 4)
    {
        uint32_t *in32 = (uint32_t*) in;
        for (int col = 0; col < 320; ++col)
        {
            uint16_t *out = buffer + col;
            uint32_t words = (240 * bpp) / 32;
            uint32_t mask = (1 << bpp) - 1;
            uint32_t bi = (lcd.control & (1 << 9)) ? 0 : 24;
            if (!(lcd.control & (1 << 10)))
                bi ^= (8 - bpp);
            do {
                uint32_t word = *in32++;
                int bitpos = 32;
                do {
                    uint16_t color = lcd.palette[word >> ((bitpos -= bpp) ^ bi) & mask];
                    *out = color + (color & 0xFFE0) + (color >> 10 & 0x20);
                    out += 320;
                } while (bitpos != 0);
            } while (--words != 0);
        }
    }
    else // TODO: Support for other modes
        return;
}

/* Cursor colors as RGB565 */
static uint16_t cursor_palette_cache[2];

/* Convert a single RGB888 pixel to RGB565 */
static uint16_t rgb888_to_rgb565(uint32_t val)
{
    /* Not entirely correct rounding. */
    return (val >> 8 & 0xF800) | (val >> 5 & 0x7E0) | (val >> 3 & 0x1F);
}

/* Treating cursor RAM as 4096px, draw one cursor pixel. */
static void lcd_draw_cursorpx(uint16_t *buffer, size_t srcindex, size_t dstindex)
{
    assert(srcindex < 64*64);
    assert(dstindex < 320*240);

    uint8_t cursor_px = lcd.cursor_ram[srcindex >> 2]; // Load the byte containing the px
    cursor_px >>= (~srcindex & 3) << 1; // Shift px to the right
    cursor_px &= 0b11; // Mask px
    switch(cursor_px)
    {
    case 0b00:
    case 0b01: // Palette color
        buffer[dstindex] = cursor_palette_cache[cursor_px];
        break;
    case 0b10: // Transparent
        break;
    case 0b11: // Invert framebuffer
        buffer[dstindex] ^= ~0u;
        break;
    }
}

/* Draw the cursor onto the 320x240 RGB565 buffer */
static void lcd_draw_cursor(uint16_t *buffer)
{
    cursor_palette_cache[0] = rgb888_to_rgb565(lcd.cursor_palette[0]);
    cursor_palette_cache[1] = rgb888_to_rgb565(lcd.cursor_palette[1]);

    if(lcd.cursor_config & 0b110000)
    {
        gui_debug_printf("Cursor index != 0 not implemented");
        return;
    }

    const uint8_t srcx = lcd.cursor_clip & 0x3F,
                  srcy = (lcd.cursor_clip >> 8) & 0x3F;

    uint16_t dstx = (lcd.cursor_xy) & 0xFFF,
             dsty = (lcd.cursor_xy >> 16) & 0xFFF;

    const uint8_t cs = 32 << (lcd.cursor_config & 0b1); // 32 or 64

    if((features & FEATURE_HWW) == FEATURE_HWW)
    {
        gui_debug_printf("Cursor on 240x320 not implemented");
        return;
    }

    for(int cy = srcy; cy < cs; cy++)
        for(int cx = srcx; cx < cs; cx++)
        {
            // For some reason, X/Y = (Y/-X) here
            uint16_t x = cy - srcy + dsty,
                     y = 239 - (cx - srcx + dstx);
            if(x < 320 && y < 240)
                lcd_draw_cursorpx(buffer, cy*cs+cx, y*320+x);
        }
}

/* Draw the current screen into a 16bpp bitmap. */
void lcd_cx_draw_frame(uint16_t *buffer) {
    uint32_t mode = lcd.control >> 1 & 7;
    uint32_t bpp;
    if (mode <= 5)
        bpp = 1 << mode;
    else
        bpp = 16;

    // HW-W features a new 240x320 LCD instead of the usual 320x240px one
    if((features & FEATURE_HWW) == FEATURE_HWW)
        lcd_cx_w_draw_frame(buffer);
    else
    {
        uint32_t *in = (uint32_t *)phys_mem_ptr(lcd.framebuffer, (320 * 240) / 8 * bpp);
        if (!in || !lcd.framebuffer) {
            memset(buffer, 0, 320 * 240 * 2);
            return;
        }

        // In STN mode, only the 4 MSB of the red channel are used
        uint32_t pal_shift = 0, pal_mask = 0xFFFF;
        if(!(lcd.control & (1 << 5))) {
            pal_shift = (lcd.control & (1 << 8)) ? 11 : 1;
            pal_mask &= 0xF;
        }

        for (int row = 0; row < 240; ++row) {
            uint16_t *out = buffer + (row * 320);
            uint32_t words = (320 / 32) * bpp;
            if (bpp < 16) {
                uint32_t mask = (1 << bpp) - 1;
                uint32_t bi = (lcd.control & (1 << 9)) ? 0 : 24;
                if (!(lcd.control & (1 << 10)))
                    bi ^= (8 - bpp);
                do {
                    uint32_t word = *in++;
                    int bitpos = 32;
                    do {
                        uint16_t color = lcd.palette[word >> ((bitpos -= bpp) ^ bi) & mask] >> pal_shift;
                        color &= pal_mask;
                        *out++ = color + (color & 0xFFE0) + (color >> 10 & 0x20);
                    } while (bitpos != 0);
                } while (--words != 0);
            } else if (mode == 4) {
                uint32_t i, bi = lcd.control >> 9 & 1;
                for (i = 0; i < 320; i++) {
                    uint16_t color = ((uint16_t *)in)[i ^ bi];
                    uint8_t b = color & 0x1F,
                            g = (color >> 5) & 0x1F,
                            r = (color >> 10) & 0x1F;

                    out[i] = (r << 11) | (g << 6) | b | (color >> 10 & 0x20);
                }
                in += 160;
            } else if (mode == 5) {
                // 32bpp mode: Convert 888 to 565
                do {
                    uint32_t word = *in++;
                    *out++ = (word >> 8 & 0xF800) | (word >> 5 & 0x7E0) | (word >> 3 & 0x1F);
                } while (--words != 0);
            } else {
                if (!(lcd.control & (1 << 9))) {
                    memcpy(out, in, 640);
                    in += 160;
                } else {
                    uint32_t *outw = (uint32_t *)out;
                    do {
                        uint32_t word = *in++;
                        *outw++ = word << 16 | word >> 16;
                    } while (--words != 0);
                }
            }
        }
    }

    if(emulate_cx)
    {
        if (mode == 7)
        {
            // Convert RGB444 to RGB565
            uint16_t *buffer_end = buffer + 320*240;
            while(buffer < buffer_end)
            {
                uint16_t color = *buffer;
                *buffer++ = (color & 0xF00) << 4 | (color & 0x0F0) << 3 | (color & 0x00F) << 1;
            }
        }

        if (!(lcd.control & (1 << 8)))
        {
            // Convert BGR565 to RGB565
            uint16_t *buffer_end = buffer + 320*240;
            while(buffer < buffer_end)
            {
                uint16_t color = *buffer;
                *buffer++ = (color & 0x001F) << 11 | (color & 0x07E0) | (color & 0xF800) >> 11;
            }
        }
    }

    // Draw the cursor on top
    if(lcd.cursor_control & 1)
        lcd_draw_cursor(buffer);
}

static void lcd_event(int index) {
    int pcd = 1;
    if (!(lcd.timing[2] & (1 << 26)))
        pcd = (lcd.timing[2] >> 27 << 5) + (lcd.timing[2] & 0x1F) + 2;
    int htime = (lcd.timing[0] >> 24 &  0xFF) + 1  // Back porch
            + (lcd.timing[0] >> 16 &  0xFF) + 1  // Front porch
            + (lcd.timing[0] >>  8 &  0xFF) + 1  // Sync pulse
            + (lcd.timing[2] >> 16 & 0x3FF) + 1; // Active
    int vtime = (lcd.timing[1] >> 24 &  0xFF)      // Back porch
            + (lcd.timing[1] >> 16 &  0xFF)      // Front porch
            + (lcd.timing[1] >> 10 &  0x3F) + 1  // Sync pulse
            + (lcd.timing[1]       & 0x3FF) + 1; // Active
    event_repeat(index, pcd * htime * vtime);
    // for now, assuming vcomp occurs at same time UPBASE is loaded
    lcd.framebuffer = lcd.upbase;
    lcd.int_status |= 0xC;
    int_set(INT_LCD, lcd.int_status & lcd.int_mask);

    gif_new_frame();
}

void lcd_reset() {
    // Palette is unchanged on a reset
    memset(&lcd, 0, (char *)&lcd.palette - (char *)&lcd);
    sched.items[SCHED_LCD].clock = emulate_cx ? CLOCK_12M : CLOCK_27M;
    sched.items[SCHED_LCD].second = -1;
    sched.items[SCHED_LCD].proc = lcd_event;
}

uint32_t lcd_read_word(uint32_t addr) {
    uint32_t offset = addr & 0xFFF;
    if (offset < 0x200) {
        switch (offset) {
            case 0x000: case 0x004: case 0x008: case 0x00C:
                return lcd.timing[offset >> 2];
            case 0x010: return lcd.upbase;
            case 0x014: return lcd.lpbase;
            case 0x018: return emulate_cx ? lcd.control : lcd.int_mask; break;
            case 0x01C: return emulate_cx ? lcd.int_mask : lcd.control; break;
            case 0x020:
                cycle_count_delta = 0; // Avoid slowdown by fast-forwarding through polling loops
                return lcd.int_status;
            case 0x024:
                cycle_count_delta = 0; // Avoid slowdown by fast-forwarding through polling loops
                return lcd.int_status & lcd.int_mask;
        }
    } else if (offset < 0x400) {
        return *(uint32_t *)((uint8_t *)lcd.palette + offset - 0x200);
    } else if (offset < 0x800) {
        return bad_read_word(addr);
    } else if (emulate_cx && offset < 0xC00) {
        return *(uint32_t *)((uint8_t *)lcd.cursor_ram + offset - 0x800);
    } else if (emulate_cx && offset < 0xC30) {
        switch (offset) {
        case 0xC00: return lcd.cursor_control;
        case 0xC04: return lcd.cursor_config;
        case 0xC08: return lcd.cursor_palette[0];
        case 0xC0C: return lcd.cursor_palette[1];
        case 0xC10: return lcd.cursor_xy;
        case 0xC14: return lcd.cursor_clip;
        case 0xC20: return lcd.cursor_int_mask;
        case 0xC28: return lcd.cursor_int_status;
        case 0xC2C: return lcd.cursor_int_status & lcd.cursor_int_mask;
        }
    } else if (offset >= 0xFE0) {
        static const uint8_t id[2][8] = {
            /* ARM PrimeCell Color LCD Controller (PL110), Revision 2 */
            { 0x10, 0x11, 0x24, 0x00, 0x0D, 0xF0, 0x05, 0xB1 },
            /* ARM PrimeCell Color LCD Controller (PL111), Revision 1 */
            { 0x11, 0x11, 0x14, 0x00, 0x0D, 0xF0, 0x05, 0xB1 }
        };
        return id[emulate_cx][(offset - 0xFE0) >> 2];
    }
    return bad_read_word(addr);
}

void lcd_write_word(uint32_t addr, uint32_t value) {
    uint32_t offset = addr & 0xFFF;
    if (offset < 0x200) {
        switch (offset) {
            case 0x000: case 0x004: case 0x008: case 0x00C:
                lcd.timing[offset >> 2] = value;
                return;
            case 0x010: lcd.upbase = value & ~0b111; if(value & 0b111) gui_debug_printf("Warning: LCD framebuffer not 8-byte aligned!"); return;
            case 0x014: lcd.lpbase = value & ~0b111; if(value & 0b111) gui_debug_printf("Warning: LCD framebuffer not 8-byte aligned!"); return;
            case 0x018:
                if (emulate_cx) goto write_control;
write_int_mask:
                lcd.int_mask = value & 0x1E;
                int_set(INT_LCD, lcd.int_status & lcd.int_mask);
                return;
            case 0x01C:
                if (emulate_cx) goto write_int_mask;
write_control:
                if ((value ^ lcd.control) & 1) {
                    if (value & 1)
                        event_set(SCHED_LCD, 0);
                    else
                        event_clear(SCHED_LCD);
                }
                lcd.control = value;
                return;
            case 0x028:
                lcd.int_status &= ~value;
                int_set(INT_LCD, lcd.int_status & lcd.int_mask);
                return;
        }
    } else if (offset < 0x400) {
        *(uint32_t *)((uint8_t *)lcd.palette + offset - 0x200) = value;
        return;
    } else if (offset < 0x800) {
        bad_write_word(addr, value);
        return;
    } else if (emulate_cx && offset < 0xC00) {
        *(uint32_t *)((uint8_t *)lcd.cursor_ram + offset - 0x800) = value;
        return;
    } else if (emulate_cx && offset < 0xC30) {
        switch (offset) {
        case 0xC00: lcd.cursor_control = value; return;
        case 0xC04: lcd.cursor_config = value; return;
        case 0xC08: lcd.cursor_palette[0] = value; return;
        case 0xC0C: lcd.cursor_palette[1] = value; return;
        case 0xC10: lcd.cursor_xy = value; return;
        case 0xC14: lcd.cursor_clip = value; return;
        case 0xC20: lcd.cursor_int_mask = value; return;
        case 0xC24: lcd.cursor_int_status &= ~value; return;
        }
    }
    bad_write_word(addr, value);
    return;
}

bool lcd_suspend(emu_snapshot *snapshot)
{
    return snapshot_write(snapshot, &lcd, sizeof(lcd));
}

bool lcd_resume(const emu_snapshot *snapshot)
{
    return snapshot_read(snapshot, &lcd, sizeof(lcd));
}
