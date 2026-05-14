import QtQuick 2.14
import QtQuick.Controls 2.14
import QtQuick.Layouts 1.14
import UITheme 1.0

Rectangle {
    id: root
    color: Theme.inputBg
    border.color: Theme.inputBorder
    radius: Theme.radiusNormal

    property string titleText: ""
    property var rowsModel: []
    property string firstHeader: "主题"
    property string firstField: "topic"

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: Theme.spacingSmall
        spacing: Theme.spacingSmall

        Text {
            text: root.titleText
            color: Theme.textPrimary
            font.bold: true
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 28
            color: Theme.accentWeak
            border.color: Theme.inputBorder
            border.width: 1

            RowLayout {
                anchors.fill: parent
                spacing: 0

                Text {
                    Layout.fillWidth: true
                    leftPadding: 8
                    text: root.firstHeader
                    color: Theme.textPrimary
                    font.bold: true
                }
                Text {
                    Layout.preferredWidth: 64
                    horizontalAlignment: Text.AlignHCenter
                    text: "计数"
                    color: Theme.textPrimary
                    font.bold: true
                }
                Text {
                    Layout.preferredWidth: 80
                    horizontalAlignment: Text.AlignHCenter
                    text: "长度"
                    color: Theme.textPrimary
                    font.bold: true
                }
            }
        }

        ListView {
            id: listView
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            model: root.rowsModel

            delegate: Rectangle {
                width: listView.width
                height: 30
                color: index % 2 === 0 ? Theme.cardBg : Theme.inputBg
                border.color: Theme.inputBorder
                border.width: 1

                RowLayout {
                    anchors.fill: parent
                    spacing: 0

                    Text {
                        Layout.fillWidth: true
                        leftPadding: 8
                        rightPadding: 6
                        text: String(modelData[root.firstField] || modelData.topic || modelData.plugin || "(unknown)")
                        color: Theme.textPrimary
                        elide: Text.ElideRight
                    }
                    Text {
                        Layout.preferredWidth: 64
                        horizontalAlignment: Text.AlignHCenter
                        text: String(modelData.count || 0)
                        color: Theme.textPrimary
                    }
                    Text {
                        Layout.preferredWidth: 80
                        horizontalAlignment: Text.AlignHCenter
                        text: String(modelData.bytes || 0)
                        color: Theme.textPrimary
                    }
                }
            }

            ScrollBar.vertical: ScrollBar {
                policy: ScrollBar.AsNeeded
            }
        }
    }
}
