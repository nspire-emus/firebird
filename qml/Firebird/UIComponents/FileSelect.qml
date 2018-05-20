import QtQuick 2.0
import QtQuick.Controls 1.0
import QtQuick.Dialogs 1.0
import QtQuick.Layouts 1.0
import Firebird.Emu 1.0

RowLayout {
    id: root
    property string filePath: ""
    property bool selectExisting: true

    Loader {
        id: dialogLoader
        active: false
        sourceComponent: FileDialog {
            folder: Emu.dir(filePath)
            selectExisting: root.selectExisting
            onAccepted: filePath = Emu.toLocalFile(fileUrl)
        }
    }

    FBLabel {
        id: filenameLabel
        elide: "ElideRight"

        Layout.fillWidth: true

        font.italic: filePath === ""
        text: filePath === "" ? qsTr("(none)") : Emu.basename(filePath)
        color: (!selectExisting || filePath === "" || Emu.fileExists(filePath)) ? "" : "red"
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
