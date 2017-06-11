#include "emuthread.h"

#include <cassert>
#include <cerrno>
#include <cstdarg>
#include <chrono>

#include <QDir>
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

static debug_input_cb debug_callback;

void gui_debugger_request_input(debug_input_cb callback)
{
    debug_callback = callback;
    emu_thread->debugInputRequested(callback != nullptr);
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

void gui_set_busy(bool busy)
{
    emit emu_thread->isBusy(busy);
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
            bool success = emu_suspend(snapshot_path.c_str());
            do_suspend = false;
            emit suspended(success);
        }

        if(enter_debugger)
        {
            setPaused(false);
            enter_debugger = false;
            if(!in_debugger)
                debugger(DBG_USER, 0);
        }

        if(is_paused && wait)
            msleep(100);

    } while(is_paused && wait);
}

void EmuThread::run()
{
    setTerminationEnabled();

    path_boot1 = QDir::toNativeSeparators(boot1).toStdString();
    path_flash = QDir::toNativeSeparators(flash).toStdString();

    bool do_reset = !do_resume;
    bool success = emu_start(port_gdb, port_rdbg, do_resume ? snapshot_path.c_str() : nullptr);

    if(do_resume)
        emit resumed(success);
    else
        emit started(success);

    do_resume = false;

    if(success)
        emu_loop(do_reset);

    emit stopped();
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

void EmuThread::debuggerInput(QString str)
{
    debug_input = str.toStdString();
    debug_callback(debug_input.c_str());
}

void EmuThread::setPaused(bool paused)
{
    this->is_paused = paused;
    emit this->paused(paused);
}

bool EmuThread::stop()
{
    if(!isRunning())
        return true;

    exiting = true;
    setPaused(false);
    do_suspend = false;

    // Cause the cpu core to leave the loop and check for events
    arm.cycle_count_delta = 0;

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

    arm.cpu_events |= EVENT_RESET;
}

bool EmuThread::resume(QString path)
{
    snapshot_path = QDir::toNativeSeparators(path).toStdString();
    do_resume = true;
    if(!stop())
        return false;

    start();
    return true;
}

void EmuThread::suspend(QString path)
{
    snapshot_path = QDir::toNativeSeparators(path).toStdString();
    do_suspend = true;
}
