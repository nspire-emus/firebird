#include "emuthread.h"

#include <cerrno>
#include <cstdarg>
#include <QEventLoop>

#include "debug.h"
#include "emu.h"
#include "mainwindow.h"
#include "usblink_queue.h"

EmuThread *emu_thread = nullptr;

void gui_do_stuff()
{
    emu_thread->doStuff();
}

void gui_debug_printf(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);

    gui_debug_vprintf(fmt, ap);

    va_end(ap);
}

void gui_debug_vprintf(const char *fmt, va_list ap)
{
    QString str;
    str.vsprintf(fmt, ap);
    emu_thread->debugStr(str);
}

void gui_status_printf(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);

    QString str;
    str.vsprintf(fmt, ap);
    emu_thread->statusMsg(str);

    va_end(ap);
}

void gui_perror(const char *msg)
{
    gui_debug_printf("%s: %s\n", msg, strerror(errno));
}

char *gui_debug_prompt()
{
    QEventLoop ev;
    ev.connect(main_window, SIGNAL(debuggerCommand()), &ev, SLOT(quit()));
    ev.exec();
    return main_window->debug_command.data();
}

void gui_putchar(char c)
{
    if(true)
        emu_thread->serialChar(c);
    else
        putchar(c);
}

int gui_getchar()
{
    return -1;
}

void gui_show_speed(double d)
{
    emu_thread->speedChanged(d);
}

void gui_usblink_changed(bool state)
{
    emu_thread->usblinkChanged(state);
}

void throttle_timer_off()
{
    emu_thread->setThrottleTimer(false);
}

void throttle_timer_on()
{
    emu_thread->setThrottleTimer(true);
}

void throttle_timer_wait()
{
    main_window->throttleTimerWait();
}

EmuThread::EmuThread(QObject *parent) :
    QThread(parent)
{}

//Called occasionally, only way to do something in the same thread the emulator runs in.
void EmuThread::doStuff()
{
    if(enter_debugger)
    {
        enter_debugger = false;
        debugger(DBG_USER, 0);
    }

    while(paused)
        msleep(100);
}

void EmuThread::run()
{
    setTerminationEnabled();

    path_boot1 = emu_path_boot1.c_str();
    path_flash = emu_path_flash.c_str();

    usblink_queue_start();

    int ret = emulate(port_gdb, port_rdbg);

    emit exited(ret);
}

void EmuThread::enterDebugger()
{
    enter_debugger = true;
}

void EmuThread::setPaused(bool paused)
{
    this->paused = paused;
}

bool EmuThread::stop()
{
    usblink_queue_stop();

    exiting = true;
    paused = false;
    throttle_timer_off();
    if(!this->wait(1000))
    {
        terminate();
        if(!this->wait(1000))
            return false;
    }

    emu_cleanup();
    return true;
}

void EmuThread::reset()
{
    usblink_queue_reset();

    cpu_events |= EVENT_RESET;
}
