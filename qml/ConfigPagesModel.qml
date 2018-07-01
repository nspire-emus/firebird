import QtQuick 2.0

ListModel {
    ListElement {
        title: QT_TR_NOOP("Flash & Boot1")
        icon: "qrc:/icons/resources/icons/media-flash.png"
        file: "qrc:/qml/qml/ConfigPageKits.qml"
    }
    ListElement {
        title: QT_TR_NOOP("Emulation")
        icon: "qrc:/icons/resources/icons/flash-create.png"
        file: "qrc:/qml/qml/ConfigPageEmulation.qml"
    }
    ListElement {
        title: QT_TR_NOOP("File Transfer")
        icon: "qrc:/icons/resources/icons/drive-removable-media-usb.png"
        file: "qrc:/qml/qml/ConfigPageFileTransfer.qml"
    }
    ListElement {
        title: QT_TR_NOOP("Debugging")
        icon: "qrc:/icons/resources/icons/tools-report-bug.png"
        file: "qrc:/qml/qml/ConfigPageDebug.qml"
    }
}
