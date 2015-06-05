#include <iostream>
#include <cassert>

#include "qmlbridge.h"

#include "keypad.h"

QMLBridge::QMLBridge(QObject *parent) : QObject(parent)
{}

constexpr const int ROWS = 8, COLS = 11;

void QMLBridge::keypadStateChanged(int keymap_id, bool state)
{
    int col = keymap_id % 11, row = keymap_id / 11;
    assert(row < ROWS);
    //assert(col < COLS); Not needed.

    if(state)
        key_map[row] |= 1 << col;
    else
        key_map[row] &= ~(1 << col);
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
    touchpad_down = touchpad_contact = state;

    if(state)
    {
        touchpad_x = x * TOUCHPAD_X_MAX;
        touchpad_y = (1.0f-y) * TOUCHPAD_Y_MAX;
    }

    kpc.gpio_int_active |= 0x800;
    keypad_int_check();
}

static QObject *qml_touchpad;

void QMLBridge::registerTouchpad(QVariant touchpad)
{
    qml_touchpad = touchpad.value<QObject*>();
}

void notifyKeypadStateChanged(int row, int col, bool state)
{
    assert(row < ROWS);
    assert(col < COLS);

    if(!buttons[row][col])
    {
        qWarning() << "Warning: Button " << row*11+col << " not present in keypad!";
        return;
    }

    QQmlProperty::write(buttons[row][col], "state", state);
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
    notifyTouchpadStateChanged(float(touchpad_x)/TOUCHPAD_X_MAX, 1.0f-(float(touchpad_y)/TOUCHPAD_Y_MAX), touchpad_contact);
}
