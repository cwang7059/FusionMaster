import QtQuick 2.14
import QtQuick.Controls 2.14

CheckBox {
    id: root

    property color borderColor: "#4D95C5"
    property color fillColor: "#2B78AC"
    property color textColor: "#EAF4FF"

    spacing: 6
    implicitHeight: 24

    indicator: Rectangle {
        implicitWidth: 16
        implicitHeight: 16
        x: 0
        y: (root.height - height) / 2
        radius: 2
        color: root.checked ? root.fillColor : "#102F4E"
        border.color: root.borderColor

        Text {
            anchors.centerIn: parent
            text: root.checked ? "\u2713" : ""
            color: "#F3FAFF"
            font.pixelSize: 12
            font.bold: true
        }
    }

    contentItem: Text {
        text: root.text
        color: root.textColor
        height: root.height
        leftPadding: root.indicator.width + root.spacing
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight
        font.pixelSize: 14
    }
}
