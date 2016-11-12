#include <iostream>
#include <cassert>
#include <unistd.h>

#include "qmlbridge.h"

#ifndef MOBILE_UI
#include "flashdialog.h"
#endif

#include "core/emu.h"
#include "core/flash.h"
#include "core/keypad.h"
#include "core/usblink_queue.h"
#include "core/os/os.h"

QMLBridge *the_qml_bridge = nullptr;

QMLBridge::QMLBridge(QObject *parent) : QObject(parent)
{
    assert(the_qml_bridge == nullptr);
    the_qml_bridge = this;

    qmlRegisterType<KitModel>("Firebird.Emu", 1, 0, "KitModel");
    qRegisterMetaTypeStreamOperators<KitModel>();
    qRegisterMetaType<KitModel>();

    //Migrate old settings
    if(settings.contains(QStringLiteral("usbdir")) && !settings.contains(QStringLiteral("usbdirNew")))
        setUSBDir(QStringLiteral("/") + settings.value(QStringLiteral("usbdir")).toString());

    // Kits need to be loaded manually
    if(!settings.contains(QStringLiteral("kits")))
    {
        // Migrate
        kit_model.addKit(tr("Default"), settings.value(QStringLiteral("boot1")).toString(), settings.value(QStringLiteral("flash")).toString(), settings.value(QStringLiteral("snapshotPath")).toString());
    }
    else
        kit_model = settings.value(QStringLiteral("kits")).value<KitModel>();

    // Same for debug_on_*
    debug_on_start = getDebugOnStart();
    debug_on_warn = getDebugOnWarn();

    connect(&kit_model, SIGNAL(anythingChanged()), this, SLOT(saveKits()), Qt::QueuedConnection);
    #ifdef MOBILE_UI
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
    return settings.value(QStringLiteral("gdbEnabled"), true).toBool();
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
    return settings.value(QStringLiteral("rdbgEnabled"), true).toBool();
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
    return settings.value(QStringLiteral("debugOnStart"), true).toBool();
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

constexpr const int ROWS = 8, COLS = 11;

void QMLBridge::keypadStateChanged(int keymap_id, bool state)
{
    int col = keymap_id % 11, row = keymap_id / 11;
    assert(row < ROWS);
    //assert(col < COLS); Not needed.

    if(state)
        keypad.key_map[row] |= 1 << col;
    else
        keypad.key_map[row] &= ~(1 << col);
}

static QObject *buttons[ROWS][COLS];

void QMLBridge::registerNButton(int keymap_id, QVariant button)
{
    int col = keymap_id % COLS, row = keymap_id / COLS;
    assert(row < ROWS);
    //assert(col < COLS); Not needed.

    if(buttons[row][col])
        qWarning() << "Warning: Button " << keymap_id << " already registered as " << buttons[row][col] << "!";
    else
        buttons[row][col] = button.value<QObject*>();
}

void QMLBridge::touchpadStateChanged(qreal x, qreal y, bool state)
{
    keypad.touchpad_down = keypad.touchpad_contact = state;

    if(state)
    {
        keypad.touchpad_x = x * TOUCHPAD_X_MAX;
        keypad.touchpad_y = (1.0f-y) * TOUCHPAD_Y_MAX;
    }

    keypad.kpc.gpio_int_active |= 0x800;
    keypad_int_check();
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
    char separator = QDir::separator().toLatin1();
    return QString::fromStdString(std::string(
                std::find_if(pathname.rbegin(), pathname.rend(), [=](char c) { return c == separator; }).base(),
                                      pathname.end()));
}

QString QMLBridge::dir(QString path)
{
    std::string pathname = path.toStdString();
    char separator = QDir::separator().toLatin1();
    return QString::fromStdString(std::string(
                pathname.begin(),
                                      std::find_if(pathname.rbegin(), pathname.rend(), [=](char c) { return c == separator; }).base()));
}

QString QMLBridge::toLocalFile(QUrl url)
{
    return url.toLocalFile();
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
        kit_model.setData(kitIndex, f, KitModel::FlashRole);
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

bool QMLBridge::restart()
{
    if(emu_thread.isRunning() && !emu_thread.stop())
    {
        toastMessage(tr("Could not stop emulation"));
        return false;
    }

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
    if(!snapshot_path.isEmpty())
        emu_thread.suspend(snapshot_path);
}

void QMLBridge::resume()
{
    toastMessage(tr("Resuming emulation"));
    if(!snapshot_path.isEmpty())
        emu_thread.resume(snapshot_path);
}


void QMLBridge::useDefaultKit()
{
    int kitIndex = kitIndexForID(getDefaultKit());
    if(kitIndex < 0)
        return;

    auto &&kit = kit_model.getKits()[kitIndex];
    emu_thread.boot1 = kit.boot1;
    emu_thread.flash = kit.flash;
    snapshot_path = kit.snapshot;
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
    return snapshot_path;
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

#endif

void notifyKeypadStateChanged(int row, int col, bool state)
{
    assert(row < ROWS);
    assert(col < COLS);

    if(!buttons[row][col])
    {
        qWarning() << "Warning: Button " << row*11+col << " not present in keypad!";
        return;
    }

    QQmlProperty::write(buttons[row][col], QStringLiteral("state"), state);
}

QObject *qmlBridgeFactory(QQmlEngine *engine, QJSEngine *scriptEngine)
{
    Q_UNUSED(engine)
    Q_UNUSED(scriptEngine)

    return new QMLBridge();
}

void notifyTouchpadStateChanged(qreal x, qreal y, bool state)
{
    if(!qml_touchpad)
    {
        qWarning("Warning: No touchpad registered!");
        return;
    }

    QVariant ret;

    if(state)
        QMetaObject::invokeMethod(qml_touchpad, "showHighlight", Q_RETURN_ARG(QVariant, ret), Q_ARG(QVariant, QVariant(x)), Q_ARG(QVariant, QVariant(y)));
    else
        QMetaObject::invokeMethod(qml_touchpad, "hideHighlight");
}

void notifyTouchpadStateChanged()
{
    notifyTouchpadStateChanged(float(keypad.touchpad_x)/TOUCHPAD_X_MAX, 1.0f-(float(keypad.touchpad_y)/TOUCHPAD_Y_MAX), keypad.touchpad_contact);
}

QDataStream &operator<<(QDataStream &out, const Kit &kit)
{
    out << kit.id
        << kit.name
        << kit.boot1
        << kit.flash
        << kit.snapshot
        << kit.type;

    return out;
}

QDataStream &operator>>(QDataStream &in, Kit &kit)
{
    in >> kit.id
       >> kit.name
       >> kit.boot1
       >> kit.flash
       >> kit.snapshot
       >> kit.type;

    return in;
}

QDataStream &operator<<(QDataStream &out, const KitModel &kits)
{
    out << kits.kits << kits.nextID;
    return out;
}

QDataStream &operator>>(QDataStream &in, KitModel &kits)
{
    in >> kits.kits >> kits.nextID;
    return in;
}

int KitModel::rowCount(const QModelIndex &) const
{
    return kits.count();
}

QHash<int, QByteArray> KitModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[IDRole] = "id";
    roles[NameRole] = "name";
    roles[TypeRole] = "type";
    roles[FlashRole] = "flash";
    roles[Boot1Role] = "boot1";
    roles[SnapshotRole] = "snapshot";
    return roles;
}

QVariant KitModel::data(const QModelIndex &index, int role) const
{
    return getData(index.row(), role);
}

QVariant KitModel::getData(const int row, int role) const
{
    if(row < 0 || row >= kits.count())
        return QVariant();

    switch(role)
    {
    case IDRole:
        return kits[row].id;
    case NameRole:
        return kits[row].name;
    case TypeRole:
        return kits[row].type;
    case FlashRole:
        return kits[row].flash;
    case Boot1Role:
        return kits[row].boot1;
    case SnapshotRole:
        return kits[row].snapshot;
    default:
        return QVariant();
    }
}

bool KitModel::setData(const int row, const QVariant &value, int role)
{
    if(row < 0 || row >= kits.count())
        return false;

    switch(role)
    {
    case IDRole:
        qWarning("ID not assignable");
        return false;
        break;
    case NameRole:
        kits[row].name = value.toString();
        break;
    case TypeRole:
        kits[row].type = value.toString();
        break;
    case FlashRole:
        kits[row].flash = value.toString();
        break;
    case Boot1Role:
        kits[row].boot1 = value.toString();
        break;
    case SnapshotRole:
        kits[row].snapshot = value.toString();
        break;
    default:
        return false;
    }

    if(role == FlashRole)
    {
        // Refresh type as well
        kits[row].type = typeForFlash(value.toString());
        emit dataChanged(index(row), index(row), QVector<int>({FlashRole, TypeRole}));
    }
    else
        emit dataChanged(index(row), index(row), QVector<int>({role}));

    emit anythingChanged();
    return true;
}

bool KitModel::copy(const int row)
{
    if(row < 0 || row >= kits.count())
        return false;

    beginInsertRows({}, row, row);
    kits.insert(row, kits[row]);
    kits[row].id = nextID++;
    endInsertRows();

    emit anythingChanged();

    return true;
}

bool KitModel::remove(const int row)
{
    if(row < 0 || row >= kits.count())
        return false;

    // Do not remove the last remaining kit
    if(kits.count() == 1)
        return false;

    beginRemoveRows({}, row, row);
    kits.removeAt(row);
    endRemoveRows();

    emit anythingChanged();

    return true;
}

void KitModel::addKit(QString name, QString boot1, QString flash, QString snapshot_path)
{
    int row = kits.size();
    beginInsertRows({}, row, row);
    kits.append({nextID++, name, typeForFlash(flash), boot1, flash, snapshot_path});
    endInsertRows();

    emit anythingChanged();
}

int KitModel::indexForID(const unsigned int id)
{
    auto it = std::find_if(kits.begin(), kits.end(),
                           [&] (const Kit &kit) { return kit.id == id; });
    return it == kits.end() ? -1 : (it - kits.begin());
}

QString KitModel::typeForFlash(QString flash)
{
    QString type = QStringLiteral("???");
    QByteArray flash_path = flash.toUtf8();
    FILE *file = fopen_utf8(flash_path.data(), "rb");
    if(file)
    {
        std::string s = flash_read_type(file);
        if(!s.empty())
            type = QString::fromStdString(s);
        fclose(file);
    }

    return type;
}
