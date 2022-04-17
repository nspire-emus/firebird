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

    Q_PROPERTY(unsigned int gdbPort READ getGDBPort WRITE setGDBPort NOTIFY gdbPortChanged)
    Q_PROPERTY(bool gdbEnabled READ getGDBEnabled WRITE setGDBEnabled NOTIFY gdbEnabledChanged)
    Q_PROPERTY(unsigned int rdbPort READ getRDBPort WRITE setRDBPort NOTIFY rdbPortChanged)
    Q_PROPERTY(bool rdbEnabled READ getRDBEnabled WRITE setRDBEnabled NOTIFY rdbEnabledChanged)
    Q_PROPERTY(bool debugOnStart READ getDebugOnStart WRITE setDebugOnStart NOTIFY debugOnStartChanged)
    Q_PROPERTY(bool debugOnWarn READ getDebugOnWarn WRITE setDebugOnWarn NOTIFY debugOnWarnChanged)
    Q_PROPERTY(bool printOnWarn READ getPrintOnWarn WRITE setPrintOnWarn NOTIFY printOnWarnChanged)
    Q_PROPERTY(bool autostart READ getAutostart WRITE setAutostart NOTIFY autostartChanged)
    Q_PROPERTY(unsigned int defaultKit READ getDefaultKit WRITE setDefaultKit NOTIFY defaultKitChanged)
    Q_PROPERTY(bool leftHanded READ getLeftHanded WRITE setLeftHanded NOTIFY leftHandedChanged)
    Q_PROPERTY(bool suspendOnClose READ getSuspendOnClose WRITE setSuspendOnClose NOTIFY suspendOnCloseChanged)
    Q_PROPERTY(QString usbdir READ getUSBDir WRITE setUSBDir NOTIFY usbDirChanged)
    Q_PROPERTY(QString version READ getVersion CONSTANT)
    Q_PROPERTY(bool isRunning READ getIsRunning NOTIFY isRunningChanged)
    Q_PROPERTY(KitModel* kits READ getKitModel CONSTANT)

    Q_PROPERTY(double speed READ getSpeed NOTIFY speedChanged)
    Q_PROPERTY(bool turboMode READ getTurboMode WRITE setTurboMode NOTIFY turboModeChanged)

    Q_PROPERTY(int mobileX READ getMobileX WRITE setMobileX NOTIFY neverEmitted)
    Q_PROPERTY(int mobileY READ getMobileY WRITE setMobileY NOTIFY neverEmitted)
    Q_PROPERTY(int mobileWidth READ getMobileWidth WRITE setMobileWidth NOTIFY neverEmitted)
    Q_PROPERTY(int mobileHeight READ getMobileHeight WRITE setMobileHeight NOTIFY neverEmitted)

    unsigned int getGDBPort();
    void setGDBPort(unsigned int port);
    void setGDBEnabled(bool e);
    bool getGDBEnabled();
    unsigned int getRDBPort();
    void setRDBPort(unsigned int port);
    void setRDBEnabled(bool e);
    bool getRDBEnabled();
    void setDebugOnWarn(bool e);
    bool getDebugOnWarn();
    void setDebugOnStart(bool e);
    bool getDebugOnStart();
    void setPrintOnWarn(bool p);
    bool getPrintOnWarn();
    void setAutostart(bool e);
    bool getAutostart();
    unsigned int getDefaultKit();
    void setDefaultKit(unsigned int id);
    bool getLeftHanded();
    void setLeftHanded(bool e);
    bool getSuspendOnClose();
    void setSuspendOnClose(bool e);
    QString getUSBDir();
    void setUSBDir(QString dir);
    bool getIsRunning();
    QString getVersion();

    double getSpeed();
    bool getTurboMode();
    void setTurboMode(bool e);

    int getMobileX();
    void setMobileX(int x);
    int getMobileY();
    void setMobileY(int y);
    int getMobileWidth();
    void setMobileWidth(int w);
    int getMobileHeight();
    void setMobileHeight(int h);

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
    Q_INVOKABLE int kitIndexForID(unsigned int id);

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
};

extern QMLBridge *the_qml_bridge;

#endif // QMLBRIDGE_H
