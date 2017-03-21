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

    Layout.fillWidth: true
    Layout.minimumHeight: label.contentHeight * 2

    signal clicked()

    MouseArea {
        id: mouseArea
        anchors.fill: parent
        onClicked: {
            if(!disabled)
                parent.clicked()
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
        visible: true
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
        visible: true
    }

    color: {
        if(disabled)
            return "#EEEEEEEE";

        if(mouseArea.pressed)
            return "#CCC";

        return "#00000000";
    }

    Image {
        id: image

        x: 10

        height: parent.height * 0.8

        anchors.verticalCenter: parent.verticalCenter

        fillMode: Image.PreserveAspectFit
    }

    FBLabel {
        id: label

        color: disabled ? "grey" : "disabled"

        x: image.x + image.width + 10

        anchors {
            top: parent.top
            bottom: parent.bottom
        }

        font.pixelSize: TextMetrics.title2Size
        verticalAlignment: Text.AlignVCenter
    }
}
