import QtQuick 2.14
import QtQuick.Controls 2.14
import UITheme 1.0

Button {
    id: root
    padding: 8

    property color normalColor: Theme.accent
    property color hoverColor: Qt.rgba(96 / 255, 165 / 255, 250 / 255, 1.0)
    property color pressedColor: Theme.accentPressed
    property color disabledColor: Qt.rgba(75 / 255, 85 / 255, 99 / 255, 1.0)
    property color borderColor: Theme.inputBorder
    property color textColor: Theme.accentText

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
