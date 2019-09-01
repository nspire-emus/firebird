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
    uint32_t cxfifo;    // 120
    uint32_t gimr_all;  // 130
    uint32_t gimr[3];   // 134-
    uint32_t gisr_all;  // 140
    uint32_t gisr[3];   // 144-
    uint32_t dmafifo;   // 1C0
    uint32_t dmactrl;   // 1C8

    uint8_t dma_data[1024];
    uint16_t dma_size;
};

extern struct usb_cx2_state usb_cx2;

void usb_cx2_reset(void);
uint8_t usb_cx2_read_byte(uint32_t addr);
uint16_t usb_cx2_read_half(uint32_t addr);
uint32_t usb_cx2_read_word(uint32_t addr);
void usb_cx2_write_word(uint32_t addr, uint32_t value);

#ifdef __cplusplus
}
#endif

#endif // USB_CX2_H
