import QtQuick 2.0
import QtQuick.Controls 2.0
import QtQuick.Layouts 1.0
import Qt.labs.platform 1.1
import Firebird.Emu 1.0
import Firebird.UIComponents 1.0

ColumnLayout {
    spacing: 5

    FBLabel {
        text: qsTr("File Transfer")
        font.pixelSize: TextMetrics.title2Size
        Layout.topMargin: 5
        Layout.bottomMargin: 5
    }

    FBLabel {
        Layout.fillWidth: true
        wrapMode: Text.WordWrap
        text: qsTr("If you are unable to use the main window's file transfer using either drag'n'drop or the file explorer, you can send files here.")
        font.pixelSize: TextMetrics.normalSize
        visible: !Emu.isMobile()
    }

    FBLabel {
        Layout.fillWidth: true
        wrapMode: Text.WordWrap
        text: qsTr("Here you can send files into the target folder specified below.")
        font.pixelSize: TextMetrics.normalSize
    }

    Loader {
        id: fileDialogLoader
        active: false
        sourceComponent: FileDialog {
            nameFilters: [ qsTr("TNS Documents") +"(*.tns)", qsTr("Operating Systems") + "(*.tno *.tnc *.tco *.tcc *.tlo *.tmo *.tmc *.tco2 *.tcc2 *.tct2)" ]
            fileMode: FileDialog.OpenFiles
            onAccepted: {
                transferStatus.text = qsTr("Starting");
                transferProgress.indeterminate = true;
                for(let i = 0; i < files.length; ++i)
                    Emu.sendFile(files[i], Emu.usbdir);
            }
        }
    }

    RowLayout {
        Layout.fillWidth: true

        Button {
            text: qsTr("Send files")
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

        ProgressBar {
            id: transferProgress
            Layout.fillWidth: true
            from: 0
            to: 100
        }
    }

    RowLayout {
        Layout.fillWidth: true

        FBLabel {
            id: transferStatusLabel
            text: qsTr("Status:")
        }

        FBLabel {
            id: transferStatus
            Layout.fillWidth: true
            text: qsTr("Idle")
        }

        Connections {
            target: Emu
            function onUsblinkProgressChanged(percent) {
                if(percent < 0)
                    transferStatus.text = qsTr("Failed!");
                else
                {
                    transferStatus.text = (percent >= 100) ? qsTr("Done!") : (percent + "%");
                    transferProgress.value = percent;
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
        Layout.fillWidth: true
        wrapMode: Text.WordWrap
        text: qsTr("When dragging files onto Firebird, it will try to send them to the emulated system.")
        font.pixelSize: TextMetrics.normalSize
    }

    RowLayout {
        Layout.fillWidth: true

        FBLabel {
            Layout.fillWidth: true
            text: qsTr("Target folder for dropped files:")
            wrapMode: Text.WordWrap
        }

        TextField {
            Layout.fillWidth: true
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
