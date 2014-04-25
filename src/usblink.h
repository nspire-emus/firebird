/* Declarations for usblink.c */

bool usblink_put_file(char *filepath, char *folder);
void usblink_send_os(char *filepath);

void usblink_reset();
void usblink_connect();
void *usblink_save_state(size_t *size);
void usblink_reload_state(void *state);
