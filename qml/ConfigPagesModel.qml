import QtQuick 2.0

ListModel {
    ListElement {
        title: qsTr("Flash & Boot1")
        icon: "qrc:/icons/resources/icons/media-flash.png"
        file: "ConfigPageKits.qml"
    }
    ListElement {
        title: qsTr("Emulation")
        icon: "qrc:/icons/resources/icons/flash-create.png"
        file: "ConfigPageEmulation.qml"
    }
    ListElement {
        title: qsTr("File Transfer")
        icon: "qrc:/icons/resources/icons/drive-removable-media-usb.png"
        file: "ConfigPageFileTransfer.qml"
    }
    ListElement {
        title: qsTr("Debugging")
        icon: "qrc:/icons/resources/icons/tools-report-bug.png"
        file: "ConfigPageDebug.qml"
    }
}
