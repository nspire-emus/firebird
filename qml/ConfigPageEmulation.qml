import QtQuick 2.0
import QtQuick.Controls 1.0
import QtQuick.Layouts 1.0
import Firebird.Emu 1.0
import Firebird.UIComponents 1.0

ColumnLayout {
    spacing: 5

    FBLabel {
        text: qsTr("Startup")
        font.pixelSize: TextMetrics.title2Size
        Layout.topMargin: 5
        Layout.bottomMargin: 5
    }

    FBLabel {
        // The hell, QML!
        Layout.maximumWidth: parent.width
        wrapMode: Text.WordWrap
        text: {
            if(!Emu.isMobile())
                return qsTr("When opening Firebird, the selected Kit will be started. If available, it will resume the emulation from the provided snapshot.")
            else
                return qsTr("Choose the Kit selected on startup and after restarting. If the checkbox is active, it will be launched when Firebird starts.")
        }

        font.pixelSize: TextMetrics.normalSize
    }

    RowLayout {
        // No spacing so that the spin box looks like part of the label
        spacing: 0
        width: parent.width
        Layout.fillWidth: true

        LabeledCheckBox {
            Layout.maximumWidth: parent.parent.width - startupKit.width
            text: qsTr("On Startup, run Kit")

            checked: Emu.autostart
            onCheckedChanged: Emu.autostart = checked
        }

        ComboBox {
            id: startupKit
            Layout.maximumWidth: parent.parent.width * 0.4
            textRole: "name"
            model: Emu.kits
            currentIndex: Emu.kitIndexForID(Emu.defaultKit)
            onCurrentIndexChanged: {
                Emu.defaultKit = model.getDataRow(currentIndex, KitModel.IDRole);
                currentIndex = Qt.binding(function() { return Emu.kitIndexForID(Emu.defaultKit); });
            }
        }
    }

    FBLabel {
        text: qsTr("Shutdown")
        font.pixelSize: TextMetrics.title1Size
        Layout.topMargin: 10
        Layout.bottomMargin: 5
        visible: Qt.platform.os !== "ios"
    }

    FBLabel {
        Layout.maximumWidth: parent.width
        wrapMode: Text.WordWrap
        text: {
            if(Qt.platform.os === "android")
                return qsTr("When closing firebird using the back button, save the current state to the current snapshot. Does not work when firebird is in the background.")
            else
                return qsTr("On Application end, save the current state to the current snapshot.");
        }
        font.pixelSize: TextMetrics.normalSize
        visible: Qt.platform.os !== "ios"
    }

    LabeledCheckBox {
        text: qsTr("Save snapshot on shutdown")

        checked: Emu.suspendOnClose
        visible: Qt.platform.os !== "ios"
        onCheckedChanged: {
            Emu.suspendOnClose = checked;
            checked = Qt.binding(function() { return Emu.suspendOnClose; });
        }
    }

    FBLabel {
        text: qsTr("UI Preferences")
        font.pixelSize: TextMetrics.title2Size
        Layout.topMargin: 10
        Layout.bottomMargin: 5
        visible: Emu.isMobile()
    }

    FBLabel {
        Layout.maximumWidth: parent.width
        wrapMode: Text.WordWrap
        text: qsTr("Change the side of the keypad in landscape orientation.")
        font.pixelSize: TextMetrics.normalSize
        visible: Emu.isMobile()
    }

    LabeledCheckBox {
        text: qsTr("Left-handed mode")

        checked: Emu.leftHanded
        visible: Emu.isMobile()
        onCheckedChanged: {
            Emu.leftHanded = checked;
            checked = Qt.binding(function() { return Emu.leftHanded; });
        }
    }

    Item {
        Layout.fillHeight: true
    }

}
