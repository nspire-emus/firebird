import QtQuick 2.0
import Firebird.Emu 1.0

Rectangle {
    property string active_color: "#555"
    property string back_color: "#223"
    property string font_color: "#fff"
    property alias text: label.text
    property bool active: pressed || mouseArea.containsMouse
    property bool pressed: false
    // Pressing the right mouse button "locks" the button in enabled state
    property bool fixed: false
    property int keymap_id: 1

    signal clicked()

    border.width: active ? 2 : 1
    border.color: "#888"
    radius: 4
    color: active ? active_color : back_color

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

    Text {
        id: label
        text: "Foo"
        anchors.fill: parent
        anchors.centerIn: parent
        font.pixelSize: height*0.55
        color: font_color
        font.bold: true
        renderType: Text.QtRendering
        // Workaround: Text.AutoText doesn't seem to work for properties (?)
        textFormat: text.indexOf(">") == -1 ? Text.PlainText : Text.RichText
        verticalAlignment: Text.AlignVCenter
        horizontalAlignment: Text.AlignHCenter
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

        onPressed: function(mouse) {
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

        onReleased: function(mouse) {
            mouse.accepted = true;

            if(mouse.button == Qt.LeftButton
                    && !fixed)
                parent.pressed = false;
        }
    }
}
