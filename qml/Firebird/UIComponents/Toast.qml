import QtQuick 2.0

import Firebird.UIComponents 1.0

Rectangle {
    property int maxWidth: parent.width * 0.9
    height: message.contentHeight + 8
    width: message.contentWidth + 10

    color: "#dd222222"
    border.color: "#ccc"
    border.width: 0

    opacity: 0
    visible: opacity > 0

    Behavior on opacity { NumberAnimation { duration: 200 } }

    function showMessage(str) {
        message.text = str;
        opacity = 1;
        timer.restart();
    }

    FBLabel {
        id: message
        text: "Text"
        color: "#eee"
        width: parent.maxWidth

        anchors.centerIn: parent

        horizontalAlignment: Text.Center
        font.pixelSize: TextMetrics.title1Size
        wrapMode: Text.WordWrap

        Timer {
            id: timer
            interval: 2000
            onTriggered: parent.parent.opacity = 0;
        }
    }

    MouseArea {
        anchors.fill: parent
        onClicked: {
            timer.stop();
            parent.opacity = 0;
        }
    }
}
