#include <cassert>
#include <queue>

#include "emu.h"
#include "mem.h"
#include "usb_cx2.h"
#include "usblink_cx2.h"

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

    if (usb_cx2.rxzlp)
        usb_cx2.gisr[2] |= 1 << 6;
    if (usb_cx2.txzlp)
        usb_cx2.gisr[2] |= 1 << 5;
    for(int i = 0; i < 3; ++i)
        if(usb_cx2.gisr[i] & ~usb_cx2.gimr[i])
            usb_cx2.gisr_all |= 1 << i;

    if(usb_cx2.dmasr & ~usb_cx2.dmamr)
        usb_cx2.gisr_all |= 1 << 3; // Device DMA interrupt

    // Update controller interrupt status
    usb_cx2.isr = 0;

    if((usb_cx2.gisr_all & ~usb_cx2.gimr_all) && (usb_cx2.devctrl & 0b100))
        usb_cx2.isr |= 1 << 0; // Device interrupt

    if(usb_cx2.otgisr & usb_cx2.otgier)
        usb_cx2.isr |= 1 << 1; // OTG interrupt

    if(usb_cx2.usbsts & usb_cx2.usbintr)
        usb_cx2.isr |= 1 << 2; // Host interrupt

    int_set(INT_USB, usb_cx2.isr & ~usb_cx2.imr);
}

struct usb_packet {
    usb_packet(uint16_t size) : size(size) {}
    uint16_t size;
    uint8_t data[1024];
};

std::queue<usb_packet> send_queue;

static bool usb_cx2_real_packet_to_calc(uint8_t ep, const uint8_t *packet, size_t size)
{
    uint8_t fifo = (usb_cx2.epmap[ep > 4] >> (8 * ((ep - 1) & 0b11) + 4)) & 0b11;

    // +1 to adjust for the hack below
    if(size + 1 > sizeof(usb_cx2.fifo[fifo].data) - usb_cx2.fifo[fifo].size)
        return false;

    memcpy(&usb_cx2.fifo[fifo].data[usb_cx2.fifo[fifo].size], packet, size);

    /* Hack ahead! Counterpart to the receiving side in usblink_cx2.
     * The nspire code has if(size & 0x3F == 0) ++size; for some reason,
     * so send them that way here as well. It's probably to avoid having to
     * deal with zero-length packets.
     * TODO: Move to usblink_cx2.cpp as NNSE specific. */
    if((size & 0x3F) == 0)
        ++size;

    usb_cx2.fifo[fifo].size += size;
    usb_cx2.gisr[1] |= 1 << (fifo * 2); // FIFO OUT IRQ
    auto packet_size = usb_cx2.epout[(ep - 1) & 7] & 0x7ff;
    if(size % packet_size) // Last pkt is short?
        usb_cx2.gisr[1] |= 1 << ((fifo * 2) + 1); // FIFO SPK IRQ
    else // ZLP needed
        error("Sending zero-length packets not implemented");

    usb_cx2_int_check();
    return true;
}

bool usb_cx2_packet_to_calc(uint8_t ep, const uint8_t *packet, size_t size)
{
    if(size > sizeof(usb_cx2.fifo[0].data) || size > sizeof(usb_packet::data))
        return false;

    // If the FIFO is busy, queue it for sending later
    uint8_t fifo = (usb_cx2.epmap[ep > 4] >> (8 * ((ep - 1) & 0b11) + 4)) & 0b11;
    if(usb_cx2.fifo[fifo].size)
    {
        send_queue.emplace(size);
        memcpy(send_queue.back().data, packet, size);
        return true;
    }
    else
        return usb_cx2_real_packet_to_calc(ep, packet, size);
}

static void usb_cx2_packet_from_calc(uint8_t ep, uint8_t *packet, size_t size)
{
    if(ep != 1)
        error("Got packet on unknown EP");

    if(!usblink_cx2_handle_packet(packet, size))
        warn("Packet not handled");
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
    usb_cx2.gisr[1] |= 0b1111 << 16;
    usb_cx2.gisr[2] |= 1;

    usb_cx2_int_check();
    usblink_cx2_reset();
}

