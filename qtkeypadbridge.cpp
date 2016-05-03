#include "qtkeypadbridge.h"

#include "keymap.h"
#include "core/keypad.h"
#include "qmlbridge.h"

QtKeypadBridge qt_keypad_bridge;

int keyToKeypad(QKeyEvent *event)
{
    static const int ALT   = 0x02000000;

    // Prefer virtual key, as it can be keyboard depandent key such as ParenLeft
    auto button = event->key();

    // Compose Alt into the unused bit of the keycode
    if (event->modifiers() & Qt::AltModifier)
        button |= ALT;

    switch (button)
    {
        // Touchpad left buttons
    case Qt::Key_Escape: return keymap::esc;
    case Qt::Key_End: case Qt::Key_S | ALT: return keymap::pad;
    case Qt::Key_Tab: return keymap::tab;

        // Touchpad right buttons
    case Qt::Key_Home: case Qt::Key_Escape | ALT: return keymap::on;
    case Qt::Key_PageUp: case Qt::Key_D | ALT: return keymap::doc;
    case Qt::Key_PageDown: case Qt::Key_M | ALT: return keymap::menu;

        // Touchpad bottom buttons
    case Qt::Key_Control: return keymap::ctrl;
    case Qt::Key_Shift: return keymap::shift;
    case Qt::Key_Insert: case Qt::Key_V | ALT: return keymap::var;
    case Qt::Key_Backspace: case Qt::Key_Delete: return keymap::del;

        // Alpha buttons
    case Qt::Key_A: return keymap::aa;
    case Qt::Key_B: return keymap::ab;
    case Qt::Key_C: return keymap::ac;
    case Qt::Key_D: return keymap::ad;
    case Qt::Key_E: return keymap::ae;
    case Qt::Key_F: return keymap::af;
    case Qt::Key_G: return keymap::ag;
    case Qt::Key_H: return keymap::ah;
    case Qt::Key_I: return keymap::ai;
    case Qt::Key_J: return keymap::aj;
    case Qt::Key_K: return keymap::ak;
    case Qt::Key_L: return keymap::al;
    case Qt::Key_M: return keymap::am;
    case Qt::Key_N: return keymap::an;
    case Qt::Key_O: return keymap::ao;
    case Qt::Key_P: return keymap::ap;
    case Qt::Key_Q: return keymap::aq;
    case Qt::Key_R: return keymap::ar;
    case Qt::Key_S: return keymap::as;
    case Qt::Key_T: return keymap::at;
    case Qt::Key_U: return keymap::au;
    case Qt::Key_V: return keymap::av;
    case Qt::Key_W: return keymap::aw;
    case Qt::Key_X: return keymap::ax;
    case Qt::Key_Y: return keymap::ay;
    case Qt::Key_Z: return keymap::az;
    case Qt::Key_Less: case Qt::Key_E | ALT: return keymap::ee;
    case Qt::Key_Bar: return keymap::pi;
    case Qt::Key_Comma: return keymap::comma;
    case Qt::Key_Question: case Qt::Key_W | ALT: return keymap::punct;
    case Qt::Key_Greater: case Qt::Key_F | ALT: return keymap::flag;
    case Qt::Key_Space: return keymap::space;
    case Qt::Key_Enter | ALT: case Qt::Key_Return | ALT: return keymap::ret;

        // Numpad buttons
    case Qt::Key_0: return keymap::n0;
    case Qt::Key_1: return keymap::n1;
    case Qt::Key_2: return keymap::n2;
    case Qt::Key_3: return keymap::n3;
    case Qt::Key_4: return keymap::n4;
    case Qt::Key_5: return keymap::n5;
    case Qt::Key_6: return keymap::n6;
    case Qt::Key_7: return keymap::n7;
    case Qt::Key_8: return keymap::n8;
    case Qt::Key_9: return keymap::n9;
    case Qt::Key_Period: return keymap::dot;
    case Qt::Key_Minus | ALT: case Qt::Key_QuoteLeft: return keymap::neg;

        // Left buttons
    case Qt::Key_Equal: case Qt::Key_Q | ALT: return keymap::equ;
    case Qt::Key_Backslash: case Qt::Key_T | ALT: return keymap::trig;
    case Qt::Key_AsciiCircum: case Qt::Key_P | ALT: return keymap::pow;
    case Qt::Key_At: case Qt::Key_2 | ALT: return keymap::squ;
    case Qt::Key_BracketLeft: case Qt::Key_X | ALT: return keymap::exp;
    case Qt::Key_BracketRight: case Qt::Key_1 | ALT: return keymap::pow10;
    case Qt::Key_ParenLeft: case Qt::Key_F1: return keymap::pleft;
    case Qt::Key_ParenRight: case Qt::Key_F2: return keymap::pright;

        // Right buttons
    case Qt::Key_Semicolon: case Qt::Key_O | ALT: return keymap::metrix;
    case Qt::Key_Apostrophe: case Qt::Key_C | ALT: return keymap::cat;
    case Qt::Key_Asterisk: case Qt::Key_A | ALT: return keymap::mult;
    case Qt::Key_Slash: case Qt::Key_F3: return keymap::div;
    case Qt::Key_Plus: case Qt::Key_Equal | ALT: return keymap::plus;
    case Qt::Key_Minus: case Qt::Key_Underscore: return keymap::minus;
    case Qt::Key_Enter: case Qt::Key_Return: return keymap::enter;
    }
    return keymap::nobtn;
}

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
        if (button != keymap::nobtn)
        {
            if (button == keymap::on)
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
        if (button != keymap::nobtn)
            setKeypad(button, false);

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
