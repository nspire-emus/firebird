#include <iostream>
#include <cassert>
#include <unistd.h>

#include <QUrl>

#include "qmlbridge.h"

#ifndef MOBILE_UI
    #include "mainwindow.h"
    #include "flashdialog.h"
#endif

#include "core/emu.h"
#include "core/keypad.h"
#include "core/usblink_queue.h"

QMLBridge *the_qml_bridge = nullptr;

QMLBridge::QMLBridge(QObject *parent) : QObject(parent)
#ifdef IS_IOS_BUILD
/* This is needed for iOS, as the app location changes at reinstall */
, settings(QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation) + QStringLiteral("/firebird.ini"), QSettings::IniFormat)
#endif
{
    assert(the_qml_bridge == nullptr);
    the_qml_bridge = this;

    //Migrate old settings
    if(settings.contains(QStringLiteral("usbdir")) && !settings.contains(QStringLiteral("usbdirNew")))
        settings.setValue(QStringLiteral("usbdirNew"), QStringLiteral("/") + settings.value(QStringLiteral("usbdir")).toString());

    bool add_default_kit = false;

    // Kits need to be loaded manually
    if(!settings.contains(QStringLiteral("kits")))
    {
        // Migrate
        add_default_kit = true;
    }
    else
    {
        kit_model = settings.value(QStringLiteral("kits")).value<KitModel>();

        // No kits is a bad situation to be in, as kits can only be duplicated...
        if(kit_model.rowCount() == 0)
            add_default_kit = true;
    }

    if(add_default_kit)
    {
        kit_model.addKit(tr("Default"),
                     settings.value(QStringLiteral("boot1")).toString(),
                     settings.value(QStringLiteral("flash")).toString(),
                     settings.value(QStringLiteral("snapshotPath")).toString());
    }

    // Same for debug_on_*
    ::debug_on_start = debug_on_start = settings.value(QStringLiteral("debugOnStart"), false).toBool();;
    connect(this, &QMLBridge::debugOnStartChanged, [&] {
        ::debug_on_start = debug_on_start;
        settings.setValue(QStringLiteral("debugOnStart"), debug_on_start);
    });

    ::debug_on_warn = debug_on_warn = settings.value(QStringLiteral("debugOnWarn"), !isMobile()).toBool();
    connect(this, &QMLBridge::debugOnWarnChanged, [&] {
        ::debug_on_warn = debug_on_warn;
        settings.setValue(QStringLiteral("debugOnWarn"), debug_on_warn);
    });

    ::print_on_warn = print_on_warn = settings.value(QStringLiteral("printOnWarn"), true).toBool();
    connect(this, &QMLBridge::printOnWarnChanged, [&] {
        ::print_on_warn = print_on_warn;
        settings.setValue(QStringLiteral("printOnWarn"), print_on_warn);
    });

    gdb_port = settings.value(QStringLiteral("gdbPort"), 3333).toInt();;
    connect(this, &QMLBridge::gdbPortChanged,
            [&] { settings.setValue(QStringLiteral("gdbPort"), gdb_port); });

    gdb_enabled = settings.value(QStringLiteral("gdbEnabled"), !isMobile()).toBool();
    connect(this, &QMLBridge::gdbEnabledChanged,
            [&] { settings.setValue(QStringLiteral("gdbEnabled"), gdb_enabled); });

    rdb_enabled = settings.value(QStringLiteral("rdbgEnabled"), !isMobile()).toBool();
    connect(this, &QMLBridge::rdbEnabledChanged,
            [&] { settings.setValue(QStringLiteral("rdbgEnabled"), rdb_enabled); });

    rdb_port = settings.value(QStringLiteral("rdbgPort"), 3334).toInt();
    connect(this, &QMLBridge::rdbPortChanged,
            [&] { settings.setValue(QStringLiteral("rdbgPort"), rdb_port); });

    autostart = settings.value(QStringLiteral("emuAutostart"), true).toBool();
    connect(this, &QMLBridge::autostartChanged,
            [&] { settings.setValue(QStringLiteral("emuAutostart"), autostart); });

    default_kit_id = settings.value(QStringLiteral("defaultKit"), 0).toUInt();
    connect(this, &QMLBridge::defaultKitChanged,
            [&] { settings.setValue(QStringLiteral("defaultKit"), default_kit_id); });

    left_handed = settings.value(QStringLiteral("leftHanded"), false).toBool();
    connect(this, &QMLBridge::leftHandedChanged,
            [&] { settings.setValue(QStringLiteral("leftHanded"), left_handed); });

    suspend_on_close = settings.value(QStringLiteral("suspendOnClose"), true).toBool();
    connect(this, &QMLBridge::suspendOnCloseChanged,
            [&] { settings.setValue(QStringLiteral("suspendOnClose"), suspend_on_close); });

    usb_dir = settings.value(QStringLiteral("usbdirNew"), QStringLiteral("/ndless")).toString();
    connect(this, &QMLBridge::usbDirChanged,
            [&] { settings.setValue(QStringLiteral("usbdirNew"), usb_dir); });

    mobile_x = settings.value(QStringLiteral("mobileX"), -1).toInt();
    connect(this, &QMLBridge::mobileXChanged,
            [&] { settings.setValue(QStringLiteral("mobileX"), mobile_x); });

    mobile_y = settings.value(QStringLiteral("mobileY"), -1).toInt();
    connect(this, &QMLBridge::mobileYChanged,
            [&] { settings.setValue(QStringLiteral("mobileY"), mobile_y); });

    mobile_w = settings.value(QStringLiteral("mobileWidth"), -1).toInt();
    connect(this, &QMLBridge::mobileWChanged,
            [&] { settings.setValue(QStringLiteral("mobileWidth"), mobile_w); });

    mobile_h = settings.value(QStringLiteral("mobileHeight"), -1).toInt();
    connect(this, &QMLBridge::mobileHChanged,
            [&] { settings.setValue(QStringLiteral("mobileHeight"), mobile_h); });

    connect(&kit_model, SIGNAL(anythingChanged()), this, SLOT(saveKits()), Qt::QueuedConnection);

    setActive(true);
}

