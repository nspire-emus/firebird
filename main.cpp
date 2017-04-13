#include <QApplication>
#include <QTranslator>

#ifndef MOBILE_UI
#include "mainwindow.h"
#else
#include <QQuickView>
#endif

#include "qtframebuffer.h"
#include "qmlbridge.h"

int main(int argc, char **argv)
{
    #ifdef Q_OS_ANDROID
        QGuiApplication::setAttribute(Qt::AA_DisableHighDpiScaling);
    #endif
    QGuiApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);

    QApplication app(argc, argv);

    /* On iOS, sometimes garbage text gets rendered:
     * https://bugreports.qt.io/browse/QTBUG-47399
     * https://bugreports.qt.io/browse/QTBUG-56765 */
    #ifdef IS_IOS_BUILD
        app.setFont(QFont(QStringLiteral("Helvetica Neue")));
    #endif

    QTranslator appTranslator;
    appTranslator.load(QLocale::system().name(), QStringLiteral(":/i18n/i18n/"));
    app.installTranslator(&appTranslator);

    QCoreApplication::setOrganizationName(QStringLiteral("ndless"));
    QCoreApplication::setApplicationName(QStringLiteral("firebird"));

    // Register QMLBridge for Keypad<->Emu communication
    qmlRegisterSingletonType<QMLBridge>("Firebird.Emu", 1, 0, "Emu", qmlBridgeFactory);
    // Register QtFramebuffer for QML display
    qmlRegisterType<QMLFramebuffer>("Firebird.Emu", 1, 0, "EmuScreen");

    #ifndef MOBILE_UI
        MainWindow mw;
        main_window = &mw;
        mw.show();
    #else
        QQmlApplicationEngine mobile_ui;
        mobile_ui.addImportPath(QStringLiteral("qrc:/qml/qml/"));
        mobile_ui.load(QUrl(QStringLiteral("qrc:/qml/qml/MobileUI.qml")));
    #endif

    return app.exec();
}
