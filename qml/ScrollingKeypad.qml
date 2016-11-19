import QtQuick 2.0
import QtQuick.Controls 1.0

ScrollView {
    property alias keypad: keypad
    horizontalScrollBarPolicy: Qt.ScrollBarAlwaysOff
    verticalScrollBarPolicy: Qt.ScrollBarAlwaysOn

    Keypad {
        id: keypad
    }
}
