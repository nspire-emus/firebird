import QtQuick 2.7
import QtQuick.Layouts 1.0
import QtQuick.Controls 1.4
import QtQuick.Controls.Styles 1.4
import Firebird.UIComponents 1.0

TabView {
    id: tabView
    Layout.fillHeight: true
    Layout.fillWidth: true

    property var model: ConfigPagesModel {}

/*    style: TabViewStyle {
        tab: Rectangle {
            color: styleData.selected ? "darkgrey" :"lightgrey"

            implicitWidth: tabView.width / model.count
            implicitHeight: title.contentHeight + 10

            FBLabel {
                id: title
                anchors.fill: parent
                text: styleData.title
                font.pixelSize: TextMetrics.title1Size
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                elide: Text.ElideRight
                color: styleData.selected ? "white" : "black"
            }
        }
        frame: Rectangle { color: "#eeeeee" }
    }*/

    Repeater {
        id: rep
        model: tabView.model

        Tab {
            title: rep.model.get(index).title

            Loader {
                source: file
            }
        }
    }
}
