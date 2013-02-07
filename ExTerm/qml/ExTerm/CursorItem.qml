import QtQuick 2.0

Rectangle {
    z: 1
    id: cursor
    width: parent.charWidth
    height: parent.lineHeight

    property bool cursorBlinking: false
    property bool cursorVisible: true
    property int blinkingTime: 500

    color: "white"
    opacity: 0.5
    property bool blink
    visible: cursorVisible && (!cursorBlinking || blink)
    SequentialAnimation on visible {
        running: cursorBlinking
        loops: Animation.Infinite
        PropertyAction { target: cursor; property: "blink"; value: true }
        PauseAnimation { duration: blinkingTime }
        PropertyAction { target: cursor; property: "blink"; value: false }
        PauseAnimation { duration: blinkingTime }
    }
}
