import QtQuick 2.14
import QtQuick.Controls 2.14
import QtQuick.Layouts 1.14
import UITheme 1.0

Rectangle {
    id: root

    property string title: ""
    property var rows: []
    property Component delegateComponent

    color: "#102F42"
    border.color: "#31566B"
    radius: 2
    clip: true

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 8
        spacing: 5

        RowLayout {
            Layout.fillWidth: true

            Text {
                Layout.fillWidth: true
                color: Theme.textPrimary
                font.pixelSize: Theme.fontSizeNormal
                font.bold: true
                text: root.title
                elide: Text.ElideRight
            }

            Text {
                color: Theme.textSecondary
                text: root.rows ? root.rows.length : 0
            }
        }

        Rectangle {
            Layout.fillWidth: true
            height: 1
            color: "#31566B"
        }

        ListView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            model: root.rows || []
            delegate: root.delegateComponent
            boundsBehavior: Flickable.StopAtBounds
            ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }
        }
    }
}
