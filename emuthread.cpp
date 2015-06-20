#include "emuthread.h"

#include <cassert>
#include <cerrno>
#include <cstdarg>
#include <QEventLoop>

#include "core/debug.h"
#include "core/emu.h"
#include "core/usblink_queue.h"
#include "mainwindow.h"

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
    #ifdef MOBILE_UI
        return const_cast<char*>("c");
    #else
        QEventLoop ev;
        ev.connect(main_window, SIGNAL(debuggerCommand()), &ev, SLOT(quit()));
        ev.exec();
        return main_window->debug_command.data();
    #endif
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
    emu_thread->throttleTimerWait();
}

EmuThread::EmuThread(QObject *parent) :
    QThread(parent)
{
    throttle_timer.setInterval(throttle_delay);
    throttle_timer.moveToThread(this);

    assert(emu_thread == nullptr);
    emu_thread = this;
}

//Called occasionally, only way to do something in the same thread the emulator runs in.
void EmuThread::doStuff()
{
    if(enter_debugger)
    {
        enter_debugger = false;
        debugger(DBG_USER, 0);
    }

    if(throttle_timer.isActive() != throttle_timer_state)
    {
        if(throttle_timer_state)
            throttle_timer.start();
        else
        {
            throttle_timer.stop();
            // We abuse a signal here to quit the event loop whenever we want
            throttle_timer.setObjectName(throttle_timer.objectName().isEmpty() ? "throttle_timer" : "");
        }
    }

    while(paused)
        msleep(100);
}

void EmuThread::run()
{
    setTerminationEnabled();

    path_boot1 = boot1.c_str();
    path_flash = flash.c_str();

    usblink_queue_start();

    int ret = emulate(port_gdb, port_rdbg);

    emit exited(ret);
}

void EmuThread::throttleTimerWait()
{
    if(!throttle_timer.isActive())
        return;

    // e mustn't be deleted as there may be pending signals
    static QEventLoop e;
    throttle_timer.disconnect();
    e.quit();
    connect(&throttle_timer, SIGNAL(timeout()), &e, SLOT(quit()));
    // See below
    connect(&throttle_timer, SIGNAL(objectNameChanged(QString)), &e, SLOT(quit()));
    e.exec();
}

void EmuThread::setThrottleTimer(bool state)
{
    throttle_timer_state = state;
    emit throttleTimerChanged(state);
}

void EmuThread::setThrottleTimerDeactivated(bool state)
{
    setThrottleTimer(!state);
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
