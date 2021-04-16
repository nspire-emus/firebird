import QtQuick 2.0
import QtQuick.Controls 2.0

FBLabel {
    signal clicked

    MouseArea {
        id: mouseArea
        anchors.fill: parent
        hoverEnabled: true
        cursorShape: Qt.PointingHandCursor
        acceptedButtons: Qt.LeftButton | Qt.RightButton
        onClicked: parent.clicked()
    }
    
    color: "white"
    font.bold: true
}
