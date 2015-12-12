#ifndef QMLFRAMEBUFFER_H
#define QMLFRAMEBUFFER_H

#include <QQuickPaintedItem>
#include <QTimer>

class QMLFramebuffer : public QQuickPaintedItem
{
public:
    QMLFramebuffer(QQuickItem *parent = 0);
    virtual void paint(QPainter *p) override;

    virtual QVariant inputMethodQuery(Qt::InputMethodQuery query) const override;
    virtual void inputMethodEvent(QInputMethodEvent *ev) override;
public slots:
    void resetKeypad();
private:
    QTimer timer;
};

QImage renderFramebuffer();
void paintFramebuffer(QPainter *p);

#endif // QMLFRAMEBUFFER_H
