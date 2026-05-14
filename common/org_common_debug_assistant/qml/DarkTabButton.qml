import QtQuick 2.14
import QtQuick.Controls 2.14
import UITheme 1.0

TabButton {
    id: root
    padding: 8

    contentItem: Text {
        text: root.text
        color: root.checked ? Theme.accentText : Theme.textPrimary
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight
        font.pixelSize: 15
    }

    background: Rectangle {
        radius: Theme.radiusSmall
        border.color: root.checked ? Theme.accent : Theme.inputBorder
        color: root.checked ? Theme.accent : Theme.inputBg
    }
}
