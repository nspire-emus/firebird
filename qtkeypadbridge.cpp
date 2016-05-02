#include "qtkeypadbridge.h"

#include "keymap.h"
#include "core/keypad.h"
#include "qmlbridge.h"

QtKeypadBridge qt_keypad_bridge;

void setKeypad(int key, bool state)
{
    int row = key / 11;
    int col = key % 11;

    if (state)
        keypad.key_map[row] |= 1 << col;
    else
        keypad.key_map[row] &= ~(1 << col);

    notifyKeypadStateChanged(row, col, state);
    keypad_int_check();

}

void QtKeypadBridge::keyPressEvent(QKeyEvent *event)
{
    Qt::Key key = static_cast<Qt::Key>(event->key());

    switch(key)
    {
    case Qt::Key_Down:
        keypad.touchpad_x = TOUCHPAD_X_MAX / 2;
        keypad.touchpad_y = 0;
        break;
    case Qt::Key_Up:
        keypad.touchpad_x = TOUCHPAD_X_MAX / 2;
        keypad.touchpad_y = TOUCHPAD_Y_MAX;
        break;
    case Qt::Key_Left:
        keypad.touchpad_y = TOUCHPAD_Y_MAX / 2;
        keypad.touchpad_x = 0;
        break;
    case Qt::Key_Right:
        keypad.touchpad_y = TOUCHPAD_Y_MAX / 2;
        keypad.touchpad_x = TOUCHPAD_X_MAX;
        break;
    default:
        int button = keyToKeypad(event);
        if (button)
        {
            if (button == on)
                keypad_on_pressed();

            setKeypad(button, true);
        }

        return;
    }

    keypad.touchpad_contact = keypad.touchpad_down = true;
    notifyTouchpadStateChanged();
    keypad.kpc.gpio_int_active |= 0x800;

    keypad_int_check();
}

void QtKeypadBridge::keyReleaseEvent(QKeyEvent *event)
{
    Qt::Key key = static_cast<Qt::Key>(event->key());

    switch(key)
    {
    case Qt::Key_Down:
        if(keypad.touchpad_x == TOUCHPAD_X_MAX / 2
            && keypad.touchpad_y == 0)
            keypad.touchpad_contact = keypad.touchpad_down = false;
        break;
    case Qt::Key_Up:
        if(keypad.touchpad_x == TOUCHPAD_X_MAX / 2
            && keypad.touchpad_y == TOUCHPAD_Y_MAX)
            keypad.touchpad_contact = keypad.touchpad_down = false;
        break;
    case Qt::Key_Left:
        if(keypad.touchpad_y == TOUCHPAD_Y_MAX / 2
            && keypad.touchpad_x == 0)
            keypad.touchpad_contact = keypad.touchpad_down = false;
        break;
    case Qt::Key_Right:
        if(keypad.touchpad_y == TOUCHPAD_Y_MAX / 2
            && keypad.touchpad_x == TOUCHPAD_X_MAX)
            keypad.touchpad_contact = keypad.touchpad_down = false;
        break;
    default:
        int button = keyToKeypad(event);
        if (button != -1)
        {
            if (button == on)
                keypad_on_pressed();

            setKeypad(button, false);
        }

        return;
    }

    notifyTouchpadStateChanged();
    keypad.kpc.gpio_int_active |= 0x800;
    keypad_int_check();
}

bool QtKeypadBridge::eventFilter(QObject *obj, QEvent *event)
{
    Q_UNUSED(obj);

    if(event->type() == QEvent::KeyPress)
        keyPressEvent(static_cast<QKeyEvent*>(event));
    else if(event->type() == QEvent::KeyRelease)
        keyReleaseEvent(static_cast<QKeyEvent*>(event));
    else
        return false;

    return true;
}
