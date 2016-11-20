import QtQuick 2.0
import QtQuick.Layouts 1.0
import QtQuick.Controls 1.4
import QtQuick.Controls.Styles 1.4
import Firebird.UIComponents 1.0

ColumnLayout {
    TabView {
        Layout.fillHeight: true
        Layout.fillWidth: true

        style: TabViewStyle {
            tab: Rectangle {
                color: styleData.selected ? "darkgrey" :"lightgrey"
                border.color:  "grey"
                implicitWidth: Math.max(text.width + 4, 80)
                implicitHeight: text.height + 4
                radius: 2
                FBLabel {
                    id: text
                    anchors.centerIn: parent
                    text: styleData.title
                    font.pixelSize: TextMetrics.normalSize
                    color: styleData.selected ? "white" : "black"
                }
            }
            frame: Rectangle { color: "#eeeeee" }
        }

        Repeater {
            id: rep
            model: ConfigPagesModel {}

            Tab {
                title: rep.model.get(index).title
                anchors.leftMargin: 2
                Loader {
                    source: file
                }
            }
        }
    }
}

