import QtQuick 2.14
import QtQuick.Controls 2.14
import UITheme 1.0

Button {
    id: control

    property color fillColor: Theme.accentWeak
    property color fillPressedColor: Qt.darker(fillColor, 1.14)
    property color fillHoverColor: Qt.lighter(fillColor, 1.08)
    property color textColor: Theme.textPrimary
    property color strokeColor: Qt.rgba(1, 1, 1, 0.16)
    property color hoverStrokeColor: Qt.rgba(148 / 255, 163 / 255, 184 / 255, 0.52)
    property color focusStrokeColor: Qt.rgba(91 / 255, 127 / 255, 174 / 255, 0.38)

    hoverEnabled: true
    leftPadding: 14
    rightPadding: 14
    topPadding: 7
    bottomPadding: 7
    implicitWidth: Math.max(96, contentItem.implicitWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(34, contentItem.implicitHeight + topPadding + bottomPadding)

    contentItem: Text {
        text: control.text
        color: control.enabled ? control.textColor : Qt.rgba(1, 1, 1, 0.45)
        font.pixelSize: Theme.fontSizeNormal
        font.weight: Font.Medium
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight
        maximumLineCount: 1
        clip: true
    }

    background: Rectangle {
        radius: Theme.radiusNormal
        border.width: 1
        border.color: {
            if (!control.enabled) {
                return Qt.rgba(1, 1, 1, 0.1)
            }
            if (control.activeFocus || control.down) {
                return control.focusStrokeColor
            }
            if (control.hovered) {
                return control.hoverStrokeColor
            }
            return control.strokeColor
        }
        color: {
            if (!control.enabled) {
                return Qt.rgba(1, 1, 1, 0.1)
            }
            if (control.down) {
                return control.fillPressedColor
            }
            if (control.hovered) {
                return control.fillHoverColor
            }
            return control.fillColor
        }
    }
}
