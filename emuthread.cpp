#include "emuthread.h"

#include <cerrno>
#include <cstdarg>
#include <QEventLoop>

#include "mainwindow.h"

EmuThread *emu_thread = nullptr;

extern "C" {
    #include "debug.h"
    #include "emu.h"

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

    int ret = emulate(
      /* flag_debug         */   1,
      /* flag_large_nand    */   0,
      /* flag_large_sdram   */   0,
      /* flag_debug_on_warn */   1,
      /* flag_verbosity     */   -1,
      /* port_gdb           */   3333,
      /* port_rdbg          */   3334,
      /* keypad             */   4,
      /* product            */   0x0F0,
      /* addr_boot2         */   0,
      ///* path_boot1         */   "/home/fabian/Arbeitsfläche/Meine Projekte/nspire/nspire_emu/boot1.img",
                                "/sdcard/boot1_classic.img",
      /* path_boot2         */   nullptr,
      ///* path_flash         */   "/home/fabian/Arbeitsfläche/Meine Projekte/nspire/nspire_emu/flash_3.9.img",	//
                                "/sdcard/flash_3.9_nothing.img",
      //                          nullptr,
      /* path_commands      */   nullptr,
      /* path_log           */   nullptr,
      /* pre_boot2          */   nullptr,
      /* pre_diags          */   nullptr,
      /* pre_os             */   nullptr
                );

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
