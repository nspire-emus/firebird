import QtQuick 2.0
import QtQuick.Controls 1.0
import Firebird.Emu 1.0

Rectangle {
    property alias currentItem: listView.currentItem
    property alias currentIndex: listView.currentIndex
    property alias kitModel: sortedModel.sourceModel

    SystemPalette {
        id: paletteActive
    }

    color: paletteActive.base
    border {
        color: paletteActive.alternateBase
        width: 1
    }

    QSortFilterProxyModel {
        id: sortedModel
        sortRole: KitModel.TypeRole
    }

    ScrollView {
        anchors.margins: parent.border.width
        anchors.fill: parent

        activeFocusOnTab: true

        ListView {
            id: listView

            anchors.centerIn: parent
            anchors.fill: parent
            focus: true
            highlightMoveDuration: 50
            highlightResizeDuration: 0

            model: sortedModel

            highlight: Rectangle {
                color: paletteActive.highlight
                anchors.horizontalCenter: parent ? parent.horizontalCenter : undefined
            }

            delegate: Item {
                property variant myData: model

                height: item.height + 10
                width: listView.width - listView.anchors.margins

                MouseArea {
                    anchors.fill: parent
                    onClicked: function() {
                        parent.ListView.view.currentIndex = index
                        parent.forceActiveFocus()
                    }
                }

                Rectangle {
                    anchors {
                        left: parent.left
                        right: parent.right
                        bottom: parent.bottom
                    }

                    color: paletteActive.shadow
                    height: 1
                }

                KitItem {
                    id: item
                    width: parent.width - 15
                    anchors.centerIn: parent

                    kitName: name
                    flashFile: Emu.basename(flash)
                    stateFile: Emu.basename(snapshot)
                }

                FBLink {
                    anchors {
                        top: parent.top
                        right: copyButton.left
                        topMargin: 5
                        rightMargin: 5
                    }

                    text: qsTr("Remove")
                    visible: parent.ListView.view.currentIndex === index && parent.ListView.view.count > 1
                    onClicked: {
                        kitModel.removeRows(index, 1)
                    }
                }

                FBLink {
                    id: copyButton

                    anchors {
                        top: parent.top
                        right: parent.right
                        topMargin: 5
                        rightMargin: 5
                    }

                    text: qsTr("Copy")
                    visible: parent.ListView.view.currentIndex === index
                    onClicked: {
                        kitModel.copy(index)
                    }
                }
            }

            section.property: "type"
            section.criteria: ViewSection.FullString
            section.delegate: Rectangle {
                anchors.horizontalCenter: parent.horizontalCenter
                color: paletteActive.window
                height: label.implicitHeight + 4
                width: listView.width - listView.anchors.margins

                FBLabel {
                    id: label
                    font.bold: true
                    anchors.fill: parent
                    anchors.leftMargin: 5
                    verticalAlignment: Text.AlignVCenter
                    text: section
                }

                Rectangle {
                    anchors {
                        left: parent.left
                        right: parent.right
                        bottom: parent.bottom
                    }

                    color: paletteActive.shadow
                    height: 1
                }
            }
        }
    }
}
