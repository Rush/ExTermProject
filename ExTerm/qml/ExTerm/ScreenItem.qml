import QtQuick 2.0
import QtWebKit 3.0

Rectangle {
    id: screenItem

    property font font: parent.font
    property real fontWidth: parent.charWidth
    property real fontHeight: parent.lineHeight

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
        onFocusChanged: {
            focus = true;
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
