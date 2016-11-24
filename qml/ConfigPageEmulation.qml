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
            onCurrentIndexChanged: Emu.defaultKit = model.getDataRow(currentIndex, KitModel.IDRole)
        }
    }

    FBLabel {
        text: qsTr("Shutdown")
        font.pixelSize: TextMetrics.title1Size
        Layout.topMargin: 10
        Layout.bottomMargin: 5
        visible: !Emu.isMobile()
    }

    FBLabel {
        Layout.maximumWidth: parent.width
        wrapMode: Text.WordWrap
        text: qsTr("On Application end, write the current state in the current snapshot.")
        font.pixelSize: TextMetrics.normalSize
        visible: !Emu.isMobile()
    }

    LabeledCheckBox {
        text: qsTr("Save snapshot on shutdown")

        checked: Emu.suspendOnClose
        onCheckedChanged: Emu.suspendOnClose = checked
        visible: !Emu.isMobile()
    }

    FBLabel {
        text: qsTr("UI Preferences")
        font.pixelSize: TextMetrics.title1Size
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
        onCheckedChanged: Emu.leftHanded = checked
        visible: Emu.isMobile()
    }

    Item {
        Layout.fillHeight: true
    }

}
