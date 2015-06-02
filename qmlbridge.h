#ifndef QMLBRIDGE_H
#define QMLBRIDGE_H

#include <QObject>
#include <QtQml>

class QMLBridge : public QObject
{
    Q_OBJECT
public:
    explicit QMLBridge(QObject *parent = 0);
    ~QMLBridge() {}

    Q_INVOKABLE void keypadStateChanged(int keymap_id, bool state);

signals:

public slots:
};

QObject *qmlBridgeFactory(QQmlEngine *engine, QJSEngine *scriptEngine);

#endif // QMLBRIDGE_H
