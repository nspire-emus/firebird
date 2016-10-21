import QtQuick 2.6

Item {
    property alias model: pageList.model

    PageList {
        id: pageList
        width: 100

        anchors {
            top: parent.top
            bottom: parent.bottom
            left: parent.left
        }
    }

    Flickable {
        id: content
        contentWidth: pageList.model.count * width
        clip: true
        interactive: false
        contentX: width * pageList.currentIndex
        Behavior on contentX { NumberAnimation { duration: 120 } }

        anchors {
            top: parent.top
            bottom: parent.bottom
            left: pageList.right
            right: parent.right
            leftMargin: 5
        }

        Row {
            Repeater {
                model: pageList.model

                delegate: Loader {
                    width:  content.width
                    height: content.height
                    source: file
                }
            }
        }
    }
}
