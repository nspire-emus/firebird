import QtQuick 2.0
import QtQuick.Shapes 1.15
import Firebird.Emu 1.0

Rectangle {


    id: rectangle2
    width: 100
    height: 70
    color: "#222222"
    radius: 10
    border.width: 2
    border.color: "#eeeeee"

    property color colorSvg1: "white"
    property color colorSvg2: "blue"



    Rectangle {
        id: rectangle1
        x: 29
        y: 18
        width: 35
        height: 35
        color: "#00000000"
        radius: 8
        border.color: "#ffffff"
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.verticalCenter: parent.verticalCenter
        Rectangle {
            anchors.top: parent.top
            anchors.bottom: parent.verticalCenter
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.topMargin: 5
            NSvg{
                colorSvg: colorSvg2
                svg:"m 2.2079853,-25.056601 -17.1717093,0.18593 -2.697202,4.100588 h -2.214701 l -4.408602,7.507416 0.1221,0.0717 -5.336503,4.8025368 -0.1633,0.3831998 c -0.886201,2.0729088 -0.316801,4.6327173 1.511901,6.0228465 1.828701,1.39012924 4.489602,1.72445905 7.726604,0.99062947 0.4763,-0.0362 0.6488,-0.21936987 0.9582,-0.54765967 0,0 3.147302,-3.2335381 7.483004,-3.2429681 2.6390019,-0.006 4.563403,0.6928596 5.7713036,1.697859 1.2079007,1.0049894 1.811601,2.29510866 1.779701,4.05194764 -0.045,2.45429856 -0.8057004,3.56875796 -2.1082012,4.42616746 -1.3026007,0.8574095 -3.3831018,1.3165892 -5.8022034,1.3937492 -1.433001,0.0457 -3.217602,-0.7337496 -4.594502,-1.6351591 -1.377001,-0.9013995 -2.272302,-1.7958889 -2.272302,-1.7958889 -0.3236,-0.4344898 -0.5746,-0.4675398 -1.0799,-0.5367197 -2.160101,-0.2971699 -4.374103,0.0888 -6.030203,1.4800791 -1.656201,1.3912692 -2.376602,3.9244278 -1.562701,6.4960865 l 0.1386,0.43984 12.170006,12.169133 h 36.790421 v 1.651889 h 8.854505 V -4.4583724 H 23.438897 Z M 0.69258444,-21.260123 21.217796,-1.3466542 V 19.625204 h -35.22562 L -24.368329,9.2638797 c -0.291101,-1.1782494 -0.025,-1.6450291 0.4814,-2.0704988 0.4916,-0.4129598 1.498101,-0.6594397 2.613901,-0.6121097 0.493901,0.4412098 1.209101,1.0408994 2.267002,1.733389 1.716901,1.1239694 4.064902,2.3367588 6.784703,2.2499988 2.7845018,-0.0888 5.5200034,-0.53954 7.7598046,-2.0138589 2.2398012,-1.4743292 3.74710206,-4.1142977 3.80900209,-7.5148457 0.05,-2.7684884 -1.09320059,-5.3213469 -3.14100169,-7.0251859 -2.0479012,-1.7038391 -4.8866027,-2.5798586 -8.196705,-2.5726586 -5.093602,0.0111 -8.599104,2.9011484 -9.717605,3.9507778 -2.243501,0.4157897 -3.617702,0.028 -4.154502,-0.3800698 -0.4823,-0.3666098 -0.548801,-0.6985896 -0.3824,-1.2410193 l 6.438503,-5.7939466 15.4414086,-0.26796 7.5725042,6.0715766 c 2.7779016,1.0719894 5.6095031,0.5540097 4.1893023,-3.407358 L -2.4242173,-20.770083 H -13.137123 l 0.2256,-0.34317 z m -18.40471044,4.269528 h 13.6455078 l 0.8371005,0.896679 -15.1637083,0.26309 z"
            }
        }
        Rectangle {
            anchors.top: parent.verticalCenter
            anchors.bottom: parent.bottom
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.bottomMargin: 5
            NSvg{
                colorSvg: colorSvg1
                svg:"m -9.0560467,-33.276501 v 15.933074 h 3.8260346 V -33.276501 Z M 14.811816,-28.280437 0.97889372,-14.446774 3.9766785,-11.448985 17.810338,-25.28191 Z m -47.658217,18.7942947 v 3.8260345 h 15.933072 V -9.4861423 Z M -14.016677,0.54880308 -27.849599,14.381729 -24.851814,17.380251 -11.018893,3.5465885 Z M 27.095045,7.5177461 -7.4181129,-7.37404 7.4736632,27.139133 10.669632,18.09442 25.851709,33.276501 32.846401,26.281808 17.664323,11.099727 Z"
            }
        }

    }
    Rectangle{//up
        anchors.top: parent.top
        anchors.bottom: rectangle1.top
        anchors.horizontalCenter: parent.horizontalCenter
        Rectangle {
            anchors.top: parent.top
            anchors.bottom: parent.verticalCenter
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.topMargin: 5
            NSvg{
                colorSvg: colorSvg2
                svg:"M 9.555036,-3.7423981 H 28.370743 V 6.2189033 H 9.555036 Z m -21.509008,0 H 6.861835 V 6.2189033 H -11.953972 Z M 31.064044,-20.2299 h 18.815807 v 9.961301 H 31.064044 Z m -21.509008,0 h 18.815707 v 9.961301 H 9.555036 Z m -21.509008,0 H 6.861835 v 9.961301 h -18.815807 z m -33.568333,33.534804 c 2.734961,-6.2482006 9.207044,-13.5751016 16.167426,-17.4672022 l 3.828402,6.630701 6.930402,-16.3098028 -17.589826,-2.1529 3.420511,5.924401 c -8.633523,4.7942007 -15.944266,12.4489018 -16.867576,22.897103 -1.355781,15.342002 3.275961,2.384601 4.110661,0.4777 z"
            }
        }
        Rectangle {
            anchors.top: parent.verticalCenter
            anchors.bottom: parent.bottom
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.bottomMargin: 5
            NSvg{
                colorSvg: colorSvg1
                svg:"m 14.79145,9.8223502 h -29.5829 L 3.3966508e-7,-9.8223502 Z"
            }
        }
    }
    Rectangle{//down
        anchors.top: rectangle1.bottom
        anchors.bottom: parent.bottom
        anchors.horizontalCenter: parent.horizontalCenter
        Rectangle {
            anchors.top: parent.top
            anchors.bottom: parent.verticalCenter
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.topMargin: 5
            NSvg{
                colorSvg: colorSvg1
                svg:"m -14.79145,-9.8223502 h 29.5829 L 0,9.8223502 Z"
            }
        }
        Rectangle {
            anchors.top: parent.verticalCenter
            anchors.bottom: parent.bottom
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.bottomMargin: 5
            NSvg{
                colorSvg: colorSvg2
                svg:"m 48.750202,-27.959877 c -1.078899,-0.0537 -3.171998,5.70298 -3.706698,6.924611 -2.734898,6.248241 -9.206995,13.5751016 -16.167392,17.4671921 L 25.047814,-10.198745 18.117317,6.1110275 35.707209,8.2639578 32.28671,2.339537 c 8.633496,-4.7941707 15.944193,-12.448862 16.867592,-22.897073 0.4872,-5.513531 0.2011,-7.372201 -0.4041,-7.402341 z m -98.151353,6.750001 v 3.40156 45.768567 h 62.73567 V -5.8827242 L -2.3511734,-21.209876 Z m 6.803297,6.803131 H -4.5521723 V -3.7885839 H 6.5314224 V 21.15712 H -42.597854 Z"
            }
        }
    }
    Rectangle{//left
        anchors.left: parent.left
        anchors.right: rectangle1.left
        anchors.verticalCenter: parent.verticalCenter
        Rectangle {
            anchors.left: parent.left
            anchors.right: parent.horizontalCenter
            anchors.verticalCenter: parent.verticalCenter
            anchors.leftMargin: 5
            NSvg{
                colorSvg: colorSvg2
                svg:"M 9.9999939e-6,14.791456 V -14.791456 L -15.262051,-4.9999969e-6 Z M -31.367851,-24.585051 v 3.401578 45.768524 H 31.367851 V -9.2578896 L 15.68212,-24.585051 Z m 6.803346,6.803136 h 38.045657 v 10.6181542 h 11.083563 l 2e-5,24.9456758 h -49.12924 z"
            }
        }
        Rectangle {
            anchors.left: parent.horizontalCenter
            anchors.right: parent.right
            anchors.verticalCenter: parent.verticalCenter
            anchors.rightMargin: 5
            NSvg{
                colorSvg: colorSvg1
                svg:"m 9.8223502,-14.79145 v 29.5829 L -9.8223502,0 Z"
            }
        }
    }
    Rectangle{//right
        anchors.left: rectangle1.right
        anchors.right: parent.right
        anchors.verticalCenter: parent.verticalCenter
        Rectangle {
            anchors.left: parent.left
            anchors.right: parent.horizontalCenter
            anchors.verticalCenter: parent.verticalCenter
            anchors.leftMargin: 5
            NSvg{
                colorSvg: colorSvg1
                svg:"m -9.8223502,14.79145 v -29.5829 L 9.8223502,0 Z"
            }
        }
        Rectangle {
            anchors.left: parent.horizontalCenter
            anchors.right: parent.right
            anchors.verticalCenter: parent.verticalCenter
            anchors.rightMargin: 5
            NSvg{
                colorSvg: colorSvg2
                svg:"M -5.0873001,-14.791466 V 14.791456 L 10.1747,-4.9999971e-6 Z m -26.2806009,-9.793585 v 3.401578 45.768524 H 31.367901 V -9.2579096 L 15.6821,-24.585051 Z m 6.8034,6.803136 H 13.4812 V -7.1637608 H 24.564701 V 17.781915 h -49.129202 z"
            }
        }
    }


    Rectangle {
        id: highlight
        x: 0
        y: 0
        width: 10
        height: 10
        color: "#b3edf200"
        radius: 10
        visible: false
    }

    /* Click and hold at same place -> Down
       Click and quick release -> Down
       Click and move -> Contact */
    MouseArea {
        anchors.rightMargin: 0
        anchors.bottomMargin: 0
        anchors.fill: parent
        acceptedButtons: Qt.LeftButton
        preventStealing: true

        property int origX
        property int origY
        property int moveThreshold: 5
        property bool isDown

        Connections {
            target: Emu
            function onTouchpadStateChanged(x, y, contact, down) {
                if(contact || down)
                {
                    highlight.x = x*width - highlight.width/2;
                    highlight.y = y*height - highlight.height/2;
                    highlight.color = down ? "#b3edf200" : "#b38080ff";
                }

                highlight.visible = contact || down;
            }
        }

        Timer {
            id: clickOnHoldTimer
            interval: 200
            running: false
            repeat: false
            onTriggered: {
                parent.isDown = true;
                parent.submitState();
            }
        }

        Timer {
            id: clickOnReleaseTimer
            interval: 100
            running: false
            repeat: false
            onTriggered: {
                parent.isDown = false;
                parent.submitState();
            }
        }

        function submitState() {
            Emu.setTouchpadState(mouseX/width, mouseY/height, isDown || pressed, isDown);
        }

        onMouseXChanged: {
            if(Math.abs(mouseX - origX) > moveThreshold)
                clickOnHoldTimer.stop();

            submitState();
        }

        onMouseYChanged: {
            if(Math.abs(mouseY - origY) > moveThreshold)
                clickOnHoldTimer.stop();

            submitState();
        }

        onReleased: {
            if(clickOnHoldTimer.running)
            {
                clickOnHoldTimer.stop();
                isDown = true;
                clickOnReleaseTimer.restart();
            }
            else
                isDown = false;

            submitState();
        }

        onPressed: {
            origX = mouse.x;
            origY = mouse.y;
            isDown = false;
            clickOnHoldTimer.restart();
        }
    }
}

