// qmllint disable import
import QtQuick 6.0
import Firebird.Emu 1.0
import Firebird.UIComponents 1.0 as FBUI

Rectangle {
    id: root
    SystemPalette {
        id: paletteActive
    }

    readonly property color accent: FBUI.Theme.accent
    property color active_color: accent
    property color back_color: FBUI.Theme.surface
    property color font_color: FBUI.Theme.text
    property color active_font_color: FBUI.Theme.textOnAccent
    property color border_color: FBUI.Theme.border
    property alias text: label.text
    property string svgPath: ""
    property bool active: pressed || mouseArea.containsMouse
    property bool pressed: false
    // Pressing the right mouse button "locks" the button in enabled state
    property bool fixed: false
    property int keymap_id: 1
    // Guard to prevent feedback loop: when C++ sets pressed via
    // onButtonStateChanged, we must NOT call Emu.setButtonState() again.
    property bool _fromCpp: false

    signal clicked()

    function forceRelease() {
        if (!pressed && !fixed)
            return;
        fixed = false;
        pressed = false;
    }

    border.width: active ? 2 : 1
    border.color: border_color
    radius: 3
    color: active ? active_color : back_color

    onPressedChanged: {
        if(pressed)
            clicked();

        if(!pressed)
            fixed = false;

        if(!_fromCpp)
            Emu.setButtonState(root.keymap_id, root.pressed);
    }

    onVisibleChanged: {
        if (!visible)
            forceRelease();
    }

    onEnabledChanged: {
        if (!enabled)
            forceRelease();
    }

    Component.onDestruction: {
        if (pressed && !_fromCpp)
            Emu.setButtonState(root.keymap_id, false);
    }

    Connections {
        target: Emu
        function onButtonStateChanged(id, state) {
            if(id === root.keymap_id) {
                root._fromCpp = true;
                root.pressed = state;
                root._fromCpp = false;
            }
        }
    }

    Text {
        id: label
        visible: root.svgPath === ""
        text: "Foo"
        anchors.fill: parent
        anchors.centerIn: parent
        font.pixelSize: height*0.55
        color: root.active ? root.active_font_color : root.font_color
        font.bold: true
        // Workaround: Text.AutoText doesn't seem to work for properties (?)
        textFormat: text.indexOf(">") == -1 ? Text.PlainText : Text.RichText
        verticalAlignment: Text.AlignVCenter
        horizontalAlignment: Text.AlignHCenter
    }

    SvgIcon {
        visible: root.svgPath !== ""
        anchors.fill: parent
        pathData: root.svgPath
        fillColor: root.active ? root.active_font_color : root.font_color
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

        onCanceled: {
            if (!root.fixed)
                parent.pressed = false;
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

            if(mouse.button === Qt.LeftButton)
            {
                if(!root.fixed)
                    parent.pressed = true;
            }
            else if(root.fixed === parent.pressed) // Right button
            {
                root.fixed = !root.fixed;
                parent.pressed = !parent.pressed;
            }
        }

        onReleased: function(mouse) {
            mouse.accepted = true;

            if(mouse.button === Qt.LeftButton
                    && !root.fixed)
                parent.pressed = false;
        }

        onCanceled: {
            if (!root.fixed)
                parent.pressed = false;
        }

        onExited: {
            if (!root.fixed)
                parent.pressed = false;
        }
    }
}
