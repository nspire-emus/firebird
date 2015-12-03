import QtQuick 2.0
import QtQuick.Layouts 1.0
import QtQuick.Controls 1.3
import QtQuick.Window 2.2
import QtQuick.Dialogs 1.0
import Ndless.Emu 1.0

Rectangle {    
    id: rectangle1
    width: 265
    height: 240

    color: "#444"

    Label {
        id: title
        x: 8
        y: 8
        color: "#f0f0f0"
        text: qsTr("Settings")
        font.pointSize: 11*Screen.logicalPixelDensity/2
    }

    FileDialog {
        id: fileDialogBoot1
        visible: false
        title: qsTr("Select Boot1")

        onAccepted: {
            Emu.setBoot1Path(fileDialogBoot1.fileUrl)
            boot1Name.text = Emu.basename(fileDialogBoot1.fileUrl)
        }
    }

    FileDialog {
        id: fileDialogFlash
        visible: false
        title: qsTr("Select Flash")

        onAccepted: {
            Emu.setFlashPath(fileDialogFlash.fileUrl);
            flashName.text = Emu.basename(fileDialogFlash.fileUrl);
        }
    }

    FileDialog {
        id: fileDialogSnapshot
        visible: false
        selectExisting: false
        title: qsTr("Select Snapshot Location")

        onAccepted: {
            Emu.setSnapshotPath(fileDialogSnapshot.fileUrl);
            snapshotName.text = Emu.basename(fileDialogSnapshot.fileUrl);
        }
    }

    GridLayout {
        id: gridLayout1
        rows: 2
        columns: 3
        anchors.right: parent.right
        anchors.rightMargin: 8
        anchors.left: parent.left
        anchors.leftMargin: 8
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 103
        anchors.top: title.bottom
        anchors.topMargin: 6

        Label {
            id: label2
            color: title.color
            text: qsTr("Boot1:")
        }

        Label {
            id: boot1Name
            color: title.color
            text: qsTr("None")

            Component.onCompleted: text = Emu.basename(Emu.getBoot1Path())
        }

        Button {
            id: button1
            text: qsTr("Select")
            anchors.right: parent.right
            anchors.rightMargin: 0

            onClicked: fileDialogBoot1.visible = true
        }

        Label {
            id: label1
            color: title.color
            text: qsTr("Flash:")
        }

        Label {
            id: flashName
            color: title.color
            text: qsTr("None")

            Component.onCompleted: text = Emu.basename(Emu.getFlashPath())
        }

        Button {
            id: button2
            text: qsTr("Select")
            anchors.right: parent.right
            anchors.rightMargin: 0

            onClicked: fileDialogFlash.visible = true
        }

        Label {
            id: label3
            color: title.color
            text: qsTr("Snapshot\nlocation:")
        }

        Label {
            id: snapshotName
            color: title.color
            text: qsTr("None")

            Component.onCompleted: text = Emu.basename(Emu.getSnapshotPath())
        }

        Button {
            id: button3
            text: qsTr("Select")
            anchors.right: parent.right
            anchors.rightMargin: 0

            onClicked: fileDialogSnapshot.visible = true
        }
    }
}

