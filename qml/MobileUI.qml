import QtQuick 2.0
import QtQuick.Layouts 1.0
import QtQuick.Dialogs 1.1
import Firebird.Emu 1.0
import QtQuick.Controls 1.3

Rectangle {
    id: mobileui
    width: 320
    height: 480
    color: "#eee"

    Component.onCompleted: {
        // FIXME: The toast might not yet be registered here

        Emu.useDefaultKit();

        if(Emu.autostart
            && Emu.getFlashPath() !== ""
            && Emu.getSnapshotPath() !== ""
            && Emu.getBoot1Path() !== "")
            Emu.resume();
    }

    Connections {
        target: Qt.application
        onStateChanged: {
            switch (Qt.application.state)
            {
                case Qt.ApplicationSuspended:
                case Qt.ApplicationHidden:
                    Emu.setPaused(true);
                break;
                case Qt.ApplicationActive:
                    Emu.setPaused(false);
                break;
            }
        }
    }

    GridLayout {
        id: screenAndBar
        anchors {
            top: mobileui.top
            left: mobileui.left
            right: mobileui.right
            bottom: undefined
        }
        height: (mobileui.width - sidebar.preferredSize) / 320 * 240
        columns: 2

        rowSpacing: 0
        columnSpacing: 0

        EmuScreen {
            id: screen
            focus: true
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.preferredWidth: mobileui.width * 0.85
            Layout.preferredHeight: width/320 * 240

            Timer {
                interval: 20
                running: true; repeat: true
                onTriggered: screen.update()
            }
        }

        GridLayout {
            id: sidebar
            columns: 1
            Layout.fillHeight: true
            Layout.fillWidth: false

            property int preferredSize: mobileui.height * 0.1

            columnSpacing: (screenAndBar.width - preferredSize * 4) / 5
            rowSpacing: (screenAndBar.height - preferredSize * 4) / 5
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
        width: mobileui.width

        boundsBehavior: Flickable.StopAtBounds
        flickableDirection: Flickable.HorizontalAndVerticalFlick

        contentWidth: controlsRow.width
        contentHeight: controlsRow.height
        clip: true

        Row {
            id: controlsRow

            Item {
                id: mobilecontrol1
                height: keypad.height*controls.width/keypad.width + iosmargin.height
                width: controls.width

                Keypad {
                    id: keypad
                    transform: Scale { origin.x: 0; origin.y: 0; xScale: controls.width/keypad.width; yScale: controls.width/keypad.width }
                }

                Item {
                    id: iosmargin
                    // This is needed to avoid opening the control center
                    height: Qt.platform.os === "ios" ? 20 : 0
                }
            }

            MobileControl2 {
                id: control2
                height: mobilecontrol1.height
                width: controls.width
            }
        }
    }

    Rectangle {
        id: toast
        x: 60
        z: 1
        implicitWidth: message.width+2*5
        implicitHeight: message.height+2*5

        anchors.bottom: parent.bottom
        anchors.bottomMargin: 61
        anchors.horizontalCenter: parent.horizontalCenter

        radius: 5
        color: "#d3c7c7"
        border.color: "#e66e6e6e"
        border.width: 3

        opacity: 0
        visible: opacity > 0

        Component.onCompleted: Emu.registerToast(this)

        Behavior on opacity { NumberAnimation { duration: 200 } }

        function showMessage(str) {
            message.text = str;
            opacity = 1;
            timer.restart();
        }

        Text {
            id: message
            text: "Text"
            anchors.centerIn: parent
            font.pointSize: 12

            Timer {
                id: timer
                interval: 2000
                onTriggered: parent.parent.opacity = 0;
            }
        }

        MouseArea {
            anchors.fill: parent
            onClicked: {
                timer.stop();
                parent.opacity = 0;
            }
        }
    }

    states: State {
        name: "tabletMode"
        when: mobileui.width > mobileui.height

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
            height: (mobileui.width - controls.width) / 320 * 240
            columns: 1
        }

        /* Horizontal instead of veritcal orientation */
        PropertyChanges {
            target: sidebar
            columns: 4
            Layout.fillWidth: true
            preferredSize: Math.min(screenAndBar.width / 4, mobileui.height * 0.15)
        }
    }
}

