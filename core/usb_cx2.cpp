#include "emu.h"
#include "mem.h"
#include "usb_cx2.h"
#include "usblink.h"

// TODO: Move into a separate header for usb stuff and add packed attrib
struct usb_setup {
    uint8_t bmRequestType;
    uint8_t bRequest;
    uint16_t wValue;
    uint16_t wIndex;
    uint16_t wLength;
};

usb_cx2_state usb_cx2;

static void usb_cx2_int_check()
{
    // Update device controller group interrupt status
    usb_cx2.gisr_all = 0;

    for(int i = 0; i < 3; ++i)
        if(usb_cx2.gisr[i] & ~usb_cx2.gimr[i])
            usb_cx2.gisr_all |= 1 << i;

    //if(usb_cx2.dmasr & ~usb_cx2.dmamr)
    //    usb_cx2.gisr_all |= 1 << 0; // Device DMA interrupt (?)

    // Update controller interrupt status
    usb_cx2.isr = 0;

    if((usb_cx2.gisr_all & ~usb_cx2.gimr_all) && (usb_cx2.devctrl & 0b100))
    {
        usb_cx2.isr |= 1 << 0; // Device interrupt

        gui_debug_printf("USB_INT!\n");
        gui_debug_printf("Group 0: %x %x\n", usb_cx2.gisr[0], usb_cx2.gimr[0]);
        gui_debug_printf("Group 1: %x %x\n", usb_cx2.gisr[1], usb_cx2.gimr[1]);
        gui_debug_printf("Group 2: %x %x\n", usb_cx2.gisr[2], usb_cx2.gimr[2]);
    }

    if(usb_cx2.otgisr & usb_cx2.otgier)
        usb_cx2.isr |= 1 << 1; // OTG interrupt

    if(usb_cx2.usbsts & usb_cx2.usbintr)
        usb_cx2.isr |= 1 << 2; // Host interrupt

    int_set(INT_USB, usb_cx2.isr & ~usb_cx2.imr);
}

bool usb_cx2_packet_to_calc(uint8_t ep, uint8_t *packet, size_t size)
{
    assert(ep == 1);

    if(size > sizeof(usb_cx2.dma_data))
        return false;

    memcpy(usb_cx2.dma_data, packet, size);

    // Send packet to EP 0x1
    uint8_t fifo = (usb_cx2.epmap14 >> (8 * ep + 4)) & 3;
    usb_cx2.dma_size = size;
    usb_cx2.gisr[1] |= 1 << (fifo * 2); // FIFO OUT IRQ
    usb_cx2_int_check();

    return true;
}

