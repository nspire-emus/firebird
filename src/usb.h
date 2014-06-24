/* Declarations for usb.c */

#ifndef _H_USB
#define _H_USB

extern struct usb_state {
        u32 usbcmd;      // +140
        u32 usbsts;      // +144
        u32 usbintr;     // +148
        u32 deviceaddr;  // +154
        u32 eplistaddr;  // +158
        u32 portsc;      // +184
        u32 otgsc;       // +1A4
        u32 epsetupsr;   // +1AC
        u32 epsr;        // +1B8
        u32 epcomplete;  // +1BC
} usb;
void usb_reset(void);
u8 usb_read_byte(u32 addr);
u16 usb_read_half(u32 addr);
u32 usb_read_word(u32 addr);
void usb_write_word(u32 addr, u32 value);

#endif
