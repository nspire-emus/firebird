#include <iostream>
#include <cassert>
#include <unistd.h>

#include <QUrl>

#include "qmlbridge.h"

#ifndef MOBILE_UI
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

    qmlRegisterType<KitModel>("Firebird.Emu", 1, 0, "KitModel");
    qRegisterMetaTypeStreamOperators<KitModel>();
    qRegisterMetaType<KitModel>();

    //Migrate old settings
    if(settings.contains(QStringLiteral("usbdir")) && !settings.contains(QStringLiteral("usbdirNew")))
        setUSBDir(QStringLiteral("/") + settings.value(QStringLiteral("usbdir")).toString());

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
    debug_on_start = getDebugOnStart();
    debug_on_warn = getDebugOnWarn();

    connect(&kit_model, SIGNAL(anythingChanged()), this, SLOT(saveKits()), Qt::QueuedConnection);
    #ifdef MOBILE_UI
        connect(&emu_thread, SIGNAL(speedChanged(double)), this, SLOT(speedChanged(double)), Qt::QueuedConnection);
        connect(&emu_thread, SIGNAL(turboModeChanged(bool)), this, SIGNAL(turboModeChanged()), Qt::QueuedConnection);
        connect(&emu_thread, SIGNAL(stopped()), this, SIGNAL(isRunningChanged()), Qt::QueuedConnection);
        connect(&emu_thread, SIGNAL(started(bool)), this, SIGNAL(isRunningChanged()), Qt::QueuedConnection);
        connect(&emu_thread, SIGNAL(suspended(bool)), this, SIGNAL(isRunningChanged()), Qt::QueuedConnection);
        connect(&emu_thread, SIGNAL(resumed(bool)), this, SIGNAL(isRunningChanged()), Qt::QueuedConnection);
        connect(&emu_thread, SIGNAL(started(bool)), this, SLOT(started(bool)), Qt::QueuedConnection);
        connect(&emu_thread, SIGNAL(resumed(bool)), this, SLOT(resumed(bool)), Qt::QueuedConnection);
        connect(&emu_thread, SIGNAL(suspended(bool)), this, SLOT(suspended(bool)), Qt::QueuedConnection);
    #endif
}

QMLBridge::~QMLBridge()
{
    #ifdef MOBILE_UI
        emu_thread.stop();
    #endif
}

unsigned int QMLBridge::getGDBPort()
{
    return settings.value(QStringLiteral("gdbPort"), 3333).toInt();
}

void QMLBridge::setGDBPort(unsigned int port)
{
    if(getGDBPort() == port)
        return;

    settings.setValue(QStringLiteral("gdbPort"), port);
    emit gdbPortChanged();
}
bool QMLBridge::getGDBEnabled()
{
    return settings.value(QStringLiteral("gdbEnabled"), !isMobile()).toBool();
}

void QMLBridge::setGDBEnabled(bool e)
{
    if(getGDBEnabled() == e)
        return;

    settings.setValue(QStringLiteral("gdbEnabled"), e);
    emit gdbEnabledChanged();
}

unsigned int QMLBridge::getRDBPort()
{
    return settings.value(QStringLiteral("rdbgPort"), 3334).toInt();
}

void QMLBridge::setRDBPort(unsigned int port)
{
    if(getRDBPort() == port)
        return;

    settings.setValue(QStringLiteral("rdbgPort"), port);
    emit rdbPortChanged();
}

bool QMLBridge::getRDBEnabled()
{
    return settings.value(QStringLiteral("rdbgEnabled"), !isMobile()).toBool();
}

void QMLBridge::setRDBEnabled(bool e)
{
    if(getRDBEnabled() == e)
        return;

    settings.setValue(QStringLiteral("rdbgEnabled"), e);
    emit rdbEnabledChanged();
}

bool QMLBridge::getDebugOnWarn()
{
    return settings.value(QStringLiteral("debugOnWarn"), true).toBool();
}

