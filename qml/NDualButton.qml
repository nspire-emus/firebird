import QtQuick 2.0

Rectangle {
    id: rectangle3
    property alias text1: nbutton1.text
    property alias text2: nbutton2.text
    property alias topText1: textTop1.text
    property alias topText2: textTop2.text
    property alias button1: nbutton1
    property alias button2: nbutton2
    property alias id1: nbutton1.keymap_id
    property alias id2: nbutton2.keymap_id

    width: 50
    height: 30
    color: "#00000000"

    NButton {
        id: nbutton1
        x: 0
        y: 10
        keymap_id: 1
        width: 25
        height: 20
        text: "a"
    }

    NButton {
        id: nbutton2
        keymap_id: 2
        x: 25
        y: 10
        width: 25
        height: 20
        text: "b"
    }

    Rectangle {
        id: rectangle1
        width: 8
        height: 20
        color: "#222233"
        radius: 0
        anchors.verticalCenterOffset: 5
        anchors.horizontalCenterOffset: 0
        z: 0
        anchors.verticalCenter: parent.verticalCenter
        anchors.horizontalCenter: parent.horizontalCenter
        border.width: 1
        border.color: "#888"
    }

    Rectangle {
        id: rectangle2
        width: 8
        height: 18
        color: "#222233"
        radius: 0
        anchors.verticalCenterOffset: 5
        anchors.horizontalCenterOffset: 0
        z: 1
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.verticalCenter: parent.verticalCenter
    }

    Text {
        id: textTop1
        x: 0
        y: -2
        width: 25
        height: 13
        color: "#68cce0"
        text: "aa"
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignBottom
        z: -1
        font.pixelSize: 10
    }

    Text {
        id: textTop2
        x: 25
        y: -2
        width: 25
        height: 13
        color: "#68cce0"
        text: "bb"
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignBottom
        font.pixelSize: 10
        z: -2
    }
}

