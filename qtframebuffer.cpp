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

    if(!emulate_cx)
    {
        format = QImage::Format_RGB444;
        uint16_t *px = framebuffer;
        for(unsigned int i = 0; i < 320*240; ++i)
        {
            uint8_t pix = *px & 0xF;
            uint16_t n = pix << 8 | pix << 4 | pix;
            *px = ~n;
            ++px;
        }
    }

    QImage image(reinterpret_cast<const uchar*>(framebuffer), 320, 240, 320 * 2, format, free, framebuffer);

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

    QImage image = renderFramebuffer().scaled(p->window().size(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
    p->drawImage(p->window(), image);
}

void QMLFramebuffer::paint(QPainter *p)
{
    paintFramebuffer(p);
}
