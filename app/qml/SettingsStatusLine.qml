import QtQuick 2.14
import QtQuick.Layouts 1.14

RowLayout {
    id: root

    property string label: ""
    property string value: ""
    property color labelColor: "#A7B7C8"
    property color valueColor: "#A7B7C8"
    property color strongColorA: "#00D38A"
    property color strongColorB: "#00D7FF"
    property bool pulse: false

    Layout.fillWidth: true
    spacing: 10

    Text {
        Layout.fillWidth: true
        text: root.label
        color: root.labelColor
        font.pixelSize: 14
        elide: Text.ElideRight
    }

    RowLayout {
        spacing: 7

        Rectangle {
            Layout.preferredWidth: 9
            Layout.preferredHeight: 9
            radius: 5
            color: root.valueColor
            visible: root.pulse

            SequentialAnimation on opacity {
                running: root.pulse
                loops: Animation.Infinite
                NumberAnimation { from: 0.45; to: 1.0; duration: 700 }
                NumberAnimation { from: 1.0; to: 0.45; duration: 700 }
            }
        }

        Text {
            text: root.value
            color: root.valueColor
            font.pixelSize: 14
            font.bold: root.valueColor === root.strongColorA || root.valueColor === root.strongColorB
        }
    }
}
