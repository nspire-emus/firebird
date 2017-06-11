import QtQuick 2.0
import QtQuick.Layouts 1.0
import QtQuick.Controls 1.4
import QtQuick.Controls.Styles 1.4

import Firebird.UIComponents 1.0

Item {
    Layout.fillHeight: true
    Layout.fillWidth: true

    TabView {
        id: tabView
        anchors {
            left: parent.left
            right: swipeBar.left
            top: parent.top
            bottom: autoSaveLabel.top
            bottomMargin: 2
        }

        property var model: ConfigPagesModel {}

    /*    style: TabViewStyle {
            tab: Rectangle {
                color: styleData.selected ? "darkgrey" :"lightgrey"

                implicitWidth: tabView.width / model.count
                implicitHeight: title.contentHeight + 10

                FBLabel {
                    id: title
                    anchors.fill: parent
                    text: styleData.title
                    font.pixelSize: TextMetrics.title1Size
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    elide: Text.ElideRight
                    color: styleData.selected ? "white" : "black"
                }
            }
            frame: Rectangle { color: "#eeeeee" }
        }*/

        Repeater {
            id: rep
            model: tabView.model

            Tab {
                title: rep.model.get(index).title

                Loader {
                    source: file
                }
            }
        }
    }

    FBLabel {
        id: autoSaveLabel

        anchors {
            left: parent.left
            leftMargin: 2
            right: swipeBar.left
            bottom: parent.bottom
        }

        text: qsTr("Changes are saved automatically")
        font.italic: true
        color: "grey"
    }

    VerticalSwipeBar {
        id: swipeBar

        anchors {
            right: parent.right
            top: parent.top
            bottom: parent.bottom
        }

        onClicked: {
            listView.openDrawer();
        }
    }
}
