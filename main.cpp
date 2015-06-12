#include <QApplication>
#include <QTranslator>
#include <QtQml>

#include "mainwindow.h"
#include "qmlbridge.h"

int main(int argc, char **argv)
{
    QApplication app(argc, argv);

    QTranslator appTranslator;
    appTranslator.load(QLocale::system().name(), ":/i18n/i18n/");
    app.installTranslator(&appTranslator);

    QCoreApplication::setOrganizationName("ndless");
    QCoreApplication::setApplicationName("nspire_emu");
    app.setAttribute(Qt::AA_UseHighDpiPixmaps);

    // Register QMLBridge for Keypad<->Emu communication
    qmlRegisterSingletonType<QMLBridge>("Ndless.Emu", 1, 0, "Emu", qmlBridgeFactory);

    MainWindow mw;

    main_window = &mw;

    mw.show();

    return app.exec();
}
