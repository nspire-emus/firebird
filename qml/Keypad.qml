import QtQuick 2.0
import QtQuick.Layouts 1.0

Rectangle {
    id: rectangle1
    width: 265
    height: 340

    color: "#444"

    ColumnLayout {
        id: columnLayout1
        width: 33
        anchors.bottom: rectangle3.bottom
        anchors.bottomMargin: 0
        z: 4
        anchors.left: parent.left
        anchors.leftMargin: 5
        anchors.top: rectangle3.top
        anchors.topMargin: 0

        NBigButton {
            id: nButton1
            anchors.horizontalCenter: parent.horizontalCenter
            text: "esc"
            Layout.fillWidth: true
            keymap_id: 73
        }

        NBigButton {
            id: nButton2
            anchors.horizontalCenter: parent.horizontalCenter
            text: "pad"
            Layout.fillWidth: true
            keymap_id: 65

            Text {
                id: text6
                x: 0
                y: -11
                width: 33
                height: 11
                color: "#68cce0"
                text: "save"
                horizontalAlignment: Text.AlignHCenter
                font.pixelSize: 9
                anchors.bottomMargin: 0
                font.bold: false
                verticalAlignment: Text.AlignVCenter
                anchors.bottom: parent.top
            }
        }

        NBigButton {
            id: nButton3
            anchors.horizontalCenter: parent.horizontalCenter
            text: "tab"
            Layout.fillWidth: true
            keymap_id: 75
        }
    }

    ColumnLayout {
        id: columnLayout2
        x: -7
        width: 33
        anchors.bottom: rectangle3.bottom
        anchors.bottomMargin: 0
        z: 3
        anchors.top: rectangle3.top
        anchors.topMargin: 0
        anchors.right: parent.right
        anchors.rightMargin: 5
        NBigButton {
            id: nButton4
            anchors.horizontalCenter: parent.horizontalCenter
            text: "⌂on"
            Layout.fillWidth: true
            keymap_id: 9
        }

        NBigButton {
            id: nButton5
            anchors.horizontalCenter: parent.horizontalCenter
            text: "doc"
            Layout.fillWidth: true
            keymap_id: 69

            Text {
                id: text7
                x: 0
                y: -11
                width: 33
                height: 11
                color: "#68cce0"
                text: "+page"
                horizontalAlignment: Text.AlignHCenter
                font.pixelSize: 9
                anchors.bottomMargin: 0
                font.bold: false
                verticalAlignment: Text.AlignVCenter
                anchors.bottom: parent.top
            }
        }

        NBigButton {
            id: nButton6
            width: 30
            anchors.horizontalCenter: parent.horizontalCenter
            text: "menu"
            Layout.fillWidth: true
            keymap_id: 71

            Text {
                id: text8
                x: 0
                y: -11
                width: 33
                height: 11
                color: "#68cce0"
                text: "▤▸"
                horizontalAlignment: Text.AlignHCenter
                font.pixelSize: 10
                anchors.bottomMargin: 0
                font.bold: false
                verticalAlignment: Text.AlignVCenter
                anchors.bottom: parent.top
            }
        }
    }

    Touchpad {
        id: touchpad1
        width: gridLayout1.width
        border.color: "#abc2c7"
        anchors.bottom: rectangle3.bottom
        anchors.bottomMargin: 6
        z: 2
        anchors.horizontalCenter: gridLayout1.horizontalCenter
        anchors.top: parent.top
        anchors.topMargin: 6
    }

    GridLayout {
        id: gridLayout1
        rowSpacing: 10
        rows: 5
        columns: 3
        columnSpacing: 10
        antialiasing: false
        transformOrigin: Item.Center
        anchors.right: nDualButton7.left
        anchors.rightMargin: 18
        anchors.left: nDualButton1.right
        anchors.leftMargin: 18
        anchors.bottom: nButton8.bottom
        anchors.bottomMargin: 0
        anchors.top: rectangle3.bottom
        anchors.topMargin: 10

        NBigButton {
            id: nButton11
            width: 33
            text: "shift"
            keymap_id: 85

            Text {
                id: text5
                x: 0
                y: -11
                width: 33
                height: 11
                color: "#68cce0"
                text: "CAPS"
                anchors.bottom: parent.top
                horizontalAlignment: Text.AlignHCenter
                font.pixelSize: 10
                anchors.bottomMargin: 0
                font.bold: false
                verticalAlignment: Text.AlignVCenter
            }
        }

        Rectangle {
            id: rectangle2
            width: 30
            height: 20
            color: "#00000000"
        }

        NBigButton {
            id: nButton12
            width: 33
            text: "var"
            Layout.preferredWidth: -1
            Layout.fillWidth: false
            border.width: 1
            clip: false
            keymap_id: 56

            Text {
                id: text4
                x: 0
                y: -11
                width: 33
                height: 11
                color: "#68cce0"
                text: "sto→"
                anchors.bottom: parent.top
                horizontalAlignment: Text.AlignHCenter
                font.pixelSize: 10
                anchors.bottomMargin: 0
                font.bold: false
                verticalAlignment: Text.AlignVCenter
            }
        }

        NNumButton {
            id: nNumButton1
            text: "7"
            keymap_id: 40
        }

        NNumButton {
            id: nNumButton2
            text: "8"
            keymap_id: 72
        }

        NNumButton {
            id: nNumButton3
            text: "9"
            Layout.alignment: Qt.AlignRight | Qt.AlignVCenter
            keymap_id: 36
        }

        NNumButton {
            id: nNumButton4
            text: "4"
            keymap_id: 29
        }

        NNumButton {
            id: nNumButton5
            text: "5"
            keymap_id: 61
        }

        NNumButton {
            id: nNumButton6
            text: "6"
            keymap_id: 25
        }

        NNumButton {
            id: nNumButton7
            text: "1"
            keymap_id: 18
        }

        NNumButton {
            id: nNumButton8
            text: "2"
            keymap_id: 70
        }

        NNumButton {
            id: nNumButton9
            text: "3"
            keymap_id: 14
        }

        NNumButton {
            id: nNumButton10
            text: "0"
            keymap_id: 7
        }

        NNumButton {
            id: nNumButton11
            text: "."
            keymap_id: 59

            Text {
                id: text3
                x: 0
                y: -11
                width: 33
                height: 11
                color: "#68cce0"
                text: "capture"
                anchors.bottom: parent.top
                horizontalAlignment: Text.AlignHCenter
                font.pixelSize: 9
                anchors.bottomMargin: 0
                font.bold: false
                verticalAlignment: Text.AlignVCenter
            }
        }

        NNumButton {
            id: nNumButton12
            text: "(-)"
            keymap_id: 3

            Text {
                id: text2
                x: 0
                y: -11
                width: 33
                height: 11
                color: "#68cce0"
                text: "ans"
                anchors.bottom: parent.top
                horizontalAlignment: Text.AlignHCenter
                font.pixelSize: 9
                anchors.bottomMargin: 0
                font.bold: false
                verticalAlignment: Text.AlignVCenter
            }
        }
    }

    NBigButton {
        id: nButton7
        width: 33
        color: "#68cce0"
        text: "ctrl"
        anchors.top: gridLayout1.top
        anchors.topMargin: 0
        keymap_id: 86
        anchors.left: parent.left
        anchors.leftMargin: 5
        active_color: "#cce"
        back_color: "#bbf"
        active: false
        font_color: "#222"
    }

    NBigButton {
        id: nButton10
        x: 230
        width: 33
        text: "del"
        anchors.top: gridLayout1.top
        anchors.topMargin: 0
        keymap_id: 64
        anchors.right: parent.right
        anchors.rightMargin: 5

        Text {
            id: text9
            x: 0
            y: -11
            width: 33
            height: 11
            color: "#68cce0"
            text: "clear"
            horizontalAlignment: Text.AlignHCenter
            font.pixelSize: 10
            anchors.bottomMargin: 0
            font.bold: false
            verticalAlignment: Text.AlignVCenter
            anchors.bottom: parent.top
        }
    }

    GridLayout {
        id: gridLayout2
        anchors.top: gridLayout1.bottom
        anchors.topMargin: 11
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 4
        anchors.left: nDualButton4.left
        anchors.leftMargin: 0
        anchors.right: nButton8.right
        anchors.rightMargin: 0
        rowSpacing: 0
        columnSpacing: 10
        columns: 9

        NAlphaButton { width: 21; text: "EE" ;keymap_id: 30 }
        NAlphaButton { text: "A" ;keymap_id: 50 }
        NAlphaButton { text: "B" ;keymap_id: 49 }
        NAlphaButton { text: "C" ;keymap_id: 48 }
        NAlphaButton { text: "D" ;keymap_id: 46 }
        NAlphaButton { text: "E" ;keymap_id: 45 }
        NAlphaButton { text: "F" ;keymap_id: 44 }
        NAlphaButton { text: "G" ;keymap_id: 39 }
        NAlphaButton { width: 21; text: "?!▸";keymap_id: 8 }
        NAlphaButton { width: 21; text: "π▸";keymap_id: 19 }
        NAlphaButton { text: "H" ;keymap_id: 38 }
        NAlphaButton { text: "I" ;keymap_id: 37 }
        NAlphaButton { text: "J" ;keymap_id: 35 }
        NAlphaButton { text: "K" ;keymap_id: 34 }
        NAlphaButton { text: "L" ;keymap_id: 33 }
        NAlphaButton { text: "M" ;keymap_id: 28 }
        NAlphaButton { text: "N" ;keymap_id: 27 }
        NAlphaButton { width: 21; text: " ⚑";keymap_id: 66 }
        NAlphaButton { width: 21; text: "," ;keymap_id: 87 }
        NAlphaButton { text: "O" ;keymap_id: 26 }
        NAlphaButton { text: "P" ;keymap_id: 24 }
        NAlphaButton { text: "Q" ;keymap_id: 23 }
        NAlphaButton { text: "R" ;keymap_id: 22 }
        NAlphaButton { text: "S" ;keymap_id: 17 }
        NAlphaButton { id: nAlphaButtonT; text: "T" ;keymap_id: 16 }
        NAlphaButton { id: nAlphaButtonU; text: "U" ;keymap_id: 15 }
        NAlphaButton { width: 21; text: "↵";keymap_id: 0 }
        Rectangle {
            color: "#00000000"
            width: 15
            height: 15
        }
        NAlphaButton { text: "V" ;keymap_id: 13 }
        NAlphaButton { text: "W" ;keymap_id: 12 }
        NAlphaButton { text: "X" ;keymap_id: 11 }
        NAlphaButton { text: "Y" ;keymap_id: 6 }
        NAlphaButton { text: "Z" ;keymap_id: 5 }

        NAlphaButton {
            text: "space"
            keymap_id: 4
            anchors.right: nAlphaButtonU.right
            anchors.rightMargin: 0
            anchors.left: nAlphaButtonT.left
            anchors.leftMargin: 0
        }
    }

    NDualButton {
        id: nDualButton1
        x: 5
        anchors.top: nButton7.bottom
        anchors.topMargin: 0
        topText1: "|≠≥▸"
        topText2: "(?)"
        id1: 51
        id2: 20
        anchors.left: parent.left
        anchors.leftMargin: 5
        text2: "trig"
        text1: "="

    }

    NDualButton {
        id: nDualButton2
        x: 5
        topText1: "ⁿ√x"
        topText2: "√"
        anchors.top: nDualButton1.bottom
        anchors.topMargin: 0
        id1: 53
        id2: 31
        anchors.left: parent.left
        anchors.leftMargin: 5
        text2: "x²"
        text1: "^"
    }

    NDualButton {
        id: nDualButton3
        x: 5
        topText2: "log"
        topText1: "ln"
        anchors.top: nDualButton2.bottom
        anchors.topMargin: 0
        id1: 42
        id2: 21
        anchors.left: parent.left
        anchors.leftMargin: 5
        text2: "10˟"
        text1: "e˟"
    }

    NDualButton {
        id: nDualButton4
        topText2: "{ }"
        topText1: "[ ]"
        anchors.top: nDualButton3.bottom
        anchors.topMargin: 0
        id1: 60
        id2: 58
        anchors.left: parent.left
        anchors.leftMargin: 5
        text2: ")"
        text1: "("
    }

    NDualButton {
        id: nDualButton5
        x: 210
        anchors.top: nButton10.bottom
        anchors.topMargin: 0
        topText2: "∞ß°"
        topText1: ":="
        id1: 63
        id2: 62
        anchors.right: parent.right
        anchors.rightMargin: 5
        text2: "❘◫❘"
        text1: "❘□❘{□"
    }

    NDualButton {
        id: nDualButton6
        x: 210
        topText2: "÷"
        topText1: "\"□\""
        anchors.top: nDualButton5.bottom
        anchors.topMargin: 0
        id1: 52
        id2: 41
        anchors.right: parent.right
        anchors.rightMargin: 5
        text2: "÷"
        text1: "×"
    }

    NDualButton {
        id: nDualButton7
        x: 210
        topText2: "◂◑"
        topText1: "◐‣"
        anchors.top: nDualButton6.bottom
        anchors.topMargin: 0
        id1: 68
        id2: 57
        anchors.right: parent.right
        anchors.rightMargin: 5
        text2: "‒"
        text1: "+"
    }

    NButton {
        id: nButton8
        x: 210
        anchors.right: parent.right
        anchors.rightMargin: 5
        width: 50
        height: 20
        text: "enter"
        anchors.top: nDualButton7.bottom
        anchors.topMargin: 10

        Text {
            id: text1
            x: 0
            y: -12
            width: 50
            height: 11
            color: "#68cce0"
            text: "≈"
            font.bold: false
            anchors.bottom: nButton8.top
            anchors.bottomMargin: 1
            verticalAlignment: Text.AlignVCenter
            horizontalAlignment: Text.AlignHCenter
            font.pixelSize: 17
        }
    }

    Rectangle {
        id: rectangle3
        height: 90
        color: "#111111"
        anchors.top: parent.top
        anchors.topMargin: 0
        anchors.left: parent.left
        anchors.leftMargin: 0
        anchors.right: parent.right
        anchors.rightMargin: 0
        z: 1
    }
}

