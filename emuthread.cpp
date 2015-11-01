#include "emuthread.h"

#include <cassert>
#include <cerrno>
#include <cstdarg>
#include <chrono>

#include <QEventLoop>
#include <QTimer>

#include "core/debug.h"
#include "core/emu.h"
#include "core/usblink_queue.h"
#include "mainwindow.h"

EmuThread *emu_thread = nullptr;

void gui_do_stuff(bool wait)
{
    emu_thread->doStuff(wait);
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

void gui_debugger_entered_or_left(bool entered)
{
    emu_thread->debuggerEntered(entered);
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
    emit emu_thread->speedChanged(d);
}

void gui_usblink_changed(bool state)
{
    emit emu_thread->usblinkChanged(state);
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

    // Set default settings
    debug_on_start = debug_on_warn = false;
}

//Called occasionally, only way to do something in the same thread the emulator runs in.
void EmuThread::doStuff(bool wait)
{
    do
    {
        if(do_suspend)
        {
            emu_suspend(snapshot_path.c_str());
            do_suspend = false;
            //TODO: Signal
        }

        if(enter_debugger)
        {
            //TODO: Signal that no longer paused
            paused = false;
            enter_debugger = false;
            debugger(DBG_USER, 0);
        }

        if(paused && wait)
            msleep(100);

    } while(paused && wait);
}

void EmuThread::run()
{
    setTerminationEnabled();

    path_boot1 = boot1.c_str();
    path_flash = flash.c_str();

    bool do_reset = !do_resume;
    bool ret = emu_start(port_gdb, port_rdbg, do_resume ? snapshot_path.c_str() : nullptr);
    do_resume = false;

    if(ret)
        emu_loop(do_reset);

    emit exited(ret);
}

void EmuThread::throttleTimerWait()
{
    unsigned int now = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
    unsigned int throttle = throttle_delay * 1000;
    unsigned int left = throttle - (now % throttle);
    if(left > 0)
        QThread::usleep(left);
}

void EmuThread::setTurboMode(bool enabled)
{
    turbo_mode = enabled;
    emit turboModeChanged(enabled);
}

void EmuThread::toggleTurbo()
{
    setTurboMode(!turbo_mode);
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
    if(!isRunning())
        return true;

    exiting = true;
    paused = false;
    do_suspend = false;
    setTurboMode(true);
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
