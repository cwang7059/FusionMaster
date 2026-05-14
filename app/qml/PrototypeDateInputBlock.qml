import QtQuick 2.14
import QtQuick.Layouts 1.14

ColumnLayout {
    id: root

    property string title: ""
    property string valueText: ""
    property color labelColor: "#A7B7C8"
    property color valueColor: "#A7B7C8"
    property color panelColor: "#071A2C"
    property color borderColor: "#16314C"

    Layout.fillWidth: true
    spacing: 8

    Text {
        text: root.title
        color: root.labelColor
        font.pixelSize: 13
    }

    Rectangle {
        Layout.fillWidth: true
        Layout.preferredHeight: 38
        radius: 4
        color: root.panelColor
        border.color: root.borderColor
        border.width: 1

        Text {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.verticalCenter: parent.verticalCenter
            anchors.leftMargin: 12
            anchors.rightMargin: 12
            text: root.valueText
            color: root.valueColor
            font.pixelSize: 14
            elide: Text.ElideRight
        }
    }
}
