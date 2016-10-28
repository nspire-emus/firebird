#include <iostream>
#include <cassert>
#include <unistd.h>
#include <QMessageBox>

#include "qmlbridge.h"

#include "core/flash.h"
#include "core/keypad.h"

QMLBridge::QMLBridge(QObject *parent) : QObject(parent)
{
    qmlRegisterType<KitModel>("Firebird.Emu", 1, 0, "KitModel");

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
    gdbPortChanged();
}

void QMLBridge::setGDBEnabled(bool e)
{
    if(getGDBEnabled() == e)
        return;

    settings.setValue(QStringLiteral("gdbEnabled"), e);
    gdbEnabledChanged();
}

bool QMLBridge::getGDBEnabled()
{
    return settings.value(QStringLiteral("gdbEnabled"), true).toBool();
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
    rdbPortChanged();
}

void QMLBridge::setRDBEnabled(bool e)
{
    if(getRDBEnabled() == e)
        return;

    settings.setValue(QStringLiteral("rdbgEnabled"), e);
    rdbEnabledChanged();
}

bool QMLBridge::getRDBEnabled()
{
    return settings.value(QStringLiteral("rdbgEnabled"), true).toBool();
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

QString QMLBridge::basename(QString path)
{
    if(path.isEmpty())
        return tr("None");

    std::string pathname = path.toStdString();
    return QString::fromStdString(std::string(
                std::find_if(pathname.rbegin(), pathname.rend(), [](char c) { return c == '/'; }).base(),
            pathname.end()));
}

#ifdef MOBILE_UI

bool QMLBridge::restart()
{
    if(emu_thread.isRunning() && !emu_thread.stop())
    {
        toastMessage(tr("Could not stop emulation"));
        return false;
    }

    emu_thread.boot1 = getBoot1Path();
    emu_thread.flash = getFlashPath();

    if(!emu_thread.boot1.isEmpty() && !emu_thread.flash.isEmpty()) {
        toastMessage(tr("Starting emulation"));
        emu_thread.start();
        return true;
    } else {
        QMessageBox::warning(nullptr, tr("Error"), tr("You need to select a proper boot1 and flash image before.\nSwipe the keypad to the left to show the settings menu."));
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
    QString snapshot_path = settings.value(QStringLiteral("snapshotPath")).toString();
    if(!snapshot_path.isEmpty())
        emu_thread.suspend(snapshot_path);
}

void QMLBridge::resume()
{
    toastMessage(tr("Resuming emulation"));
    QString snapshot_path = settings.value(QStringLiteral("snapshotPath")).toString();
    if(!snapshot_path.isEmpty())
        emu_thread.resume(snapshot_path);
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
    return settings.value(QStringLiteral("boot1"), QString()).toString();
}

void QMLBridge::setBoot1Path(QUrl path)
{
    settings.setValue(QStringLiteral("boot1"), path.toLocalFile());
}

QString QMLBridge::getFlashPath()
{
    return settings.value(QStringLiteral("flash"), QString()).toString();
}

void QMLBridge::setFlashPath(QUrl path)
{
    settings.setValue(QStringLiteral("flash"), path.toLocalFile());
}

QString QMLBridge::getSnapshotPath()
{
    return settings.value(QStringLiteral("snapshotPath"), QString()).toString();
}

void QMLBridge::setSnapshotPath(QUrl path)
{
    settings.setValue(QStringLiteral("snapshotPath"), path.toLocalFile());
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

    QQmlProperty::write(toast, QStringLiteral("text"), msg);
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

int KitModel::rowCount(const QModelIndex &) const
{
    return kits.count();
}

QHash<int, QByteArray> KitModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[NameRole] = "name";
    roles[TypeRole] = "type";
    roles[FlashRole] = "flash";
    roles[Boot1Role] = "boot1";
    roles[SnapshotRole] = "snapshot";
    return roles;
}

QVariant KitModel::data(const QModelIndex &index, int role) const
{
    const int row = index.row();
    if(row < 0 || row >= kits.count())
        return QVariant();

    switch(role)
    {
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

    emit dataChanged(index(row), index(row), QVector<int>({role}));
    return true;
}

bool KitModel::copy(const int row)
{
    if(row < 0 || row >= kits.count())
        return false;

    beginInsertRows({}, row, row);
    kits.insert(row, kits[row]);
    endInsertRows();

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

    return true;
}
