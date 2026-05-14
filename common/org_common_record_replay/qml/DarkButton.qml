import QtQuick 2.14
import QtQuick.Controls 2.14

Button {
    id: root
    padding: 8

    property color normalColor: "#2B78AC"
    property color hoverColor: "#388BC4"
    property color pressedColor: "#245E8A"
    property color disabledColor: "#4A5D6F"
    property color borderColor: "#4D95C5"
    property color textColor: "#F3FAFF"

    contentItem: Text {
        text: root.text
        color: root.textColor
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight
        font.pixelSize: 15
    }

    background: Rectangle {
        radius: 2
        border.color: root.borderColor
        color: !root.enabled
            ? root.disabledColor
            : (root.checked || root.down
               ? root.pressedColor
               : (root.hovered ? root.hoverColor : root.normalColor))
    }
}
