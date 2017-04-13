import QtQuick 2.0
import QtQuick.Controls 1.0
import QtQuick.Layouts 1.0

Item {
    property alias checked: checkBox.checked
    property alias text: label.text
    property int maxWidth: width

    implicitWidth: row.width
    implicitHeight: row.height

    MouseArea {
        anchors.fill: parent
        onClicked: checkBox.checked = !checkBox.checked
    }

    RowLayout {
        id: row
        spacing: 0

        CheckBox {
            id: checkBox
        }

        FBLabel {
            id: label
            anchors {
                left: checkBox.right
                leftMargin: Qt.platform.os === "windows" ? -5 : 0;
            }

            Layout.fillWidth: true
            Layout.maximumWidth: parent.parent.maxWidth - checkBox.width
            wrapMode: Text.WordWrap

            //Disabled for now, might be mistaken for a disabled control
            //color: checkBox.checked ? "" : "grey"
            text: ""
        }
    }
}
