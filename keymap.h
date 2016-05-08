#ifndef KEYMAP_H
#define KEYMAP_H

namespace keymap {

constexpr const int ROWS = 8, COLS = 11;

enum {
    ret,    enter,  none,   neg,    space,  az,     ay,     n0,     punct,  on,     none1,
    ax,     aw,     av,     n3,     au,     at,     as,     n1,     pi,     trig,   pow10,
    ar,     aq,     ap,     n6,     ao,     an,     am,     n4,     ee,     squ,    none2,
    al,     ak,     aj,     n9,     ai,     ah,     ag,     n7,     div,    exp,    none3,
    af,     ae,     ad,     none4,  ac,     ab,     aa,     equ,    mult,   pow,    none5,
    none6,  var,    minus,  pright, dot,    pleft,  n5,     cat,    metrix, del,    pad,
    flag,   none7,  plus,   doc,    n2,     menu,   n8,     esc,    none8,  tab,    none9,
    none10, none11, none12, none13, none14, none15, none16, none17, shift,  ctrl,   comma
};

}

#endif // KEYMAP_H
