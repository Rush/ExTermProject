import QtQuick 2.0

Item {
    property bool doubleWidth
    width: parent.width

    Scale { id: scaleId; origin.x: 0; origin.y: 0; xScale: 2}

    transform: doubleWidth ? scaleId : null
}
