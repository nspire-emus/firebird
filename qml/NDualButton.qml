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
        width: 8
        height: 21
        color: "#888"
        anchors.verticalCenterOffset: 5
        anchors.centerIn: parent
    }

    Rectangle {
        width: 8
        height: 18
        color: "#222233"
        anchors.verticalCenterOffset: 4
        anchors.centerIn: parent
        z: 1
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
        verticalAlignment: Text.AlignVCenter
        z: -1
        font.pixelSize: 8
        font.bold: true
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
        verticalAlignment: Text.AlignVCenter
        font.pixelSize: 8
        z: -2
        font.bold: true
    }
}

