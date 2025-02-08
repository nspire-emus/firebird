#include <cassert>
#include <unistd.h>

#include <QUrl>

#include "emuthread.h"
#include "qmlbridge.h"

#ifndef MOBILE_UI
    #include "mainwindow.h"
#endif

#include "core/emu.h"
#include "core/os/os.h"
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

    print_on_warn = getPrintOnWarn();

    connect(&kit_model, SIGNAL(anythingChanged()), this, SLOT(saveKits()), Qt::QueuedConnection);

    setActive(true);
}

QMLBridge::~QMLBridge()
{}

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
    return settings.value(QStringLiteral("debugOnWarn"), !isMobile()).toBool();
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

void QMLBridge::setPrintOnWarn(bool p)
{
    if (getPrintOnWarn() == p)
        return;

    print_on_warn = p;
    settings.setValue(QStringLiteral("printOnWarn"), p);
    emit printOnWarnChanged();
}

bool QMLBridge::getPrintOnWarn()
{
    return settings.value(QStringLiteral("printOnWarn"), true).toBool();
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
    return emu_thread.isRunning();
}

QString QMLBridge::getVersion()
{
    #define STRINGIFYMAGIC(x) #x
    #define STRINGIFY(x) STRINGIFYMAGIC(x)
    return QStringLiteral(STRINGIFY(FB_VERSION));
}

void QMLBridge::setButtonState(int id, bool state)
{
    int col = id % KEYPAD_COLS, row = id / KEYPAD_COLS;

    ::keypad_set_key(row, col, state);
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
    auto local = toLocalFile(url);
    auto remote = dir + QLatin1Char('/') + basename(local);
    usblink_queue_put_file(local.toStdString(), remote.toStdString(), QMLBridge::usblink_progress_changed, this);
}

void QMLBridge::sendExitPTT()
{
    usblink_queue_new_dir("/Press-to-Test", nullptr, nullptr);
    usblink_queue_put_file(std::string(), "/Press-to-Test/Exit Test Mode.tns", QMLBridge::usblink_progress_changed, this);
}

QString QMLBridge::basename(QString path)
{
    if(path.isEmpty())
        return tr("None");

#ifdef Q_OS_ANDROID
    QScopedPointer<char, QScopedPointerPodDeleter> android_bn{android_basename(path.toUtf8().data())};
    if(android_bn)
        return QString::fromUtf8(android_bn.data());
#endif

    return QFileInfo(path).fileName();
}

QUrl QMLBridge::dir(QString path)
{
    return QUrl{QUrl::fromLocalFile(path).toString(QUrl::RemoveFilename)};
}

