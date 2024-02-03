import QtQuick 2.7
import QtQuick.Layouts 1.2
import QtQuick.Controls 2.0

import Firebird.UIComponents 1.0

Item {
    Layout.fillHeight: true
    Layout.fillWidth: true

    ColumnLayout {
        anchors {
            left: parent.left
            right: swipeBar.left
            top: parent.top
            bottom: autoSaveLabel.top
            bottomMargin: 2
        }

        TabBar {
            id: bar
            property var model: ConfigPagesModel {}

            Repeater {
                id: rep
                model: bar.model

                TabButton {
                    text: qsTranslate("ConfigPagesModel", rep.model.get(index).title)
                }
            }
        }

        SwipeView {
            Layout.fillHeight: true
            Layout.fillWidth: true
            currentIndex: bar.currentIndex
            clip: true

            Repeater {
                model: bar.model

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
