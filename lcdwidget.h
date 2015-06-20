#ifndef LCDWIDGET_H
#define LCDWIDGET_H

#include <QGraphicsView>
#include <QKeyEvent>
#include <QTimer>

class LCDWidget : public QWidget
{
public:
    LCDWidget(QWidget *parent);

public slots:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;

protected:
    virtual void paintEvent(QPaintEvent *) override;

private:
    QTimer refresh_timer;
};

#endif // LCDWIDGET_H
