import QtQuick 2.14

Rectangle {
    id: root

    property string value: ""
    property color textColor: "#A7B7C8"
    property color rowColor: "#0B192A"
    property color lineColor: "#102B44"
    property bool mono: false

    height: 48
    color: rowColor
    border.color: lineColor
    border.width: 1

    Text {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.verticalCenter: parent.verticalCenter
        anchors.leftMargin: 12
        anchors.rightMargin: 8
        text: root.value
        color: root.textColor
        font.pixelSize: 13
        font.family: root.mono ? "Consolas" : ""
        elide: Text.ElideRight
    }
}
