#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QTimer>
#include <QMainWindow>
#include <QGraphicsScene>
#include <QSettings>

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
    void closeEvent(QCloseEvent *) override;

    //Timer for LCD refreshing
    void refresh();

    //Menu
    void restart();

    //Serial
    void serialChar(const char c);

    //Debugging
    void debugStr(QString str);
    void debugCommand();

    //Settings
    void selectBoot1();
    void selectFlash();

signals:
    void debuggerCommand();

public:
    QByteArray debug_command;

private:
    void selectBoot1(QString path);
    void selectFlash(QString path);

    Ui::MainWindow *ui;

    QTimer refresh_timer;
    QGraphicsScene lcd_scene;
    EmuThread emu;
    QSettings settings;
};

extern MainWindow *main_window;

#endif // MAINWINDOW_H
