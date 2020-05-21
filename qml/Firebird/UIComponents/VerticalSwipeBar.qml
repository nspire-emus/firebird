import QtQuick 2.0

Rectangle {
    property alias text: label.text
    signal clicked

    implicitWidth: label.contentHeight + 2 * 2
    color: "transparent"

    FBLabel {
        id: label
        rotation: -90
        anchors.centerIn: parent
        anchors.rightMargin: 2
        text: qsTr("Swipe here")
    }

    MouseArea {
        property point orig;
        anchors.fill: parent

        onPressed: {
            orig.x = mouse.x;
            orig.y = mouse.y;
        }

        onReleased: {
            if(Math.abs(orig.x - mouse.x) < 5
               && Math.abs(orig.y - mouse.y) < 5)
            parent.clicked();
        }
    }
}