QMLBridge::~QMLBridge()
{}

bool QMLBridge::getIsRunning()
{
    return emu_thread.isRunning();
}

QString QMLBridge::getVersion()
{
    #define STRINGIFYMAGIC(x) #x
    #define STRINGIFY(x) STRINGIFYMAGIC(x)
    return QStringLiteral(STRINGIFY(FB_VERSION));
}

void QMLBridge::keypadStateChanged(int keymap_id, bool state)
{
    int col = keymap_id % KEYPAD_COLS, row = keymap_id / KEYPAD_COLS;

    ::keypad_set_key(row, col, state);
}

static std::multimap<int , QObject *> buttons;

void QMLBridge::registerNButton(unsigned int keymap_id, QVariant button)
{
    buttons.insert(std::make_pair(keymap_id, button.value<QObject*>()));
}

void QMLBridge::setTouchpadState(qreal x, qreal y, bool contact, bool down)
{
    ::touchpad_set_state(x, y, contact, down);

    touchpadStateChanged();
}

bool QMLBridge::isMobile()
{
    // TODO: Mobile UI on desktop? Q_OS_ANDROID doesn't work somehow.
    #ifdef MOBILE_UI
        return true;
    #else
        return false;
    #endif
}

void QMLBridge::sendFile(QUrl url, QString dir)
{
    usblink_queue_put_file(toLocalFile(url).toStdString(), dir.toStdString(), QMLBridge::usblink_progress_changed, this);
}

QString QMLBridge::basename(QString path)
{
    if(path.isEmpty())
        return tr("None");

    if(path.startsWith(QStringLiteral("content://")))
    {
        auto parts = path.splitRef(QStringLiteral("%2F"), QString::SkipEmptyParts, Qt::CaseInsensitive);
        if(parts.length() > 1)
            return parts.last().toString();

        return tr("(Android File)");
    }

    return QFileInfo(path).fileName();
}

QUrl QMLBridge::dir(QString path)
{
    return QUrl{QUrl::fromLocalFile(path).toString(QUrl::RemoveFilename)};
}

QString QMLBridge::toLocalFile(QUrl url)
{
    if(url.scheme() == QStringLiteral("content"))
        return url.toString(); // Pass through Android content url

    return url.toLocalFile();
}

bool QMLBridge::fileExists(QString path)
{
    if(path.startsWith(QStringLiteral("content://")))
        return true; // Android content URL, can't do much

    return QFile::exists(path);
}

#ifndef MOBILE_UI
void QMLBridge::createFlash(const QModelIndex &kitIndex)
{
    FlashDialog dialog;

    connect(&dialog, &FlashDialog::flashCreated, [&] (QString f){
        kit_model.setData(kitIndex, f, KitModel::FlashRole);
    });

    dialog.show();
    dialog.exec();
}

void QMLBridge::switchUIMode(bool mobile_ui)
{
    main_window->switchUIMode(mobile_ui);
}
#endif

