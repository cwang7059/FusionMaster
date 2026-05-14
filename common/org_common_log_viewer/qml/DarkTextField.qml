import QtQuick 2.14
import QtQuick.Controls 2.14
import UITheme 1.0

TextField {
    id: root
    color: Theme.textPrimary
    selectByMouse: true
    persistentSelection: true
    placeholderTextColor: Theme.textSecondary
    selectionColor: Theme.accent
    selectedTextColor: Theme.accentText
    leftPadding: 10
    rightPadding: 10
    topPadding: 6
    bottomPadding: 6

    background: Rectangle {
        radius: 2
        color: Theme.inputBg
        border.color: Theme.inputBorder
    }
}
