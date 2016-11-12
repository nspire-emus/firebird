import QtQuick 2.0
import QtQuick.Controls 1.0
import QtQuick.Layouts 1.0
import Firebird.Emu 1.0
import Firebird.UIComponents 1.0

ColumnLayout {
    spacing: 5

    FBLabel {
        text: qsTr("Startup")
        font.pixelSize: 14
        Layout.topMargin: 5
        Layout.bottomMargin: 5
    }

    FBLabel {
        // The hell, QML!
        Layout.maximumWidth: parent.width
        wrapMode: Text.WordWrap
        text: {
            if(!Emu.isMobile())
                return qsTr("When opening Firebird, the selected Kit will be started.\nIf available, it will resume the emulation from the provided snapshot.")
            else
                return qsTr("Choose the Kit selected on startup and after restarting. If the checkbox is active, it will be launched when Firebird starts.")
        }

        font.pixelSize: 12
    }

    RowLayout {
        // No spacing so that the spin box looks like part of the label
        spacing: 0

        LabeledCheckBox {
            text: qsTr("On Startup, run Kit")

            checked: Emu.autostart
            onCheckedChanged: Emu.autostart = checked
        }

        ComboBox {
            textRole: "name"
            model: Emu.kits
            currentIndex: Emu.kitIndexForID(Emu.defaultKit)
            onCurrentIndexChanged: Emu.defaultKit = model.getData(currentIndex, KitModel.IDRole)
        }
    }

    FBLabel {
        text: qsTr("Shutdown")
        font.pixelSize: 14
        Layout.topMargin: 10
        Layout.bottomMargin: 5
        visible: !Emu.isMobile()
    }

    FBLabel {
        Layout.maximumWidth: parent.width
        wrapMode: Text.WordWrap
        text: qsTr("On Application end, write the current state in the current snapshot.")
        font.pixelSize: 12
        visible: !Emu.isMobile()
    }

    LabeledCheckBox {
        text: qsTr("Save snapshot on shutdown")

        checked: Emu.suspendOnClose
        onCheckedChanged: Emu.suspendOnClose = checked
        visible: !Emu.isMobile()
    }

    Item {
        Layout.fillHeight: true
    }

}
