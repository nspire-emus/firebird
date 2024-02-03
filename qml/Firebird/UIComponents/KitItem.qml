import QtQuick 2.0
import QtQuick.Controls 2.0
import QtQuick.Layouts 1.0

ColumnLayout {
    property alias kitName: label.text
    property alias flashFile: flashName.text
    property alias stateFile: stateName.text

    RowLayout {
        Layout.maximumWidth: parent.width

        FBLabel {
            id: label
            font.bold: true
            elide: Text.ElideMiddle

            text: "Name"

            Layout.maximumWidth: parent.width
        }

        /*FBLabel {
            id: type
            Layout.fillWidth: true
            horizontalAlignment: Text.AlignRight
            Layout.leftMargin: 10

            text: "CX CAS (HW J)"
        }*/
    }

    RowLayout {
        FBLabel {
            id: flashName
            text: "flash.img"
        }

        FBLabel {
            id: stateName
            Layout.fillWidth: true
            horizontalAlignment: Text.AlignRight
            Layout.leftMargin: 10

            text: "state"
        }
    }
}
