#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSettings>
#include <QLabel>

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
    //Miscellaneous
    void closeEvent(QCloseEvent *) override;
    void dockVisibilityChanged(bool v);
    void showStatusMsg(QString str);

    //Drag & Drop
    void dropEvent(QDropEvent* event) override;
    void dragEnterEvent(QDragEnterEvent *ev) override;

    //Menu "Emulator"
    void restart();

    //Menu "Tools"
    void screenshot();
    void recordGIF();
    void connectUSB();
    void usblinkChanged(bool state);
    void xmodemSend();

    //Menu "State"
    void resume();
    void suspend();
    void resumeFromFile();
    void suspendToFile();

    //Menu "Flash"
    void saveFlash();
    void createFlash();

    //State
    void started(bool success);
    void resumed(bool success);
    void suspended(bool success);
    void stopped();

    //Serial
    void serialChar(const char c);

    //Debugging
    void debugInputRequested(bool b);
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
    void setSuspendOnClose(bool b);
    void setResumeOnOpen(bool b);
    void changeSnapshotPath();

    //Tool bar (above screen)
    void showSpeed(double value);

    //FlashDialog
    void flashCreated(QString path);

signals:
    void debuggerCommand(QString input);
    void usblink_progress_changed(int progress);

public:
    //usblink callbacks
    static void usblink_dirlist_callback_nested(struct usblink_file *file, void *data);
    static void usblink_dirlist_callback(struct usblink_file *file, void *data);
    static void usblink_progress_callback(int progress, void *data);

private:
    void suspendToPath(QString path);
    void resumeFromPath(QString path);
    void selectBoot1(QString path);
    void selectFlash(QString path);

    void updateUIActionState(bool emulation_running);
    void raiseDebugger();

    Ui::MainWindow *ui = nullptr;

    // On Mac you can't show docks after all of them are hidden...
    // Keep track of them to count whether that's about to happen
    std::vector<QDockWidget *> docks;

    // Used to show a status message permanently
    QLabel status_label;

    // Whether to call usblink_dirlist when the tab is selected
    // Small hack: static as used in static callbacks...
    static bool refresh_filebrowser;
    EmuThread emu;
    QSettings *settings = nullptr;
    FlashDialog flash_dialog;
    // To make it possible to activate the debugger
    QDockWidget *dock_debugger = nullptr;

    // Used for autosuspend on close.
    // The close event has to be deferred until the suspend operation completed successfully.
    bool close_after_suspend = false;
};

// Used as global instance by EmuThread and friends
extern MainWindow *main_window;

#endif // MAINWINDOW_H