void usb_cx2_bus_reset_on()
{
    usb_cx2.portsc &= ~1;
    usb_cx2.portsc |= 0x0C000100;
    usb_cx2.usbsts |= 0x40;
    usb_cx2_int_check();
    //gui_debug_printf("usb reset on\n");
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
    //gui_debug_printf("usb reset off\n");
}

void usb_cx2_receive_setup_packet(const usb_setup *packet)
{
    // Copy data into DMA buffer
    static_assert(sizeof(*packet) == sizeof(usb_cx2.setup_packet), "");
    memcpy(usb_cx2.setup_packet, packet, sizeof(usb_cx2.setup_packet));

    // EP0 Setup packet
    usb_cx2.gisr[0] |= 1;

    usb_cx2_int_check();
}
}

void usb_cx2_dma1_update()
{
    if(!(usb_cx2.fdma[1].ctrl & 1))
        return; // DMA disabled

    bool fromMemory = usb_cx2.fdma[1].ctrl & 0b10;
    if(fromMemory)
        error("Not implemented");

    int fifo = 0;

    size_t length = (usb_cx2.fdma[1].ctrl >> 8) & 0x1ffff;
    length = std::min(length, size_t(usb_cx2.fifo[fifo].size));
    uint8_t *ptr = (uint8_t*) phys_mem_ptr(usb_cx2.fdma[1].addr, length);
    memcpy(ptr, usb_cx2.fifo[fifo].data, length);

    usb_cx2.dmasr |= 1 << (fifo + 1); // DMA done

    // Move the remaining data to the start
    usb_cx2.fifo[fifo].size -= length;
    if(usb_cx2.fifo[fifo].size)
        memmove(usb_cx2.fifo[fifo].data, usb_cx2.fifo[fifo].data + length, usb_cx2.fifo[fifo].size);
    else
    {
        usb_cx2.gisr[1] &= ~(0b11 << (fifo * 2)); // FIFO0 OUT/SPK

        if(send_queue.size())
        {
            usb_cx2_packet_to_calc(1, send_queue.front().data, send_queue.front().size);
            send_queue.pop();
        }
    }

    usb_cx2_int_check();
}

void usb_cx2_dma2_update()
{
    if(!(usb_cx2.fdma[2].ctrl & 1))
        return; // DMA disabled

    bool fromMemory = usb_cx2.fdma[2].ctrl & 0b10;
    if(!fromMemory)
        error("Not implemented");

    int fifo = 1;

    size_t length = (usb_cx2.fdma[2].ctrl >> 8) & 0x1ffff;
    uint8_t *ptr = (uint8_t*) phys_mem_ptr(usb_cx2.fdma[2].addr, length);
    usb_cx2.fifo[fifo].size = 0;

    uint8_t ep = (usb_cx2.fifomap >> (fifo * 8)) & 0xF;
    usb_cx2_packet_from_calc(ep, ptr, length);

    usb_cx2.dmasr |= 1 << (fifo + 1); // DMA done
    //usb_cx2.gisr[1] |= 1 << (16 + fifo); // FIFO1 IN

    usb_cx2_int_check();
}

