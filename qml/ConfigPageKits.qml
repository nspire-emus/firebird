import QtQuick 2.0
import QtQuick.Controls 1.0
import QtQuick.Layouts 1.0
import Firebird.Emu 1.0
import Firebird.UIComponents 1.0

ColumnLayout {
    spacing: 5

    FBLabel {
        text: qsTr("Kits")
        font.bold: true
        font.pixelSize: 20
    }

    KitList {
        Layout.fillHeight: true
        Layout.fillWidth: true

        model: ListModel {
            ListElement {
                name: "3.1 with nLaunch"
                type: "CX CAS (HW A)"
                flash: "flash31.img"
            }
            ListElement {
                name: "4.3 for tests"
                type: "CX CAS (HW A)"
                flash: "flash43.img"
            }
            ListElement {
                name: "3.6 for crafti"
                type: "CX CAS (HW J)"
                flash: "flash36.img"
            }
            ListElement {
                name: "4.2 on HW-W"
                type: "CX CAS (HW W)"
                flash: "flash42.img"
            }
            ListElement {
                name: "Crappy screen calc"
                type: "Clickpad CAS"
                flash: "flashCPC.img"
            }
            ListElement {
                name: "Crappy screen calc with TPAD"
                type: "Touchpad CAS"
                flash: "flashTPC.img"
            }
            ListElement {
                name: "Crappy screen calc for crafti"
                type: "Clickpad"
                flash: "flashCP.img"
            }
        }
    }
}
