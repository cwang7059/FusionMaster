import QtQuick 2.14
import QtQuick.Layouts 1.14

RowLayout {
    id: root

    property string iconText: ""
    property color iconColor: "#00D7FF"
    property string titleText: ""
    property color textColor: "#F8FBFF"

    Layout.fillWidth: true
    spacing: 8

    Text {
        text: root.iconText
        color: root.iconColor
        font.pixelSize: 20
        font.bold: true
    }

    Text {
        Layout.fillWidth: true
        text: root.titleText
        color: root.textColor
        font.pixelSize: 18
        font.bold: true
        elide: Text.ElideRight
    }
}
