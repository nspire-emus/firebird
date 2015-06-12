#ifndef LCDWIDGET_H
#define LCDWIDGET_H

#include <QGraphicsView>
#include <QKeyEvent>

class LCDWidget : public QWidget
{
public:
    LCDWidget(QWidget *parent) : QWidget(parent) {}

public slots:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;

protected:
    virtual void paintEvent(QPaintEvent *) override;
};

#endif // LCDWIDGET_H