static void usb_cx2_packet_from_calc(uint8_t ep, uint8_t *packet, size_t size)
{
    gui_debug_printf("Got packet ep %d size %d (seq %d)!\n", ep, size, packet[9] | packet[8] << 8);

    // TODO: Get receiving working first
    return;

    uint8_t pkt[13] = {};
    pkt[1] = 1;
    pkt[2] = 0xfe;
    pkt[3] = 1;
    pkt[7] = 0xD;
    pkt[11] = 0xf0;
    pkt[12] = 0x01;
    usb_cx2_packet_to_calc(1, pkt, sizeof(pkt));
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

void usb_cx2_dma1_update()
{
    if(!(usb_cx2.dma1ctrl & 1))
        return; // DMA disabled

    bool fromMemory = usb_cx2.dma1ctrl & 0b10;
    if(fromMemory)
        error("Not implemented");

    size_t length = (usb_cx2.dma1ctrl >> 8) & 0x1ffff;
    length = std::min(length, size_t(usb_cx2.dma_size));
    uint8_t *ptr = (uint8_t*) phys_mem_ptr(usb_cx2.dma1addr, length);
    memcpy(ptr, usb_cx2.dma_data, length);

    // Set DMA done bit in dmasr
    usb_cx2.dmasr |= 1 << (1 + 16);

    // Move the remaining data to the start
    usb_cx2.dma_size -= length;
    if(usb_cx2.dma_size)
        memmove(usb_cx2.dma_data, usb_cx2.dma_data + length, usb_cx2.dma_size);
    else
        usb_cx2.gisr[1] &= ~(1 << (0 * 2));

    usb_cx2_int_check();
}

void usb_cx2_dma2_update()
{
    if(!(usb_cx2.dma2ctrl & 1))
        return; // DMA disabled

    bool fromMemory = usb_cx2.dma2ctrl & 0b10;
    if(!fromMemory)
        error("Not implemented");

    size_t length = (usb_cx2.dma2ctrl >> 8) & 0x1ffff;
    uint8_t *ptr = (uint8_t*) phys_mem_ptr(usb_cx2.dma2addr, length);
    usb_cx2.dma_size = 0;

    /* Why does this not work?
    uint8_t ep = (usb_cx2.fifomap >> (2 * 8)) & 0xF;
    gui_debug_printf("EP14map: %x\n", usb_cx2.epmap14);
    gui_debug_printf("FIFOmap: %x\n", usb_cx2.fifomap);
    gui_debug_printf("FIFOcfg: %x\n", usb_cx2.fifocfg);*/

    uint8_t ep = 1;
    usb_cx2_packet_from_calc(ep, ptr, length);

    // Apparently what's down here is not sufficient :-/

    // Set DMA done bit in dmasr
    usb_cx2.dmasr |= 1 << 2;

    usb_cx2.gisr[2] |= 1 << 7; // DMA complete

    usb_cx2_int_check();
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
    case 0x008: // HCCPARAMS
        return 0;
    case 0x010: // USBCMD
        return usb_cx2.usbcmd;
    case 0x014: // USBSTS
        return usb_cx2.usbsts;
    case 0x018: // USBINTR
        return usb_cx2.usbintr;
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
    case 0x108:
        return usb_cx2.devtest;
    case 0x114:
        return usb_cx2.phytest;
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
    case 0x14C:
        return usb_cx2.gisr[(offset - 0x144) >> 2];
    case 0x160: case 0x164:
    case 0x168: case 0x16c:
        return usb_cx2.epin[(offset - 0x160) >> 2];
    case 0x180: case 0x184:
    case 0x188: case 0x18c:
        return usb_cx2.epout[(offset - 0x180) >> 2];
    case 0x1A0: // EP 1-4 map register
        return usb_cx2.epmap14;
    case 0x1A8: // FIFO map register
        return usb_cx2.fifomap;
    case 0x1AC: // FIFO config register
        return usb_cx2.fifocfg;
    case 0x1B0: // FIFO 0 status
    case 0x1B4: // FIFO 1 status
    case 0x1B8: // FIFO 2 status
        return 0;
    case 0x1C0:
        return usb_cx2.dmafifo;
    case 0x1C8:
        return usb_cx2.dmactrl;
    case 0x1D0: // CX FIFO PIO register
    {
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
    case 0x328:
        return usb_cx2.dmasr;
    case 0x32C:
        return usb_cx2.dmamr;
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
    case 0x000: // CAPLENGTH + HCIVERSION
        // This is actually read-only, but the OS seems to write a 2 in here.
        // I guess it should've been a write to 0x010 instead.
        return;
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
    case 0x01C: // FRINDEX
    case 0x020: // CTRLDSSEGMENT
    case 0x024: // PERIODICLISTBASE
    case 0x028: // ASYNCLISTADDR
        return; // USB HOST stuff, just ignore
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
    {
        usb_cx2.devaddr = value;

        if(value == 1) // Address set
        {
            /* TODO: How did it work previously without a timer?
            Sending SET_CONFIGURATION on all writes to 0x104 results in a 0x80, so an invalid address.

            struct usb_setup packet = { 0, 9, 1, 0, 0 };
            gui_debug_printf("Sending SET_CONFIGURATION\n");
            usb_cx2_receive_setup_packet(&packet);*/
            usblink_state = 5;
        }
        else if(value == 0x81) // Configuration set
            gui_debug_printf("Completed SET_CONFIGURATION\n");

        return;
    }
    case 0x108:
        usb_cx2.devtest = value;
        return;
    case 0x110: // SOF mask timer
        return;
    case 0x114:
        usb_cx2.phytest = value;
        return;
    case 0x120:
    {
        uint32_t bits = value ^ usb_cx2.cxfifo;
        if(bits & 0b1000) // Clear FIFO
        {
            bits &= ~0b1000;
            usb_cx2.dma_size = 0;
        }

        if(bits & 1) // Transfer done
        {
            bits &= ~1;

            // Clear EP0 OUT/IN/SETUP packet IRQ
            usb_cx2.gisr[0] &= ~0b111;
            usb_cx2_int_check();
        }

        if(bits != 0)
            error("Not implemented");

        usb_cx2_cxfifo_update();
        return;
    }
    case 0x124: // IDLE counter
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
    case 0x150: // Rx zero length pkt
    case 0x154: // Tx zero length pkt
        if(value != 0)
            error("Not implemented");
        return;
    case 0x160: case 0x164:
    case 0x168: case 0x16c:
        usb_cx2.epin[(offset - 0x160) >> 2] = value;
        return;
    case 0x180: case 0x184:
    case 0x188: case 0x18c:
        usb_cx2.epout[(offset - 0x180) >> 2] = value;
        return;
    case 0x1A0: // EP 1-4 map register
        usb_cx2.epmap14 = value;
        return;
    case 0x1A8: // FIFO map register
        usb_cx2.fifomap = value;
        return;
    case 0x1AC: // FIFO config register
        usb_cx2.fifocfg = value;
        return;
    case 0x1B0: // FIFO 0 status
    case 0x1B4: // FIFO 1 status
    case 0x1B8: // FIFO 2 status
        if(value & (1 << 12))
            usb_cx2.dma_size = 0;
        return;
    case 0x1C0:
        usb_cx2.dmafifo = value;
        if(value != 0 && value != 0x10)
            error("Not implemented");

        usb_cx2_cxfifo_update();
        return;
    case 0x1C8:
        usb_cx2.dmactrl = value;
        if(value != 0)
            error("Not implemented");
        return;
    case 0x308:
        usb_cx2.dma1ctrl = value;
        usb_cx2_dma1_update();
        return;
    case 0x30C:
        usb_cx2.dma1addr = value;
        return;
    case 0x310:
        usb_cx2.dma2ctrl = value;
        usb_cx2_dma2_update();
        return;
    case 0x314:
        usb_cx2.dma2addr = value;
        return;
    case 0x328:
        usb_cx2.dmasr &= ~value;
        usb_cx2_int_check();
        return;
    case 0x32c:
        usb_cx2.dmamr = value;
        usb_cx2_int_check();
        return;
    case 0x330:
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
