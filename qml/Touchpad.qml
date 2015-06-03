import QtQuick 2.0
import Ndless.Emu 1.0

Rectangle {
    id: rectangle2
    width: 100
    height: 70
    color: "#222222"
    radius: 10
    border.width: 2
    border.color: "#eeeeee"

    Rectangle {
        id: rectangle1
        x: 29
        y: 18
        width: 35
        height: 35
        color: "#00000000"
        radius: 8
        border.color: "#ffffff"
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.verticalCenter: parent.verticalCenter
    }

    Rectangle {
        id: highlight
        x: 0
        y: 0
        width: 10
        height: 10
        color: "#b3edf200"
        radius: 10
        visible: false
    }

    MouseArea {
        id: mouseArea1
        anchors.rightMargin: 0
        anchors.bottomMargin: 0
        anchors.fill: parent
        acceptedButtons: Qt.LeftButton

        Component.onCompleted: {
            Emu.registerTouchpad(this);
        }

        function submitState() {
            var p = pressed;
            if(mouseX < 0 || mouseX > width
                    || mouseY < 0 || mouseY > height)
                p = false;

            Emu.touchpadStateChanged(mouseX/width, mouseY/height, p);
            highlight.x = mouseX - highlight.width/2;
            highlight.y = mouseY - highlight.height/2;
            highlight.visible = p;
        }

        function showHighlight(x, y) {
            highlight.x = x*width - highlight.width/2;
            highlight.y = y*height - highlight.height/2;
            highlight.visible = true;
        }

        function hideHighlight() {
            highlight.visible = false;
        }

        onMouseXChanged: {
            submitState();
        }

        onMouseYChanged: {
            submitState();
        }

        onReleased: {
            submitState();
        }

        onPressed: {
            submitState();
        }
    }
}

