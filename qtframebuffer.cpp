#include "qtframebuffer.h"

#include <array>
#include <cassert>

#include <QImage>
#include <QPainter>
#include <QGuiApplication>
#include <QScreen>

#include "core/debug.h"
#include "core/emu.h"
#include "core/lcd.h"
#include "core/misc.h"

#include "qtkeypadbridge.h"

QImage renderFramebuffer()
{
    static std::array<uint16_t, 320 * 240> framebuffer;

    lcd_cx_draw_frame(framebuffer.data());
    QImage::Format format = QImage::Format_RGB16;

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
#ifdef IS_IOS_BUILD
    // Apparently, this is needed (will be 2 on retina screens)
    // TODO: actually make sure Android doesn't need that as well
    static const double devicePixelRatio = ((QGuiApplication*)QCoreApplication::instance())->primaryScreen()->devicePixelRatio();
#else
    // Has to be 1 on desktop, even on retina (tested on OS X 10.11 with one retina, one non-retina, and both ; same on Win VM)
    static const double devicePixelRatio = 1;
#endif

    QRect painterWindowScaled(p->window().topLeft(), p->window().size() / devicePixelRatio);

    if(hdq1w.lcd_contrast == 0)
    {
        p->fillRect(painterWindowScaled, emulate_cx ? Qt::black : Qt::white);
        p->setPen(emulate_cx ? Qt::white : Qt::black);
        p->drawText(painterWindowScaled, Qt::AlignCenter, QObject::tr("LCD turned off"));
    }
    else
    {
        QImage image = renderFramebuffer().scaled(p->window().size(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
        image.setDevicePixelRatio(devicePixelRatio);
        p->drawImage((p->window().width() - image.width()) / 2, (p->window().height() - image.height()) / 2, image);
    }

    if(in_debugger)
    {
        p->setCompositionMode(QPainter::CompositionMode_SourceOver);
        p->fillRect(painterWindowScaled, QColor(30, 30, 30, 150));
        p->setPen(Qt::white);
        p->drawText(painterWindowScaled, Qt::AlignCenter, QObject::tr("In debugger"));
    }
}

QMLFramebuffer::QMLFramebuffer(QQuickItem *parent)
 : QQuickPaintedItem(parent)
{
    installEventFilter(&qt_keypad_bridge);
}

void QMLFramebuffer::paint(QPainter *p)
{
    paintFramebuffer(p);
}
