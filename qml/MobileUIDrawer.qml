import QtQuick 2.0
import QtQuick.Dialogs 1.1
import QtQuick.Layouts 1.0

import Firebird.Emu 1.0
import Firebird.UIComponents 1.0

Rectangle {
    color: "white"

    function closeDrawer() {
        listView.closeDrawer();
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 5

        Image {
            id: logo
            source: "qrc:/icons/resources/firebird.png"

            Layout.maximumWidth: parent.width * 0.5
            Layout.maximumHeight: parent.width * 0.5
            anchors.horizontalCenter: parent.horizontalCenter
            fillMode: Image.PreserveAspectFit
            antialiasing: true
            smooth: true
        }

        ColumnLayout {
            Layout.fillWidth: true
            spacing: 0

            DrawerButton {
                 id: restartButton

                 title: qsTr("Start")
                 icon: "qrc:/icons/resources/icons/edit-bomb.png"

                 onClicked: {
                     Emu.useDefaultKit();
                     Emu.restart();
                     closeDrawer();
                 }
             }

             DrawerButton {
                 id: resetButton

                 disabled: !Emu.isRunning

                 borderTopVisible: false

                 title: qsTr("Reset")
                 icon: "qrc:/icons/resources/icons/system-reboot.png"

                 onClicked: {
                     Emu.reset();
                     closeDrawer();
                 }
             }

             DrawerButton {
                 id: resumeButton

                 borderTopVisible: false

                 title: qsTr("Resume")
                 icon: "qrc:/icons/resources/icons/system-suspend-hibernate.png"

                 onClicked: {
                     Emu.useDefaultKit();
                     Emu.resume()
                     closeDrawer();
                 }
             }

             DrawerButton {
                 id: saveButton

                 disabled: !Emu.isRunning

                 borderTopVisible: false

                 title: qsTr("Save")
                 icon: "qrc:/icons/resources/icons/media-floppy.png"

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
                             Emu.suspend();
                         else
                             snapWarnDialog.visible = true;
                     }

                     closeDrawer();
                }
            }

            Item {
                Layout.minimumHeight: 100
            }

            DrawerButton {
                id: configButton

                title: qsTr("Configuration")
                icon: "qrc:/icons/resources/icons/preferences-other.png"

                onClicked: listView.openConfiguration();
            }
        }

        Item {
            Layout.fillHeight: true
        }

        DrawerButton {
            id: aboutButton

            title: qsTr("Firebird Emu v1.2")

            MessageDialog {
                id: aboutDialog
                title: qsTr("About Firebird")
                text: qsTr("")
            }

            onClicked: {
                aboutDialog.visible = true;
            }
        }
    }
}
