#ifndef LCDWIDGET_H
#define LCDWIDGET_H

#include <QGraphicsView>
#include <QKeyEvent>

class LCDWidget : public QGraphicsView
{
public:
    LCDWidget();
    LCDWidget(QWidget *parent);

public slots:
    void keyPressEvent(QKeyEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
};

#endif // LCDWIDGET_H
