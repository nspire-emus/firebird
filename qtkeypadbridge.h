#ifndef QTKEYPADBRIDGE_H
#define QTKEYPADBRIDGE_H

#include <QTimer>
#include <QKeyEvent>

/* This class is used by every Widget which wants to interact with the
 * virtual keypad. Simply call QtKeypadBridge::keyPressEvent or keyReleaseEvent
 * to relay the key events into the virtual calc. */

class QtKeypadBridge : public QObject
{
public:
    QtKeypadBridge(QObject *parent = nullptr);
    static void keyPressEvent(QKeyEvent *event);
    static void keyReleaseEvent(QKeyEvent *event);

    virtual bool eventFilter(QObject *obj, QEvent *event);

private slots:
    void arrowPressHelper();
    void arrowReleaseHelper();

private:
    int scrollDelta = 0;
    Qt::Key pressedKey = static_cast<Qt::Key>(0);
    QTimer pressTimer, releaseTimer;
};

extern QtKeypadBridge qt_keypad_bridge;

#endif // QTKEYPADBRIDGE_H
