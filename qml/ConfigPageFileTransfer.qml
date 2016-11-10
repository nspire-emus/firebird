import QtQuick 2.0
import QtQuick.Controls 1.0
import QtQuick.Layouts 1.0
import Firebird.Emu 1.0
import Firebird.UIComponents 1.0

ColumnLayout {
    spacing: 5

    FBLabel {
        text: qsTr("File Transfer")
        font.bold: true
        font.pixelSize: 20
    }

    FBLabel {
        text: qsTr("Target Directory")
        font.pixelSize: 14
        Layout.topMargin: 5
        Layout.bottomMargin: 5
    }

    FBLabel {
        text: qsTr("When dragging files onto Firebird,\nit will try to send the file to the emulated system.")
        font.pixelSize: 12
    }

    RowLayout {
        FBLabel {
            text: qsTr("Target folder for dropped files:")
        }

        TextField {
            text: Emu.usbdir
            onTextChanged: Emu.usbdir = text
        }
    }

    Item {
        Layout.fillHeight: true
    }

}