uint32_t usb_cx2_read_word(uint32_t addr)
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
    {
        uint32_t value = usb_cx2.cxfifo.size << 24;
        for (int fifo = 0; fifo < 4; fifo++)
            if (!usb_cx2.fifo[fifo].size)
                value |= 1 << (8 + fifo); // FIFOE
        if (!usb_cx2.cxfifo.size)
            value |= 1 << 5; // CX FIFO empty
        if (usb_cx2.cxfifo.size == sizeof(usb_cx2.cxfifo.data))
            value |= 1 << 4; // CX FIFO full
        return value;
    }
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
    case 0x150:
        return usb_cx2.rxzlp;
    case 0x154:
        return usb_cx2.txzlp;
    case 0x160: case 0x164:
    case 0x168: case 0x16c:
        return usb_cx2.epin[(offset - 0x160) >> 2];
    case 0x180: case 0x184:
    case 0x188: case 0x18c:
        return usb_cx2.epout[(offset - 0x180) >> 2];
    case 0x1A0: // EP 1-4 map register
    case 0x1A4: // EP 5-8 map register
        return usb_cx2.epmap[(offset - 0x1A0) >> 2];
    case 0x1A8: // FIFO map register
        return usb_cx2.fifomap;
    case 0x1AC: // FIFO config register
        return usb_cx2.fifocfg;
    case 0x1B0: // FIFO 0 status
    case 0x1B4: // FIFO 1 status
    case 0x1B8: // FIFO 2 status
    case 0x1BC: // FIFO 3 status
        return usb_cx2.fifo[(offset - 0x1B0) >> 2].size;
    case 0x1C0:
        return usb_cx2.dmafifo;
    case 0x1C8:
        return usb_cx2.dmactrl;
    case 0x1D0: // CX FIFO PIO register
    {
        uint32_t ret = usb_cx2.setup_packet[0];
        usb_cx2.setup_packet[0] = usb_cx2.setup_packet[1];
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

void usb_cx2_write_word(uint32_t addr, uint32_t value)
{
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
        usb_cx2.usbsts &= ~(value & 0x3F);
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
    case 0x0C0:
        usb_cx2.isr &= ~value;
        usb_cx2_int_check();
        return;
    case 0x0C4:
        usb_cx2.imr = value & 0b111;
        usb_cx2_int_check();
        return;
    case 0x100:
        usb_cx2.devctrl = value;
        usb_cx2_int_check();
        return;
    case 0x104:
        usb_cx2.devaddr = value;

        /*if(value == 0x81) // Configuration set
            gui_debug_printf("Completed SET_CONFIGURATION\n");*/

        return;
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
        if (value & 0b1000) // Clear CX FIFO
            usb_cx2.cxfifo.size = 0;

        if (value & 0b100) // Stall CX FIFO
            error("control endpoint stall");

        if (value & 0b10) // Test transfer finished
            error("test transfer finished");

        if (value & 0b1) // Setup transfer finished
        {
            // Clear EP0 OUT/IN/SETUP packet IRQ
            usb_cx2.gisr[0] &= ~0b111;
            usb_cx2_int_check();

            if (usb_cx2.devaddr == 1)
            {
                struct usb_setup packet = { 0, 9, 1, 0, 0 };
                usb_cx2_receive_setup_packet(&packet);
            }
        }

        return;
    }
    case 0x124: // IDLE counter
        return;
    case 0x130: // GIMR_ALL
        usb_cx2.gimr_all = value & 0b111;
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
        usb_cx2.rxzlp = value;
        if(value)
            error("Not implemented");
        usb_cx2_int_check();
        return;
    case 0x154: // Tx zero length pkt
        usb_cx2.txzlp = value;
        if(value)
            error("Not implemented");
        usb_cx2_int_check();
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
    case 0x1A4: // EP 5-8 map register
        usb_cx2.epmap[(offset - 0x1A0) >> 2] = value;
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
    case 0x1BC: // FIFO 3 status
        if(value & (1 << 12))
            usb_cx2.fifo[(offset - 0x1B0) >> 2].size = 0;
        return;
    case 0x1C0:
        usb_cx2.dmafifo = value;
        if(value != 0 && value != 0x10)
            error("Not implemented");
        return;
    case 0x1C8:
        usb_cx2.dmactrl = value;
        if(value != 0)
            error("Not implemented");
        return;
    case 0x308:
        usb_cx2.fdma[1].ctrl = value;
        usb_cx2_dma1_update();
        return;
    case 0x30C:
        usb_cx2.fdma[1].addr = value;
        return;
    case 0x310:
        usb_cx2.fdma[2].ctrl = value;
        usb_cx2_dma2_update();
        return;
    case 0x314:
        usb_cx2.fdma[2].addr = value;
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

bool usb_cx2_suspend(emu_snapshot *snapshot)
{
    snapshot->mem.usb_cx2 = usb_cx2;
    return true;
}

bool usb_cx2_resume(const emu_snapshot *snapshot)
{
    usb_cx2 = snapshot->mem.usb_cx2;
    return true;
}
