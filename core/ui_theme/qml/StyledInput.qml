import QtQuick 2.14
import QtQuick.Controls 2.14
import UITheme 1.0

TextField {
    id: root

    color: Theme.textPrimary
    placeholderTextColor: Theme.textSecondary
    font.pixelSize: Theme.fontSizeNormal
    selectByMouse: true
    leftPadding: 12
    rightPadding: 12
    topPadding: 7
    bottomPadding: 7
    implicitHeight: 34

    background: Rectangle {
        radius: Theme.radiusNormal
        color: Theme.inputBg
        border.color: root.activeFocus ? Theme.accent : Theme.inputBorder
        border.width: 1
    }
}
