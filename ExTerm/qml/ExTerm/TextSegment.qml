import QtQuick 2.0

Rectangle {
    property string text
    property color foregroundColor
    property color backgroundColor
    property font font

    property bool doubleHeight
    property bool doubleHeightBottom

    Scale { id: scaleId; origin.x: 0; origin.y: 0; xScale: .5}

    y: doubleHeightBottom ? -height/2 : 0
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

    transform: doubleHeight ? scaleId : null
}
