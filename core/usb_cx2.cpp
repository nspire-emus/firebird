#include "emu.h"
#include "mem.h"
#include "usb_cx2.h"
#include "usblink.h"

usb_cx2_state usb_cx2;

static void usb_cx2_int_check()
{
    // Update device controller group interrupt status
    usb_cx2.gisr_all = 0;

    for(int i = 0; i < 3; ++i)
        if(usb_cx2.gisr[i] & ~usb_cx2.gimr[i])
            usb_cx2.gisr_all |= 1 << i;

    // Update controller interrupt status
    usb_cx2.isr = 0;

    if((usb_cx2.gisr_all & ~usb_cx2.gimr_all) && (usb_cx2.devctrl & 0b100))
        usb_cx2.isr |= 1 << 0; // Device interrupt

    if(usb_cx2.otgisr & usb_cx2.otgier)
        usb_cx2.isr |= 1 << 1; // OTG interrupt

    if(usb_cx2.usbsts & usb_cx2.usbintr)
        usb_cx2.isr |= 1 << 2; // Host interrupt

    /*static int i = 0;
    if(i++ < 80)
    {
        gui_debug_printf("gisr_all %x & ~%x = %x\n", usb_cx2.gisr_all, usb_cx2.gimr_all, usb_cx2.gisr_all & ~usb_cx2.gimr_all);
gui_debug_printf("%x & ~%x = %x\n", usb_cx2.isr, usb_cx2.imr, usb_cx2.isr & ~usb_cx2.imr);
    }
    else
        exiting = true;*/

    int_set(INT_USB, usb_cx2.isr & ~usb_cx2.imr);
}

extern "C" {
void usb_cx2_reset()
{
    usb_cx2 = {};
    usb.usbcmd = 0x80000;
    usb.portsc = 0xEC000004;
    usb_cx2.usbcmd = 0x80000;
    // All IRQs masked
    usb_cx2.imr = 0xF;
    usb_cx2.otgier = 0;

    // High speed, B-device, Acts as device
    usb_cx2.otgcs = (2 << 22) | (1 << 21) | (1 << 20);
    // Reset IRQ
    usb_cx2.gisr[2] |= 1;

    usb_cx2_int_check();
    usblink_reset();
}

void usb_cx2_bus_reset_on()
{
    usb_cx2.portsc &= ~1;
    usb_cx2.portsc |= 0x0C000100;
    usb_cx2.usbsts |= 0x40;
    usb_cx2_int_check();
    gui_debug_printf("usb reset on\n");
}

void usb_cx2_bus_reset_off()
{
    usb_cx2.otgcs = (2 << 22) | (1 << 21) | (1 << 20) | (1 << 17);
    usb_cx2.otgisr = (1 << 9) | (1 << 8);

    // Device idle
    usb_cx2.gisr[2] |= (1 << 9);

    usb_cx2.portsc &= ~0x0C000100;
    usb_cx2.portsc |= 1;
    usb_cx2.usbsts |= 4;
    usb_cx2_int_check();
    gui_debug_printf("usb reset off\n");
}

void usb_cx2_receive_setup_packet(const void *packet)
{
    // Copy data into DMA buffer
    usb_cx2.dma_size = 8;
    memcpy(usb_cx2.dma_data, packet, 8);
gui_debug_printf("RECEIVE SETUP PACKET\n");
    // EP0 Setup packet
    usb_cx2.gisr[0] |= 1;

    usb_cx2_int_check();
}
}

void usb_cx2_cxfifo_update()
{
    if(usb_cx2.dmafifo != 0x10)
    {
        // Not enabled -> empty
        usb_cx2.cxfifo = 1 << 5;
        return;
    }

    usb_cx2.cxfifo = usb_cx2.dma_size << 8;
    if(usb_cx2.dma_size == 0)
        usb_cx2.cxfifo |= 1 << 5; // Empty
}


