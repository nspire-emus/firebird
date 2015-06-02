#include "qmlbridge.h"

#include "keypad.h"

QMLBridge::QMLBridge(QObject *parent) : QObject(parent)
{

}

void QMLBridge::keypadStateChanged(int keymap_id, bool state)
{
    int col = keymap_id % 11, row = keymap_id / 11;

    if(state)
        key_map[row] |= 1 << col;
    else
        key_map[row] &= ~(1 << col);
}

QObject *qmlBridgeFactory(QQmlEngine *engine, QJSEngine *scriptEngine)
{
    Q_UNUSED(engine)
    Q_UNUSED(scriptEngine)

    return new QMLBridge();
}
