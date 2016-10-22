import QtQuick 2.0

ListModel {
    ListElement {
        title: qsTr("Flash & Boot1")
        icon: "qrc:/icons/resources/icons/media-floppy.png"
        file: "qrc:/qml/qml/ConfigPageKits.qml"
    }
    ListElement {
        title: qsTr("Debugging")
        icon: "qrc:/icons/resources/icons/tools-report-bug.png"
        file: "qrc:/qml/qml/ConfigPageDebug.qml"
    }
    ListElement {
        title: qsTr("Emulation")
        icon: "qrc:/icons/resources/icons/flash-create.png"
        file: "qrc:/qml/qml/ConfigPageDebug.qml"
    }
}
