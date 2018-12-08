import QtQuick 2.0

Rectangle {
    property alias currentItem: listView.currentItem
    property alias currentIndex: listView.currentIndex
    property alias model: listView.model

    SystemPalette {
        id: paletteActive
    }

    color: paletteActive.base
    border {
        color: paletteActive.alternateBase
        width: 1
    }

    Item {
        anchors.margins: parent.border.width
        anchors.fill: parent
        clip: true

        ListView {
            id: listView

            anchors.centerIn: parent
            anchors.fill: parent
            anchors.topMargin: width * 0.05
            anchors.bottomMargin: width * 0.05
            focus: true
            highlightMoveDuration: 150

            highlight: Rectangle {
                color: paletteActive.highlight
                anchors.horizontalCenter: parent.horizontalCenter
            }

            delegate: PageDelegate {
                text: qsTranslate("ConfigPagesModel", title)
                iconSource: icon
            }
        }
    }
}
