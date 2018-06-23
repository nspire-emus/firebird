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
            source: "qrc:/icons/resources/org.firebird-emus.firebird-emu.png"

            Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter
            Layout.fillWidth: true
            Layout.maximumWidth: parent.width * 0.5
            Layout.maximumHeight: parent.width * 0.5
            fillMode: Image.PreserveAspectFit
            antialiasing: true
            smooth: true
        }

        ColumnLayout {
            Layout.fillWidth: true
            spacing: -1 // Collapse borders

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

                 title: qsTr("Reset")
                 icon: "qrc:/icons/resources/icons/system-reboot.png"

                 onClicked: {
                     Emu.reset();
                     closeDrawer();
                 }
             }

             DrawerButton {
                 id: resumeButton

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
                Layout.minimumHeight: 60
            }

            DrawerButton {
                id: desktopUIButton
                visible: !Emu.isMobile()

                title: qsTr("Desktop UI")
                icon: "qrc:/icons/resources/icons/video-display.png"

                onClicked: Emu.switchUIMode(false);
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

        ColumnLayout {
            Layout.fillWidth: true
            spacing: -1 // Collapse borders

            DrawerButton {
                id: speedButton
                toggle: true
                toggleState: Emu.turboMode

                title: qsTr("Speed: %1 %").arg(Math.round(100*Emu.speed))
                font.pixelSize: TextMetrics.normalSize
                onToggleStateChanged: {
                    Emu.turboMode = toggleState
                    toggleState = Qt.binding(function() { return Emu.turboMode })
                }
            }

            DrawerButton {
                id: aboutButton

                borderBottomVisible: false

                title: qsTr("Firebird Emu v" + Emu.version)

                font.pixelSize: TextMetrics.normalSize

                MessageDialog {
                    id: aboutDialog
                    title: qsTr("About Firebird")
                    text: qsTr("Authors:<br>
                               Fabian Vogt (<a href='https://github.com/Vogtinator'>Vogtinator</a>)<br>
                               Adrien Bertrand (<a href='https://github.com/adriweb'>Adriweb</a>)<br>
                               Antonio Vasquez (<a href='https://github.com/antoniovazquezblanco'>antoniovazquezblanco</a>)<br>
                               Lionel Debroux (<a href='https://github.com/debrouxl'>debrouxl</a>)<br>
                               Denis Avashurov (<a href='https://github.com/denisps'>denisps</a>)<br>
                               Based on nspire_emu v0.70 by Goplat<br><br>
                               This work is licensed under the GPLv3.<br>
                               To view a copy of this license, visit <a href='https://www.gnu.org/licenses/gpl-3.0.html'>https://www.gnu.org/licenses/gpl-3.0.html</a>")
                }

                onClicked: {
                    aboutDialog.visible = true;
                }
            }
        }
    }
}
