#ifndef QMLBRIDGE_H
#define QMLBRIDGE_H

#include <QObject>
#include <QtQml>

#include "emuthread.h"
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
    Q_PROPERTY(bool autostart READ getAutostart WRITE setAutostart NOTIFY autostartChanged)
    Q_PROPERTY(unsigned int defaultKit READ getDefaultKit WRITE setDefaultKit NOTIFY defaultKitChanged)
    Q_PROPERTY(bool leftHanded READ getLeftHanded WRITE setLeftHanded NOTIFY leftHandedChanged)
    Q_PROPERTY(bool suspendOnClose READ getSuspendOnClose WRITE setSuspendOnClose NOTIFY suspendOnCloseChanged)
    Q_PROPERTY(QString usbdir READ getUSBDir WRITE setUSBDir NOTIFY usbDirChanged)
    Q_PROPERTY(QString version READ getVersion)
    Q_PROPERTY(bool isRunning READ getIsRunning NOTIFY isRunningChanged)
    Q_PROPERTY(KitModel* kits READ getKitModel)

    Q_PROPERTY(double speed READ getSpeed NOTIFY speedChanged)
    Q_PROPERTY(bool turboMode READ getTurboMode WRITE setTurboMode NOTIFY turboModeChanged)

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

    KitModel *getKitModel() { return &kit_model; }
    Q_INVOKABLE void keypadStateChanged(int keymap_id, bool state);
    Q_INVOKABLE void registerNButton(unsigned int keymap_id, QVariant button);

    // Coordinates: (0/0) = top left (1/1) = bottom right
    Q_INVOKABLE void touchpadStateChanged(qreal x, qreal y, bool contact, bool down);
    Q_INVOKABLE void registerTouchpad(QVariant touchpad);

    Q_INVOKABLE bool isMobile();

    Q_INVOKABLE void sendFile(QUrl url, QString dir);

    // Various utility functions
    Q_INVOKABLE QString basename(QString path);
    Q_INVOKABLE QUrl dir(QString path);
    Q_INVOKABLE QString toLocalFile(QUrl url);
    Q_INVOKABLE bool fileExists(QString path);
    Q_INVOKABLE int kitIndexForID(unsigned int id);

    Q_INVOKABLE void useDefaultKit();

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

    Q_INVOKABLE void registerToast(QVariant toast);
    Q_INVOKABLE void toastMessage(QString msg);

    Q_INVOKABLE void createFlash(unsigned int kitIndex);

    #ifndef MOBILE_UI
        Q_INVOKABLE void switchUIMode(bool mobile_ui);
    #endif

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
    void autostartChanged();
    void defaultKitChanged();
    void leftHandedChanged();
    void suspendOnCloseChanged();
    void usbDirChanged();
    void isRunningChanged();
    void speedChanged();
    void turboModeChanged();

    void usblinkProgressChanged(int percent);

private:
    static void usblink_progress_changed(int percent, void *qml_bridge_p);

    QObject *toast = nullptr;

    unsigned int current_kit_id;
    QString fallback_snapshot_path;

    double speed;
    KitModel kit_model;
    QSettings settings;
};

extern QMLBridge *the_qml_bridge;

void notifyKeypadStateChanged(int row, int col, bool state);
void notifyTouchpadStateChanged();
void notifyTouchpadStateChanged(qreal x, qreal y, bool contact, bool down);
QObject *qmlBridgeFactory(QQmlEngine *engine, QJSEngine *scriptEngine);


#endif // QMLBRIDGE_H
