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
};

#endif // LCDWIDGET_H
