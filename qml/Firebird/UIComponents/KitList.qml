import QtQuick 2.0
import QtQuick.Controls 1.0

Rectangle {
    property alias currentItem: listView.currentItem
    property alias currentIndex: listView.currentIndex
    property alias model: listView.model

    color: "white"
    border {
        color: "darkgrey"
        width: 1
    }

    ScrollView {
        anchors.margins: parent.border.width
        anchors.fill: parent

        ListView {
            id: listView

            anchors.centerIn: parent
            anchors.fill: parent
            anchors.margins: 2
            focus: true
            highlightMoveDuration: 50
            highlightResizeDuration: 0

            highlight: Rectangle {
                color: "#40b3d5"
                anchors.horizontalCenter: parent.horizontalCenter
            }

            delegate: Item {
                height: item.height + 10
                width: listView.width - listView.anchors.margins
                anchors.horizontalCenter: parent.horizontalCenter

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

                    color: "grey"
                    height: 1
                }

                KitItem {
                    id: item
                    width: parent.width - 15
                    anchors.centerIn: parent
                    kitName: name
                    flashFile: flash
                }
            }

            section.property: "type"
            section.criteria: ViewSection.FullString
            section.delegate: Rectangle {
                anchors.horizontalCenter: parent.horizontalCenter
                color: "#b0dede"
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

                    color: "grey"
                    height: 1
                }
            }
        }
    }
}
