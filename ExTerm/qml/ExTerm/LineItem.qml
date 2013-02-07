import QtQuick 2.0

Item {
    width: parent.width

    Scale { id: scaleId; origin.x: 0; origin.y: 0; xScale: 2}

    function setDoubleWidth() {
        transform = scaleId;
    }
}
