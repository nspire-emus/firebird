import QtQuick 2.0
import QtQuick.Controls 1.0
import QtQuick.Layouts 1.0
import Firebird.Emu 1.0
import Firebird.UIComponents 1.0

ColumnLayout {
    spacing: 5

    FBLabel {
        text: qsTr("Remote GDB debugging")
        font.pixelSize: TextMetrics.title2Size
        Layout.topMargin: 5
        Layout.bottomMargin: 5
    }

    FBLabel {
        Layout.maximumWidth: parent.width
        wrapMode: Text.WordWrap
        text: qsTr("If enabled, a remote GDB debugger can be connected to the port and be used for debugging.")
        font.pixelSize: TextMetrics.normalSize
    }

    RowLayout {
        // No spacing so that the spin box looks like part of the label
        spacing: 0
        width: parent.width
        Layout.fillWidth: true

        LabeledCheckBox {
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
            Layout.maximumWidth: TextMetrics.normalSize * 8

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
        text: qsTr("Remote access to internal debugger")
        font.pixelSize: TextMetrics.title2Size
        Layout.topMargin: 10
        Layout.bottomMargin: 5
    }

    FBLabel {
        Layout.maximumWidth: parent.width
        wrapMode: Text.WordWrap
        text: qsTr("Enable this to access the internal debugger via TCP (telnet/netcat), like for firebird-send.")
        font.pixelSize: TextMetrics.normalSize
    }

    RowLayout {
        // No spacing so that the spin box looks like part of the label
        spacing: 0
        width: parent.width
        Layout.fillWidth: true

        LabeledCheckBox {
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
            Layout.maximumWidth: TextMetrics.normalSize * 8

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
        font.pixelSize: TextMetrics.title2Size
        Layout.topMargin: 5
        Layout.bottomMargin: 5
    }

    FBLabel {
        Layout.maximumWidth: parent.width
        wrapMode: Text.WordWrap
        text: qsTr("Configure which situations cause the emulator to trap into the debugger.")
        font.pixelSize: TextMetrics.normalSize
    }

    LabeledCheckBox {
        Layout.maximumWidth: parent.width
        text: qsTr("Enter Debugger on Startup")

        checked: Emu.debugOnStart
        onCheckedChanged: {
            Emu.debugOnStart = checked;
            checked = Qt.binding(function() { return Emu.debugOnStart; });
        }
    }

    LabeledCheckBox {
        Layout.maximumWidth: parent.width
        text: qsTr("Enter Debugger on Warnings and Errors")

        checked: Emu.debugOnWarn
        onCheckedChanged: {
            Emu.debugOnWarn = checked;
            checked = Qt.binding(function() { return Emu.debugOnWarn; });
        }
    }

    Item {
        Layout.fillHeight: true
    }
}
