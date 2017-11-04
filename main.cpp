#ifdef MOBILE_UI
#include <QGuiApplication>
#else
#include <QApplication>
#include "mainwindow.h"
#endif
#include <QTranslator>

#include <QQmlApplicationEngine>

#include "qtframebuffer.h"
#include "qmlbridge.h"
#include "kitmodel.h"

/* This function reads all keys of all sections available in a QSettings
 * instance created with org and app as parameters. */
static QVariantHash readOldSettings(const QString &org, const QString &app)
{
    QSettings settings(org, app);

    const auto keys = settings.allKeys();

    if(keys.isEmpty())
        return {};

    QVariantHash ret;

    for(auto & key : keys)
        ret[key] = settings.value(key);

    return ret;
}

/* If a default-constructed QSettings does not have version != 1,
 * this function tries to load settings from the old locations and
 * imports them. */
static void migrateSettings()
{
    QSettings current;
    if(current.value(QStringLiteral("version"), 0).toInt() == 0)
    {
        qDebug("Trying to import old settings");

        QVariantHash old = readOldSettings(QStringLiteral("ndless"), QStringLiteral("firebird"));
        if(!old.count())
            old = readOldSettings(QStringLiteral("ndless"), QStringLiteral("nspire_emu"));

        if(old.count())
        {
            // Copy over values
            for (auto it = old.begin(); it != old.end(); ++it)
                current.setValue(it.key(), it.value());

            current.setValue(QStringLiteral("version"), 1);
            current.sync();

            qDebug("Settings imported");
        }
        else
            qDebug("No previous settings found");
    }
}

int main(int argc, char **argv)
{
    #ifdef Q_OS_ANDROID
        QGuiApplication::setAttribute(Qt::AA_DisableHighDpiScaling);
    #endif
    QGuiApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);

    #ifdef MOBILE_UI
        QGuiApplication app(argc, argv);
    #else
        QApplication app(argc, argv);
    #endif

    /* On iOS, sometimes garbage text gets rendered:
     * https://bugreports.qt.io/browse/QTBUG-47399
     * https://bugreports.qt.io/browse/QTBUG-56765 */
    #ifdef IS_IOS_BUILD
        app.setFont(QFont(QStringLiteral("Helvetica Neue")));
    #endif

    QTranslator appTranslator;
    appTranslator.load(QLocale::system().name(), QStringLiteral(":/i18n/i18n/"));
    app.installTranslator(&appTranslator);

    QCoreApplication::setOrganizationName(QStringLiteral("org.firebird"));
    QCoreApplication::setApplicationName(QStringLiteral("firebird-emu"));

    // Needed for settings migration
    qRegisterMetaTypeStreamOperators<KitModel>();
    qRegisterMetaType<KitModel>();

    migrateSettings();

    // Register QMLBridge for Keypad<->Emu communication
    qmlRegisterSingletonType<QMLBridge>("Firebird.Emu", 1, 0, "Emu", qmlBridgeFactory);
    // Register QtFramebuffer for QML display
    qmlRegisterType<QMLFramebuffer>("Firebird.Emu", 1, 0, "EmuScreen");
    // Register KitModel
    qmlRegisterType<KitModel>("Firebird.Emu", 1, 0, "KitModel");

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

    app.connect(&app, &QGuiApplication::lastWindowClosed, [&] { emu_thread.stop(); });

    return app.exec();
}
