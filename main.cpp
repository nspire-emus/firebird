#include <QApplication>
#include <QTranslator>

#include "mainwindow.h"
#include <QQmlApplicationEngine>

#include "qtframebuffer.h"
#include "qmlbridge.h"

#ifdef Q_OS_ANDROID
    #include "AndroidWrapper.h"
#endif

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
#ifdef Q_OS_ANDROID
    // Register AndroidWrapper for Android file chooser
    qmlRegisterType<AndroidWrapper>("Firebird.AndroidWrapper", 1, 0, "AndroidWrapper");
#endif

    #ifndef MOBILE_UI
        MainWindow mw;
        main_window = &mw;
    #else
        QQmlApplicationEngine engine;
        engine.addImportPath(QStringLiteral("qrc:/qml/qml/"));
        engine.load(QUrl(QStringLiteral("qrc:/qml/qml/MobileUI.qml")));
        /*mobile_ui.setResizeMode(QQuickView::SizeRootObjectToView);
        mobile_ui.show();*/
    #endif

    return app.exec();
}
