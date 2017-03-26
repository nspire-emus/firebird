import Firebird.Emu 1.0
import Firebird.UIComponents 1.0

import QtQuick 2.0
import QtQuick.Controls 1.2
import QtQuick.Dialogs 1.1
import QtQuick.Layouts 1.0

ApplicationWindow {
    id: app
    title: "Firebird Emu"
    width: 320
    height: 480
    visible: true

    Component.onCompleted: {
        // FIXME: The toast might not yet be registered here

        Emu.useDefaultKit();

        if(Emu.autostart
            && Emu.getFlashPath() !== ""
            && Emu.getBoot1Path() !== "")
        {
            if(Emu.getSnapshotPath() !== "")
                Emu.resume();
            else
                Emu.restart();
        }
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

    Toast {
        id: toast
        x: 60
        z: 1

        anchors.bottom: parent.bottom
        anchors.bottomMargin: 61
        anchors.horizontalCenter: parent.horizontalCenter

        Component.onCompleted: Emu.registerToast(this)
    }

    ListView {
        id: listView

        anchors.fill: parent
        orientation: Qt.Horizontal
        snapMode: ListView.SnapOneItem
        boundsBehavior: ListView.StopAtBounds
        pixelAligned: true

        // TODO: Hack #1! The keypad uses Emu.registerNButton and all that, so it must not be destroyed
        cacheBuffer: width * count

        model: [ "MobileUIConfig.qml", "MobileUIDrawer.qml", "MobileUIFront.qml" ]

        /* The delegates write their X offsets into this array, so that we can use
           them as values for contentX. */
        property var pageX: []

        Component.onCompleted: {
            // Open drawer if emulation does not start automatically
            Emu.useDefaultKit(); // We need this here to get the default values
            if(!Emu.autostart
               || Emu.getBoot1Path() === ""
               || Emu.getFlashPath() === "")
                positionViewAtIndex(1, ListView.SnapPosition);
            else
                positionViewAtIndex(2, ListView.SnapPosition);
        }

        NumberAnimation {
            id: anim
            target: listView
            property: "contentX"
            duration: 200
            easing.type: Easing.InQuad
        }

        function animateToIndex(i) {
            anim.to = pageX[i];
            anim.from = listView.contentX;
            anim.start();
        }

        function closeDrawer() {
            animateToIndex(2);
        }

        function openDrawer() {
            animateToIndex(1);
        }

        function openConfiguration() {
            animateToIndex(0);
        }

        delegate: Item {
            // TODO: Hack #2! The keypad uses Emu.registerNButton and all that, so it must not be destroyed
            ListView.delayRemove: true

            onXChanged: {
                listView.pageX[index] = x;
            }

            width: modelData === "MobileUIDrawer.qml" ? app.width * 0.6 : app.width
            height: app.height

            Rectangle {
                id: overlay
                z: 1
                anchors.fill: parent
                color: "black"
                opacity: {
                    var xOffset = listView.contentX - parent.x;
                    return Math.min(Math.max(0.0, Math.abs(xOffset) / listView.width), 0.6);
                }
            }

            Loader {
                z: 0
                anchors.fill: parent
                source: modelData
            }
        }
    }
}

