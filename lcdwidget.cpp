#include "lcdwidget.h"
#include "core/keypad.h"
#include "qtkeypadbridge.h"
#include "qmlbridge.h"
#include "qtframebuffer.h"

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

    notifyTouchpadStateChanged();
    kpc.gpio_int_active |= 0x800;
    keypad_int_check();
}

void LCDWidget::mouseReleaseEvent(QMouseEvent *event)
{
    if(event->button() == Qt::RightButton)
        touchpad_down = touchpad_contact = false;
    else
        touchpad_contact = false;

    notifyTouchpadStateChanged();
    kpc.gpio_int_active |= 0x800;
    keypad_int_check();
}

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
    touchpad_vel_x = vel_x;
    touchpad_vel_y = vel_y;

    touchpad_x = new_x;
    touchpad_y = new_y;

    notifyTouchpadStateChanged();
    kpc.gpio_int_active |= 0x800;
    keypad_int_check();
}

void LCDWidget::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    paintFramebuffer(&painter);
}
