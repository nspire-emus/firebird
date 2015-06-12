#ifndef QMLFRAMEBUFFER_H
#define QMLFRAMEBUFFER_H

#include <QQuickPaintedItem>

QImage renderFramebuffer();
void paintFramebuffer(QPainter *p);

#endif // QMLFRAMEBUFFER_H
