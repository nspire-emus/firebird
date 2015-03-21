/* Declarations for usblink.c */

#ifndef _H_USBLINK
#define _H_USBLINK

#include <stdint.h>

extern bool usblink_sending, usblink_connected;
extern int usblink_state;

struct usblink_file {
    const char *filename;
    uint64_t size;
    bool is_dir;
};


//f = NULL marks end of enumeration
typedef void (*usblink_dirlist_cb)(struct usblink_file *f, void *user_data);

void usblink_dirlist(const char *path, usblink_dirlist_cb callback, void *user_data);
bool usblink_put_file(const char *filepath, const char *folder);
void usblink_send_os(const char *filepath);

void usblink_reset();
void usblink_connect();

#endif
