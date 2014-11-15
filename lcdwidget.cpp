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
