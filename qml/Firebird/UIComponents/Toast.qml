import QtQuick 2.0

import Firebird.UIComponents 1.0

Rectangle {
    property int maxWidth: parent.width * 0.9
    height: message.contentHeight + 2*radius
    width: message.contentWidth + 2*radius

    radius: 5
    color: "#d3c7c7"
    border.color: "#e66e6e6e"
    border.width: 3

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
        width: parent.maxWidth

        anchors.centerIn: parent

        horizontalAlignment: Text.Center
        font.pointSize: 12
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
