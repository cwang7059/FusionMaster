import QtQuick 2.14
import QtQuick.Controls 2.14
import UITheme 1.0

TextArea {
    id: control

    color: Theme.textPrimary
    placeholderTextColor: Theme.textSecondary
    font.pixelSize: Theme.fontSizeNormal
    selectByMouse: true

    leftPadding: 10
    rightPadding: 10
    topPadding: 8
    bottomPadding: 8

    background: Rectangle {
        radius: Theme.radiusNormal
        color: Theme.inputBg
        border.color: Theme.inputBorder
        border.width: 1
    }
}

