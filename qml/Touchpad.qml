import QtQuick 2.0
import Firebird.Emu 1.0

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

    /* Click and hold at same place -> Down
       Click and quick release -> Down
       Click and move -> Contact */
    MouseArea {
        anchors.rightMargin: 0
        anchors.bottomMargin: 0
        anchors.fill: parent
        acceptedButtons: Qt.LeftButton
        preventStealing: true

        property int origX
        property int origY
        property int moveThreshold: 5
        property bool isDown

        Component.onCompleted: {
            Emu.registerTouchpad(this);
        }

        Timer {
            id: clickOnHoldTimer
            interval: 200
            running: false
            repeat: false
            onTriggered: {
                parent.isDown = true;
                parent.submitState();
            }
        }

        Timer {
            id: clickOnReleaseTimer
            interval: 100
            running: false
            repeat: false
            onTriggered: {
                parent.isDown = false;
                parent.submitState();
            }
        }

        function submitState() {
            Emu.touchpadStateChanged(mouseX/width, mouseY/height, isDown | pressed, isDown);
        }

        function showHighlight(x, y, down) {
            highlight.x = x*width - highlight.width/2;
            highlight.y = y*height - highlight.height/2;
            highlight.color = down ? "#b3edf200" : "#b38080ff";
            highlight.visible = true;
        }

        function hideHighlight() {
            highlight.visible = false;
        }

        onMouseXChanged: {
            if(Math.abs(mouseX - origX) > moveThreshold)
                clickOnHoldTimer.stop();

            submitState();
        }

        onMouseYChanged: {
            if(Math.abs(mouseY - origY) > moveThreshold)
                clickOnHoldTimer.stop();

            submitState();
        }

        onReleased: {
            if(clickOnHoldTimer.running)
            {
                clickOnHoldTimer.stop();
                isDown = true;
                clickOnReleaseTimer.restart();
            }
            else
                isDown = false;

            submitState();
        }

        onPressed: {
            origX = mouse.x;
            origY = mouse.y;
            isDown = false;
            clickOnHoldTimer.restart();
        }
    }
}

