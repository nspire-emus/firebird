#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QTimer>
#include <QMainWindow>
#include <QGraphicsScene>

#include "emuthread.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

public slots:
    void refresh();
    void putchar(char c);
    void debugCommand();

signals:
    void debuggerCommand();

public:
    QByteArray debug_command;

private:
    Ui::MainWindow *ui;

    QTimer refresh_timer;
    QGraphicsScene lcd_scene;
    EmuThread emu;
};

extern MainWindow *main_window;

#endif // MAINWINDOW_H
