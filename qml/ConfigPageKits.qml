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
        Layout.fillWidth: true
        Layout.minimumWidth: contentItem.Layout.minimumWidth
        Layout.bottomMargin: -1
        title: qsTr("Kit Properties")

        GridLayout {
            anchors.fill: parent
            columns: (width < 550 || Qt.platform.os === "android") ? 3 : 6

            FBLabel {
                Layout.columnSpan: parent.columns
                Layout.fillWidth: true
                color: "red"
                visible: kitList.currentItem.myData.boot1 === "" || kitList.currentItem.myData.flash === ""
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
                Layout.columnSpan: 2
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

            FBLabel {
                property string filePath: kitList.currentItem.myData.boot1
                elide: "ElideRight"

                Layout.fillWidth: true
                // Allow the label to shrink below its implicitWidth.
                // Without this, the layout doesn't allow it to go smaller...
                Layout.preferredWidth: 100

                font.italic: filePath === ""
                text: filePath === "" ? qsTr("(none)") : Emu.basename(filePath)
            }

            IconButton {
                icon: "qrc:/icons/resources/icons/document-edit.png"
                onClicked: Emu.loadFile(kitList.currentIndex, KitModel.Boot1Role)
            }

            FBLabel {
                text: qsTr("Flash:")
                elide: Text.ElideMiddle
            }

            FBLabel {
                property string filePath: kitList.currentItem.myData.flash
                elide: "ElideRight"

                Layout.fillWidth: true
                // Allow the label to shrink below its implicitWidth.
                // Without this, the layout doesn't allow it to go smaller...
                Layout.preferredWidth: 100

                font.italic: filePath === ""
                text: filePath === "" ? qsTr("(none)") : Emu.basename(filePath)
            }

            IconButton {
                icon: "qrc:/icons/resources/icons/document-edit.png"
                onClicked: Emu.loadFile(kitList.currentIndex, KitModel.FlashRole)
            }

            FBLabel {
                text: qsTr("Snapshot:")
                elide: Text.ElideMiddle
            }

            FBLabel {
                property string filePath: kitList.currentItem.myData.snapshot
                elide: "ElideRight"

                Layout.fillWidth: true
                // Allow the label to shrink below its implicitWidth.
                // Without this, the layout doesn't allow it to go smaller...
                Layout.preferredWidth: 100

                font.italic: filePath === ""
                text: filePath === "" ? qsTr("(none)") : Emu.basename(filePath)
            }

            IconButton {
                icon: "qrc:/icons/resources/icons/document-edit.png"
                onClicked: Emu.loadFile(kitList.currentIndex, KitModel.SnapshotRole)
            }
        }
    }
}
