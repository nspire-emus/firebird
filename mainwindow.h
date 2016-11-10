#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <atomic>

#include <QMainWindow>
#include <QSettings>
#include <QLabel>
#include <QTreeWidgetItem>

#include "emuthread.h"
#include "flashdialog.h"
#include "lcdwidget.h"
#include "qmlbridge.h"

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
    void showStatusMsg(QString str);
    void kitDataChanged(QModelIndex, QModelIndex, QVector<int> roles);
    void kitAnythingChanged();

    //Drag & Drop
    void dropEvent(QDropEvent* event) override;
    void dragEnterEvent(QDragEnterEvent *ev) override;

    //Menu "Emulator"
    void restart();
    void openConfiguration();

    //Menu "Tools"
    void screenshot();
    void recordGIF();
    void connectUSB();
    void usblinkChanged(bool state);
    void setExtLCD(bool state);
    void xmodemSend();

    //Menu "State"
    bool resume();
    void suspend();
    void resumeFromFile();
    void suspendToFile();

    //Menu "Flash"
    void saveFlash();
    void createFlash();

    //Menu "About"
    void showAbout();

    //State
    void isBusy(bool busy);
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
    void changeProgress(int value);
    void usblinkDownload(int progress);
    void usblinkProgress(int progress);

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
    static void usblink_progress_callback(int progress, void *data);

private:
    void suspendToPath(QString path);
    bool resumeFromPath(QString path);
    void setPathBoot1(QString path);
    void setPathFlash(QString path);

    void updateUIActionState(bool emulation_running);
    void raiseDebugger();

    void refillKitMenus();

    // QMLBridge is used as settings storage,
    // so the settings have to be read from there
    // and emu_thread configured appropriately.
    void applyQMLBridgeSettings();

    // Configure everything according to the Kit's
    // configuration.
    void setCurrentKit(const Kit& kit);

    Ui::MainWindow *ui = nullptr;

    // Used to show a status message permanently
    QLabel status_label;

    EmuThread emu;
    QSettings *settings = nullptr;
    QString snapshot_path;
    FlashDialog flash_dialog;
    // To make it possible to activate the debugger
    QDockWidget *dock_debugger = nullptr;

    // Second LCDWidget for use as external window
    LCDWidget lcd{this, Qt::Window};

    // The QML Config Dialog
    QObject *config_dialog = nullptr;

    // Used for autosuspend on close.
    // The close event has to be deferred until the suspend operation completed successfully.
    bool close_after_suspend = false;
};

// Used as global instance by EmuThread and friends
extern MainWindow *main_window;

#endif // MAINWINDOW_H
