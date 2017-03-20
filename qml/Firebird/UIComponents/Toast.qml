import QtQuick 2.0

Rectangle {
    implicitWidth: message.width+2*5
    implicitHeight: message.height+2*5

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

    Text {
        id: message
        text: "Text"
        anchors.centerIn: parent
        font.pointSize: 12

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