void QMLBridge::setActive(bool b)
{
    if(b)
    {
        connect(&emu_thread, SIGNAL(speedChanged(double)), this, SLOT(speedChanged(double)), Qt::QueuedConnection);
        connect(&emu_thread, SIGNAL(turboModeChanged(bool)), this, SIGNAL(turboModeChanged()), Qt::QueuedConnection);
        connect(&emu_thread, SIGNAL(stopped()), this, SIGNAL(isRunningChanged()), Qt::QueuedConnection);
        connect(&emu_thread, SIGNAL(started(bool)), this, SIGNAL(isRunningChanged()), Qt::QueuedConnection);
        connect(&emu_thread, SIGNAL(suspended(bool)), this, SIGNAL(isRunningChanged()), Qt::QueuedConnection);
        connect(&emu_thread, SIGNAL(resumed(bool)), this, SIGNAL(isRunningChanged()), Qt::QueuedConnection);
        connect(&emu_thread, SIGNAL(started(bool)), this, SLOT(started(bool)), Qt::QueuedConnection);
        connect(&emu_thread, SIGNAL(resumed(bool)), this, SLOT(resumed(bool)), Qt::QueuedConnection);
        connect(&emu_thread, SIGNAL(suspended(bool)), this, SLOT(suspended(bool)), Qt::QueuedConnection);

        // We might have missed some events.
        turboModeChanged();
        speedChanged();
        isRunningChanged();
    }
    else
    {
        disconnect(&emu_thread, SIGNAL(speedChanged(double)), this, SLOT(speedChanged(double)));
        disconnect(&emu_thread, SIGNAL(turboModeChanged(bool)), this, SIGNAL(turboModeChanged()));
        disconnect(&emu_thread, SIGNAL(stopped()), this, SIGNAL(isRunningChanged()));
        disconnect(&emu_thread, SIGNAL(started(bool)), this, SIGNAL(isRunningChanged()));
        disconnect(&emu_thread, SIGNAL(suspended(bool)), this, SIGNAL(isRunningChanged()));
        disconnect(&emu_thread, SIGNAL(resumed(bool)), this, SIGNAL(isRunningChanged()));
        disconnect(&emu_thread, SIGNAL(started(bool)), this, SLOT(started(bool)));
        disconnect(&emu_thread, SIGNAL(resumed(bool)), this, SLOT(resumed(bool)));
        disconnect(&emu_thread, SIGNAL(suspended(bool)), this, SLOT(suspended(bool)));
    }
}

void QMLBridge::saveKits()
{
    settings.setValue(QStringLiteral("kits"), QVariant::fromValue(kit_model));
}

void QMLBridge::usblink_progress_changed(int percent, void *qml_bridge_p)
{
    auto &&qml_bridge = reinterpret_cast<QMLBridge*>(qml_bridge_p);
    emit qml_bridge->usblinkProgressChanged(percent);
}

void QMLBridge::setTurboMode(bool b)
{
    emu_thread.setTurboMode(b);
}

bool QMLBridge::restart()
{
    if(emu_thread.isRunning() && !emu_thread.stop())
    {
        toastMessage(tr("Could not stop emulation"));
        return false;
    }

    emu_thread.port_gdb = gdb_enabled ? gdb_port : 0;
    emu_thread.port_rdbg = getRDBEnabled() ? getRDBPort() : 0;

    if(!emu_thread.boot1.isEmpty() && !emu_thread.flash.isEmpty()) {
        toastMessage(tr("Starting emulation"));
        emu_thread.start();
        return true;
    } else {
        toastMessage(tr("No boot1 or flash selected.\nSwipe keypad left for configuration."));
        return false;
    }
}

void QMLBridge::setPaused(bool b)
{
    emu_thread.setPaused(b);
}

void QMLBridge::reset()
{
    emu_thread.reset();
}

void QMLBridge::suspend()
{
    toastMessage(tr("Suspending emulation"));
    auto snapshot_path = getSnapshotPath();
    if(!snapshot_path.isEmpty())
        emu_thread.suspend(snapshot_path);
    else
        toastMessage(tr("The current kit does not have a snapshot file configured"));
}

void QMLBridge::resume()
{
    toastMessage(tr("Resuming emulation"));

    emu_thread.port_gdb = gdb_enabled ? gdb_port : 0;
    emu_thread.port_rdbg = rdb_enabled ? rdb_port : 0;

    auto snapshot_path = getSnapshotPath();
    if(!snapshot_path.isEmpty())
        emu_thread.resume(snapshot_path);
    else
        toastMessage(tr("The current kit does not have a snapshot file configured"));
}

