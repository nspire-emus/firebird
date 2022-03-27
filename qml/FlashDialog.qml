import QtQuick 2.0
import QtQuick.Layouts 1.1
import QtQuick.Controls 1.2
import QtQuick.Dialogs 1.3
import Firebird.Emu 1.0
import Firebird.UIComponents 1.0

Dialog {
    id: flashDialog
    title: qsTr("Create Flash Image")
    standardButtons: Dialog.Save | Dialog.Cancel
    onVisibleChanged: {
        // For some reason the initial size on wayland is too big.
        // Setting it to -1 initially appears to work around that.
        if (Qt.platform.pluginName.startsWith("wayland")) {
            width = -1;
            height = -1;
        }
    }

    signal flashCreated(string filePath)

    GridLayout {
        id: layout
        width: parent.width
        columns: width > modelCombo.implicitWidth * 2 ? 2 : 1

        FBLabel {
            Layout.minimumHeight: implicitHeight
            Layout.alignment: Qt.AlignLeft
            text: qsTr("Model:")
        }

        ComboBox {
            id: modelCombo
            property bool cxSelected: modelCombo.currentIndex == 2 || modelCombo.currentIndex == 3
            property bool cx2Selected: modelCombo.currentIndex == 4
            property int productId: [0x0E0, 0x0C2, 0x100, 0x0F0, 0x1C0][currentIndex]
            Layout.fillWidth: true
            model: ["Touchpad", "Touchpad CAS", "CX", "CX CAS", "CX II (/-T/CAS)"]
            currentIndex: 3
        }

        FBLabel {
            visible: subtypeCombo.visible
            Layout.alignment: Qt.AlignLeft
            text: qsTr("CX Subtype:")
        }

        ComboBox {
            id: subtypeCombo
            property int featureValue: [0x005, 0x085, 0x185][currentIndex]
            Layout.fillWidth: true
            visible: modelCombo.cxSelected
            model: ["HW-A", "HW-J (CXCR)", "HW-W (CXCR4)"]
        }

        FBLabel {
            Layout.alignment: Qt.AlignLeft
            text: qsTr("Manuf:")
        }

        FileSelect {
            id: manufSelect
            Layout.maximumWidth: parent.width
            Layout.fillWidth: true
            subtext: filePath ? Emu.manufDescription(filePath) : ""
        }

        // <-- Unless CX II
        FBLabel {
            visible: boot2Select.visible
            Layout.alignment: Qt.AlignLeft
            text: qsTr("Boot2:")
        }

        FileSelect {
            id: boot2Select
            visible: !modelCombo.cx2Selected
            Layout.fillWidth: true
            subtext: filePath ? Emu.componentDescription(filePath, "BOOT2     ") : ""
        }

        FBLabel {
            visible: osSelect.visible
            Layout.alignment: Qt.AlignLeft
            text: qsTr("OS:")
        }

        FileSelect {
            id: osSelect
            visible: !modelCombo.cx2Selected
            Layout.fillWidth: true
            subtext: filePath ? Emu.osDescription(filePath) : ""
        }
        // -->

        // <-- Only CX II
        FBLabel {
            visible: bootloaderSelect.visible
            Layout.alignment: Qt.AlignLeft
            text: qsTr("Bootloader:")
        }

        FileSelect {
            id: bootloaderSelect
            visible: modelCombo.cx2Selected
            Layout.fillWidth: true
            subtext: filePath ? Emu.componentDescription(filePath, "BOOT LOADE") : ""
        }

        FBLabel {
            visible: installerSelect.visible
            Layout.alignment: Qt.AlignLeft
            text: qsTr("Installer:")
        }

        FileSelect {
            id: installerSelect
            visible: modelCombo.cx2Selected
            Layout.fillWidth: true
            subtext: filePath ? Emu.componentDescription(filePath, "INSTALLER ") : ""
        }
        // -->

        FBLabel {
            Layout.alignment: Qt.AlignLeft
            text: qsTr("Diags:")
        }

        FileSelect {
            id: diagsSelect
            Layout.fillWidth: true
            subtext: filePath ? Emu.componentDescription(filePath, "DIAGS     ") : ""
        }

        property string validationText: (modelCombo.cx2Selected && !manufSelect.filePath) ? qsTr("Manuf required for CX II") : ""

        FBLabel {
            Layout.alignment: Qt.AlignCenter
            Layout.columnSpan: 2
            color: "red"
            text: parent.validationText
        }
    }

    MessageDialog {
        id: failureDialog
        title: qsTr("Flash saving failed")
        text: qsTr("Saving the flash file failed!")
    }

    Loader {
        id: fileDialogLoader
        active: false
        sourceComponent: FileDialog {
            selectExisting: false
            onAccepted: {
                var filePath = Emu.toLocalFile(fileUrl);
                var success = false;
                if (!modelCombo.cx2Selected)
                    success = Emu.createFlash(filePath, modelCombo.productId, subtypeCombo.featureValue, manufSelect.filePath, boot2Select.filePath, osSelect.filePath, diagsSelect.filePath);
                else
                    success = Emu.createFlash(filePath, modelCombo.productId, subtypeCombo.featureValue, manufSelect.filePath, bootloaderSelect.filePath, installerSelect.filePath, diagsSelect.filePath);

                if (success) {
                    flashCreated(filePath);
                    flashDialog.visible = false;
                }
                else
                    failureDialog.visible = true;
            }
        }
    }

    onActionChosen: {
        if (action.button === Dialog.Save) {
            // Don't close the dialog now, but only
            // after successful saving
            action.accepted = false;

            if (!layout.validationText) {
                fileDialogLoader.active = true;
                fileDialogLoader.item.visible = true;
            }
        }
    }
}
