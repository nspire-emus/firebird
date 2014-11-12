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
}

void EmuThread::run()
{
    // int emulate(int flag_debug, int flag_large_nand, int flag_large_sdram, int flag_debug_on_warn, int flag_verbosity, int port_gdb, int port_rdbg, int keypad, int product, uint32_t addr_boot2, const char *path_boot1, const char *path_boot2, const char *path_flash, const char *path_commands, const char *path_log, const char *pre_boot2, const char *pre_diags, const char *pre_os)

    int ret = emulate(0, 1, 1, 1, -1, 3333, 3334, 4, 0x0F0, 0,
            "/home/fabian/Arbeitsfläche/Meine Projekte/nspire/nspire_emu/boot1_classic.img", nullptr, "/home/fabian/Arbeitsfläche/Meine Projekte/nspire/nspire_emu/flash_3.9_nothing.img",
            //"/sdcard1/boot1_classic.img", nullptr, "/sdcard1/flash_3.9_nothing.img",
            nullptr, nullptr, nullptr, nullptr, nullptr);
    emit exited(ret);
}

void EmuThread::enterDebugger()
{
    enter_debugger = true;
}
