#include "lcdwidget.h"
#include "core/keypad.h"
#include "qtkeypadbridge.h"
#include "qmlbridge.h"
#include "qtframebuffer.h"

LCDWidget::LCDWidget(QWidget *parent, Qt::WindowFlags f)
    : QWidget(parent, f)
{
    connect(&refresh_timer, SIGNAL(timeout()), this, SLOT(update()));

    refresh_timer.setInterval(1000 / 30); // 30 fps
}

void LCDWidget::mousePressEvent(QMouseEvent *event)
{
    if(event->button() == Qt::RightButton)
        keypad.touchpad_down = keypad.touchpad_contact = true;
    else
        keypad.touchpad_contact = true;

    if(event->x() >= 0 && event->x() < width()
            && event->y() >= 0 && event->y() < height())
    {
        keypad.touchpad_x = event->x() * TOUCHPAD_X_MAX/width();
        keypad.touchpad_y = TOUCHPAD_Y_MAX - (event->y() * TOUCHPAD_Y_MAX/height());
    }

    notifyTouchpadStateChanged();
    keypad.kpc.gpio_int_active |= 0x800;
    keypad_int_check();
}

void LCDWidget::mouseReleaseEvent(QMouseEvent *event)
{
    if(event->button() == Qt::RightButton)
        keypad.touchpad_down = keypad.touchpad_contact = false;
    else
        keypad.touchpad_contact = false;

    notifyTouchpadStateChanged();
    keypad.kpc.gpio_int_active |= 0x800;
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

    int vel_x = new_x - keypad.touchpad_x;
    int vel_y = new_y - keypad.touchpad_y;
    keypad.touchpad_rel_x += vel_x;
    keypad.touchpad_rel_y += vel_y;

    keypad.touchpad_x = new_x;
    keypad.touchpad_y = new_y;

    notifyTouchpadStateChanged();
    keypad.kpc.gpio_int_active |= 0x800;
    keypad_int_check();
}

void LCDWidget::showEvent(QShowEvent *e)
{
    QWidget::showEvent(e);

    refresh_timer.start();
}

void LCDWidget::hideEvent(QHideEvent *e)
{
    QWidget::hideEvent(e);

    refresh_timer.stop();
}

void LCDWidget::closeEvent(QCloseEvent *e)
{
    QWidget::closeEvent(e);

    emit closed();
}

void LCDWidget::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    paintFramebuffer(&painter);
}
