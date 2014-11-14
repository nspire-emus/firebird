/* Declarations for usblink.c */

#ifndef _H_USBLINK
#define _H_USBLINK

bool usblink_put_file(char *filepath, char *folder);
void usblink_send_os(char *filepath);

void usblink_reset();
void usblink_connect();

#endif
