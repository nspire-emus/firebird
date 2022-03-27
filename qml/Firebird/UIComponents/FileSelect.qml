import QtQuick 2.0
import QtQuick.Controls 1.0
import QtQuick.Dialogs 1.0
import QtQuick.Layouts 1.0
import Firebird.Emu 1.0

RowLayout {
    property string filePath: ""
    property bool selectExisting: true
    property alias subtext: subtextLabel.text
    property bool showCreateButton: false
    signal create()

    // Hack to force reevaluation of Emu.fileExists(filePath) after reselection.
    // Needed on Android due to persistent permissions.
    property int forceRefresh: 0
    Loader {
        id: dialogLoader
        active: false
        sourceComponent: FileDialog {
            folder: filePath ? Emu.dir(filePath) : Global.lastFileDialogDir
            // If save dialogs are not supported, force an open dialog
            selectExisting: parent.selectExisting || !Emu.saveDialogSupported()
            onAccepted: {
                filePath = Emu.toLocalFile(fileUrl);
                Global.lastFileDialogDir = Emu.dir(filePath);
                forceRefresh++;
            }
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
            // Allow the label to shrink below its implicitWidth.
            // Without this, the layout doesn't allow it to go smaller...
            Layout.preferredWidth: 100

            font.italic: filePath === ""
            text: { forceRefresh; return filePath === "" ? qsTr("(none)") : Emu.basename(filePath); }
            color: { forceRefresh; return ((!selectExisting && Emu.saveDialogSupported()) || filePath === "" || Emu.fileExists(filePath)) ? paletteActive.text : "red"; }
        }

        FBLabel {
            id: subtextLabel
            elide: "ElideRight"

            font.pixelSize: TextMetrics.normalSize * 0.8
            Layout.fillWidth: true
            visible: text !== ""
        }
    }

    // Button for either custom creation functionality (onCreate) or
    // if the open file dialog doesn't allow creation, to open a file creation dialog.
    IconButton {
        visible: showCreateButton || (!selectExisting && !Emu.saveDialogSupported())
        icon: "qrc:/icons/resources/icons/document-new.png"

        Loader {
            id: createDialogLoader
            active: false
            sourceComponent: FileDialog {
                folder: Emu.dir(filePath)
                selectExisting: false
                onAccepted: {
                    filePath = Emu.toLocalFile(fileUrl);
                    forceRefresh++;
                }
            }
        }

        onClicked: {
            if(showCreateButton)
                parent.create()
            else {
                createDialogLoader.active = true;
                createDialogLoader.item.visible = true;
            }
        }
    }

    IconButton {
        icon: "qrc:/icons/resources/icons/document-edit.png"
        onClicked: {
            dialogLoader.active = true;
            dialogLoader.item.visible = true;
        }
    }
}
