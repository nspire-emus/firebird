#include "emuthread.h"

#include <cassert>
#include <cerrno>
#include <cstdarg>

#include <QEventLoop>
#include <QTimer>

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
    emu_thread->setTurboMode(true);
}

void throttle_timer_on()
{
    emu_thread->setTurboMode(false);
}

void throttle_timer_wait()
{
    emu_thread->throttleTimerWait();
}

EmuThread::EmuThread(QObject *parent) :
    QThread(parent)
{
    assert(emu_thread == nullptr);
    emu_thread = this;

    connect(&throttle_timer, SIGNAL(timeout()), this, SLOT(quit()));
    throttle_timer.start(throttle_delay);

    // Set default settings
    debug_on_start = debug_on_warn = false;
}

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

    path_boot1 = boot1.c_str();
    path_flash = flash.c_str();

    int ret = emulate(port_gdb, port_rdbg);

    emit exited(ret);
}

void EmuThread::throttleTimerWait()
{
    exec(); // Start an event loop until throttle_timer times out
}

void EmuThread::setTurboMode(bool enabled)
{
    turbo_mode = enabled;
    emit turboModeChanged(enabled);
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
    exiting = true;
    paused = false;
    setTurboMode(true);
    // Signal processing may have stopped, so quit the event loop/throttle manually
    this->quit();
    if(!this->wait(200))
    {
        terminate();
        if(!this->wait(200))
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
