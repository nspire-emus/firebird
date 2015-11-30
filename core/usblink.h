/* Declarations for usblink.c */

#ifndef _H_USBLINK
#define _H_USBLINK

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

extern bool usblink_sending, usblink_connected;
extern int usblink_state;

struct usblink_file {
    const char *filename;
    uint64_t size;
    bool is_dir;
};

/* f = NULL marks end of enumeration or error */
typedef void (*usblink_dirlist_cb)(struct usblink_file *f, void *user_data);
/* progress is 0-100 if successful or negative if an error occured.
   A value of 100 means complete and successful. */
typedef void (*usblink_progress_cb)(int progress, void *user_data);

void usblink_delete(const char *path, bool is_dir, usblink_progress_cb callback, void *user_data);
void usblink_dirlist(const char *path, usblink_dirlist_cb callback, void *user_data);
bool usblink_get_file(const char *path, const char *dest, usblink_progress_cb callback, void *user_data);
bool usblink_put_file(const char *filepath, const char *folder, usblink_progress_cb callback, void *user_data);
void usblink_move(const char *old_path, const char *new_path, usblink_progress_cb callback, void *user_data);
bool usblink_send_os(const char *filepath, usblink_progress_cb callback, void *user_data);

void usblink_reset();
void usblink_connect();

#ifdef __cplusplus
}
#endif

#endif
