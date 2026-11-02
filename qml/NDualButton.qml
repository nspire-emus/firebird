import QtQuick 2.0
import QtQuick.Shapes 1.15

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
    property alias svg1: nbutton1.svg
    property alias svg2: nbutton2.svg
    property alias topSvg1: svgTop1.svg
    property alias topSvg2: svgTop2.svg
    property alias colorSvg1: nbutton1.colorSvg
    property alias colorSvg2: nbutton2.colorSvg
    property alias colorTopSvg1: svgTop1.colorSvg
    property alias colorTopSvg2: svgTop2.colorSvg

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

        colorSvg: "blue"
        svg:"M 2.13e-5,-32.828401 C -18.090394,-32.828371 -32.828751,-18.09081 -32.828751,-3.4049999e-4 -32.828711,18.090021 -18.090394,32.828401 2.13e-5,32.828401 18.090394,32.828401 32.828711,18.090057 32.828751,-3.4049999e-4 32.828751,-18.09081 18.090394,-32.828401 2.13e-5,-32.828401 Z"
    }

    NButton {
        id: nbutton2
        keymap_id: 2
        x: 25
        y: 10
        width: 25
        height: 20
        text: "b"
        colorSvg: "red"
        svg:"M 2.13e-5,-32.828401 C -18.090394,-32.828371 -32.828751,-18.09081 -32.828751,-3.4049999e-4 -32.828711,18.090021 -18.090394,32.828401 2.13e-5,32.828401 18.090394,32.828401 32.828711,18.090057 32.828751,-3.4049999e-4 32.828751,-18.09081 18.090394,-32.828401 2.13e-5,-32.828401 Z"

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
        verticalAlignment: Text.AlignVCenter
        z: -1
        font.pixelSize: 8
        font.bold: true
        NSvg{
            id: svgTop1
            colorSvg: "green"
            svg:"M 2.13e-5,-32.828401 C -18.090394,-32.828371 -32.828751,-18.09081 -32.828751,-3.4049999e-4 -32.828711,18.090021 -18.090394,32.828401 2.13e-5,32.828401 18.090394,32.828401 32.828711,18.090057 32.828751,-3.4049999e-4 32.828751,-18.09081 18.090394,-32.828401 2.13e-5,-32.828401 Z"
        }
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
        NSvg{
            id: svgTop2
            colorSvg: "pink"
            svg:"M 2.13e-5,-32.828401 C -18.090394,-32.828371 -32.828751,-18.09081 -32.828751,-3.4049999e-4 -32.828711,18.090021 -18.090394,32.828401 2.13e-5,32.828401 18.090394,32.828401 32.828711,18.090057 32.828751,-3.4049999e-4 32.828751,-18.09081 18.090394,-32.828401 2.13e-5,-32.828401 Z"
        }
    }
}

