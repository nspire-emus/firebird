#include "qtkeypadbridge.h"

#include "keymap.h"
#include "core/keypad.h"
#include "qmlbridge.h"

QtKeypadBridge qt_keypad_bridge;

void QtKeypadBridge::keyPressEvent(QKeyEvent *event)
{
    Qt::Key key = static_cast<Qt::Key>(event->key());

    switch(key)
    {
    case Qt::Key_Down:
        touchpad_x = TOUCHPAD_X_MAX / 2;
        touchpad_y = 0;
        break;
    case Qt::Key_Up:
        touchpad_x = TOUCHPAD_X_MAX / 2;
        touchpad_y = TOUCHPAD_Y_MAX;
        break;
    case Qt::Key_Left:
        touchpad_y = TOUCHPAD_Y_MAX / 2;
        touchpad_x = 0;
        break;
    case Qt::Key_Right:
        touchpad_y = TOUCHPAD_Y_MAX / 2;
        touchpad_x = TOUCHPAD_X_MAX;
        break;
    case Qt::Key_Return:
        key = Qt::Key_Enter;

    default:
        auto& keymap = keymap_tp;
        for(unsigned int row = 0; row < sizeof(keymap)/sizeof(*keymap); ++row)
        {
            for(unsigned int col = 0; col < sizeof(*keymap)/sizeof(**keymap); ++col)
            {
                if(key == keymap[row][col].key && keymap[row][col].alt == (bool(event->modifiers() & Qt::AltModifier) || bool(event->modifiers() & Qt::MetaModifier)))
                {
                    if(row == 0 && col == 9)
                        keypad_on_pressed();

                    key_map[row] |= 1 << col;
                    notifyKeypadStateChanged(row, col, true);
                    keypad_int_check();
                    return;
                }
            }
        }
        return;
    }

    touchpad_contact = touchpad_down = true;
    notifyTouchpadStateChanged();
    kpc.gpio_int_active |= 0x800;

    keypad_int_check();
}

void QtKeypadBridge::keyReleaseEvent(QKeyEvent *event)
{
    Qt::Key key = static_cast<Qt::Key>(event->key());

    switch(key)
    {
    case Qt::Key_Down:
        if(touchpad_x == TOUCHPAD_X_MAX / 2
            && touchpad_y == 0)
            touchpad_contact = touchpad_down = false;
        break;
    case Qt::Key_Up:
        if(touchpad_x == TOUCHPAD_X_MAX / 2
            && touchpad_y == TOUCHPAD_Y_MAX)
            touchpad_contact = touchpad_down = false;
        break;
    case Qt::Key_Left:
        if(touchpad_y == TOUCHPAD_Y_MAX / 2
            && touchpad_x == 0)
            touchpad_contact = touchpad_down = false;
        break;
    case Qt::Key_Right:
        if(touchpad_y == TOUCHPAD_Y_MAX / 2
            && touchpad_x == TOUCHPAD_X_MAX)
            touchpad_contact = touchpad_down = false;
        break;
    case Qt::Key_Return:
        key = Qt::Key_Enter;
    default:
        auto& keymap = keymap_tp;
        for(unsigned int row = 0; row < sizeof(keymap)/sizeof(*keymap); ++row)
        {
            for(unsigned int col = 0; col < sizeof(*keymap)/sizeof(**keymap); ++col)
            {
                if(key == keymap[row][col].key && keymap[row][col].alt == (bool(event->modifiers() & Qt::AltModifier) || bool(event->modifiers() & Qt::MetaModifier)))
                {
                    key_map[row] &= ~(1 << col);
                    notifyKeypadStateChanged(row, col, false);
                    keypad_int_check();
                    return;
                }
            }
        }
        return;
    }

    notifyTouchpadStateChanged();
    kpc.gpio_int_active |= 0x800;
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
