#include <QApplication>
#include <QTranslator>
#include <QDebug>
#include "mainwindow.h"
#include "emuthread.h"

int main(int argc, char **argv)
{
    QApplication app(argc, argv);

    QCoreApplication::setOrganizationName("ndless");
    QCoreApplication::setApplicationName("nspire_emu");

    MainWindow mw;

    main_window = &mw;

    mw.show();

    return app.exec();
}
