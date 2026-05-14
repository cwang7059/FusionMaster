import QtQuick 2.14
import QtQuick.Controls 2.14
import UITheme 1.0

Button {
    id: root
    padding: 8

    property color normalColor: Theme.accent
    property color hoverColor: Theme.accent
    property color pressedColor: Theme.accentPressed
    property color disabledColor: Theme.accentWeak
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
        radius: Theme.radiusSmall
        border.color: root.borderColor
        color: !root.enabled
            ? root.disabledColor
            : (root.checked || root.down
               ? root.pressedColor
               : (root.hovered ? root.hoverColor : root.normalColor))
    }
}
