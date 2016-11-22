import QtQuick 2.0
import QtQuick.Controls 1.3
import QtQuick.Layouts 1.0
import Firebird.UIComponents 1.0

ToolButton {
    property alias icon: image.source
    property alias title: label.text

    Rectangle {
        anchors.fill: parent
        color: "#CCC"
        visible: parent.pressed
    }

    FBLabel {
        id: label
        x: 12
        anchors {
            top: parent.top
            left: parent.left
            right: parent.right
        }

        horizontalAlignment: Text.AlignHCenter
        fontSizeMode: Text.HorizontalFit
        font.pixelSize: TextMetrics.normalSize
        width: parent.width
    }

    Image {
        id: image
        anchors {
            top: label.bottom
            left: parent.left
            bottom: parent.bottom
            right: parent.right
        }

        fillMode: Image.PreserveAspectFit
    }
}
