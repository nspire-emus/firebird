import QtQuick 2.0
import QtQuick.Controls 1.0
import QtQuick.Layouts 1.0
import Firebird.Emu 1.0
import Firebird.UIComponents 1.0

ColumnLayout {
    property var kitModel
    property bool triggerSignals: true
    Component.onCompleted: kitModel = Emu.kits

    spacing: 5

    KitList {
        id: kitList

        Layout.fillHeight: true
        Layout.fillWidth: true

        Layout.rightMargin: 1
        Layout.leftMargin: 1
        Layout.topMargin: 5

        onCurrentItemChanged: {
            triggerSignals = false
            nameEdit.text = Qt.binding(function() { return currentItem.myData.name; })
            flashEdit.filePath = Qt.binding(function() { return currentItem.myData.flash;})
            boot1Edit.filePath = Qt.binding(function() { return currentItem.myData.boot1;})
            snapshotEdit.filePath = Qt.binding(function() { return currentItem.myData.snapshot;})
            triggerSignals = true
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
            columns: width < 450 ? 2 : 4

            FBLabel {
                text: qsTr("Name:")
                elide: Text.ElideMiddle
            }

            TextField {
                id: nameEdit
                placeholderText: qsTr("Name")
                Layout.fillWidth: true

                onTextChanged: if(triggerSignals) kitModel.setData(kitList.currentIndex, text, KitModel.NameRole)
            }

            FBLabel {
                text: qsTr("Boot1:")
                elide: Text.ElideMiddle
            }

            FileSelect {
                id: boot1Edit
                Layout.fillWidth: true
                onFilePathChanged: if(triggerSignals) kitModel.setData(kitList.currentIndex, filePath, KitModel.Boot1Role)
            }

            FBLabel {
                text: qsTr("Flash:")
                elide: Text.ElideMiddle
            }

            RowLayout {
                Layout.fillWidth: true

                FileSelect {
                    id: flashEdit
                    Layout.fillWidth: true
                    onFilePathChanged: if(triggerSignals)  kitModel.setData(kitList.currentIndex, filePath, KitModel.FlashRole)
                }

                Button {
                    text: qsTr("Create")
                    visible: !Emu.isMobile()
                    onClicked: Emu.createFlash(kitList.currentIndex)
                }
            }

            FBLabel {
                text: qsTr("Snapshot file:")
                elide: Text.ElideMiddle
            }

            FileSelect {
                id: snapshotEdit
                Layout.fillWidth: true
                onFilePathChanged: if(triggerSignals) kitModel.setData(kitList.currentIndex, filePath, KitModel.SnapshotRole)
                dialog.selectExisting: false
            }
        }
    }
}
