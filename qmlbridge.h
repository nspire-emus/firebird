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
    ~QMLBridge() {}

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
        Q_INVOKABLE bool stop();

        Q_INVOKABLE QString getBoot1Path();
        Q_INVOKABLE void setBoot1Path(QUrl path);
        Q_INVOKABLE QString getFlashPath();
        Q_INVOKABLE void setFlashPath(QUrl path);

        Q_INVOKABLE QString basename(QString path);

        EmuThread emu_thread;
    #endif

private:
    QSettings settings;
};

void notifyKeypadStateChanged(int row, int col, bool state);
void notifyTouchpadStateChanged();
void notifyTouchpadStateChanged(qreal x, qreal y, bool state);
QObject *qmlBridgeFactory(QQmlEngine *engine, QJSEngine *scriptEngine);


#endif // QMLBRIDGE_H