void QMLBridge::setDebugOnWarn(bool e)
{
    if(getDebugOnWarn() == e)
        return;

    debug_on_warn = e;
    settings.setValue(QStringLiteral("debugOnWarn"), e);
    emit debugOnWarnChanged();
}

bool QMLBridge::getDebugOnStart()
{
    return settings.value(QStringLiteral("debugOnStart"), false).toBool();
}

void QMLBridge::setDebugOnStart(bool e)
{
    if(getDebugOnStart() == e)
        return;

    debug_on_start = e;
    settings.setValue(QStringLiteral("debugOnStart"), e);
    emit debugOnStartChanged();
}

bool QMLBridge::getAutostart()
{
    return settings.value(QStringLiteral("emuAutostart"), true).toBool();
}

void QMLBridge::setAutostart(bool e)
{
    if(getAutostart() == e)
        return;

    settings.setValue(QStringLiteral("emuAutostart"), e);
    emit autostartChanged();
}

unsigned int QMLBridge::getDefaultKit()
{
    return settings.value(QStringLiteral("defaultKit"), 0).toUInt();
}

void QMLBridge::setDefaultKit(unsigned int id)
{
    if(getDefaultKit() == id)
        return;

    settings.setValue(QStringLiteral("defaultKit"), id);
    emit defaultKitChanged();
}

bool QMLBridge::getLeftHanded()
{
    return settings.value(QStringLiteral("leftHanded"), false).toBool();
}

void QMLBridge::setLeftHanded(bool e)
{
    if(getLeftHanded() == e)
        return;

    settings.setValue(QStringLiteral("leftHanded"), e);
    emit leftHandedChanged();
}

bool QMLBridge::getSuspendOnClose()
{
    return settings.value(QStringLiteral("suspendOnClose"), true).toBool();
}

void QMLBridge::setSuspendOnClose(bool e)
{
    if(getSuspendOnClose() == e)
        return;

    settings.setValue(QStringLiteral("suspendOnClose"), e);
    emit suspendOnCloseChanged();
}

QString QMLBridge::getUSBDir()
{
    return settings.value(QStringLiteral("usbdirNew"), QStringLiteral("/ndless")).toString();
}

void QMLBridge::setUSBDir(QString dir)
{
    if(getUSBDir() == dir)
        return;

    settings.setValue(QStringLiteral("usbdirNew"), dir);
    emit usbDirChanged();
}

bool QMLBridge::getIsRunning()
{
    #ifdef MOBILE_UI
        return emu_thread.isRunning();
    #else
        //TODO: Non mobile-ui
        return true;
#endif
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

    ::keypadStateChanged(row, col, state);
}

static QObject *buttons[KEYPAD_ROWS][KEYPAD_COLS];

void QMLBridge::registerNButton(unsigned int keymap_id, QVariant button)
{
    int col = keymap_id % KEYPAD_COLS, row = keymap_id / KEYPAD_COLS;
    assert(row < KEYPAD_ROWS);
    //assert(col < KEYPAD_COLS); Not needed.

    if(buttons[row][col])
        qWarning() << "Warning: Button " << keymap_id << " already registered as " << buttons[row][col] << "!";
    else
        buttons[row][col] = button.value<QObject*>();
}

void QMLBridge::touchpadStateChanged(qreal x, qreal y, bool contact, bool down)
{
    ::touchpadStateChanged(x, y, contact, down);

    notifyTouchpadStateChanged();
}

static QObject *qml_touchpad;

void QMLBridge::registerTouchpad(QVariant touchpad)
{
    qml_touchpad = touchpad.value<QObject*>();
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
    usblink_queue_put_file(url.toLocalFile().toStdString(), dir.toStdString(), QMLBridge::usblink_progress_changed, this);
}

QString QMLBridge::basename(QString path)
{
    if(path.isEmpty())
        return tr("None");

    std::string pathname = path.toStdString();
    return QString::fromStdString(std::string(
                std::find_if(pathname.rbegin(), pathname.rend(), [=](char c) { return c == '/'; }).base(),
                                      pathname.end()));
}

