import QtQuick 2.0
import QtQuick.Controls 1.0
import QtQuick.Dialogs 1.0
import QtQuick.Layouts 1.0
import Firebird.Emu 1.0

RowLayout {
    property string filePath: ""
    property alias dialog: dialog

    // This should normally happen automatically...
    onFilePathChanged: dialog.folder = Emu.dir(filePath)

    FileDialog {
        id: dialog
        visible: false
        folder: Emu.dir(filePath)

        onAccepted: filePath = Emu.toLocalFile(dialog.fileUrl)
    }

    FBLabel {
        id: filenameLabel
        elide: "ElideRight"

        Layout.fillWidth: true

        font.italic: filePath === ""
        text: filePath === "" ? qsTr("(none)") : Emu.basename(filePath)
        color: (!dialog.selectExisting || filePath === "" || Emu.fileExists(filePath)) ? "" : "red"
    }

    Button {
        id: selectButton
        text: qsTr("Select")

        onClicked: dialog.visible = true
    }
}
