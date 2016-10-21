import QtQuick 2.0
import QtQuick.Controls 1.0
import QtQuick.Layouts 1.0

ColumnLayout {
    property alias kitName: label.text
    property alias flashFile: flashName.text
    property alias stateFile: stateName.text

    RowLayout {
        Label {
            id: label
            font.bold: true

            text: "Name"
        }

        /*Label {
            id: type
            Layout.fillWidth: true
            horizontalAlignment: Text.AlignRight
            Layout.leftMargin: 10

            text: "CX CAS (HW J)"
        }*/
    }

    RowLayout {
        Label {
            id: flashName
            text: "flash.img"
        }

        Label {
            id: stateName
            Layout.fillWidth: true
            horizontalAlignment: Text.AlignRight
            Layout.leftMargin: 10

            text: "state"
        }
    }
}
