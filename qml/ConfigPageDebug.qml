import QtQuick 2.7
import QtQuick.Controls 1.0
import QtQuick.Layouts 1.0
import Firebird.Emu 1.0
import Firebird.UIComponents 1.0

ScrollView {
    id: sv
    // TODO: Find out why this breaks on desktop
    flickableItem.interactive: Emu.isMobile()

    ColumnLayout {
    spacing: 5
    width: sv.viewport.width

    FBLabel {
        text: qsTr("Remote GDB debugging")
        font.pixelSize: TextSize.title2Size
        Layout.topMargin: 5
        Layout.bottomMargin: 5
    }

    FBLabel {
        Layout.fillWidth: true
        wrapMode: Text.WordWrap
        text: qsTr("If enabled, a remote GDB debugger can be connected to the port and be used for debugging.")
        font.pixelSize: TextSize.normalSize
    }

    RowLayout {
        // No spacing so that the spin box looks like part of the label
        spacing: 0
        width: parent.width
        Layout.fillWidth: true

        CheckBox {
            Layout.maximumWidth: parent.parent.width - gdbPort.width
            id: gdbCheckBox
            text: qsTr("Enable GDB stub on Port")

            checked: Emu.gdbEnabled
            onCheckedChanged: {
                Emu.gdbEnabled = checked;
                checked = Qt.binding(function() { return Emu.gdbEnabled; });
            }
        }

        SpinBox {
            id: gdbPort
            Layout.maximumWidth: TextSize.normalSize * 8

            minimumValue: 1
            maximumValue: 65535

            value: Emu.gdbPort
            onValueChanged: {
                Emu.gdbPort = value;
                value = Qt.binding(function() { return Emu.gdbPort; });
            }
        }
    }

    FBLabel {
        Layout.fillWidth: true
        text: qsTr("Remote access to internal debugger")
        wrapMode: Text.WordWrap
        font.pixelSize: TextSize.title2Size
        Layout.topMargin: 10
        Layout.bottomMargin: 5
    }

    FBLabel {
        Layout.fillWidth: true
        wrapMode: Text.WordWrap
        text: qsTr("Enable this to access the internal debugger via TCP (telnet/netcat), like for firebird-send.")
        font.pixelSize: TextSize.normalSize
    }

    RowLayout {
        // No spacing so that the spin box looks like part of the label
        spacing: 0
        Layout.fillWidth: true

        CheckBox {
            Layout.maximumWidth: parent.parent.width - rdbPort.width
            id: rgbCheckBox
            text: qsTr("Enable internal debugger on Port")

            checked: Emu.rdbEnabled
            onCheckedChanged: {
                Emu.rdbEnabled = checked;
                checked = Qt.binding(function() { return Emu.rdbEnabled; });
            }
        }

        SpinBox {
            id: rdbPort
            Layout.maximumWidth: TextSize.normalSize * 8

            minimumValue: 1
            maximumValue: 65535

            value: Emu.rdbPort
            onValueChanged: {
                Emu.rdbPort = value;
                value = Qt.binding(function() { return Emu.rdbPort; });
            }
        }
    }

    FBLabel {
        text: qsTr("Enter into Debugger")
        font.pixelSize: TextSize.title2Size
        Layout.topMargin: 5
        Layout.bottomMargin: 5
    }

    FBLabel {
        Layout.fillWidth: true
        wrapMode: Text.WordWrap
        text: qsTr("Configure which situations cause the emulator to trap into the debugger.")
        font.pixelSize: TextSize.normalSize
    }

    CheckBox {
        Layout.fillWidth: true
        text: qsTr("Enter Debugger on Startup")

        checked: Emu.debugOnStart
        onCheckedChanged: {
            Emu.debugOnStart = checked;
            checked = Qt.binding(function() { return Emu.debugOnStart; });
        }
    }

    CheckBox {
        Layout.fillWidth: true
        text: qsTr("Enter Debugger on Warnings and Errors")

        checked: Emu.debugOnWarn
        onCheckedChanged: {
            Emu.debugOnWarn = checked;
            checked = Qt.binding(function() { return Emu.debugOnWarn; });
        }
    }

    CheckBox {
        Layout.fillWidth: true
        text: qsTr("Print a message on Warnings")

        enabled: !Emu.debugOnWarn

        checked: Emu.printOnWarn
        onCheckedChanged: {
            Emu.printOnWarn = checked;
            checked = Qt.binding(function() { return Emu.printOnWarn; });
        }
    }

    FBLabel {
        text: qsTr("Debug Messages")
        font.pixelSize: TextSize.title2Size
        Layout.topMargin: 5
        Layout.bottomMargin: 5
        visible: debugMessages.visible
    }

    TextArea {
        id: debugMessages
        Layout.fillHeight: true
        Layout.fillWidth: true
        Layout.minimumHeight: TextSize.normalSize * 12
        font.pixelSize: TextSize.normalSize
        font.family: "monospace"
        readOnly: true
        visible: Emu.isMobile()

        Connections {
            target: Emu
            enabled: debugMessages.visible
            function onDebugStr(str) {
                debugMessages.insert(debugMessages.length, str);
            }
        }
    }
}
}