bool QMLBridge::useDefaultKit()
{
    if(setCurrentKit(default_kit_id))
        return true;

    setCurrentKit(kit_model.getKits()[0].id); // Use first kit as fallback
    return false;
}

bool QMLBridge::setCurrentKit(unsigned int id)
{
    const Kit *kit = useKit(id);
    if(!kit)
         return false;

    current_kit_id = id;
    emit currentKitChanged(*kit);

    return true;
}

int QMLBridge::getCurrentKitId()
{
    return current_kit_id;
}

const Kit *QMLBridge::useKit(unsigned int id)
{
    const QModelIndex kitIndex = kit_model.indexForID(id);
    if(!kitIndex.isValid())
        return nullptr;

    auto &&kit = kit_model.getKits()[kitIndex.row()];
    emu_thread.boot1 = kit.boot1;
    emu_thread.flash = kit.flash;
    fallback_snapshot_path = kit.snapshot;

    return &kit;
}

bool QMLBridge::stop()
{
    return emu_thread.stop();
}

bool QMLBridge::saveFlash()
{
    return flash_save_changes();
}

QString QMLBridge::getBoot1Path()
{
    return emu_thread.boot1;
}

QString QMLBridge::getFlashPath()
{
    return emu_thread.flash;
}

QString QMLBridge::getSnapshotPath()
{
    const QModelIndex kitIndex = kit_model.indexForID(current_kit_id);
    if(kitIndex.isValid())
        return kit_model.getKits()[kitIndex.row()].snapshot;
    else
        return fallback_snapshot_path;
}

void QMLBridge::registerToast(QVariant toast)
{
    this->toast = toast.value<QObject*>();
}

void QMLBridge::toastMessage(QString msg)
{
    if(!toast)
    {
        qWarning() << "Warning: No toast QML component registered!";
        return;
    }

    QVariant ret;
    QMetaObject::invokeMethod(toast, "showMessage", Q_RETURN_ARG(QVariant, ret), Q_ARG(QVariant, QVariant(msg)));
}

bool QMLBridge::saveDialogSupported()
{
    #ifdef Q_OS_ANDROID
        // Starting with Qt 5.13, the Android "File Picker" is used, but that doesn't allow creation of files.
        return QVersionNumber::fromString(QString::fromUtf8(qVersion())) < QVersionNumber(5, 13);
    #else
        return true;
    #endif
}

void QMLBridge::speedChanged(double speed)
{
    this->speed = speed;
    emit speedChanged();
}

void QMLBridge::started(bool success)
{
    if(success)
        toastMessage(tr("Emulation started"));
    else
        toastMessage(tr("Couldn't start emulation"));
}

void QMLBridge::resumed(bool success)
{
    if(success)
        toastMessage(tr("Emulation resumed"));
    else
        toastMessage(tr("Could not resume"));
}

void QMLBridge::suspended(bool success)
{
    if(success)
        toastMessage(tr("Flash and snapshot saved")); // When clicking on save, flash is saved as well
    else
        toastMessage(tr("Couldn't save snapshot"));

    emit emuSuspended(success);
}

double QMLBridge::getSpeed()
{
    return speed;
}

bool QMLBridge::getTurboMode()
{
    return turbo_mode;
}

void notifyKeypadStateChanged(int row, int col, bool state)
{
    assert(row < KEYPAD_ROWS);
    assert(col < KEYPAD_COLS);

    int keymap_id = col + row * KEYPAD_COLS;
    auto it = buttons.lower_bound(keymap_id),
         end = buttons.upper_bound(keymap_id);

    if(it == buttons.end())
    {
        qWarning() << "Warning: Button " << row*KEYPAD_COLS+col << " not present in keypad!";
        return;
    }

    do
    {
        QQmlProperty::write(it->second, QStringLiteral("pressed"), state);
    }
    while (++it != end);
}

QObject *qmlBridgeFactory(QQmlEngine *engine, QJSEngine *scriptEngine)
{
    Q_UNUSED(engine)
    Q_UNUSED(scriptEngine)

    return new QMLBridge();
}

void QMLBridge::touchpadStateChanged()
{
    touchpadStateChanged(float(keypad.touchpad_x)/TOUCHPAD_X_MAX, 1.0f-(float(keypad.touchpad_y)/TOUCHPAD_Y_MAX), keypad.touchpad_contact, keypad.touchpad_down);
}
