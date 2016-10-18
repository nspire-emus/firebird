import QtQuick 2.0
import QtQuick.Controls 1.0
import QtQuick.Layouts 1.0
import Firebird.UIComponents 1.0

ColumnLayout {
    id: column1
    spacing: 5

    Text {
        text: qsTr("Debugging")
        font.bold: true
        font.pixelSize: 19
    }

    Text {
        text: qsTr("Remote GDB debuggging")
        font.pixelSize: 14
        Layout.topMargin: 10
        Layout.bottomMargin: 5
    }

    Text {
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
        }

        SpinBox {
            id: gdbPort

            minimumValue: 1
            maximumValue: 65535
        }
    }

    Text {
        text: qsTr("Remote access to internal debugger")
        font.pixelSize: 14
        Layout.topMargin: 10
        Layout.bottomMargin: 5
    }

    Text {
        text: qsTr("Enable this to access the internal debugger via TCP (telnet/netcat),\nlike for firebird-send.")
        font.pixelSize: 12
    }

    RowLayout {
        // No spacing so that the spin box looks like part of the label
        spacing: 0

        LabeledCheckBox {
            id: rgbCheckBox
            text: qsTr("Enable internal debugger on Port")
        }

        SpinBox {
            id: rdbPort

            minimumValue: 1
            maximumValue: 65535
        }
    }

    Item {
        Layout.fillHeight: true
    }

}
