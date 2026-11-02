import QtQuick 2.0
import QtQuick.Shapes 1.15

Shape {
    property alias svg: svg.path
    property alias colorSvg: psvg.fillColor

    transform:Scale {xScale: 0.15;yScale: 0.15;}
    anchors.centerIn: parent
    ShapePath {
        id:psvg
        strokeWidth:-1
        fillColor: "blue"
        PathSvg {
            id:svg;
            path: "M 2.13e-5,-32.828401 C -18.090394,-32.828371 -32.828751,-18.09081 -32.828751,-3.4049999e-4 -32.828711,18.090021 -18.090394,32.828401 2.13e-5,32.828401 18.090394,32.828401 32.828711,18.090057 32.828751,-3.4049999e-4 32.828751,-18.09081 18.090394,-32.828401 2.13e-5,-32.828401 Z"
        }
    }
}
