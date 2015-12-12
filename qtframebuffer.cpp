#include "qtframebuffer.h"

#include <array>
#include <cassert>

#include <QImage>
#include <QPainter>

#include "core/debug.h"
#include "core/emu.h"
#include "core/lcd.h"
#include "core/misc.h"

QImage renderFramebuffer()
{
    static std::array<uint16_t, 320 * 240> framebuffer;

    uint32_t bitfields[] = { 0x01F, 0x000, 0x000};

    lcd_cx_draw_frame(framebuffer.data(), bitfields);
    QImage::Format format = bitfields[0] == 0x00F ? QImage::Format_RGB444 : QImage::Format_RGB16;

    if(!emulate_cx)
    {
        format = QImage::Format_RGB444;
        uint16_t *px = framebuffer.data();
        for(unsigned int i = 0; i < 320*240; ++i)
        {
            uint8_t pix = *px & 0xF;
            uint16_t n = pix << 8 | pix << 4 | pix;
            *px = ~n;
            ++px;
        }
    }

    QImage image(reinterpret_cast<const uchar*>(framebuffer.data()), 320, 240, 320 * 2, format);

    return image;
}

void paintFramebuffer(QPainter *p)
{
    if(hdq1w.lcd_contrast == 0)
    {
        p->fillRect(p->window(), emulate_cx ? Qt::black : Qt::white);
        p->setPen(emulate_cx ? Qt::white : Qt::black);
        p->drawText(p->window(), Qt::AlignCenter, QObject::tr("LCD turned off"));
    }
    else
    {
        QImage image = renderFramebuffer().scaled(p->window().size(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
        p->drawImage(p->window(), image);
    }

    if(in_debugger)
    {
        p->setCompositionMode(QPainter::CompositionMode_SourceOver);
        p->fillRect(p->window(), QColor(30, 30, 30, 150));
        p->setPen(Qt::white);
        p->drawText(p->window(), Qt::AlignCenter, QObject::tr("In debugger"));
    }
}

QMLFramebuffer::QMLFramebuffer(QQuickItem *parent) : QQuickPaintedItem(parent)
{
    connect(&timer, &QTimer::timeout, this, &QMLFramebuffer::resetKeypad);
}

void QMLFramebuffer::paint(QPainter *p)
{
    paintFramebuffer(p);
}

QVariant QMLFramebuffer::inputMethodQuery(Qt::InputMethodQuery query) const
{
    switch(query)
    {
    case Qt::ImEnabled:
        return true;
    case Qt::ImHints:
        return Qt::ImhMultiLine;
    default:
        return {};
    }
}
#include "qtkeypadbridge.h"
void QMLFramebuffer::inputMethodEvent(QInputMethodEvent *ev)
{
    if(ev->commitString().length() <= ev->preeditString().length())
        return;

    for(int i = ev->preeditString().length(); i < ev->commitString().length(); ++i)
    {
        // From https://stackoverflow.com/questions/14034209/convert-string-representation-of-keycode-to-qtkey-or-any-int-and-back
        QKeySequence seq(ev->commitString()[i]);
        int keyCode;

        if(seq.count() == 1)
            keyCode = seq[0];
        else {
            assert(seq.count() == 0);

            QKeyEvent k(QEvent::KeyPress, Qt::Key_Shift, Qt::NoModifier);
            qt_keypad_bridge.keyPressEvent(&k);

            // Add a non-modifier key "A" to the picture because QKeySequence
            // seems to need that to acknowledge the modifier. We know that A has
            // a keyCode of 65 (or 0x41 in hex)
            seq = QKeySequence(ev->commitString()[i] + QStringLiteral("+A"));
            assert(seq.count() == 1);
            assert(seq[0] > 65);
            keyCode = seq[0] - 65;
        }

        QKeyEvent k(QEvent::KeyPress, keyCode, Qt::NoModifier);
        qt_keypad_bridge.keyPressEvent(&k);
    }

    timer.start(100);
}

void QMLFramebuffer::resetKeypad()
{
    memset(keypad.key_map, 0, sizeof(keypad.key_map));
    keypad_int_check();
}
