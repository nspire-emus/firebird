#ifndef KEYMAP_H
#define KEYMAP_H

//Don't use this as header file.

#include <QKeyEvent>

struct HostKey {
    Qt::Key key;
    QString name;
};

HostKey ret{Qt::Key_Return, "Return"},
        enter{Qt::Key_Enter, "Enter"},
        space{Qt::Key_Space, "Space"},
        z{Qt::Key_Z, "Z"},
        y{Qt::Key_Y, "Y"},
        n0{Qt::Key_0, "0"},
        n1{Qt::Key_1, "1"},
        esc{Qt::Key_Escape, "Esc"},
        back{Qt::Key_Backspace, "Backsp"};

HostKey none{static_cast<Qt::Key>(0), ""};

HostKey keymap_tp[7][11] =
{
{ ret, enter, none, none, space, z, y, n0, none, none, none},
{ none, none, none, none, none, none, none, n1, none, none, none}};

#endif // KEYMAP_H
