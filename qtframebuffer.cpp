#include "qtframebuffer.h"

#include <cassert>

#include <QImage>
#include <QPainter>

#include "core/lcd.h"
#include "core/misc.h"

QImage renderFramebuffer()
{
    uint16_t *framebuffer = reinterpret_cast<uint16_t*>(malloc(320 * 240 * 2));
    assert(framebuffer);

    uint32_t bitfields[] = { 0x01F, 0x000, 0x000};

    lcd_cx_draw_frame(framebuffer, bitfields);
    QImage::Format format = bitfields[0] == 0x00F ? QImage::Format_RGB444 : QImage::Format_RGB16;

    QImage image(reinterpret_cast<const uchar*>(framebuffer), 320, 240, 320 * 2, format, free, framebuffer);

    if(!emulate_cx)
    {
        // Invert framebuffer
        image.invertPixels();
    }

    return image;
}

void paintFramebuffer(QPainter *p)
{
    if(lcd_contrast == 0)
    {
        p->fillRect(p->window(), emulate_cx ? Qt::black : Qt::white);
        p->setPen(emulate_cx ? Qt::white : Qt::black);
        p->drawText(p->window(), Qt::AlignCenter, QObject::tr("LCD turned off"));
        return;
    }

    p->drawImage(p->window(), renderFramebuffer());
}

void QMLFramebuffer::paint(QPainter *p)
{
    paintFramebuffer(p);
}
