import QtQuick 2.0
import QtQuick.Controls 1.0
import QtQuick.Layouts 1.0
import Firebird.Emu 1.0
import Firebird.UIComponents 1.0

ColumnLayout {
    property var kitModel
    Component.onCompleted: kitModel = Emu.kits

    spacing: 5

    FBLabel {
        text: qsTr("Kits")
        font.bold: true
        font.pixelSize: 20
    }

    KitList {
        id: kitList

        Layout.fillHeight: true
        Layout.fillWidth: true

        Layout.rightMargin: 1
        Layout.leftMargin: 1

        onCurrentItemChanged: {
            nameEdit.text = currentItem.myData.name
            flashEdit.filePath = currentItem.myData.flash
            boot1Edit.filePath = currentItem.myData.boot1
            snapshotEdit.filePath = currentItem.myData.snapshot
        }

        kitModel: parent.kitModel
    }

    GroupBox {
        id: groupBox1
        Layout.minimumWidth: parent.width
        Layout.maximumWidth: parent.width
        Layout.bottomMargin: -1
        title: qsTr("Kit Properties")

        GridLayout {
            anchors.fill: parent
            columns: 4

            FBLabel {
                text: qsTr("Name:")
                elide: Text.ElideMiddle
            }

            TextField {
                id: nameEdit
                placeholderText: qsTr("Name")
                Layout.fillWidth: true

                onTextChanged: kitModel.setData(kitList.currentIndex, text, KitModel.NameRole)
            }

            FBLabel {
                text: qsTr("Boot1:")
                elide: Text.ElideMiddle
            }

            FileSelect {
                id: boot1Edit
                Layout.fillWidth: true
                onFilePathChanged: kitModel.setData(kitList.currentIndex, filePath, KitModel.Boot1Role)
            }

            FBLabel {
                text: qsTr("Flash:")
                elide: Text.ElideMiddle
            }

            FileSelect {
                id: flashEdit
                Layout.fillWidth: true
                onFilePathChanged: kitModel.setData(kitList.currentIndex, filePath, KitModel.FlashRole)
            }

            FBLabel {
                text: qsTr("Snapshot file:")
                elide: Text.ElideMiddle
            }

            FileSelect {
                id: snapshotEdit
                Layout.fillWidth: true
                onFilePathChanged: kitModel.setData(kitList.currentIndex, filePath, KitModel.SnapshotRole)
            }
        }
    }
}
