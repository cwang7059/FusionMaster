import QtQuick 2.14
import QtQuick.Layouts 1.14

Rectangle {
    id: root

    property string title: ""
    property string iconText: ""
    property color iconColor: "#00D7FF"
    property var items: []
    property bool progressMode: false
    property color cardBg: "#0B192A"
    property color panelBg: "#071A2C"
    property color cardBorder: "#16314C"
    property color textPrimary: "#F8FBFF"
    property color textSecondary: "#A7B7C8"
    property color textMuted: "#6F849A"

    Layout.fillWidth: true
    Layout.preferredHeight: 214
    radius: 6
    color: cardBg
    border.color: cardBorder
    border.width: 1

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 18
        spacing: 16

        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            Text {
                text: root.iconText
                color: root.iconColor
                font.pixelSize: 22
                font.bold: true
            }

            Text {
                Layout.fillWidth: true
                text: root.title
                color: root.textPrimary
                font.pixelSize: 18
                font.bold: true
                elide: Text.ElideRight
            }
        }

        GridLayout {
            Layout.fillWidth: true
            columns: 2
            columnSpacing: 18
            rowSpacing: 16

            Repeater {
                model: root.items || []

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 6

                    Text {
                        Layout.fillWidth: true
                        text: modelData.name
                        color: root.textSecondary
                        font.pixelSize: 13
                        elide: Text.ElideRight
                    }

                    Item {
                        Layout.fillWidth: true
                        Layout.preferredHeight: root.progressMode ? 22 : 24

                        Text {
                            anchors.left: parent.left
                            anchors.verticalCenter: parent.verticalCenter
                            visible: !root.progressMode
                            text: modelData.value
                            color: modelData.color
                            font.pixelSize: 20
                            font.bold: true
                        }

                        RowLayout {
                            anchors.fill: parent
                            visible: root.progressMode
                            spacing: 9

                            Rectangle {
                                Layout.fillWidth: true
                                Layout.preferredHeight: 10
                                radius: 5
                                color: root.panelBg

                                Rectangle {
                                    width: parent.width * Number(modelData.sub || 0) / 100
                                    height: parent.height
                                    radius: 5
                                    color: modelData.color
                                }
                            }

                            Text {
                                text: modelData.value
                                color: modelData.color
                                font.pixelSize: 14
                                font.bold: true
                            }
                        }
                    }

                    Text {
                        Layout.fillWidth: true
                        visible: !root.progressMode && modelData.sub !== ""
                        text: modelData.sub
                        color: root.textMuted
                        font.pixelSize: 12
                        elide: Text.ElideRight
                    }
                }
            }
        }
    }
}