uint32_t usb_cx2_read_word_real(uint32_t addr)
{
    uint32_t offset = addr & 0xFFF;
    switch(offset)
    {
    case 0x000: // CAPLENGTH + HCIVERSION
        return 0x01000010;
    case 0x004: // HCSPARAMS
        return 0x00000001;
    case 0x010: // USBCMD
        return usb_cx2.usbcmd;
    case 0x014: // USBSTS
        return usb_cx2.usbsts;
    case 0x018: // USBINTR
        return usb_cx2.usbintr;
    /*case 0x01C: // FRINDEX
    case 0x020: // CTRLDSSEGMENT
    case 0x024: // PERIODICLISTBASE
    case 0x028: // ASYNCLISTADDR
        return usb_read_word(addr - 0x10 + 0x140); // Same as CX, just moved*/
    case 0x030: // PORTSC (earlier than the spec...)
        return usb_cx2.portsc;
    case 0x040:
        return usb_cx2.miscr;
    case 0x080: // OTGCS
        return usb_cx2.otgcs;
    case 0x084:
        return usb_cx2.otgisr;
    case 0x088:
        return usb_cx2.otgier;
    case 0x0C0:
        return usb_cx2.isr;
    case 0x0C4:
        return usb_cx2.imr;
    case 0x100:
        return usb_cx2.devctrl;
    case 0x104:
        return usb_cx2.devaddr;
    case 0x120:
        return usb_cx2.cxfifo;
    case 0x130:
        return usb_cx2.gimr_all;
    case 0x134: case 0x138:
    case 0x13c:
        return usb_cx2.gimr[(offset - 0x134) >> 2];
    case 0x140:
        return usb_cx2.gisr_all;
    case 0x144: case 0x148:
    case 0x14c:
        return usb_cx2.gisr[(offset - 0x144) >> 2];
    case 0x1c0:
        return usb_cx2.dmafifo;
    case 0x1c8:
        return usb_cx2.dmactrl;
    case 0x1d0: // CX FIFO PIO register
    {
        gui_debug_printf("USB FIFO READ\n");

        uint32_t ret = usb_cx2.dma_data[0]
                | usb_cx2.dma_data[1] << 8
                | usb_cx2.dma_data[2] << 16
                | usb_cx2.dma_data[3] << 24;

        if(usb_cx2.dma_size < 4)
            usb_cx2.dma_size = 0;
        else
            usb_cx2.dma_size -= 4;

        usb_cx2_cxfifo_update();
        return ret;
    }
    case 0x330:
        return 0; // ?
    }
    return bad_read_word(addr);
}

uint32_t usb_cx2_read_word(uint32_t addr)
{
    auto value = usb_cx2_read_word_real(addr);
    //gui_debug_printf("RW %x = %x @ %x @ %x\n", addr, value, arm.reg[15], arm.reg[14]);
    return value;
}

void usb_cx2_write_word(uint32_t addr, uint32_t value)
{
    gui_debug_printf("WW %x = %x @ %x @ %x\n", addr, value, arm.reg[15], arm.reg[14]);
    uint32_t offset = addr & 0xFFF;
    switch(offset)
    {
    case 0x010: // USBCMD
        if (value & 2) {
            usb_cx2_reset();
            return;
        }
        usb_cx2.usbcmd = value;
        return;
    case 0x014: // USBSTS
        usb_cx2.usbsts &= ~value;
        usb_cx2_int_check();
        return;
    case 0x018: // USBINTR
        usb_cx2.usbintr = value & 0x030101D7;
        usb_cx2_int_check();
        return;
    /*case 0x01C: // FRINDEX
    case 0x020: // CTRLDSSEGMENT
    case 0x024: // PERIODICLISTBASE
    case 0x028: // ASYNCLISTADDR
        return usb_write_word(addr - 0x10 + 0x140, value); // Same as CX, just moved*/
    case 0x030: // PORTSC (earlier than the spec...)
        break;
    case 0x040:
        usb_cx2.miscr = value;
        return;
    case 0x080: // OTGCS
        usb_cx2.otgcs = value;
        return;
    case 0x084: // OTGISR
        usb_cx2.otgisr &= ~value;
        usb_cx2_int_check();
        return;
    case 0x088:
        usb_cx2.otgier = value;
        usb_cx2_int_check();
        return;
    case 0x0C4:
        usb_cx2.imr = value;
        usb_cx2_int_check();
        return;
    case 0x100:
        usb_cx2.devctrl = value;
        usb_cx2_int_check();
        return;
    case 0x104:
        usb_cx2.devaddr = value;
        return;
    case 0x120:
    {
        uint32_t bits = value ^ usb_cx2.cxfifo;
        if(bits & 0b1000) // Clear FIFO
        {
            usb_cx2.dma_size = 0;
            bits &= ~(1 << 3);
        }

        if(bits & 1) // Transfer done
            bits &= ~1;

        if(bits != 0)
            error("Not implemented");

        usb_cx2_cxfifo_update();
        return;
    }
    case 0x1c0:
        usb_cx2.dmafifo = value;
        if(value != 0 && value != 0x10)
            error("Not implemented");
gui_debug_printf("DMAFIFO SET 0x%x\n", value);
        usb_cx2_cxfifo_update();
        return;
    case 0x1c8:
        usb_cx2.dmactrl = value;
        if(value != 0)
            error("Not implemented");
        return;
    case 0x130: // GIMR_ALL
        // Should be read only?
        return;
    case 0x134: case 0x138:
    case 0x13c:
        usb_cx2.gimr[(offset - 0x134) >> 2] = value;
        usb_cx2_int_check();
        return;
    case 0x144: case 0x148:
    case 0x14c:
        usb_cx2.gisr[(offset - 0x144) >> 2] &= ~value;
        usb_cx2_int_check();
        return;
    case 0x32c:
        // No idea
        return;
    }
    bad_write_word(addr, value);
}

uint8_t usb_cx2_read_byte(uint32_t addr)
{
    switch(addr & 0xFFF)
    {
    case 0x00: // CAPLENGTH
        return 0x10;
    }
    return bad_read_byte(addr);
}

uint16_t usb_cx2_read_half(uint32_t addr)
{
    switch(addr & 0xFFF)
    {
    case 0x02: // HCIVERSION
        return 0x100;
    }
    return bad_read_half(addr);
}
