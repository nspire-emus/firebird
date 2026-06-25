#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSettings>
#include <QLabel>
#include <QQuickWidget>

#include "emuthread.h"
#include "fbaboutdialog.h"
#include "lcdwidget.h"
#include "qmlbridge.h"

namespace Ui {
class MainWindow;
}

/* QQuickWidget does not care about QEvent::Leave,
 * which results in MouseArea::containsMouse to get stuck when
 * the mouse leaves the widget without triggering a move outside
 * the MouseArea. Work around it by translating QEvent::Leave
 * to a MouseMove to (0/0). */

class QQuickWidgetLessBroken : public QQuickWidget
{
    Q_OBJECT

public:
    explicit QQuickWidgetLessBroken(QWidget *parent) : QQuickWidget(parent) {}
    virtual ~QQuickWidgetLessBroken() {}

protected:
    bool event(QEvent *event) override;
};

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

public slots:
    //Miscellaneous
    void changeEvent(QEvent* event) override;
    void closeEvent(QCloseEvent *) override;
    void showStatusMsg(QString str);
    void kitDataChanged(QModelIndex, QModelIndex, QVector<int> roles);
    void kitAnythingChanged();
    void currentKitChanged(const Kit &kit);

    //Drag & Drop
    void dropEvent(QDropEvent* event) override;
    void dragEnterEvent(QDragEnterEvent *ev) override;

    //Menu "Emulator"
    void restart();
    void openConfiguration();
    void startKit();
    void startKitDiags();

    //Menu "Tools"
    void screenshot();
    void recordGIF();
    void connectUSB();
    void usblinkChanged(bool state);
    void setExtLCD(bool state);
    void xmodemSend();
    void switchToMobileUI();

    //Menu "State"
    bool resume();
    void suspend();
    void resumeFromFile();
    void suspendToFile();

    //Menu "Flash"
    void saveFlash();
    void createFlash();

    //Menu "Docks"
    void setUIEditMode(bool e);

    //Menu "View"
    void setDarkMode(bool enabled);

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

    //Tool bar (above screen)
    void showSpeed(double value);

signals:
    void debuggerCommand(QString input);
    void usblink_progress_changed(int progress);

public:
    static void usblink_progress_callback(int progress, void *data);
    void switchUIMode(bool mobile_ui);

private:
    void setActive(bool b);

    void suspendToPath(QString path);
    bool resumeFromPath(QString path);

    void convertTabsToDocks();
    void retranslateDocks();

    void updateUIActionState(bool emulation_running);
    void raiseDebugger();

    void refillKitMenus();
    void updateWindowTitle();

    // This changes the current GUI language to the one given in parameter, if available.
    // The change is persistent (saved in settings) if it was successful.
    void switchTranslator(const QLocale &locale);

    // QMLBridge is used as settings storage,
    // so the settings have to be read from there
    // and emu_thread configured appropriately.
    void applyQMLBridgeSettings();

    Ui::MainWindow *ui = nullptr;

    QTranslator appTranslator;

    // Used to show a status message permanently
    QLabel status_label;

    QSettings *settings = nullptr;

    // To make it possible to activate the debugger
    QDockWidget *dock_debugger = nullptr;

    // Second LCDWidget for use as external window
    LCDWidget lcd{this, Qt::Window};

    // The about dialog
    FBAboutDialog aboutDialog{this};

    // The QML engine shared by the keypad, config dialog and mobile UI
    QQmlEngine *qml_engine = nullptr;

    // The config dialog component, used to create the config_dialog
    QQmlComponent *config_component = nullptr;
    // The QML Config Dialog
    QObject *config_dialog = nullptr;

    // The flash dialog component, used to create the flash_dialog
    QQmlComponent *flash_dialog_component = nullptr;
    // The QML Config Dialog
    QObject *flash_dialog = nullptr;

    // The Mobile UI component, used to create the mobile_dialog
    QQmlComponent *mobileui_component = nullptr;
    // A Mobile UI instance
    QObject *mobileui_dialog = nullptr;

    // Used for autosuspend on close.
    // The close event has to be deferred until the suspend operation completed successfully.
    bool close_after_suspend = false;

    // Whether this MainWindow is in charge of communicating with EmuThread
    bool is_active = false;
};

// Used as global instance by EmuThread and friends
extern MainWindow *main_window;

#endif // MAINWINDOW_H
