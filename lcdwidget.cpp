#include "lcdwidget.h"

extern "C" {
    #include "keypad.h"
}

#include "keymap.h"

LCDWidget::LCDWidget()
{
}

LCDWidget::LCDWidget(QWidget *parent)
    : QGraphicsView(parent)
{

}

void LCDWidget::keyPressEvent(QKeyEvent *event)
{
    Qt::Key key = static_cast<Qt::Key>(event->key());
    auto& keymap = keymap_tp;
    for(unsigned int row = 0; row < sizeof(keymap)/sizeof(*keymap); ++row)
    {
        for(unsigned int col = 0; col < sizeof(*keymap)/sizeof(**keymap); ++col)
        {
            if(key == keymap[row][col].key)
                key_map[row] |= 1 << col;
        }
    }

    keypad_int_check();
}

void LCDWidget::keyReleaseEvent(QKeyEvent *event)
{
    Qt::Key key = static_cast<Qt::Key>(event->key());
    auto& keymap = keymap_tp;
    for(unsigned int row = 0; row < sizeof(keymap)/sizeof(*keymap); ++row)
    {
        for(unsigned int col = 0; col < sizeof(*keymap)/sizeof(**keymap); ++col)
        {
            if(key == keymap[row][col].key)
                key_map[row] &= ~(1 << col);
        }
    }

    keypad_int_check();
}

void LCDWidget::mousePressEvent(QMouseEvent *event)
{
    if(event->button() == Qt::RightButton)
        touchpad_down = true;
    else
        touchpad_contact = true;

    touchpad_x = event->x() * TOUCHPAD_X_MAX/width();
    touchpad_y = TOUCHPAD_Y_MAX - (event->y() * TOUCHPAD_Y_MAX/height());

    kpc.gpio_int_active |= 0x800;
    keypad_int_check();
}

void LCDWidget::mouseReleaseEvent(QMouseEvent *event)
{
    if(event->button() == Qt::RightButton)
        touchpad_down = false;
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

    int vel_x = new_x - touchpad_x;
    int vel_y = new_y - touchpad_y;
    touchpad_vel_x = vel_x < -127 ? -127 : vel_x > 127 ? 127 : vel_x;
    touchpad_vel_y = vel_y < -127 ? -127 : vel_y > 127 ? 127 : vel_y;

    touchpad_x = new_x;
    touchpad_y = new_y;

    kpc.gpio_int_active |= 0x800;
    keypad_int_check();
}
