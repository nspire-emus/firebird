import QtQuick 2.0
import QtQuick.Controls 1.0
import QtQuick.Dialogs 1.1
import QtQuick.Layouts 1.0
import Firebird.Emu 1.0
import Firebird.UIComponents 1.0

ColumnLayout {
    spacing: 5

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

    FileDialog {
        id: fileDialog
        nameFilters: [ "TNS Documents (*.tns)", "Operating Systems (*.tno, *.tnc, *.tco, *.tcc)" ]
        onAccepted: Emu.sendFile(fileUrl, Emu.usbdir)
    }

    FBLabel {
        text: qsTr("Single File Transfer")
        font.pixelSize: 14
        Layout.topMargin: 5
        Layout.bottomMargin: 5
    }

    FBLabel {
        Layout.maximumWidth: parent.width
        wrapMode: Text.WordWrap
        text: qsTr("If you are unable to use the main window's file transfer using either drag'n'drop or the file explorer, you can send single files here.")
        font.pixelSize: 12
        visible: !Emu.isMobile()
    }

    RowLayout {
        Button {
            text: qsTr("Send a file")
            Layout.topMargin: 5
            Layout.bottomMargin: 5
            onClicked: fileDialog.visible = true
        }

        FBLabel {
            text: qsTr("Status:")
        }

        FBLabel {
            id: transferStatus
            text: qsTr("idle")
        }

        Connections {
            target: Emu
            onUsblinkProgressChanged: {
                if(percent < 0)
                    transferStatus.text = qsTr("Failed!");
                else
                    transferStatus.text = percent + qsTr(" % sent");
            }
        }
    }

    Item {
        Layout.fillHeight: true
    }

}
