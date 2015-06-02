import QtQuick 2.0

Rectangle {
    id: rectangle2
    width: 100
    height: 62
    color: "#222222"
    radius: 10
    border.width: 2
    border.color: "#eeeeee"

    Rectangle {
        id: rectangle1
        x: 29
        y: 18
        width: 25
        height: 25
        color: "#00000000"
        radius: 5
        border.color: "#ffffff"
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.verticalCenter: parent.verticalCenter
    }
}

