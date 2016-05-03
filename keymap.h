#ifndef KEYMAP_H
#define KEYMAP_H

//Don't use this as header file.

#include <QKeyEvent>

int keyToKeypad(QKeyEvent *event);

enum {
    ret,    enter,  none,   neg,    space,  az,     ay,     n0,     punct,  on,     none1,
    ax,     aw,     av,     n3,     au,     at,     as,     n1,     pi,     trig,   pow10_,
    ar,     aq,     ap,     n6,     ao,     an,     am,     n4,     ee,     squ,    none2,
    al,     ak,     aj,     n9,     ai,     ah,     ag,     n7,     div_,   exp_,   none3,
    af,     ae,     ad,     none4,  ac,     ab,     aa,     equ,    mult,   pow_,   none5,
    none6,  var,    minus,  pright, dot,    pleft,  n5,     cat,    metrix, del,    pad,
    flag,   none7,  plus,   doc,    n2,     menu,   n8,     esc,    none8,  tab,    none9,
    none10, none11, none12, none13, none14, none15, none16, none17, shift,  ctrl,   comma
};

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
    case Qt::Key_Escape: return esc;
    case Qt::Key_End: case Qt::Key_S | ALT: return pad;
    case Qt::Key_Tab: return tab;

        // Touchpad right buttons
    case Qt::Key_Home: case Qt::Key_Escape | ALT: return on;
    case Qt::Key_PageUp: case Qt::Key_D | ALT: return doc;
    case Qt::Key_PageDown: case Qt::Key_M | ALT: return menu;

        // Touchpad bottom buttons
    case Qt::Key_Control: return ctrl;
    case Qt::Key_Shift: return shift;
    case Qt::Key_Insert: case Qt::Key_V | ALT: return var;
    case Qt::Key_Backspace: case Qt::Key_Delete: return del;

        // Alpha buttons
    case Qt::Key_A: return aa;
    case Qt::Key_B: return ab;
    case Qt::Key_C: return ac;
    case Qt::Key_D: return ad;
    case Qt::Key_E: return ae;
    case Qt::Key_F: return af;
    case Qt::Key_G: return ag;
    case Qt::Key_H: return ah;
    case Qt::Key_I: return ai;
    case Qt::Key_J: return aj;
    case Qt::Key_K: return ak;
    case Qt::Key_L: return al;
    case Qt::Key_M: return am;
    case Qt::Key_N: return an;
    case Qt::Key_O: return ao;
    case Qt::Key_P: return ap;
    case Qt::Key_Q: return aq;
    case Qt::Key_R: return ar;
    case Qt::Key_S: return as;
    case Qt::Key_T: return at;
    case Qt::Key_U: return au;
    case Qt::Key_V: return av;
    case Qt::Key_W: return aw;
    case Qt::Key_X: return ax;
    case Qt::Key_Y: return ay;
    case Qt::Key_Z: return az;
    case Qt::Key_Less: case Qt::Key_E | ALT: return ee;
    case Qt::Key_Bar: return pi;
    case Qt::Key_Comma: return comma;
    case Qt::Key_Question: case Qt::Key_W | ALT: return punct;
    case Qt::Key_Greater: case Qt::Key_F | ALT: return flag;
    case Qt::Key_Space: return space;
    case Qt::Key_Enter | ALT: case Qt::Key_Return | ALT: return ret;

        // Numpad buttons
    case Qt::Key_0: return n0;
    case Qt::Key_1: return n1;
    case Qt::Key_2: return n2;
    case Qt::Key_3: return n3;
    case Qt::Key_4: return n4;
    case Qt::Key_5: return n5;
    case Qt::Key_6: return n6;
    case Qt::Key_7: return n7;
    case Qt::Key_8: return n8;
    case Qt::Key_9: return n9;
    case Qt::Key_Period: return dot;
    case Qt::Key_Minus | ALT: case Qt::Key_QuoteLeft: return neg;

        // Left buttons
    case Qt::Key_Equal: case Qt::Key_Q | ALT: return equ;
    case Qt::Key_Backslash: case Qt::Key_T | ALT: return trig;
    case Qt::Key_AsciiCircum: case Qt::Key_P | ALT: return pow_;
    case Qt::Key_At: case Qt::Key_2 | ALT: return squ;
    case Qt::Key_BracketLeft: case Qt::Key_X | ALT: return exp_;
    case Qt::Key_BracketRight: case Qt::Key_1 | ALT: return pow10_;
    case Qt::Key_ParenLeft: case Qt::Key_F1: return pleft;
    case Qt::Key_ParenRight: case Qt::Key_F2: return pright;

        // Right buttons
    case Qt::Key_Semicolon: case Qt::Key_O | ALT: return metrix;
    case Qt::Key_Apostrophe: case Qt::Key_C | ALT: return cat;
    case Qt::Key_Asterisk: case Qt::Key_A | ALT: return mult;
    case Qt::Key_Slash: case Qt::Key_F3: return div_;
    case Qt::Key_Plus: case Qt::Key_Equal | ALT: return plus;
    case Qt::Key_Minus: case Qt::Key_Underscore: return minus;
    case Qt::Key_Enter: case Qt::Key_Return: return enter;
    }
    return -1;
}

#endif // KEYMAP_H
