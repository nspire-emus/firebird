#include "lcdwidget.h"
#include "keypad.h"
#include "keymap.h"

LCDWidget::LCDWidget()
{}

LCDWidget::LCDWidget(QWidget *parent)
    : QGraphicsView(parent)
{}

void LCDWidget::keyPressEvent(QKeyEvent *event)
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
        /*touchpad_x = TOUCHPAD_X_MAX / 2;
        touchpad_y = TOUCHPAD_Y_MAX / 2;
        touchpad_contact = touchpad_down = true;
        kpc.gpio_int_active |= 0x800;
        keypad_int_check();*/
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
                    keypad_int_check();
                    return;
                }
            }
        }
        return;
    }

    touchpad_contact = touchpad_down = true;
    kpc.gpio_int_active |= 0x800;

    keypad_int_check();
}

void LCDWidget::keyReleaseEvent(QKeyEvent *event)
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
        /*if(touchpad_x == TOUCHPAD_X_MAX / 2
            && touchpad_y == TOUCHPAD_Y_MAX / 2)
        {
            touchpad_contact = touchpad_down = false;
            kpc.gpio_int_active |= 0x800;
            keypad_int_check();
        }*/
    default:
        auto& keymap = keymap_tp;
        for(unsigned int row = 0; row < sizeof(keymap)/sizeof(*keymap); ++row)
        {
            for(unsigned int col = 0; col < sizeof(*keymap)/sizeof(**keymap); ++col)
            {
                if(key == keymap[row][col].key && keymap[row][col].alt == (bool(event->modifiers() & Qt::AltModifier) || bool(event->modifiers() & Qt::MetaModifier)))
                {
                    key_map[row] &= ~(1 << col);
                    keypad_int_check();
                    return;
                }
            }
        }
        return;
    }

    kpc.gpio_int_active |= 0x800;
    keypad_int_check();
}

void LCDWidget::mousePressEvent(QMouseEvent *event)
{
    if(event->button() == Qt::RightButton)
        touchpad_down = touchpad_contact = true;
    else
        touchpad_contact = true;

    if(event->x() >= 0 && event->x() < 320
            && event->y() >= 0 && event->y() < 240)
    {
        touchpad_x = event->x() * TOUCHPAD_X_MAX/width();
        touchpad_y = TOUCHPAD_Y_MAX - (event->y() * TOUCHPAD_Y_MAX/height());
    }

    kpc.gpio_int_active |= 0x800;
    keypad_int_check();
}

void LCDWidget::mouseReleaseEvent(QMouseEvent *event)
{
    if(event->button() == Qt::RightButton)
        touchpad_down = touchpad_contact = false;
    else
        touchpad_contact = false;

    kpc.gpio_int_active |= 0x800;
    keypad_int_check();
}
extern int8_t touchpad_vel_x, touchpad_vel_y;
void LCDWidget::mouseMoveEvent(QMouseEvent *event)
{
    int new_x = event->x() * TOUCHPAD_X_MAX/width(),
            new_y = TOUCHPAD_Y_MAX - (event->y() * TOUCHPAD_Y_MAX/height());

    if(new_x < 0)
        new_x = 0;
    if(new_x > TOUCHPAD_X_MAX)
        new_x = TOUCHPAD_X_MAX;

    if(new_y < 0)
        new_y = 0;
    if(new_y > TOUCHPAD_Y_MAX)
        new_y = TOUCHPAD_Y_MAX;

    int vel_x = new_x - touchpad_x;
    int vel_y = new_y - touchpad_y;
    touchpad_vel_x = vel_x/2;
    touchpad_vel_y = vel_y/2;

    touchpad_x = new_x * 2;
    touchpad_y = new_y * 2;

    kpc.gpio_int_active |= 0x800;
    keypad_int_check();
}
