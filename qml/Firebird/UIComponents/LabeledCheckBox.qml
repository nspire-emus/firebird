import QtQuick 2.0
import QtQuick.Controls 1.0
import QtQuick.Layouts 1.0

Item {
    property alias checked: checkBox.checked
    property alias text: label.text
    property int maxWidth: width

    implicitWidth: checkBox.implicitWidth + label.implicitWidth
    implicitHeight: row.implicitHeight

    MouseArea {
        anchors.fill: parent
        onClicked: checkBox.checked = !checkBox.checked
    }

    RowLayout {
        id: row
        spacing: Qt.platform.os === "windows" ? -5 : 0

        CheckBox {
            id: checkBox
        }

        FBLabel {
            id: label

            Layout.fillWidth: true
            Layout.maximumWidth: parent.parent.maxWidth - checkBox.width
            wrapMode: Text.WordWrap

            //Disabled for now, might be mistaken for a disabled control
            //color: checkBox.checked ? "" : "grey"
            text: ""
        }
    }
}
