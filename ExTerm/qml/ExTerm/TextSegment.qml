import QtQuick 2.0

Rectangle {
    property string text
    property color foregroundColor
    property color backgroundColor
    property font font

    Scale { id: scaleId; origin.x: 0; origin.y: 0; xScale: .5}

    y: 0
    width: textItem.paintedWidth
    height: textItem.paintedHeight

    color: backgroundColor

    Text {
        id: textItem
        text: parent.text
        color: foregroundColor
        height: paintedHeight
        width: paintedWidth
        font: parent.font
        textFormat: Text.PlainText
        //renderType: Text.NativeRendering
    }

    function setDoubleHeightTop() {
       textItem.transform = scaleId;
        textItem.font.pointSize *= 2;
       clip = true;
    }
    function setDoubleHeightBottom() {
        textItem.font.pointSize *= 2;
        textItem.y -= height;
        textItem.transform = scaleId;
        clip = true;
    }
    function setNormalHeight() {
        textItem.transform = null;
        clip = false;
    }

}
