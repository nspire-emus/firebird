#include <string.h>
#include "emu.h"
#include "interrupt.h"
#include "cpu.h"
#include "mem.h"

/* DC000000: Interrupt controller */
interrupt_state intr;

static void get_current_int(int is_fiq, int *current) {
    uint32_t masked_status = intr.status & intr.mask[is_fiq];
    int pri_limit = intr.priority_limit[is_fiq];
    int i;
    for (i = 0; i < 32; i++) {
        if (masked_status & (1u << i) && intr.priority[i] < pri_limit) {
            *current = i;
            pri_limit = intr.priority[i];
        }
    }
}

static void update() {
    uint32_t prev_raw_status = intr.raw_status;
    intr.raw_status = intr.active ^ ~intr.noninverted;

    intr.sticky_status |= (intr.raw_status & ~prev_raw_status);
    intr.status = (intr.raw_status    & ~intr.sticky)
                | (intr.sticky_status &  intr.sticky);

    int is_fiq;
    for (is_fiq = 0; is_fiq < 2; is_fiq++) {
        int i = -1;
        get_current_int(is_fiq, &i);
        if (i >= 0) {
            arm.interrupts |= 0x80 >> is_fiq;
            cpu_int_check();
        }
    }
}

uint32_t int_read_word(uint32_t addr) {
    int group = addr >> 8 & 3;
    if (group < 2) {
        int is_fiq = group;
        int current = 0;
        switch (addr & 0xFF) {
            case 0x00:
                return intr.status & intr.mask[is_fiq];
            case 0x04:
                return intr.status;
            case 0x08:
            case 0x0C:
                return intr.mask[is_fiq];
            case 0x20:
                get_current_int(is_fiq, &current);
                return current;
            case 0x24:
                get_current_int(is_fiq, &current);
                intr.prev_pri_limit[is_fiq] = intr.priority_limit[is_fiq];
                intr.priority_limit[is_fiq] = intr.priority[current];
                return current;
            case 0x28:
                current = -1;
                get_current_int(is_fiq, &current);
                if (current < 0) {
                    arm.interrupts &= ~(0x80 >> is_fiq);
                    cpu_int_check();
                }
                return intr.prev_pri_limit[is_fiq];
            case 0x2C:
                return intr.priority_limit[is_fiq];
        }
    } else if (group == 2) {
        switch (addr & 0xFF) {
            case 0x00: return intr.noninverted;
            case 0x04: return intr.sticky;
            case 0x08: return 0;
        }
    } else {
        if (!(addr & 0x80))
            return intr.priority[addr >> 2 & 0x1F];
    }
    return bad_read_word(addr);
}
void int_write_word(uint32_t addr, uint32_t value) {
    int group = addr >> 8 & 3;
    if (group < 2) {
        int is_fiq = group;
        switch (addr & 0xFF) {
            case 0x04: intr.sticky_status &= ~value; update(); return;
            case 0x08: intr.mask[is_fiq] |= value; update(); return;
            case 0x0C: intr.mask[is_fiq] &= ~value; update(); return;
            case 0x2C: intr.priority_limit[is_fiq] = value & 0x0F; update(); return;
        }
    } else if (group == 2) {
        switch (addr & 0xFF) {
            case 0x00: intr.noninverted = value; update(); return;
            case 0x04: intr.sticky = value; update(); return;
            case 0x08: return;
        }
    } else {
        if (!(addr & 0x80)) {
            intr.priority[addr >> 2 & 0x1F] = value & 7;
            return;
        }
    }
    return bad_write_word(addr, value);
}

static void update_cx() {
    uint32_t active_irqs = intr.active & intr.mask[0] & ~intr.mask[1];
    if (active_irqs != 0)
    {
        // Fallback first
        intr.irq_handler_cur = intr.irq_handler_def;

        // Vectored handling enabled?
        for(unsigned int i = 0; i < sizeof(intr.irq_ctrl_vect)/sizeof(intr.irq_ctrl_vect[0]); ++i)
        {
            uint8_t ctrl = intr.irq_ctrl_vect[i];
            if((ctrl & 0x20) == 0)
                continue; // Disabled

            if(active_irqs & (1 << (ctrl & 0x1F)))
            {
                // Vector enabled and active
                intr.irq_handler_cur = intr.irq_addr_vect[i];
                break;
            }
        }

        arm.interrupts |= 0x80;
    }
    else
        arm.interrupts &= ~0x80;

    if (intr.active & intr.mask[0] & intr.mask[1])
        arm.interrupts |= 0x40;
    else
        arm.interrupts &= ~0x40;
    cpu_int_check();
}

uint32_t int_cx_read_word(uint32_t addr) {
    uint32_t offset = addr & 0x3FFFFFF;
    if(offset < 0x100)
    {
        switch(offset)
        {
        case 0x00: return intr.active & intr.mask[0] & ~intr.mask[1];
        case 0x04: return intr.active & intr.mask[0] & intr.mask[1];
        case 0x08: return intr.active;
        case 0x0C: return intr.mask[1];
        case 0x10: return intr.mask[0];
        case 0x30: return intr.irq_handler_cur;
        case 0x34: return intr.irq_handler_def;
        }
    }
    else if(offset < 0x300)
    {
        uint8_t entry = (offset & 0xFF) >> 2;
        if(entry < 16)
        {
            if(offset < 0x200)
                return intr.irq_addr_vect[entry];
            else
                return intr.irq_ctrl_vect[entry];
        }
    }
    else if(offset >= 0xFE0 && offset < 0x1000) // ID regs
        return (uint32_t[]){0x90, 0x11, 0x04, 0x00, 0x0D, 0xF0, 0x05, 0x81}[(offset - 0xFE0) >> 2];

    return bad_read_word(addr);
}
void int_cx_write_word(uint32_t addr, uint32_t value) {
    uint32_t offset = addr & 0x3FFFFFF;
    if(offset < 0x100)
    {
        switch(offset)
        {
        case 0x004: return;
        case 0x00C: intr.mask[1] = value; update_cx(); return;
        case 0x010: intr.mask[0] |= value; update_cx(); return;
        case 0x014: intr.mask[0] &= ~value; update_cx(); return;
        case 0x01C: return;
        case 0x030: /* An ack, but ignored here. */ return;
        case 0x034: intr.irq_handler_def = value; return;
        }
    }
    else if(offset < 0x300)
    {
        uint8_t entry = (offset & 0xFF) >> 2;
        if(entry < 16)
        {
            if(offset < 0x200)
                intr.irq_addr_vect[entry] = value;
            else
                intr.irq_ctrl_vect[entry] = value;

            return;
        }
    }
    else if(offset == 0x34c)
        return; // No idea. Bug?

    bad_write_word(addr, value);
    return;
}

void int_set(uint32_t int_num, bool on) {
    if (on) intr.active |= 1 << int_num;
    else    intr.active &= ~(1 << int_num);
    if (!emulate_cx)
        update();
    else
        update_cx();
}

void int_reset() {
    memset(&intr, 0, sizeof intr);
    intr.noninverted = -1;
    intr.priority_limit[0] = 8;
    intr.priority_limit[1] = 8;
}

bool interrupt_suspend(emu_snapshot *snapshot)
{
    snapshot->mem.intr = intr;
    return true;
}

bool interrupt_resume(const emu_snapshot *snapshot)
{
    intr = snapshot->mem.intr;
    return true;
}
