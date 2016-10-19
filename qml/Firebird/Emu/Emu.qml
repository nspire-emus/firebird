pragma Singleton
import QtQuick 2.0

QtObject {
    property bool gdbEnabled: true
    property int gdbPort: 3333
    property bool rdbEnabled: true
    property int rdbPort: 3334
}
