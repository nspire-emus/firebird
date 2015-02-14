#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QTimer>
#include <QMainWindow>
#include <QGraphicsScene>
#include <QSettings>

#include "emuthread.h"
#include "flashdialog.h"

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

    //Drag & Drop
    void dropEvent(QDropEvent* event) override;
    void dragEnterEvent(QDragEnterEvent *ev) override;

    //Menu "Emulator"
    void restart();
    void setThrottleTimerDeactivated(bool b);
    void screenshot();
    void connectUSB();
    void usblinkChanged(bool state);

    //Menu "Flash"
    void saveFlash();
    void createFlash();

    //Emu stuff (has to be a signal to execute it in this thread)
    void setThrottleTimer(bool b);

    //Serial
    void serialChar(const char c);

    //Debugging
    void debugStr(QString str);
    void debugCommand();

    //Settings
    void selectBoot1();
    void selectFlash();
    void setDebuggerOnStartup(bool b);
    void setDebuggerOnWarning(bool b);
    void setAutostart(bool b);
    void setUSBPath(QString path);
    void setGDBPort(int port);
    void setRDBGPort(int port);

    void showSpeed(double percent);

signals:
    void debuggerCommand();

public:
    QByteArray debug_command;

    void throttleTimerWait();

private:
    void selectBoot1(QString path);
    void selectFlash(QString path);

    Ui::MainWindow *ui;

    QTimer refresh_timer, throttle_timer;
    QGraphicsScene lcd_scene;
    EmuThread emu;
    QSettings *settings;
    FlashDialog flash_dialog;
};

extern MainWindow *main_window;

#endif // MAINWINDOW_H
