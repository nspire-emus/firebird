import QtQuick 2.0
import QtQuick.Layouts 1.0

Rectangle {    
    id: rectangle1
    width: 265
    height: 240

    color: "#444"

    ColumnLayout {
        id: columnLayout1
        anchors.bottom: rectangle3.bottom
        anchors.bottomMargin: 0
        z: 4
        anchors.left: parent.left
        anchors.leftMargin: 5
        anchors.top: rectangle3.top
        anchors.topMargin: 0

        NBigButton {
            id: nButton1
            anchors.horizontalCenter: parent.horizontalCenter
            text: "esc"
            keymap_id: 73
        }

        NBigButton {
            id: nButton2
            anchors.horizontalCenter: parent.horizontalCenter
            text: "pad"
            keymap_id: 65
        }

        NBigButton {
            id: nButton3
            anchors.horizontalCenter: parent.horizontalCenter
            text: "tab"
            keymap_id: 75
        }
    }

    ColumnLayout {
        id: columnLayout2
        x: -7
        anchors.bottom: rectangle3.bottom
        anchors.bottomMargin: 0
        z: 3
        anchors.top: rectangle3.top
        anchors.topMargin: 0
        anchors.right: parent.right
        anchors.rightMargin: 5
        NBigButton {
            id: nButton4
            anchors.horizontalCenter: parent.horizontalCenter
            text: "⌂on"
            keymap_id: 9
        }

        NBigButton {
            id: nButton5
            anchors.horizontalCenter: parent.horizontalCenter
            text: "doc"
            keymap_id: 69
        }

        NBigButton {
            id: nButton6
            width: 30
            anchors.horizontalCenter: parent.horizontalCenter
            text: "menu"
            keymap_id: 71
        }
    }

    // Disable flicking in this area, otherwise the touchpad would be unusuable
    MouseArea {
        width: 150
        anchors.bottom: rectangle3.bottom
        anchors.bottomMargin: 6
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.top: parent.top
        anchors.topMargin: 6
        z: 2

        propagateComposedEvents: true
        preventStealing: true

        onPressed: rectangle1.parent.parent.interactive = false;
        onReleased: rectangle1.parent.parent.interactive = true;

        Touchpad {
            id: touchpad1
            anchors.fill: parent
        }
    }

    Rectangle {
        id: rectangle3
        height: 90
        color: "#111111"
        anchors.top: parent.top
        anchors.topMargin: 0
        anchors.left: parent.left
        anchors.leftMargin: 0
        anchors.right: parent.right
        anchors.rightMargin: 0
        z: 1
    }
}

