#include <QApplication>
#include <QTranslator>

#include "mainwindow.h"

int main(int argc, char **argv)
{
    QApplication app(argc, argv);

    QTranslator appTranslator;
    appTranslator.load(QLocale::system().name(), ":/i18n/i18n/");
    app.installTranslator(&appTranslator);

    QCoreApplication::setOrganizationName("ndless");
    QCoreApplication::setApplicationName("nspire_emu");
    app.setAttribute(Qt::AA_UseHighDpiPixmaps);

    MainWindow mw;

    main_window = &mw;

    mw.show();

    return app.exec();
}