QString QMLBridge::toLocalFile(QUrl url)
{
    // Pass through Android content url, see fopen_utf8
    if(url.scheme() == QStringLiteral("content"))
        return url.toString(QUrl::FullyEncoded);

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
void QMLBridge::switchUIMode(bool mobile_ui)
{
    main_window->switchUIMode(mobile_ui);
}
#endif

bool QMLBridge::createFlash(QString path, int productID, int featureValues, QString manuf, QString boot2, QString os, QString diags)
{
    bool is_cx = productID >= 0x0F0;
    std::string preload_str[4] = { manuf.toStdString(), boot2.toStdString(), diags.toStdString(), os.toStdString() };
    const char *preload[4] = { nullptr, nullptr, nullptr, nullptr };

    for(unsigned int i = 0; i < 4; ++i)
        if(preload_str[i] != "")
            preload[i] = preload_str[i].c_str();

    uint8_t *nand_data = nullptr;
    size_t nand_size;

    if(!flash_create_new(is_cx, preload, productID, featureValues, is_cx, &nand_data, &nand_size))
    {
        free(nand_data);
        return false;
    }

    QFile flash_file(path);
    if(!flash_file.open(QFile::WriteOnly) || !flash_file.write(reinterpret_cast<char*>(nand_data), nand_size))
    {
        free(nand_data);
        return false;
    }

    free(nand_data);

    flash_file.close();
    return true;
}

QString QMLBridge::componentDescription(QString path, QString expected_type)
{
    FILE *file = fopen_utf8(path.toUtf8().data(), "rb");
    if(!file)
        return tr("Open failed");

    std::string type, version;
    bool b = flash_component_info(file, type, version);
    fclose(file);
    if(!b)
        return QStringLiteral("???");

    if(type != expected_type.toStdString())
        return tr("Found %1 instead").arg(QString::fromStdString(type).trimmed());

    return QString::fromStdString(version);
}

QString QMLBridge::manufDescription(QString path)
{
    FILE *file = fopen_utf8(path.toUtf8().data(), "rb");
    if(!file)
        return tr("Open failed");

    auto raw_type = flash_read_type(file, true);
    fclose(file);
    // Reading or parsing failed
    if(raw_type == "" || raw_type == "???")
        return QStringLiteral("???");

    return QString::fromStdString(raw_type);
}

QString QMLBridge::osDescription(QString path)
{
    FILE *file = fopen_utf8(path.toUtf8().data(), "rb");
    if(!file)
        return tr("Open failed");

    std::string version;
    bool b = flash_os_info(file, version);
    fclose(file);
    if(!b)
        return QStringLiteral("???");

    return QString::fromStdString(version);
}

void QMLBridge::setActive(bool b)
{
    if(is_active == b)
        return;

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
        connect(&emu_thread, SIGNAL(debugStr(QString)), this, SIGNAL(debugStr(QString)), Qt::QueuedConnection);

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
        disconnect(&emu_thread, SIGNAL(debugStr(QString)), this, SIGNAL(debugStr(QString)));
    }

    is_active = b;
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

int QMLBridge::getMobileX()
{
    return settings.value(QStringLiteral("mobileX"), -1).toInt();
}

void QMLBridge::setMobileX(int x)
{
    settings.setValue(QStringLiteral("mobileX"), x);
}

int QMLBridge::getMobileY()
{
    return settings.value(QStringLiteral("mobileY"), -1).toInt();
}

void QMLBridge::setMobileY(int y)
{
    settings.setValue(QStringLiteral("mobileY"), y);
}

int QMLBridge::getMobileWidth()
{
    return settings.value(QStringLiteral("mobileWidth"), -1).toInt();
}

void QMLBridge::setMobileWidth(int w)
{
    settings.setValue(QStringLiteral("mobileWidth"), w);
}

int QMLBridge::getMobileHeight()
{
    return settings.value(QStringLiteral("mobileHeight"), -1).toInt();
}

void QMLBridge::setMobileHeight(int h)
{
    settings.setValue(QStringLiteral("mobileHeight"), h);
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
    else {
        toastMessage(tr("The current kit does not have a snapshot file configured"));
        emit emuSuspended(false);
    }
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

bool QMLBridge::useDefaultKit()
{
    if(setCurrentKit(getDefaultKit()))
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
    int kitIndex = kitIndexForID(id);
    if(kitIndex < 0)
        return nullptr;

    auto &&kit = kit_model.getKits()[kitIndex];
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
    int kitIndex = kitIndexForID(current_kit_id);
    if(kitIndex >= 0)
        return kit_model.getKits()[kitIndex].snapshot;
    else
        return fallback_snapshot_path;
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

void QMLBridge::notifyButtonStateChanged(int row, int col, bool state)
{
    assert(row < KEYPAD_ROWS);
    assert(col < KEYPAD_COLS);

    emit buttonStateChanged(col + row * KEYPAD_COLS, state);
}

void QMLBridge::touchpadStateChanged()
{
    touchpadStateChanged(float(keypad.touchpad_x)/TOUCHPAD_X_MAX, 1.0f-(float(keypad.touchpad_y)/TOUCHPAD_Y_MAX), keypad.touchpad_contact, keypad.touchpad_down);
}
