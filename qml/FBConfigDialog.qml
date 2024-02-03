import QtQuick 2.0
import QtQuick.Window 2.2
import QtQuick.Controls 2.0
import QtQuick.Layouts 1.0
import Firebird.UIComponents 1.0

Window {
    id: window
    title: qsTr("Firebird Emu Configuration")
    minimumHeight: 400
    minimumWidth: 500
    height: 420
    width: 540

    SystemPalette {
        id: paletteActive
    }

    color: paletteActive.window

    ConfigPages {
        anchors {
            bottom: actionRow.top
            right: parent.right
            left: parent.left
            top: parent.top
            margins: 5
        }
        model: ConfigPagesModel {
        }
    }

    RowLayout {
        id: actionRow

        anchors {
            left: parent.left
            bottom: parent.bottom
            right: parent.right
            margins: 5
            topMargin: 0
        }

        FBLabel {
            Layout.fillWidth: true
            text: qsTr("Changes are saved automatically")
            font.italic: true
            color: "grey"
        }

        Button {
            text: qsTr("Ok")
            onClicked: window.visible = false
        }
    }
}
