pragma Singleton
import QtQuick 2.0

Item {
    property int normalSize: defaultFont.font.pixelSize
    property int title1Size: defaultFont.font.pixelSize * 1.2
    property int title2Size: defaultFont.font.pixelSize * 1.4

    Text {
        id: defaultFont
    }
}
