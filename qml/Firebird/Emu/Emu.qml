pragma Singleton
import QtQuick 2.0

QtObject {
    property bool gdbEnabled: true
    property int gdbPort: 3333
    property bool rdbEnabled: true
    property int rdbPort: 3334
    property bool running: false
    property string version: "1.2"
    property var toast: null

    function useDefaultKit() {}
    function isMobile() { return true; }
    function setPaused(paused) { }
    function resume() { toast.showMessage("Resume"); }
    function dir() { return "/"; }
    function registerToast(toastref) { toast = toastref; }
    function registerNButton(keymap_id, buttonref) {}
    function restart() { toastMessage("Restart"); }
    function toastMessage(msg) { toast.showMessage(msg); }
    function touchpadStateChanged(x, y, down, contact) {}
    function keypadStateChanged(keymap_id, down) {}
}
