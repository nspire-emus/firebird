#ifndef USBLINK_QUEUE_H
#define USBLINK_QUEUE_H

#ifndef __cplusplus
#error C++ only header
#endif

#include <string>

#include "usblink.h"

// Enqueue those actions, equivalent to the functions in usblink.h
void usblink_queue_delete(std::string path, bool is_dir, usblink_progress_cb callback, void *user_data);
void usblink_queue_dirlist(std::string path, usblink_dirlist_cb callback, void *user_data);
void usblink_queue_download(std::string path, std::string destpath, usblink_progress_cb callback, void *user_data);
void usblink_queue_put_file(std::string local, std::string remote, usblink_progress_cb callback, void *user_data);
void usblink_queue_move(std::string old_path, std::string new_path, usblink_progress_cb callback, void *user_data);
void usblink_queue_new_dir(std::string path, usblink_progress_cb callback, void *user_data);
void usblink_queue_send_os(std::string filepath, usblink_progress_cb callback, void *user_data);

// Do one task from the queue
extern "C" void usblink_queue_do();

// Resets usblink as well
void usblink_queue_reset();
unsigned int usblink_queue_size();

#endif // USBLINK_QUEUE_H
