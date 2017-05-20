import QtQuick 2.0
import QtQuick.Controls 1.0
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
        id: groupBox1
        Layout.minimumWidth: parent.width
        Layout.maximumWidth: parent.width
        Layout.bottomMargin: -1
        title: qsTr("Kit Properties")


        GridLayout {
            anchors.fill: parent
            columns: (width < 550 || Qt.platform.os === "android") ? 2 : 4

            FBLabel {
                Layout.columnSpan: parent.columns
                color: "red"
                visible: boot1Edit.filePath == "" || flashEdit.filePath == ""
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

                text: kitList.currentItem.myData.name;
                onTextChanged: {
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
                filePath: kitList.currentItem.myData.boot1;
                onFilePathChanged: {
                    kitModel.setDataRow(kitList.currentIndex, filePath, KitModel.Boot1Role);
                    filePath = Qt.binding(function() { return kitList.currentItem.myData.boot1; });
                }
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
                    filePath: kitList.currentItem.myData.flash;
                    onFilePathChanged: {
                        kitModel.setDataRow(kitList.currentIndex, filePath, KitModel.FlashRole);
                        filePath = Qt.binding(function() { return kitList.currentItem.myData.flash; });
                    }
                }

                Button {
                    text: qsTr("Create")
                    visible: !Emu.isMobile()
                    onClicked: {
                        Emu.createFlash(kitList.currentIndex)
                        flashEdit.filePath = kitList.currentItem.myData.flash
                    }
                }
            }

            FBLabel {
                text: qsTr("Snapshot file:")
                elide: Text.ElideMiddle
            }

            FileSelect {
                id: snapshotEdit
                Layout.fillWidth: true
                dialog.selectExisting: false
                filePath: kitList.currentItem.myData.snapshot;
                onFilePathChanged: {
                    kitModel.setDataRow(kitList.currentIndex, filePath, KitModel.SnapshotRole);
                    filePath = Qt.binding(function() { return kitList.currentItem.myData.snapshot; });
                }
            }
        }
    }
}
