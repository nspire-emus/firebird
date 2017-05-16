import QtQuick 2.0
import QtQuick.Controls 1.0

ScrollView {
    id: controls
    property alias keypad: keypad
    horizontalScrollBarPolicy: Qt.ScrollBarAlwaysOff
    verticalScrollBarPolicy: Qt.ScrollBarAlwaysOff

    Flickable {
        flickableDirection: Flickable.VerticalFlick

        contentWidth: parent.width
        contentHeight: keypad.height*controls.width/keypad.width

        Keypad {
            id: keypad
            transform: Scale { origin.x: 0; origin.y: 0; xScale: controls.width/keypad.width; yScale: controls.width/keypad.width }
        }
    }
}
