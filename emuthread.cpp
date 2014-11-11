#include "emuthread.h"

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
    int ret = emulate(0, 1, 1, 1, -1, 3333, 3334, 4, 0x0F0, 0,
            "/home/fabian/Arbeitsfläche/Meine Projekte/nspire/nspire_emu/boot1.img", nullptr, "/home/fabian/Arbeitsfläche/Meine Projekte/nspire/nspire_emu/flash_3.9.img",
            nullptr, nullptr, nullptr, nullptr, nullptr);
    emit exited(ret);
}

void EmuThread::enterDebugger()
{
    enter_debugger = true;
}
