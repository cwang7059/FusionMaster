import QtQuick 2.14
import QtQuick.Layouts 1.14

Rectangle {
    id: root

    property string text: ""
    property color fill: "#071A2C"
    property color stroke: "#16314C"
    property color textFill: "#A7B7C8"
    property bool strongText: false
    property int preferredWidth: 92

    Layout.preferredWidth: preferredWidth
    Layout.preferredHeight: 38
    radius: 4
    color: fill
    border.color: stroke
    border.width: 1

    Text {
        anchors.centerIn: parent
        width: parent.width - 16
        text: root.text
        color: root.textFill
        font.pixelSize: 14
        font.bold: root.strongText
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight
    }
}
