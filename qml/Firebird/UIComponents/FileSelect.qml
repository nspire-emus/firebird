import QtQuick 2.0
import QtQuick.Controls 1.0
import QtQuick.Dialogs 1.0
import QtQuick.Layouts 1.0
import Firebird.Emu 1.0

RowLayout {
    id: root
    property string filePath: ""
    property bool selectExisting: true
    property alias subtext: subtextLabel.text

    Loader {
        id: dialogLoader
        active: false
        sourceComponent: FileDialog {
            folder: Emu.dir(filePath)
            // If save dialogs are not supported, force an open dialog
            selectExisting: root.selectExisting || !Emu.saveDialogSupported()
            onAccepted: filePath = Emu.toLocalFile(fileUrl)
        }
    }

    SystemPalette {
        id: paletteActive
    }

    ColumnLayout {
        Layout.fillWidth: true

        FBLabel {
            id: filenameLabel
            elide: "ElideRight"

            Layout.fillWidth: true

            font.italic: filePath === ""
            text: filePath === "" ? qsTr("(none)") : Emu.basename(filePath)
            color: (!selectExisting || filePath === "" || Emu.fileExists(filePath)) ? paletteActive.text : "red"
        }

        FBLabel {
            id: subtextLabel
            elide: "ElideRight"

            font.pixelSize: TextMetrics.normalSize * 0.8
            Layout.fillWidth: true
            visible: text !== ""
        }
    }

    Button {
        id: selectButton
        text: qsTr("Select")

        onClicked: {
            dialogLoader.active = true
            dialogLoader.item.visible = true
        }
    }
}
