import QtQuick 2.0
import QtQuick.Controls 1.0
import QtQuick.Layouts 1.0
import Firebird.Emu 1.0
import Firebird.UIComponents 1.0

ColumnLayout {
    spacing: 5

    Label {
        text: qsTr("Debugging")
        font.bold: true
        font.pixelSize: 20
    }

    Label {
        text: qsTr("Remote GDB debuggging")
        font.pixelSize: 14
        Layout.topMargin: 5
        Layout.bottomMargin: 5
    }

    Label {
        id: text4
        text: qsTr("If enabled, a remote GDB debugger can be connected\nto the port and be used for debugging")
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

    Label {
        text: qsTr("Remote access to internal debugger")
        font.pixelSize: 14
        Layout.topMargin: 10
        Layout.bottomMargin: 5
    }

    Label {
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

    Item {
        Layout.fillHeight: true
    }

}
