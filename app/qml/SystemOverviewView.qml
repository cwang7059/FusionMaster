import QtQuick 2.14
import QtQuick.Controls 2.14
import QtQuick.Layouts 1.14

Item {
    id: root

    readonly property color pageBg: "#071321"
    readonly property color cardBg: "#0B192A"
    readonly property color cardBorder: "#16314C"
    readonly property color textPrimary: "#F8FBFF"
    readonly property color textSecondary: "#A7B7C8"
    readonly property color cyan: "#00D7FF"
    readonly property color green: "#00D38A"

    readonly property var metricItems: [
        { "label": "总数据批次", "value": "128", "icon": "▣", "accent": "#0EA5E9" },
        { "label": "数据完整率", "value": "99.8%", "icon": "✓", "accent": "#00D38A" },
        { "label": "平均置信度", "value": "94.5", "icon": "∿", "accent": "#8B5CF6" },
        { "label": "待处理告警", "value": "3", "icon": "△", "accent": "#F59E0B" }
    ]

    readonly property var taskItems: [
        { "code": "T2026-0512-A", "time": "2026-05-12 14:30", "state": "已完成", "stateColor": "#00C176", "stateBg": "#063D31" },
        { "code": "T2026-0513-B", "time": "2026-05-13 09:15", "state": "已完成", "stateColor": "#00C176", "stateBg": "#063D31" },
        { "code": "T2026-0514-C", "time": "2026-05-14 10:00", "state": "分析中", "stateColor": "#FFD23C", "stateBg": "#4A3B05" }
    ]

    Rectangle {
        anchors.fill: parent
        color: root.pageBg
    }

    Flickable {
        anchors.fill: parent
        clip: true
        contentWidth: width
        contentHeight: contentColumn.implicitHeight + 48
        boundsBehavior: Flickable.StopAtBounds

        ColumnLayout {
            id: contentColumn
            x: 24
            y: 26
            width: Math.max(0, root.width - 48)
            spacing: 24

            ColumnLayout {
                Layout.fillWidth: true
                spacing: 8

                Text {
                    Layout.fillWidth: true
                    text: "系统总览"
                    color: root.textPrimary
                    font.pixelSize: 26
                    font.bold: true
                }

                Text {
                    Layout.fillWidth: true
                    text: "目标探测试验数据实时监控与分析"
                    color: root.textSecondary
                    font.pixelSize: 16
                }
            }

            GridLayout {
                id: metricsGrid
                Layout.fillWidth: true
                columns: root.width < 1120 ? 2 : 4
                columnSpacing: 16
                rowSpacing: 16

                Repeater {
                    model: root.metricItems

                    delegate: Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 130
                        radius: 6
                        color: root.cardBg
                        border.color: root.cardBorder
                        border.width: 1

                        ColumnLayout {
                            anchors.left: parent.left
                            anchors.top: parent.top
                            anchors.bottom: parent.bottom
                            anchors.leftMargin: 22
                            anchors.topMargin: 22
                            anchors.bottomMargin: 20
                            spacing: 18

                            Text {
                                text: modelData.label
                                color: root.textSecondary
                                font.pixelSize: 14
                            }

                            Text {
                                text: modelData.value
                                color: root.textPrimary
                                font.pixelSize: 32
                                font.bold: true
                            }
                        }

                        Rectangle {
                            anchors.right: parent.right
                            anchors.top: parent.top
                            anchors.rightMargin: 20
                            anchors.topMargin: 20
                            width: 40
                            height: 40
                            radius: 10
                            color: modelData.accent

                            Text {
                                anchors.centerIn: parent
                                text: modelData.icon
                                color: "#FFFFFF"
                                font.pixelSize: 22
                                font.bold: true
                            }
                        }
                    }
                }
            }

            GridLayout {
                Layout.fillWidth: true
                columns: root.width < 1060 ? 1 : 2
                columnSpacing: 16
                rowSpacing: 16

                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 296
                    radius: 6
                    color: root.cardBg
                    border.color: root.cardBorder
                    border.width: 1

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 24
                        spacing: 18

                        Text {
                            Layout.fillWidth: true
                            text: "近期试验任务"
                            color: root.textPrimary
                            font.pixelSize: 20
                            font.bold: true
                        }

                        Repeater {
                            model: root.taskItems

                            delegate: Rectangle {
                                Layout.fillWidth: true
                                Layout.preferredHeight: 56
                                radius: 4
                                color: Qt.rgba(6 / 255, 20 / 255, 35 / 255, 0.38)

                                RowLayout {
                                    anchors.fill: parent
                                    anchors.leftMargin: 12
                                    anchors.rightMargin: 12
                                    spacing: 10

                                    ColumnLayout {
                                        Layout.fillWidth: true
                                        spacing: 3

                                        Text {
                                            Layout.fillWidth: true
                                            text: modelData.code
                                            color: root.cyan
                                            font.pixelSize: 13
                                            font.bold: true
                                            elide: Text.ElideRight
                                        }

                                        Text {
                                            Layout.fillWidth: true
                                            text: modelData.time
                                            color: "#7F94AA"
                                            font.pixelSize: 12
                                            elide: Text.ElideRight
                                        }
                                    }

                                    Rectangle {
                                        Layout.preferredWidth: 54
                                        Layout.preferredHeight: 24
                                        radius: 3
                                        color: modelData.stateBg

                                        Text {
                                            anchors.centerIn: parent
                                            text: modelData.state
                                            color: modelData.stateColor
                                            font.pixelSize: 12
                                            font.bold: true
                                        }
                                    }
                                }
                            }
                        }
                    }
                }

                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 296
                    radius: 6
                    color: root.cardBg
                    border.color: root.cardBorder
                    border.width: 1

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 24
                        spacing: 18

                        Text {
                            Layout.fillWidth: true
                            text: "系统状态"
                            color: root.textPrimary
                            font.pixelSize: 20
                            font.bold: true
                        }

                        GridLayout {
                            Layout.fillWidth: true
                            columns: 2
                            rowSpacing: 18
                            columnSpacing: 18

                            Text {
                                text: "数据库连接"
                                color: root.textSecondary
                                font.pixelSize: 15
                            }

                            Row {
                                Layout.alignment: Qt.AlignRight
                                spacing: 8

                                Rectangle {
                                    width: 8
                                    height: 8
                                    radius: 4
                                    color: root.green
                                    anchors.verticalCenter: parent.verticalCenter
                                }

                                Text {
                                    text: "正常"
                                    color: root.green
                                    font.pixelSize: 15
                                    font.bold: true
                                }
                            }

                            Text {
                                text: "运行环境"
                                color: root.textSecondary
                                font.pixelSize: 15
                            }

                            Text {
                                Layout.alignment: Qt.AlignRight
                                text: "兼容良好"
                                color: root.green
                                font.pixelSize: 15
                                font.bold: true
                            }

                            Text {
                                text: "网络通信时延"
                                color: root.textSecondary
                                font.pixelSize: 15
                            }

                            Text {
                                Layout.alignment: Qt.AlignRight
                                text: "12ms"
                                color: root.cyan
                                font.pixelSize: 15
                            }

                            Text {
                                text: "自动备份"
                                color: root.textSecondary
                                font.pixelSize: 15
                            }

                            Text {
                                Layout.alignment: Qt.AlignRight
                                text: "今日 02:00 已完成"
                                color: root.textSecondary
                                font.pixelSize: 15
                            }
                        }
                    }
                }
            }
        }
    }
}
