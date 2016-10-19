#ifndef QMLBRIDGE_H
#define QMLBRIDGE_H

#include <QObject>
#include <QtQml>

#include "emuthread.h"

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

    unsigned int getGDBPort();
    void setGDBPort(unsigned int port);
    void setGDBEnabled(bool e);
    bool getGDBEnabled();
    unsigned int getRDBPort();
    void setRDBPort(unsigned int port);
    void setRDBEnabled(bool e);
    bool getRDBEnabled();

    Q_INVOKABLE void keypadStateChanged(int keymap_id, bool state);
    Q_INVOKABLE void registerNButton(int keymap_id, QVariant button);

    // Coordinates: (0/0) = top left (1/1) = bottom right
    Q_INVOKABLE void touchpadStateChanged(qreal x, qreal y, bool state);
    Q_INVOKABLE void registerTouchpad(QVariant touchpad);

    Q_INVOKABLE bool isMobile();

    #ifdef MOBILE_UI
        Q_INVOKABLE bool restart();
        Q_INVOKABLE void setPaused(bool b);
        Q_INVOKABLE void reset();
        Q_INVOKABLE void suspend();
        Q_INVOKABLE void resume();
        Q_INVOKABLE bool stop();

        Q_INVOKABLE bool saveFlash();

        Q_INVOKABLE QString getBoot1Path();
        Q_INVOKABLE void setBoot1Path(QUrl path);
        Q_INVOKABLE QString getFlashPath();
        Q_INVOKABLE void setFlashPath(QUrl path);
        Q_INVOKABLE QString getSnapshotPath();
        Q_INVOKABLE void setSnapshotPath(QUrl path);

        Q_INVOKABLE QString basename(QString path);

        Q_INVOKABLE void registerToast(QVariant toast);
        Q_INVOKABLE void toastMessage(QString msg);

        EmuThread emu_thread;

    public slots:
        void started(bool success); // Not called on resume
        void resumed(bool success);
        void suspended(bool success);

    #endif

signals:
    void gdbPortChanged();
    void gdbEnabledChanged();
    void rdbPortChanged();
    void rdbEnabledChanged();

private:
    QObject *toast = nullptr;
    QSettings settings;
};

void notifyKeypadStateChanged(int row, int col, bool state);
void notifyTouchpadStateChanged();
void notifyTouchpadStateChanged(qreal x, qreal y, bool state);
QObject *qmlBridgeFactory(QQmlEngine *engine, QJSEngine *scriptEngine);


#endif // QMLBRIDGE_H
