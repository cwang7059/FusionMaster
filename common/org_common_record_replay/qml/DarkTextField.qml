import QtQuick 2.14
import QtQuick.Controls 2.14

TextField {
    id: root
    implicitHeight: 26
    color: "#EAF4FF"
    selectByMouse: true
    persistentSelection: true
    placeholderTextColor: "#8FB2D4"
    selectionColor: "#2E6D9A"
    selectedTextColor: "#F2F9FF"
    leftPadding: 10
    rightPadding: 10
    topPadding: 4
    bottomPadding: 4

    background: Rectangle {
        radius: 2
        color: "#102F4E"
        border.color: "#2D5A83"
    }
}
