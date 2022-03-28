import QtQuick 2.0
import QtQuick.Controls 1.0

/* A push button with a symbol instead of text.
 * ToolButton and <img/> in Label don't size correctly,
 * so do it manually.
 * With QQC2, button icons have a better default size
 * and it can also be specified explicitly. */

Button {
    property alias icon: image.source

    implicitHeight: TextMetrics.normalSize * 2.5
    implicitWidth: implicitHeight

    Image {
        id: image
        height: Math.round(parent.height * 0.6)
        anchors.centerIn: parent

        fillMode: Image.PreserveAspectFit
    }
}
