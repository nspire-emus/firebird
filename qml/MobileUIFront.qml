import Firebird.Emu 1.0
import Firebird.UIComponents 1.0

import QtQuick 2.0
import QtQuick.Layouts 1.0
import QtQuick.Dialogs 1.1

Item {
    id: mobileui

    Item {
        id: screenAndBar

        height: (mobileui.width - swipeBar.width) / 320 * 240

        anchors {
            top: mobileui.top
            left: mobileui.left
            right: mobileui.right
            bottom: undefined
        }

        VerticalSwipeBar {
            id: swipeBar

            anchors {
                left: parent.left
                top: parent.top
                bottom: parent.bottom
            }

            visible: true

            text: "Swipe here"

            onClicked: {
                listView.openDrawer();
            }
        }

        EmuScreen {
            id: screen

            anchors {
                left: swipeBar.visible ? swipeBar.right : parent.left
                right: parent.right
                top: parent.top
                bottom: parent.bottom
            }

            focus: true

            Timer {
                interval: 35
                running: true
                repeat: true
                onTriggered: screen.update()
            }
        }

        RowLayout {
            id: sidebar
            visible: false

            anchors {
                left: parent.left
                leftMargin: spacing
                right: parent.right
                bottom: parent.bottom
            }

            property int preferredSize: Math.min(screenAndBar.width / 4, mobileui.height * 0.15)

            spacing: (parent.width - preferredSize * 4) / 5
            Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter

            SidebarButton {
                id: restartButton

                Layout.preferredHeight: sidebar.preferredSize
                Layout.preferredWidth: sidebar.preferredSize

                title: qsTr("Start")
                icon: "qrc:/icons/resources/icons/edit-bomb.png"

                onClicked: {
                    Emu.useDefaultKit();
                    Emu.restart();
                }
            }

            SidebarButton {
                id: resetButton

                Layout.preferredHeight: sidebar.preferredSize
                Layout.preferredWidth: sidebar.preferredSize

                title: qsTr("Reset")
                icon: "qrc:/icons/resources/icons/system-reboot.png"

                onClicked: Emu.reset();
            }

            SidebarButton {
                id: resumeButton

                Layout.preferredHeight: sidebar.preferredSize
                Layout.preferredWidth: sidebar.preferredSize

                title: qsTr("Resume")
                icon: "qrc:/icons/resources/icons/system-suspend-hibernate.png"

                onClicked: {
                    Emu.useDefaultKit();
                    Emu.resume()
                }
            }

            SidebarButton {
                id: saveButton

                Layout.preferredHeight: sidebar.preferredSize
                Layout.preferredWidth: sidebar.preferredSize

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
                }
            }

        }
    }

    Flickable {
        id: controls
        anchors {
            top: screenAndBar.bottom
            bottom: parent.bottom
            right: parent.right
            left: parent.left
        }

        boundsBehavior: Flickable.StopAtBounds
        flickableDirection: Flickable.VerticalFlick

        contentWidth: parent.width
        contentHeight: keypad.height*controls.width/keypad.width + iosmargin.height
        clip: true
        pixelAligned: true

        Keypad {
            id: keypad
            transform: Scale { origin.x: 0; origin.y: 0; xScale: controls.width/keypad.width; yScale: controls.width/keypad.width }
        }

        Rectangle {
            id: iosmargin
            color: keypad.color

            anchors {
                left: parent.left
                right: parent.right
            }

            y: keypad.y + keypad.height*controls.width/keypad.width

            // This is needed to avoid opening the control center
            height: Qt.platform.os === "ios" ? 20 : 0
        }
    }

    states: [ State {
        name: "tabletMode"
        when: (mobileui.width > mobileui.height) && !Emu.leftHanded

        /* Keypad fills right side, as wide as needed */
        PropertyChanges {
            target: controls
            anchors {
                right: mobileui.right
                top: mobileui.top
                bottom: mobileui.bottom
                left: undefined
            }
            width: keypad.width/keypad.height * controls.height
        }

        /* Screen + sidebar centered on the remaining space on the left */
        PropertyChanges {
            target: screenAndBar
            anchors {
                right: controls.left
                left: mobileui.left
                bottom: mobileui.bottom
                top: mobileui.top
            }
        }

        PropertyChanges {
            target: screen
            height: undefined
            anchors {
                left: screenAndBar.left
                right: screenAndBar.right
                top: screenAndBar.top
                bottom: sidebar.top
            }
        }

        PropertyChanges {
            target: swipeBar
            visible: false
        }

        PropertyChanges {
            target: sidebar
            visible: true
        }
    }, State {
        name: "tabletModeLeft"
        extend: "tabletMode"
        when: (mobileui.width > mobileui.height) && Emu.leftHanded

        /* Keypad fills left side, as wide as needed */
        PropertyChanges {
            target: controls
            anchors {
                left: mobileui.left
                right: undefined
            }
        }

        /* Screen + sidebar centered on the remaining space on the right */
        PropertyChanges {
            target: screenAndBar
            anchors {
                left: controls.right
                right: mobileui.right
            }
        }
    } ]
}
