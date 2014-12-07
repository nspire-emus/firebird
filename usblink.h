/* Declarations for usblink.c */

#ifndef _H_USBLINK
#define _H_USBLINK

extern bool usblink_sending, usblink_connected;
extern int usblink_state;

bool usblink_put_file(const char *filepath, const char *folder);
void usblink_send_os(const char *filepath);

void usblink_reset();
void usblink_connect();

#endif
