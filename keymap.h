#ifndef KEYMAP_H
#define KEYMAP_H

//Don't use this as header file.

#include <QKeyEvent>

struct HostKey {
    Qt::Key key;
    QString name;
    bool alt;
};

#define KEY(x, y) static HostKey x{Qt::Key_##y, QStringLiteral(#y), false}
#define ALT(x, y, z) static HostKey x{Qt::Key_##y,QStringLiteral(z), true}

KEY(enter, Enter);
KEY(tab, Tab);
KEY(ret, Return);
KEY(space, Space);
KEY(shift, Shift);
KEY(ctrl, Control);
KEY(backspace, Backspace);
KEY(escape, Escape);
KEY(dot, Period);

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

ALT(negative, Minus, "(-)");
ALT(wtf, W, "?!");
ALT(pi, P, "pi");
ALT(trig, T, "trig");
ALT(pow10_, 1, "10^x");
ALT(ee, E, "EE");
ALT(squ, 2, "x^2");
ALT(exp_, X, "e^x");
ALT(var, V, "var");
ALT(flag, F, "flag");
ALT(frac, O, "frac");
ALT(doc, D, "doc");
ALT(menu, M, "menu");
ALT(cat, C, "cat");
ALT(scratch, S, "scratch");
ALT(equ, Q, "=");
ALT(ast, A, "*");
ALT(pow_, P, "^");
ALT(on, Escape, "On");

HostKey none{static_cast<Qt::Key>(0), QStringLiteral(""), false};

HostKey keymap_tp[8][11] =
{
{ ret, enter, none, negative, space, az, ay, n0, wtf, on, none },
{ ax, aw, av, n3, au, at, as, n1, pi, trig, pow10_ },
{ ar, aq, ap, n6, ao, an, am, n4, ee, squ, none },
{ al, ak, aj, n9, ai, ah, ag, n7, f3, exp_, none },
{ af, ae, ad, none, ac, ab, aa, equ, ast, pow_, none },
{ none, var, minus, f2, dot, f1, n5, cat, frac, backspace, scratch },
{ flag, none, plus, doc, n2, menu, n8, escape, none, tab, none },
{ none, none, none, none, none, none, none, none, shift, ctrl, comma }
};

#endif // KEYMAP_H
