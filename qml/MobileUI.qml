import QtQuick 2.0
import QtQuick.Layouts 1.0
import Ndless.Emu 1.0
import QtQuick.Controls 1.3

Rectangle {
    id: rectangle1
    width: 320
    height: 480
    color: "#111111"

    ColumnLayout {
        id: rowLayout1
        width: toolButton1.width
        anchors.bottom: controls.top
        anchors.bottomMargin: 0
        anchors.top: parent.top
        anchors.topMargin: 0
        anchors.left: screen.right
        anchors.leftMargin: 0

        ToolButton {
            id: toolButton1
            iconSource: "qrc:/icons/resources/icons/system-reboot.png"
        }
    }

    Screen {
        id: screen
        y: 0
        width: parent.width - rowLayout1.width
        height: width/320*240
        anchors.left: parent.left
        anchors.leftMargin: 0

        Timer {
            interval: 20
            running: true; repeat: true
            onTriggered: screen.update()
        }
    }

    Flickable {
        id: controls
        boundsBehavior: Flickable.StopAtBounds
        flickableDirection: Flickable.HorizontalFlick
        contentWidth: width*2
        contentHeight: height
        anchors.top: screen.bottom
        anchors.topMargin: 0
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 0
        anchors.right: parent.right
        anchors.rightMargin: 0
        anchors.left: parent.left
        anchors.leftMargin: 0

        MobileControl1 {
            id: control1
            width: controls.width
            height: controls.height
        }

        MobileControl2 {
            id: control2
            width: controls.width
            height: controls.height
            anchors.left: control1.right
        }
    }

}

