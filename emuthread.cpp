#include "emuthread.h"

extern "C" {
#include "emu.h"
}

EmuThread::EmuThread(QObject *parent) :
    QThread(parent)
{}

void EmuThread::run()
{
    int ret = emulate(1, 1, 1, 1, 0, 3333, 3334, 4, 0x0F0, 0,
            "/home/fabian/Arbeitsfläche/Meine Projekte/nspire/nspire_emu/boot1.img", nullptr, "/home/fabian/Arbeitsfläche/Meine Projekte/nspire/nspire_emu/flash_3.9.img",
            nullptr, nullptr, nullptr, nullptr, nullptr);
    emit exited(ret);
}
