#ifndef QMLBRIDGE_H
#define QMLBRIDGE_H

#include <QObject>
#include <QtQml>

#include "kitmodel.h"

class QMLBridge : public QObject
{
    Q_OBJECT
public:
    explicit QMLBridge(QObject *parent = 0);
    ~QMLBridge();

    // Settings
    Q_PROPERTY(unsigned int gdbPort MEMBER gdb_port NOTIFY gdbPortChanged)
    Q_PROPERTY(bool gdbEnabled MEMBER gdb_enabled NOTIFY gdbEnabledChanged)
    Q_PROPERTY(unsigned int rdbPort MEMBER rdb_port NOTIFY rdbPortChanged)
    Q_PROPERTY(bool rdbEnabled MEMBER rdb_enabled NOTIFY rdbEnabledChanged)
    Q_PROPERTY(bool debugOnStart MEMBER debug_on_start NOTIFY debugOnStartChanged)
    Q_PROPERTY(bool debugOnWarn MEMBER debug_on_warn NOTIFY debugOnWarnChanged)
    Q_PROPERTY(bool printOnWarn MEMBER print_on_warn NOTIFY printOnWarnChanged)
    Q_PROPERTY(bool autostart MEMBER autostart NOTIFY autostartChanged)
    Q_PROPERTY(unsigned int defaultKit MEMBER default_kit_id NOTIFY defaultKitChanged)
    Q_PROPERTY(bool leftHanded MEMBER left_handed NOTIFY leftHandedChanged)
    Q_PROPERTY(bool suspendOnClose MEMBER suspend_on_close NOTIFY suspendOnCloseChanged)
    Q_PROPERTY(QString usbdir MEMBER usb_dir NOTIFY usbDirChanged)
    Q_PROPERTY(int mobileX MEMBER mobile_x NOTIFY mobileXChanged)
    Q_PROPERTY(int mobileY MEMBER mobile_y NOTIFY mobileYChanged)
    Q_PROPERTY(int mobileWidth MEMBER mobile_w NOTIFY mobileWChanged)
    Q_PROPERTY(int mobileHeight MEMBER mobile_h NOTIFY mobileHChanged)

    Q_PROPERTY(bool isRunning READ getIsRunning NOTIFY isRunningChanged)
    Q_PROPERTY(QString version READ getVersion CONSTANT)
    Q_PROPERTY(KitModel* kits READ getKitModel CONSTANT)
    Q_PROPERTY(double speed READ getSpeed NOTIFY speedChanged)
    Q_PROPERTY(bool turboMode READ getTurboMode WRITE setTurboMode NOTIFY turboModeChanged)

    unsigned int getGDBPort() { return gdb_port; }
    bool getGDBEnabled() { return gdb_enabled; }
    unsigned int getRDBPort() { return rdb_port; }
    bool getRDBEnabled() { return rdb_enabled; }
    bool getAutostart() { return autostart; }
    QString getUSBDir() { return usb_dir; }

    bool getIsRunning();
    QString getVersion();

    double getSpeed();
    bool getTurboMode();
    void setTurboMode(bool e);

    KitModel *getKitModel() { return &kit_model; }
    Q_INVOKABLE void setButtonState(int id, bool state);

    // Coordinates: (0/0) = top left (1/1) = bottom right
    Q_INVOKABLE void setTouchpadState(qreal x, qreal y, bool contact, bool down);

    Q_INVOKABLE bool isMobile();

    Q_INVOKABLE void sendFile(QUrl url, QString dir);

    // Various utility functions
    Q_INVOKABLE QString basename(QString path);
    Q_INVOKABLE QUrl dir(QString path);
    Q_INVOKABLE QString toLocalFile(QUrl url);
    Q_INVOKABLE bool fileExists(QString path);

    Q_INVOKABLE bool setCurrentKit(unsigned int id);
    Q_INVOKABLE int getCurrentKitId();
    Q_INVOKABLE const Kit *useKit(unsigned int id);
    Q_INVOKABLE bool useDefaultKit();

    Q_INVOKABLE bool restart();
    Q_INVOKABLE void setPaused(bool b);
    Q_INVOKABLE void reset();
    Q_INVOKABLE void suspend();
    Q_INVOKABLE void resume();
    Q_INVOKABLE bool stop();

    Q_INVOKABLE bool saveFlash();

    Q_INVOKABLE QString getBoot1Path();
    Q_INVOKABLE QString getFlashPath();
    Q_INVOKABLE QString getSnapshotPath();

    #ifndef MOBILE_UI
        Q_INVOKABLE void switchUIMode(bool mobile_ui);
    #endif

    Q_INVOKABLE bool createFlash(QString path, int productID, int featureValues, QString manuf, QString boot2, QString os, QString diags);
    Q_INVOKABLE QString componentDescription(QString path, QString expected_type);
    Q_INVOKABLE QString manufDescription(QString path);
    Q_INVOKABLE QString osDescription(QString path);

    Q_INVOKABLE bool saveDialogSupported();

    void setActive(bool b);

    void notifyButtonStateChanged(int row, int col, bool state);
    void touchpadStateChanged();

public slots:
    void saveKits();
    void speedChanged(double speed);
    void started(bool success); // Not called on resume
    void resumed(bool success);
    void suspended(bool success);

signals:
    // Settings
    void gdbPortChanged();
    void gdbEnabledChanged();
    void rdbPortChanged();
    void rdbEnabledChanged();
    void debugOnWarnChanged();
    void debugOnStartChanged();
    void printOnWarnChanged();
    void autostartChanged();
    void defaultKitChanged();
    void leftHandedChanged();
    void suspendOnCloseChanged();
    void usbDirChanged();
    void mobileXChanged();
    void mobileYChanged();
    void mobileWChanged();
    void mobileHChanged();

    void isRunningChanged();
    void speedChanged();
    void turboModeChanged();

    void currentKitChanged(const Kit &kit);

    void emuSuspended(bool success);
    void usblinkProgressChanged(int percent);

    void toastMessage(QString msg);

    void touchpadStateChanged(qreal x, qreal y, bool contact, bool down);
    void buttonStateChanged(int id, bool state);

    /* Never called. Used as NOTIFY value for writable properties
     * that aren't used outside of QML. */
    void neverEmitted();

private:
    static void usblink_progress_changed(int percent, void *qml_bridge_p);

    int current_kit_id = -1;
    QString fallback_snapshot_path;

    double speed = 0;
    KitModel kit_model;
    QSettings settings;
    bool is_active = false;

    // Settings
    int mobile_x, mobile_y, mobile_w, mobile_h;
    unsigned int gdb_port, rdb_port, default_kit_id;
    bool gdb_enabled, rdb_enabled, debug_on_warn, debug_on_start,
         print_on_warn, autostart, suspend_on_close, left_handed;
    QString usb_dir;};

extern QMLBridge *the_qml_bridge;

#endif // QMLBRIDGE_H
