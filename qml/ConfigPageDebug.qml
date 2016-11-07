import QtQuick 2.0
import QtQuick.Controls 1.0
import QtQuick.Layouts 1.0
import Firebird.Emu 1.0
import Firebird.UIComponents 1.0

ColumnLayout {
    spacing: 5

    FBLabel {
        text: qsTr("Debugging")
        font.bold: true
        font.pixelSize: 20
    }

    FBLabel {
        text: qsTr("Remote GDB debugging")
        font.pixelSize: 14
        Layout.topMargin: 5
        Layout.bottomMargin: 5
    }

    FBLabel {
        id: text4
        text: qsTr("If enabled, a remote GDB debugger can be connected\nto the port and be used for debugging.")
        font.pixelSize: 12
    }

    RowLayout {
        // No spacing so that the spin box looks like part of the label
        spacing: 0

        LabeledCheckBox {
            id: gdbCheckBox
            text: qsTr("Enable GDB stub on Port")

            checked: Emu.gdbEnabled
            onCheckedChanged: Emu.gdbEnabled = checked
        }

        SpinBox {
            id: gdbPort

            minimumValue: 1
            maximumValue: 65535

            value: Emu.gdbPort
            onValueChanged: Emu.gdbPort = value
        }
    }

    FBLabel {
        text: qsTr("Remote access to internal debugger")
        font.pixelSize: 14
        Layout.topMargin: 10
        Layout.bottomMargin: 5
    }

    FBLabel {
        text: qsTr("Enable this to access the internal debugger via TCP (telnet/netcat),\nlike for firebird-send.")
        font.pixelSize: 12
    }

    RowLayout {
        // No spacing so that the spin box looks like part of the label
        spacing: 0

        LabeledCheckBox {
            id: rgbCheckBox
            text: qsTr("Enable internal debugger on Port")

            checked: Emu.rdbEnabled
            onCheckedChanged: Emu.rdbEnabled = checked
        }

        SpinBox {
            id: rdbPort

            minimumValue: 1
            maximumValue: 65535

            value: Emu.rdbPort
            onValueChanged: Emu.rdbPort = value
        }
    }

    FBLabel {
        text: qsTr("Enter into Debugger")
        font.pixelSize: 14
        Layout.topMargin: 5
        Layout.bottomMargin: 5
    }

    FBLabel {
        text: qsTr("Configure which situations cause the emulator to trap into the debugger.")
        font.pixelSize: 12
    }

    LabeledCheckBox {
        text: qsTr("Enter Debugger on Startup")

        checked: Emu.debugOnStart
        onCheckedChanged: Emu.debugOnStart = checked
    }

    LabeledCheckBox {
        text: qsTr("Enter Debugger on Warnings and Errors")

        checked: Emu.gdbEnabled
        onCheckedChanged: Emu.debugOnWarn = checked
    }

    Item {
        Layout.fillHeight: true
    }
}
