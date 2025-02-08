#include "emuthread.h"

#include <cassert>
#include <cerrno>
#include <cstdarg>
#include <chrono>

#include <QDir>
#include <QEventLoop>
#include <QTimer>

#ifdef Q_OS_WINDOWS
    #include <time.h>
    #include <windows.h>
#endif

#include "core/emu.h"
#include "core/mem.h"
#include "core/mmu.h"
#include "core/schedule.h"
#include "core/usblink_queue.h"

EmuThread emu_thread;

void gui_do_stuff(bool wait)
{
    emu_thread.doStuff(wait);
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
    emu_thread.debugStr(QString::vasprintf(fmt, ap));
}

void gui_status_printf(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    emu_thread.statusMsg(QString::vasprintf(fmt, ap));
    va_end(ap);
}

void gui_perror(const char *msg)
{
    gui_debug_printf("%s: %s\n", msg, strerror(errno));
}

void gui_debugger_entered_or_left(bool entered)
{
    emu_thread.debuggerEntered(entered);
}

static debug_input_cb debug_callback;

void gui_debugger_request_input(debug_input_cb callback)
{
    debug_callback = callback;
    emu_thread.debugInputRequested(callback != nullptr);
}

void gui_putchar(char c)
{
    if(true)
        emu_thread.serialChar(c);
    else
        putchar(c);
}

int gui_getchar()
{
    return -1;
}

void gui_set_busy(bool busy)
{
    emit emu_thread.isBusy(busy);
}

void gui_show_speed(double d)
{
    emit emu_thread.speedChanged(d);
}

void gui_usblink_changed(bool state)
{
    emit emu_thread.usblinkChanged(state);
}

void throttle_timer_off()
{
    emu_thread.setTurboMode(true);
}

void throttle_timer_on()
{
    emu_thread.setTurboMode(false);
}

void throttle_timer_wait(unsigned int usec)
{
    emu_thread.throttleTimerWait(usec);
}

EmuThread::EmuThread(QObject *parent) :
    QTimer(parent)
{
    // There can only be one instance, this global one.
    assert(&emu_thread == this);

    // Set default settings
    debug_on_start = debug_on_warn = false;

    connect(this, &QTimer::timeout, this, &EmuThread::loopStep);
    connect(this, &EmuThread::speedChanged, this, [&](double d) {
        if(turbo_mode)
            setInterval(0);
        else
            setInterval(std::max(interval() * d, 1.0));
    });
}

//Called occasionally, only way to do something in the same thread the emulator runs in.
void EmuThread::doStuff(bool wait)
{
        if(do_suspend)
        {
            bool success = emu_suspend(snapshot_path.c_str());
            do_suspend = false;
            emit suspended(success);
        }
}

void EmuThread::start()
{
    path_boot1 = QDir::toNativeSeparators(boot1).toStdString();
    path_flash = QDir::toNativeSeparators(flash).toStdString();

    bool do_reset = !do_resume;
    bool success = emu_start(port_gdb, port_rdbg, do_resume ? snapshot_path.c_str() : nullptr);

    if(do_resume)
        emit resumed(success);
    else
        emit started(success);

    do_resume = false;

    if(success) {
        startup(do_reset);
        setInterval(1);
        QTimer::start();
    }
}

void EmuThread::throttleTimerWait(unsigned int usec)
{
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

void EmuThread::startup(bool reset)
{
    if(reset)
    {
        memset(mem_areas[1].ptr, 0, mem_areas[1].size);

        memset(&arm, 0, sizeof arm);
        arm.control = 0x00050078;
        arm.cpsr_low28 = MODE_SVC | 0xC0;
        cpu_events &= EVENT_DEBUG_STEP;

        sched_reset();
        sched.items[SCHED_THROTTLE].clock = CLOCK_27M;
        extern void throttle_interval_event(int index);
        sched.items[SCHED_THROTTLE].proc = throttle_interval_event;

        memory_reset();
    }

    addr_cache_flush();

    sched_update_next_event(0);

    exiting = false;
}

void EmuThread::loopStep()
{
    int i = 1000;
    while(i--)
    {
        sched_process_pending_events();
        while (!exiting && cycle_count_delta < 0)
        {
            if (cpu_events & EVENT_RESET) {
                gui_status_printf("Reset");
                startup(true);
                break;
            }

            if (cpu_events & EVENT_SLEEP) {
                assert(emulate_cx2);
                cycle_count_delta = 0;
                break;
            }

            if (cpu_events & (EVENT_FIQ | EVENT_IRQ)) {
                // Align PC in case the interrupt occurred immediately after a jump
                if (arm.cpsr_low28 & 0x20)
                    arm.reg[15] &= ~1;
                else
                    arm.reg[15] &= ~3;

                if (cpu_events & EVENT_WAITING)
                    arm.reg[15] += 4; // Skip over wait instruction

                arm.reg[15] += 4;
                cpu_exception((cpu_events & EVENT_FIQ) ? EX_FIQ : EX_IRQ);
            }
            cpu_events &= ~EVENT_WAITING;

            if (arm.cpsr_low28 & 0x20)
                cpu_thumb_loop();
            else
                cpu_arm_loop();
        }
    }
}

void EmuThread::setPaused(bool paused)
{
    this->is_paused = paused;
    emit this->paused(paused);
}

bool EmuThread::stop()
{
    QTimer::stop();

    exiting = true;
    setPaused(false);
    do_suspend = false;

    // Cause the cpu core to leave the loop and check for events
    cycle_count_delta = 0;

    emu_cleanup();
    return true;
}

void EmuThread::reset()
{
    usblink_queue_reset();

    cpu_events |= EVENT_RESET;
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
