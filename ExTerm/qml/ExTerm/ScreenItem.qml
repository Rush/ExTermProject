import QtQuick 2.0
import QtWebKit 3.0

Rectangle {
    id: screenItem

    property font font: parent.font
    property real fontWidth: parent.charWidth
    property real fontHeight: parent.lineHeight

    property real cursorX
    property real cursorY
    property bool cursorVisible
    property bool cursorBlinking

    Component.onCompleted: console.log("test" + parent);
    anchors.fill: parent

    color: "black"

    Item {
        id: keyHandler
        focus: true
        Keys.onPressed: {
            console.log("Key pressed");
           screenItem.parent.onKeyPressed(event.text, event.key, event.modifiers);
            //terminal.screen.sendKey(event.text, event.key, event.modifiers);
        }
    }

    Rectangle {
        id: cursor
        width: screenItem.fontWidth
        height: screenItem.fontHeight
        x: parent.cursorX
        y: parent.cursorY
        color: "grey"
        property bool blink
        visible: cursorVisible && (!cursorBlinking || blink)
        SequentialAnimation on visible {
            running: cursorBlinking
            loops: Animation.Infinite
            PropertyAction { target: cursor; property: "blink"; value: true }
            PauseAnimation { duration: 500 }
            PropertyAction { target: cursor; property: "blink"; value: false }
            PauseAnimation { duration: 500 }
        }
    }

    Image {
        id: image
        width: parent.width; height: parent.height
        source: "file:///home/rush/Obrazy/sleeping_beauty.jpg"
        clip: true      // only makes a difference if mode is PreserveAspectCrop
        opacity: 0.3
    }

}
