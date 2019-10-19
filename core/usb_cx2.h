#ifndef USB_CX2_H
#define USB_CX2_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

struct usb_cx2_state {
    uint32_t usbcmd;    // 10
    uint32_t usbsts;    // 14
    uint32_t usbintr;   // 18
    uint32_t portsc;    // 30
    uint32_t miscr;     // 40
    uint32_t otgcs;     // 80
    uint32_t otgisr;    // 84
    uint32_t otgier;    // 88
    uint32_t isr;       // C0
    uint32_t imr;       // C4
    uint32_t devctrl;   // 100
    uint32_t devaddr;   // 104
    uint32_t devtest;   // 108
    uint32_t phytest;   // 114
    uint32_t gimr_all;  // 130
    uint32_t gimr[3];   // 134-
    uint32_t gisr_all;  // 140
    uint32_t gisr[3];   // 144-
    uint8_t rxzlp;      // 150
    uint8_t txzlp;      // 154
    uint32_t epin[4];   // 160-
    uint32_t epout[4];  // 180-
    uint32_t epmap[2];  // 1A0, 1A4
    uint32_t fifomap;   // 1A8
    uint32_t fifocfg;   // 1AC
    uint32_t dmafifo;   // 1C0
    uint32_t dmactrl;   // 1C8

    // For DMA directly to a FIFO
    // There doesn't seem to be any public documentation
    struct {
        uint32_t ctrl;
        uint32_t addr;
    } fdma[3];

    // Also known as gisr4/gimr4.
    // Lower bits are DMA complete for the fdma,
    // Higher bits (<< 16) are DMA error.
    uint32_t dmasr;     // 328
    uint32_t dmamr;     // 32C

    struct {
        uint8_t data[1024];
        uint16_t size;
    } fifo[4];
    struct {
        uint8_t data[64];
        uint8_t size;
    } cxfifo;

    uint32_t setup_packet[2];
};

extern struct usb_cx2_state usb_cx2;

void usb_cx2_reset(void);
uint8_t usb_cx2_read_byte(uint32_t addr);
uint16_t usb_cx2_read_half(uint32_t addr);
uint32_t usb_cx2_read_word(uint32_t addr);
void usb_cx2_write_word(uint32_t addr, uint32_t value);

bool usb_cx2_packet_to_calc(uint8_t ep, const uint8_t *packet, size_t size);

#ifdef __cplusplus
}
#endif

#endif // USB_CX2_H
