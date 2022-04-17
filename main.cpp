#ifdef MOBILE_UI
#include <QGuiApplication>
#else
#include <QApplication>
#include "mainwindow.h"
#endif
#include <QTranslator>

#include <QWindow>
#include <QQmlApplicationEngine>

#include "emuthread.h"
#include "qtframebuffer.h"
#include "qmlbridge.h"
#include "kitmodel.h"

#if !defined(NO_TRANSLATION) && defined(IS_IOS_BUILD)
#include <unistd.h>
#include <sys/syscall.h>
#endif

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

        QVariantHash old = readOldSettings(QStringLiteral("org.firebird"), QStringLiteral("firebird-emu"));
        if(!old.count())
            old = readOldSettings(QStringLiteral("ndless"), QStringLiteral("firebird"));

        if(!old.count())
            old = readOldSettings(QStringLiteral("ndless"), QStringLiteral("nspire_emu"));

        if(old.count())
        {
            // Copy over values
            for (auto it = old.begin(); it != old.end(); ++it)
                current.setValue(it.key(), it.value());

            qDebug("Settings imported");
        }
        else
            qDebug("No previous settings found");

        current.setValue(QStringLiteral("version"), 1);
        current.sync();
    }
}

int main(int argc, char **argv)
{
    #ifdef Q_OS_ANDROID
        QGuiApplication::setAttribute(Qt::AA_DisableHighDpiScaling);
    #else
        QGuiApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    #endif
    QGuiApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);

    #ifdef MOBILE_UI
        QGuiApplication app(argc, argv);

        QTranslator appTranslator;
        appTranslator.load(QLocale::system().name(), QStringLiteral(":/i18n/i18n/"));
        app.installTranslator(&appTranslator);
    #else
        QApplication app(argc, argv);

        // Translator installed in MainWindow constructor
    #endif

    /* On iOS, sometimes garbage text gets rendered:
     * https://bugreports.qt.io/browse/QTBUG-47399
     * https://bugreports.qt.io/browse/QTBUG-56765 */
    #ifdef IS_IOS_BUILD
        app.setFont(QFont(QStringLiteral("Helvetica Neue")));
    #endif

    QCoreApplication::setOrganizationDomain(QStringLiteral("firebird-emus.org"));
    QCoreApplication::setOrganizationName(QStringLiteral("Firebird Emus"));
    QCoreApplication::setApplicationName(QStringLiteral("firebird-emu"));

    // Needed for settings migration
    qRegisterMetaTypeStreamOperators<KitModel>();
    qRegisterMetaType<KitModel>();

    migrateSettings();

    QMLBridge qmlBridge;
    QQmlEngine::setObjectOwnership(&qmlBridge, QQmlEngine::CppOwnership);

    // Register QMLBridge for Keypad<->Emu communication
    qmlRegisterSingletonType<QMLBridge>("Firebird.Emu", 1, 0, "Emu", [](QQmlEngine *, QJSEngine *) -> QObject* {
        return the_qml_bridge;
    });
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

    app.connect(&app, &QGuiApplication::lastWindowClosed, [&] {
        // Apparently QML ApplicationWindow does not count - although
        // it's visible this signal gets emitted. So recheck ourselves...
        for(auto win : app.topLevelWindows())
            if(win->isVisible()) return;

        emu_thread.stop();
    });

    int execRet = app.exec();

#if !defined(NO_TRANSLATION) && defined(IS_IOS_BUILD)
    // New in recent iOS versions: due to some kernel/system bug, if we leave a process
    // with PT_TRACE_ME, it will not get terminated properly and will refuse
    // to launch again.
    syscall(SYS_ptrace, 31 /* PT_DENY_ATTACH */, 0, NULL, 0);
    // for debugging uncaught exception crashes, set a breakpoint on exceptions
    // and then use `po $arg1` to dump the exception string.
#endif

    return execRet;
}
