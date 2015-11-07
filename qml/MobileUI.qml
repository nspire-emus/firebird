import QtQuick 2.0
import QtQuick.Layouts 1.0
import QtQuick.Dialogs 1.1
import Ndless.Emu 1.0
import QtQuick.Controls 1.3

Rectangle {
    id: rectangle1
    width: 320
    height: 480
    color: "#AAA"

    Component.onCompleted: {
        if(Emu.getFlashPath() !== ""
            && Emu.getSnapshotPath() !== ""
            && Emu.getBoot1Path() !== "")
            Emu.resume();
    }

    ColumnLayout {
        id: sidebar
        // In landscape mode fit whole framebuffer on screen
        width: parent.width/350 > parent.height/240 ? parent.width-320*parent.height/240 : parent.width*0.15
        anchors.bottom: controls.top
        anchors.bottomMargin: 0
        anchors.top: parent.top
        anchors.topMargin: 0
        anchors.left: screen.right
        anchors.leftMargin: 0

        ToolButton {
            id: restartButton
            anchors.horizontalCenter: parent.horizontalCenter
            Layout.fillWidth: true
            Layout.fillHeight: true

            Label {
                id: label1
                x: 12
                text: qsTr("Start")
                anchors.top: parent.top
                anchors.topMargin: 0
                anchors.horizontalCenter: parent.horizontalCenter
                font.pointSize: 8
            }

            Image {
                anchors.top: label1.bottom
                anchors.topMargin: -2
                anchors.left: parent.left
                anchors.leftMargin: 0
                anchors.bottom: parent.bottom
                anchors.bottomMargin: 0
                anchors.right: parent.right
                anchors.rightMargin: 0
                fillMode: Image.PreserveAspectFit
                anchors.margins: 3
                source: "qrc:/icons/resources/icons/edit-bomb.png"
            }


            onClicked: Emu.restart();
        }

        ToolButton {
            id: resetButton
            anchors.horizontalCenter: parent.horizontalCenter
            Layout.fillWidth: true
            Layout.fillHeight: true

            Label {
                id: label2
                x: 12
                text: qsTr("Reset")
                anchors.top: parent.top
                anchors.topMargin: 0
                anchors.horizontalCenter: parent.horizontalCenter
                font.pointSize: 8
            }

            Image {
                anchors.top: label2.bottom
                anchors.topMargin: -2
                anchors.left: parent.left
                anchors.leftMargin: 0
                anchors.bottom: parent.bottom
                anchors.bottomMargin: 0
                anchors.right: parent.right
                anchors.rightMargin: 0
                fillMode: Image.PreserveAspectFit
                anchors.margins: 3
                source: "qrc:/icons/resources/icons/system-reboot.png"
            }

            onClicked: Emu.reset();
        }

        ToolButton {
            id: resumeButton
            anchors.horizontalCenter: parent.horizontalCenter
            Layout.fillWidth: true
            Layout.fillHeight: true

            Label {
                id: label3
                x: 12
                text: qsTr("Resume")
                anchors.top: parent.top
                anchors.topMargin: 0
                anchors.horizontalCenter: parent.horizontalCenter
                font.pointSize: 8
            }

            Image {
                anchors.top: label3.bottom
                anchors.topMargin: -2
                anchors.left: parent.left
                anchors.leftMargin: 0
                anchors.bottom: parent.bottom
                anchors.bottomMargin: 0
                anchors.right: parent.right
                anchors.rightMargin: 0
                fillMode: Image.PreserveAspectFit
                anchors.margins: 3
                source: "qrc:/icons/resources/icons/system-suspend-hibernate.png"
            }

            onClicked: Emu.resume()
        }

        ToolButton {
            id: saveButton
            anchors.horizontalCenter: parent.horizontalCenter
            Layout.fillWidth: true
            Layout.fillHeight: true

            Label {
                id: label4
                x: 12
                text: qsTr("Save")
                anchors.top: parent.top
                anchors.topMargin: 0
                anchors.horizontalCenter: parent.horizontalCenter
                font.pointSize: 8
            }

            Image {
                anchors.top: label4.bottom
                anchors.topMargin: -2
                anchors.left: parent.left
                anchors.leftMargin: 0
                anchors.bottom: parent.bottom
                anchors.bottomMargin: 0
                anchors.right: parent.right
                anchors.rightMargin: 0
                fillMode: Image.PreserveAspectFit
                anchors.margins: 3
                source: "qrc:/icons/resources/icons/media-floppy.png"
            }

            MessageDialog {
                id: saveSuccessDialog
                title: qsTr("Success")
                text: qsTr("Flash saved.")
                icon: StandardIcon.Information
            }

            MessageDialog {
                id: saveFailedDialog
                title: qsTr("Error")
                text: qsTr("Failed to save changes!")
                icon: StandardIcon.Warning
            }

            MessageDialog {
                id: snapWarnDialog
                title: qsTr("Warning")
                text: qsTr("Flash saved, but no snapshot location configured.\nYou won't be able to resume.")
                icon: StandardIcon.Warning
            }

            onClicked: {
                var flash_path = Emu.getFlashPath();
                var snap_path = Emu.getSnapshotPath();

                if(flash_path === "" || !Emu.saveFlash())
                    saveFailedDialog.visible = true;
                else
                {
                    if(snap_path)
                    {
                        Emu.suspend();
                        saveSuccessDialog.visible = true;
                    }
                    else
                        snapWarnDialog.visible = true;
                }
            }
        }

    }

    EmuScreen {
        id: screen
        y: 0
        width: parent.width - sidebar.width
        height: width/320*240
        anchors.left: parent.left
        anchors.leftMargin: 0

        Timer {
            interval: 20
            running: true; repeat: true
            onTriggered: screen.update()
        }
    }

    Flickable {
        id: controls
        boundsBehavior: Flickable.StopAtBounds
        flickableDirection: Flickable.HorizontalAndVerticalFlick
        contentWidth: controlsRow.width
        contentHeight: controlsRow.height
        clip: true
        anchors.top: screen.bottom
        anchors.topMargin: 0
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 0
        anchors.right: parent.right
        anchors.rightMargin: 0
        anchors.left: parent.left
        anchors.leftMargin: 0

        Row {
            id: controlsRow

            Rectangle {
                height: keypad.height*controls.width/keypad.width
                width: controls.width

                Keypad {
                    id: keypad
                    transform: Scale { origin.x: 0; origin.y: 0; xScale: controls.width/keypad.width; yScale: controls.width/keypad.width }
                }
            }

            Rectangle {
                height: keypad.height*controls.width/keypad.width
                width: controls.width
                color: keypad.color

                MobileControl2 {
                    id: control2
                    anchors.left: parent.left
                    anchors.right: parent.right
                    //transform: Scale { origin.x: 0; origin.y: 0; xScale: controls.width/keypad.width; yScale: controls.width/keypad.width }
                }
            }
        }
    }
}

