import QtQuick 2.0

ListModel {
    ListElement {
        title: qsTr("Flash & Boot1")
        icon: "qrc:/icons/resources/icons/media-flash.png"
        file: "qrc:/qml/qml/ConfigPageKits.qml"
    }
    ListElement {
        title: qsTr("Emulation")
        icon: "qrc:/icons/resources/icons/flash-create.png"
        file: "qrc:/qml/qml/ConfigPageEmulation.qml"
    }
    ListElement {
        title: qsTr("File Transfer")
        icon: "qrc:/icons/resources/icons/drive-removable-media-usb.png"
        file: "qrc:/qml/qml/ConfigPageFileTransfer.qml"
    }
    ListElement {
        title: qsTr("Debugging")
        icon: "qrc:/icons/resources/icons/tools-report-bug.png"
        file: "qrc:/qml/qml/ConfigPageDebug.qml"
    }
}
