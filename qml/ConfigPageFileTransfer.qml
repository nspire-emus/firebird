import QtQuick 2.0
import QtQuick.Controls 1.0
import QtQuick.Dialogs 1.1
import QtQuick.Layouts 1.0
import Firebird.Emu 1.0
import Firebird.UIComponents 1.0

ColumnLayout {
    spacing: 5

    FBLabel {
        text: qsTr("Single File Transfer")
        font.pixelSize: TextMetrics.title2Size
        Layout.topMargin: 5
        Layout.bottomMargin: 5
    }

    FBLabel {
        Layout.maximumWidth: parent.width
        wrapMode: Text.WordWrap
        text: qsTr("If you are unable to use the main window's file transfer using either drag'n'drop or the file explorer, you can send single files here.")
        font.pixelSize: TextMetrics.normalSize
        visible: !Emu.isMobile()
    }

    FBLabel {
        Layout.maximumWidth: parent.width
        wrapMode: Text.WordWrap
        text: qsTr("Here you can send single files into the target folder specified below.")
        font.pixelSize: TextMetrics.normalSize
    }

    Loader {
        id: fileDialogLoader
        active: false
        sourceComponent: FileDialog {
            nameFilters: [ "TNS Documents (*.tns)", "Operating Systems (*.tno *.tnc *.tco *.tcc *.tlo *.tmo *.tmc *.tco2 *.tcc2 *.tct2)" ]
            onAccepted: {
                transferProgress.indeterminate = true;
                transferProgress.visible = true;
                Emu.sendFile(fileUrl, Emu.usbdir);
            }
        }
    }

    RowLayout {
        Layout.maximumWidth: parent.width

        Button {
            text: qsTr("Send a file")
            // If this button is disabled, the transfer directory textinput has the focus again,
            // which is annoying on mobile.
            // enabled: Emu.isRunning
            Layout.topMargin: 5
            Layout.bottomMargin: 5
            onClicked: {
                fileDialogLoader.active = true;
                fileDialogLoader.item.visible = true;
            }
        }

        FBLabel {
            id: transferStatusLabel
            visible: !transferProgress.visible
            text: qsTr("Status:")
        }

        FBLabel {
            id: transferStatus
            visible: !transferProgress.visible
            Layout.fillWidth: true
            text: qsTr("idle")
        }

        ProgressBar {
            id: transferProgress
            visible: false
            Layout.fillWidth: true
            minimumValue: 0
            maximumValue: 100
        }

        Connections {
            target: Emu
            onUsblinkProgressChanged: {
                if(percent < 0)
                {
                    transferStatus.text = qsTr("Failed!");
                    transferProgress.visible = false;
                }
                else if(percent == 100)
                {
                    transferStatus.text = qsTr("Done!");
                    transferProgress.visible = false;
                }
                else
                {
                    transferProgress.value = percent;
                    transferProgress.visible = true;
                    transferProgress.indeterminate = false;
                }
            }
        }
    }

    FBLabel {
        text: qsTr("Target Directory")
        font.pixelSize: TextMetrics.title2Size
        Layout.topMargin: 5
        Layout.bottomMargin: 5
    }

    FBLabel {
        Layout.maximumWidth: parent.width
        wrapMode: Text.WordWrap
        text: qsTr("When dragging files onto Firebird, it will try to send the file to the emulated system.")
        font.pixelSize: TextMetrics.normalSize
    }

    RowLayout {
        width: parent.width

        FBLabel {
            text: qsTr("Target folder for dropped files:")
        }

        TextField {
            text: Emu.usbdir
            onTextChanged: {
                Emu.usbdir = text
                text = Qt.binding(function() { return Emu.usbdir; });
            }
        }
    }

    Item {
        Layout.fillHeight: true
    }

}
