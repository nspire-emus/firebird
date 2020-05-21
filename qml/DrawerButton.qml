import QtQuick 2.0
import QtQuick.Controls 1.3
import QtQuick.Layouts 1.0
import Firebird.UIComponents 1.0

Rectangle {
    property alias icon: image.source
    property alias title: label.text
    property alias borderTopVisible: borderTop.visible
    property alias borderBottomVisible: borderBottom.visible
    property bool disabled: false
    property alias font: label.font
    property bool toggle: false
    property bool toggleState: false
    property int spacing: 10

    opacity: disabled ? 0.5 : 1.0

    implicitHeight: label.contentHeight * 2
    implicitWidth: spacing + image.width + spacing + label.contentWidth + spacing

    Layout.fillWidth: true

    signal clicked()

    MouseArea {
        id: mouseArea
        anchors.fill: parent
        onClicked: {
            if(disabled)
                return;

            parent.clicked()

            if(toggle)
                toggleState = !toggleState;
        }
    }

    Rectangle {
        id: borderTop
        anchors {
            left: parent.left
            right: parent.right
            top: parent.top
        }

        height: 1
        color: "black"
    }

    Rectangle {
        id: borderBottom
        anchors {
            left: parent.left
            right: parent.right
            bottom: parent.bottom
        }

        height: 1
        color: "black"
    }

    color: (mouseArea.pressed !== toggleState) && !disabled ? "#CCC" : "#00000000"
    Behavior on color { ColorAnimation { duration: 200; } }

    Image {
        id: image

        x: spacing

        height: parent.height * 0.8

        anchors.verticalCenter: parent.verticalCenter

        fillMode: Image.PreserveAspectFit
    }

    FBLabel {
        id: label

        color: "black"

        x: image.x + image.width + spacing

        anchors {
            top: parent.top
            bottom: parent.bottom
        }

        font.pixelSize: TextMetrics.title2Size
        verticalAlignment: Text.AlignVCenter
    }
}
