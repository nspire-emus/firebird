#include <cassert>
#include <atomic>
#include <queue>
#include <chrono>
#include <thread>

#include "usblink_queue.h"

struct usblink_queue_action {
    enum {
        PUT_FILE,
        SEND_OS,
        DIRLIST,
        MOVE,
        DEL_FILE,
        DEL_DIR,
        GET_FILE
    } action;

    //Members only used if the appropriate action is set
    std::string filepath;
    std::string path;
    usblink_progress_cb progress_callback = nullptr;
    usblink_dirlist_cb dirlist_callback = nullptr;
    void *user_data;
};

static std::atomic_bool busy;
static std::queue<usblink_queue_action> usblink_queue;

static void dirlist_callback(struct usblink_file *f, bool is_error, void *user_data)
{
    assert(!usblink_queue.empty());
    assert(usblink_queue.front().user_data == user_data);
    assert(usblink_queue.front().action == usblink_queue_action::DIRLIST);

    if(usblink_queue.front().dirlist_callback != nullptr)
        usblink_queue.front().dirlist_callback(f, is_error, user_data);

    if(!f)
    {
        usblink_queue.pop();
        busy = false;
    }
}

static void progress_callback(int progress, void *user_data)
{
    assert(!usblink_queue.empty());
    assert(usblink_queue.front().user_data == user_data);
    if(usblink_queue.front().progress_callback != nullptr)
    {
        auto action = usblink_queue.front().action;
        if(action == usblink_queue_action::PUT_FILE
                || action == usblink_queue_action::SEND_OS
                || action == usblink_queue_action::MOVE
                || action == usblink_queue_action::DEL_DIR
                || action == usblink_queue_action::DEL_FILE
                || action == usblink_queue_action::GET_FILE)
            usblink_queue.front().progress_callback(progress, user_data);
        else
            assert(false);
    }

    if(progress < 0 || progress == 100)
    {
        usblink_queue.pop();
        busy = false;
    }
}

void usblink_queue_do()
{
    bool b = false;
    if(!usblink_connected || usblink_queue.empty() || !busy.compare_exchange_strong(b, true))
        return;

    usblink_queue_action action = usblink_queue.front();
    switch(action.action)
    {
    case usblink_queue_action::PUT_FILE:
        if(!usblink_put_file(action.filepath.c_str(), action.path.c_str(), progress_callback, action.user_data))
        {
            progress_callback(-1, action.user_data);
            busy = false;
        }
        break;
    case usblink_queue_action::SEND_OS:
        if(!usblink_send_os(action.filepath.c_str(), progress_callback, action.user_data))
        {
            progress_callback(-1, action.user_data);
            busy = false;
        }
        break;
    case usblink_queue_action::DIRLIST:
        usblink_dirlist(action.path.c_str(), dirlist_callback, action.user_data);
        break;
    case usblink_queue_action::MOVE:
        usblink_move(action.filepath.c_str(), action.path.c_str(), progress_callback, action.user_data);
        break;
    case usblink_queue_action::DEL_DIR:
        usblink_delete(action.path.c_str(), true, progress_callback, action.user_data);
        break;
    case usblink_queue_action::DEL_FILE:
        usblink_delete(action.path.c_str(), false, progress_callback, action.user_data);
        break;
    case usblink_queue_action::GET_FILE:
        if(!usblink_get_file(action.path.c_str(), action.filepath.c_str(), progress_callback, action.user_data))
        {
            progress_callback(-1, action.user_data);
            busy = false;
        }
    }
}

void usblink_queue_reset()
{
    while(!usblink_queue.empty())
    {
        // Treat as error
        usblink_queue_action action = usblink_queue.front();
        if(action.dirlist_callback)
            action.dirlist_callback(nullptr, true, action.user_data);
        else if(action.progress_callback)
            action.progress_callback(-1, action.user_data);

        usblink_queue.pop();
    }

    busy = false;

    usblink_reset();
}

void usblink_queue_delete(std::string path, bool is_dir, usblink_progress_cb callback, void *user_data)
{
    usblink_queue_action action;
    action.action = is_dir ? usblink_queue_action::DEL_DIR : usblink_queue_action::DEL_FILE;
    action.user_data = user_data;
    action.path = path;
    action.progress_callback = callback;

    usblink_queue.push(action);
}

void usblink_queue_dirlist(std::string path, usblink_dirlist_cb callback, void *user_data)
{
    usblink_queue_action action;
    action.action = usblink_queue_action::DIRLIST;
    action.user_data = user_data;
    action.path = path;
    action.dirlist_callback = callback;

    usblink_queue.push(action);
}

void usblink_queue_download(std::string path, std::string destpath, usblink_progress_cb callback, void *user_data)
{
    usblink_queue_action action;
    action.action = usblink_queue_action::GET_FILE;
    action.user_data = user_data;
    action.filepath = destpath;
    action.path = path;
    action.progress_callback = callback;

    usblink_queue.push(action);
}

void usblink_queue_put_file(std::string filepath, std::string folder, usblink_progress_cb callback, void *user_data)
{
    usblink_queue_action action;
    action.action = usblink_queue_action::PUT_FILE;
    action.user_data = user_data;
    action.filepath = filepath;
    action.path = folder;
    action.progress_callback = callback;

    usblink_queue.push(action);
}

void usblink_queue_send_os(const char *filepath, usblink_progress_cb callback, void *user_data)
{
    usblink_queue_action action;
    action.action = usblink_queue_action::SEND_OS;
    action.user_data = user_data;
    action.filepath = filepath;
    action.progress_callback = callback;

    usblink_queue.push(action);
}

void usblink_queue_move(std::string old_path, std::string new_path, usblink_progress_cb callback, void *user_data)
{
    usblink_queue_action action;
    action.action = usblink_queue_action::MOVE;
    action.user_data = user_data;
    action.filepath = old_path;
    action.path = new_path;
    action.progress_callback = callback,

    usblink_queue.push(action);
}

unsigned int usblink_queue_size()
{
    return usblink_queue.size();
}
