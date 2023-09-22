import QtQuick 2.0
import Firebird.Emu 1.0

Rectangle {
    property string active_color: "#555"
    property string back_color: "#223"
    property string font_color: "#fff"
    property alias text: label.text
    property bool pressed: false
    // Pressing the right mouse button "locks" the button in enabled state
    property bool fixed: false
    property int keymap_id: 1

    signal clicked()

    radius: 4
    color: "#888"

    Rectangle {
        width: parent.width
        height: parent.height
        x: pressed ? -0 : -1
        y: pressed ? -0 : -1
        radius: parent.radius
        border.width: 1
        border.color: parent.color
        color: mouseArea.containsMouse ? active_color : back_color

        Text {
            id: label
            text: "Foo"
            anchors.fill: parent
            anchors.centerIn: parent
            font.pixelSize: height*0.55
            color: font_color
            font.bold: true
            // Workaround: Text.AutoText doesn't seem to work for properties (?)
            textFormat: text.indexOf(">") == -1 ? Text.PlainText : Text.RichText
            verticalAlignment: Text.AlignVCenter
            horizontalAlignment: Text.AlignHCenter
        }
    }

    onPressedChanged: {
        if(pressed)
            clicked();

        if(!pressed)
            fixed = false;

        Emu.setButtonState(keymap_id, pressed);
    }

    Connections {
        target: Emu
        function onButtonStateChanged(id, state) {
            if(id === keymap_id)
                pressed = state;
        }
    }

    // This is needed to support pressing multiple buttons at once on multitouch
    MultiPointTouchArea {
        id: multiMouseArea

        mouseEnabled: Qt.platform.os === "android" || Qt.platform.os === "ios"
        maximumTouchPoints: 1
        minimumTouchPoints: 1

        anchors.centerIn: parent
        width: parent.width * 1.3
        height: parent.height * 1.3

        onTouchUpdated: {
            var newState = false;
            for(var i in touchPoints)
            {
                var tp = touchPoints[i];
                if(tp.pressed
                   && tp.x >= 0 && tp.x < width
                   && tp.y >= 0 && tp.y < height)
                {
                    newState = true;
                    break;
                }
            }

            parent.pressed = newState;
        }
    }

    MouseArea {
        id: mouseArea

        enabled: !multiMouseArea.mouseEnabled

        preventStealing: true

        anchors.centerIn: parent
        width: parent.width * 1.3
        height: parent.height * 1.3
        acceptedButtons: Qt.LeftButton | Qt.RightButton

        hoverEnabled: !Emu.isMobile()

        onPressed: {
            mouse.accepted = true;

            if(mouse.button == Qt.LeftButton)
            {
                if(!fixed)
                    parent.pressed = true;
            }
            else if(fixed === parent.pressed) // Right button
            {
                fixed = !fixed;
                parent.pressed = !parent.pressed;
            }
        }

        onReleased: {
            mouse.accepted = true;

            if(mouse.button == Qt.LeftButton
                    && !fixed)
                parent.pressed = false;
        }
    }
}
