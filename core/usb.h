/* Declarations for usb.c */

#ifndef _H_USB
#define _H_USB

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct usb_state {
    uint32_t usbcmd;      // +140
    uint32_t usbsts;      // +144
    uint32_t usbintr;     // +148
    uint32_t deviceaddr;  // +154
    uint32_t eplistaddr;  // +158
    uint32_t portsc;      // +184
    uint32_t otgsc;       // +1A4
    uint32_t epsetupsr;   // +1AC
    uint32_t epsr;        // +1B8
    uint32_t epcomplete;  // +1BC
} usb_state;

extern usb_state usb;
void usb_reset(void);
typedef struct emu_snapshot emu_snapshot;
bool usb_suspend(emu_snapshot *snapshot);
bool usb_resume(const emu_snapshot *snapshot);
uint8_t usb_read_byte(uint32_t addr);
uint16_t usb_read_half(uint32_t addr);
uint32_t usb_read_word(uint32_t addr);
void usb_write_word(uint32_t addr, uint32_t value);

#ifdef __cplusplus
}
#endif

#endif
