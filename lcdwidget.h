#ifndef LCDWIDGET_H
#define LCDWIDGET_H

#include <QGraphicsView>
#include <QKeyEvent>
#include <QTimer>

class LCDWidget : public QWidget
{
    Q_OBJECT

public:
    LCDWidget(QWidget *parent, Qt::WindowFlags f = Qt::WindowFlags());

public slots:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void showEvent(QShowEvent *e) override;
    void hideEvent(QHideEvent *e) override;
    void closeEvent(QCloseEvent *e) override;

protected:
    virtual void paintEvent(QPaintEvent *) override;

signals:
    void closed();

private:
    QTimer refresh_timer;
};

#endif // LCDWIDGET_H
