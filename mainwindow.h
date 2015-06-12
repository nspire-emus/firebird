#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QTimer>
#include <QMainWindow>
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

    //File transfer
    void reload_filebrowser();
    void changeProgress(int value);

    //Settings
    void selectBoot1();
    void selectFlash();
    void setDebuggerOnStartup(bool b);
    void setDebuggerOnWarning(bool b);
    void setUIMode(bool docks_enabled);
    void setAutostart(bool b);
    void setBootOrder(bool diags_first);
    void setUSBPath(QString path);
    void setGDBPort(int port);
    void setRDBGPort(int port);

    void showSpeed(double percent);

signals:
    void debuggerCommand();
    void usblink_progress_changed(int progress);

public:
    QByteArray debug_command;

    void throttleTimerWait();

    //usblink callbacks
    static void usblink_dirlist_callback_nested(struct usblink_file *file, void *data);
    static void usblink_dirlist_callback(struct usblink_file *file, void *data);
    static void usblink_progress_callback(int progress, void *data);

private:
    void selectBoot1(QString path);
    void selectFlash(QString path);

    Ui::MainWindow *ui;

    // Whether to call usblink_dirlist when the tab is selected
    // Small hack: static as used in static callbacks...
    static bool refresh_filebrowser;
    QTimer refresh_timer, throttle_timer;
    EmuThread emu;
    QSettings *settings;
    FlashDialog flash_dialog;
    // To make it possible to activate the debugger
    QDockWidget *dock_debugger;
};

// Used as global instance by EmuThread and friends
extern MainWindow *main_window;

#endif // MAINWINDOW_H
