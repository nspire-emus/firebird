import QtQuick 2.0
import QtQuick.Controls 1.3
import QtQuick.Layouts 1.0
import Firebird.UIComponents 1.0

ToolButton {
    property alias icon: image.source
    property alias title: label.text

    anchors.horizontalCenter: parent.horizontalCenter
    Layout.fillWidth: true
    Layout.fillHeight: true

    Rectangle {
        anchors.fill: parent
        color: parent.pressed ? "#CCC" : "#AAA"
    }

    FBLabel {
        id: label
        x: 12
        anchors.top: parent.top
        anchors.topMargin: 0
        anchors.horizontalCenter: parent.horizontalCenter
        font.pointSize: 8
    }

    Image {
        id: image
        anchors.top: label.bottom
        anchors.topMargin: -2
        anchors.left: parent.left
        anchors.leftMargin: 0
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 0
        anchors.right: parent.right
        anchors.rightMargin: 0
        fillMode: Image.PreserveAspectFit
        anchors.margins: 3
    }
}
