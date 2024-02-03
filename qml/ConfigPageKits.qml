import QtQuick 2.0
import QtQuick.Controls 2.0
import QtQuick.Layouts 1.0
import Firebird.Emu 1.0
import Firebird.UIComponents 1.0

ColumnLayout {
    property var kitModel
    Component.onCompleted: kitModel = Emu.kits

    spacing: 5

    KitList {
        id: kitList

        Layout.fillHeight: true
        Layout.fillWidth: true

        Layout.rightMargin: 1
        Layout.leftMargin: 1
        Layout.topMargin: 5

        kitModel: parent.kitModel
    }

    GroupBox {
        Layout.fillWidth: true
        Layout.minimumWidth: contentItem.Layout.minimumWidth
        title: qsTr("Kit Properties")

        GridLayout {
            anchors.fill: parent
            columns: (width < 550 || Qt.platform.os === "android") ? 2 : 4

            FBLabel {
                Layout.columnSpan: parent.columns
                Layout.fillWidth: true
                color: "red"
                visible: boot1Edit.filePath == "" || flashEdit.filePath == ""
                wrapMode: Text.WordWrap
                text: qsTr("You need to specify files for Boot1 and Flash")
            }

            FBLabel {
                text: qsTr("Name:")
                elide: Text.ElideMiddle
            }

            TextField {
                id: nameEdit
                placeholderText: qsTr("Name")
                Layout.fillWidth: true

                text: kitList.currentItem.myData.name
                onTextChanged: {
                    if(text !== kitList.currentItem.myData.name)
                        kitModel.setDataRow(kitList.currentIndex, text, KitModel.NameRole);
                    text = Qt.binding(function() { return kitList.currentItem.myData.name; });
                }
            }

            FBLabel {
                text: qsTr("Boot1:")
                elide: Text.ElideMiddle
            }

            FileSelect {
                id: boot1Edit
                Layout.fillWidth: true
                filePath: kitList.currentItem.myData.boot1
                onFilePathChanged: {
                    if(filePath !== kitList.currentItem.myData.boot1)
                        kitModel.setDataRow(kitList.currentIndex, filePath, KitModel.Boot1Role);
                    filePath = Qt.binding(function() { return kitList.currentItem.myData.boot1; });
                }
            }

            FBLabel {
                text: qsTr("Flash:")
                elide: Text.ElideMiddle
            }

            FileSelect {
                id: flashEdit
                Layout.fillWidth: true
                filePath: kitList.currentItem.myData.flash
                onFilePathChanged: {
                    if(filePath !== kitList.currentItem.myData.flash)
                        kitModel.setDataRow(kitList.currentIndex, filePath, KitModel.FlashRole);
                    filePath = Qt.binding(function() { return kitList.currentItem.myData.flash; });
                }
                showCreateButton: true
                onCreate: flashDialog.visible = true
            }

            FlashDialog {
                id: flashDialog
                onFlashCreated: {
                    kitModel.setDataRow(kitList.currentIndex, filePath, KitModel.FlashRole);
                }
            }

            FBLabel {
                text: qsTr("Snapshot:")
                elide: Text.ElideMiddle
            }

            FileSelect {
                id: snapshotEdit
                Layout.fillWidth: true
                selectExisting: false
                filePath: kitList.currentItem.myData.snapshot
                onFilePathChanged: {
                    if(filePath !== kitList.currentItem.myData.snapshot)
                        kitModel.setDataRow(kitList.currentIndex, filePath, KitModel.SnapshotRole);
                    filePath = Qt.binding(function() { return kitList.currentItem.myData.snapshot; });
                }
            }
        }
    }
}
