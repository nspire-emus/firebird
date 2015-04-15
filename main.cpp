#include <QApplication>

#include "mainwindow.h"

int main(int argc, char **argv)
{
    QApplication app(argc, argv);

    QCoreApplication::setOrganizationName("ndless");
    QCoreApplication::setApplicationName("nspire_emu");
    app.setAttribute(Qt::AA_UseHighDpiPixmaps);

    MainWindow mw;

    main_window = &mw;

    mw.show();

    return app.exec();
}