QUrl QMLBridge::dir(QString path)
{
    return QUrl{QUrl::fromLocalFile(path).toString(QUrl::RemoveFilename)};
}

QString QMLBridge::toLocalFile(QUrl url)
{
    return url.toLocalFile();
}

bool QMLBridge::fileExists(QString path)
{
    return QFile::exists(path);
}

int QMLBridge::kitIndexForID(unsigned int id)
{
    return kit_model.indexForID(id);
}

#ifndef MOBILE_UI

void QMLBridge::createFlash(unsigned int kitIndex)
{
    FlashDialog dialog;

    connect(&dialog, &FlashDialog::flashCreated, [&] (QString f){
        kit_model.setDataRow(kitIndex, f, KitModel::FlashRole);
    });

    dialog.show();
    dialog.exec();
}

#endif

void QMLBridge::saveKits()
{
    settings.setValue(QStringLiteral("kits"), QVariant::fromValue(kit_model));
}

void QMLBridge::usblink_progress_changed(int percent, void *qml_bridge_p)
{
    auto &&qml_bridge = reinterpret_cast<QMLBridge*>(qml_bridge_p);
    emit qml_bridge->usblinkProgressChanged(percent);
}

#ifdef MOBILE_UI

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

    emu_thread.port_gdb = getGDBEnabled() ? getGDBPort() : 0;
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

    emu_thread.port_gdb = getGDBEnabled() ? getGDBPort() : 0;
    emu_thread.port_rdbg = getRDBEnabled() ? getRDBPort() : 0;

    auto snapshot_path = getSnapshotPath();
    if(!snapshot_path.isEmpty())
        emu_thread.resume(snapshot_path);
    else
        toastMessage(tr("The current kit does not have a snapshot file configured"));
}

void QMLBridge::useDefaultKit()
{
    int kitIndex = kitIndexForID(getDefaultKit());
    if(kitIndex < 0)
        return;

    auto &&kit = kit_model.getKits()[kitIndex];
    current_kit_id = kit.id;
    emu_thread.boot1 = kit.boot1;
    emu_thread.flash = kit.flash;
    fallback_snapshot_path = kit.snapshot;
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
    int kitIndex = kitIndexForID(current_kit_id);
    if(kitIndex >= 0)
        return kit_model.getKits()[kitIndex].snapshot;
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
}

double QMLBridge::getSpeed()
{
    return speed;
}

bool QMLBridge::getTurboMode()
{
    return turbo_mode;
}

#endif

void notifyKeypadStateChanged(int row, int col, bool state)
{
    assert(row < KEYPAD_ROWS);
    assert(col < KEYPAD_COLS);

    if(!buttons[row][col])
    {
        qWarning() << "Warning: Button " << row*11+col << " not present in keypad!";
        return;
    }

    QQmlProperty::write(buttons[row][col], QStringLiteral("pressed"), state);
}

QObject *qmlBridgeFactory(QQmlEngine *engine, QJSEngine *scriptEngine)
{
    Q_UNUSED(engine)
    Q_UNUSED(scriptEngine)

    return new QMLBridge();
}

void notifyTouchpadStateChanged(qreal x, qreal y, bool contact, bool down)
{
    if(!qml_touchpad)
    {
        qWarning("Warning: No touchpad registered!");
        return;
    }

    QVariant ret;

    if(contact || down)
        QMetaObject::invokeMethod(qml_touchpad, "showHighlight", Q_RETURN_ARG(QVariant, ret), Q_ARG(QVariant, QVariant(x)), Q_ARG(QVariant, QVariant(y)), Q_ARG(QVariant, down));
    else
        QMetaObject::invokeMethod(qml_touchpad, "hideHighlight");
}

void notifyTouchpadStateChanged()
{
    notifyTouchpadStateChanged(float(keypad.touchpad_x)/TOUCHPAD_X_MAX, 1.0f-(float(keypad.touchpad_y)/TOUCHPAD_Y_MAX), keypad.touchpad_contact, keypad.touchpad_down);
}
