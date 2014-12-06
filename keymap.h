#ifndef KEYMAP_H
#define KEYMAP_H

//Don't use this as header file.

#include <QKeyEvent>

struct HostKey {
    Qt::Key key;
    QString name;
};

#define KEY(x, y) HostKey x{Qt::Key_##y, #y}

KEY(enter, Enter);
KEY(tab, Tab);
KEY(ret, Return);
KEY(space, Space);
KEY(shift, Shift);
KEY(ctrl, Control);
KEY(backspace, Backspace);
KEY(escape, Escape);

KEY(aa, A);
KEY(ab, B);
KEY(ac, C);
KEY(ad, D);
KEY(ae, E);
KEY(af, F);
KEY(ag, G);
KEY(ah, H);
KEY(ai, I);
KEY(aj, J);
KEY(ak, K);
KEY(al, L);
KEY(am, M);
KEY(an, N);
KEY(ao, O);
KEY(ap, P);
KEY(aq, Q);
KEY(ar, R);
KEY(as, S);
KEY(at, T);
KEY(au, U);
KEY(av, V);
KEY(aw, W);
KEY(ax, X);
KEY(ay, Y);
KEY(az, Z);

KEY(n0, 0);
KEY(n1, 1);
KEY(n2, 2);
KEY(n3, 3);
KEY(n4, 4);
KEY(n5, 5);
KEY(n6, 6);
KEY(n7, 7);
KEY(n8, 8);
KEY(n9, 9);

KEY(minus, Minus);
KEY(plus, Plus);
KEY(paren_left, ParenLeft);
KEY(paren_right, ParenRight);

KEY(comma, Comma);
KEY(period, Period);

KEY(f1, F1);
KEY(f2, F2);
KEY(f3, F3);
KEY(f4, F4);
KEY(f5, F5);
KEY(f6, F6);
KEY(f7, F7);
KEY(f8, F8);
KEY(f9, F9);
KEY(f10, F10);
KEY(f11, F11);
KEY(f12, F12);

HostKey none{static_cast<Qt::Key>(0), ""};

HostKey keymap_tp[8][11] =
{
{ ret, enter, none, none, space, az, ay, n0, none, none, none },
{ ax, aw, av, n3, au, at, as, n1, f1, f2, f3 },
{ ar, aq, ap, n6, ao, an, am, n4, f4, f5, none },
{ al, ak, aj, n9, ai, ah, ag, n7, f4, f5, none },
{ af, ae, ad, none, ac, ab, aa, none, none, none, none },
{ none, none, minus, none, none, none, n5, none, none, backspace, none },
{ none, none, plus, none, n2, none, n8, escape, none, tab, none },
{ none, none, none, none, none, none, none, none, shift, ctrl, comma }
};

#endif // KEYMAP_H
