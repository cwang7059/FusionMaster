import QtQuick 2.14
import QtQuick.Controls 2.14
import QtQuick.Layouts 1.14

Item {
    id: root

    readonly property color pageBg: "#071321"
    readonly property color cardBg: "#0B192A"
    readonly property color panelBg: "#071A2C"
    readonly property color cardBorder: "#16314C"
    readonly property color textPrimary: "#F8FBFF"
    readonly property color textSecondary: "#A7B7C8"
    readonly property color textMuted: "#6F849A"
    readonly property color cyan: "#00D7FF"
    readonly property color green: "#00D38A"
    readonly property color yellow: "#FFD23C"
    readonly property color red: "#FF6B7A"
    readonly property color purple: "#9B7CFF"

    readonly property var seriesRows: [
        { "time": "10:00", "confidence": 92.5, "detection": 95.2 },
        { "time": "10:30", "confidence": 93.8, "detection": 96.1 },
        { "time": "11:00", "confidence": 91.2, "detection": 94.8 },
        { "time": "11:30", "confidence": 94.5, "detection": 97.3 },
        { "time": "12:00", "confidence": 93.1, "detection": 95.9 },
        { "time": "12:30", "confidence": 95.2, "detection": 98.1 },
        { "time": "13:00", "confidence": 94.8, "detection": 97.5 },
        { "time": "13:30", "confidence": 96.1, "detection": 98.8 }
    ]
    readonly property var channelRows: [
        { "channel": "可见光", "values": [92, 94, 91, 93, 95] },
        { "channel": "红外", "values": [88, 90, 87, 91, 89] },
        { "channel": "激光", "values": [95, 96, 94, 97, 96] },
        { "channel": "多传感器", "values": [97, 98, 96, 98, 99] }
    ]
    readonly property var scatterRows: [
        { "azimuth": 18, "elevation": 22, "confidence": 82 }, { "azimuth": 35, "elevation": 48, "confidence": 94 },
        { "azimuth": 62, "elevation": 34, "confidence": 88 }, { "azimuth": 84, "elevation": 57, "confidence": 91 },
        { "azimuth": 110, "elevation": 26, "confidence": 78 }, { "azimuth": 136, "elevation": 72, "confidence": 96 },
        { "azimuth": 158, "elevation": 41, "confidence": 87 }, { "azimuth": 186, "elevation": 65, "confidence": 92 },
        { "azimuth": 214, "elevation": 32, "confidence": 84 }, { "azimuth": 242, "elevation": 50, "confidence": 89 },
        { "azimuth": 268, "elevation": 75, "confidence": 97 }, { "azimuth": 294, "elevation": 18, "confidence": 74 },
        { "azimuth": 318, "elevation": 44, "confidence": 90 }, { "azimuth": 340, "elevation": 60, "confidence": 93 },
        { "azimuth": 28, "elevation": 70, "confidence": 95 }, { "azimuth": 124, "elevation": 12, "confidence": 76 },
        { "azimuth": 202, "elevation": 83, "confidence": 98 }, { "azimuth": 282, "elevation": 36, "confidence": 86 }
    ]
    readonly property var radarRows: [
        { "subject": "探测成功率", "value": 96.5 },
        { "subject": "空域覆盖率", "value": 92.3 },
        { "subject": "响应速度", "value": 88.7 },
        { "subject": "抗干扰能力", "value": 85.4 },
        { "subject": "多目标处理", "value": 90.2 },
        { "subject": "精度稳定性", "value": 94.1 }
    ]
    readonly property var barColors: ["#00D7FF", "#2F7DF6", "#9B7CFF", "#EC4899", "#F59E0B"]

    Rectangle {
        anchors.fill: parent
        color: root.pageBg
    }

    Flickable {
        anchors.fill: parent
        clip: true
        contentWidth: width
        contentHeight: contentColumn.implicitHeight + 52
        boundsBehavior: Flickable.StopAtBounds

        ColumnLayout {
            id: contentColumn
            x: 24
            y: 26
            width: Math.max(0, root.width - 48)
            spacing: 22

            ColumnLayout {
                Layout.fillWidth: true
                spacing: 8

                Text {
                    Layout.fillWidth: true
                    text: "统计分析与可视化展示"
                    color: root.textPrimary
                    font.pixelSize: 26
                    font.bold: true
                }

                Text {
                    Layout.fillWidth: true
                    text: "深度量化指标与多维度图表分析"
                    color: root.textSecondary
                    font.pixelSize: 16
                }
            }

            Rectangle {
                Layout.fillWidth: true
                implicitHeight: controlColumn.implicitHeight + 32
                radius: 6
                color: root.cardBg
                border.color: root.cardBorder
                border.width: 1

                ColumnLayout {
                    id: controlColumn
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.top: parent.top
                    anchors.margins: 16
                    spacing: 16

                    Text {
                        text: "统计维度控制面板"
                        color: root.textPrimary
                        font.pixelSize: 20
                        font.bold: true
                    }

                    GridLayout {
                        Layout.fillWidth: true
                        columns: root.width < 1040 ? 1 : 2
                        columnSpacing: 18
                        rowSpacing: 14

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 9

                            Text {
                                text: "分组统计维度"
                                color: root.textSecondary
                                font.pixelSize: 13
                            }

                            Flow {
                                Layout.fillWidth: true
                                spacing: 9

                                Repeater {
                                    model: ["按无人机台数 (1-10台)", "按目标类型", "按距离区间", "按高度区间"]

                                    PrototypeActionButton {
                                        text: modelData
                                        preferredWidth: Math.max(116, labelMetrics.advanceWidth + 24)
                                        fill: root.panelBg
                                        stroke: root.cardBorder
                                        textFill: root.textSecondary

                                        TextMetrics {
                                            id: labelMetrics
                                            text: modelData
                                            font.pixelSize: 14
                                        }
                                    }
                                }
                            }
                        }

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 9

                            Text {
                                text: "自定义计算公式"
                                color: root.textSecondary
                                font.pixelSize: 13
                            }

                            Rectangle {
                                Layout.fillWidth: true
                                Layout.preferredHeight: 42
                                radius: 4
                                color: root.panelBg
                                border.color: root.cardBorder
                                border.width: 1

                                Text {
                                    anchors.left: parent.left
                                    anchors.right: parent.right
                                    anchors.verticalCenter: parent.verticalCenter
                                    anchors.leftMargin: 14
                                    anchors.rightMargin: 14
                                    text: "成功样本数 / 统计总样本数 × 100%"
                                    color: root.cyan
                                    font.pixelSize: 14
                                    font.family: "Consolas"
                                    elide: Text.ElideRight
                                }
                            }
                        }
                    }
                }
            }

            GridLayout {
                Layout.fillWidth: true
                columns: root.width < 1040 ? 1 : 2
                columnSpacing: 16
                rowSpacing: 16

                StatisticsMetricPanel {
                    title: "目标探测精度评估"
                    iconText: "◎"
                    iconColor: root.cyan
                    cardBg: root.cardBg
                    panelBg: root.panelBg
                    cardBorder: root.cardBorder
                    textPrimary: root.textPrimary
                    textSecondary: root.textSecondary
                    textMuted: root.textMuted
                    items: [
                        { "name": "探测距离误差", "value": "±1.2m", "sub": "最大误差: ±3.5m", "color": root.cyan },
                        { "name": "方位角误差", "value": "±0.05°", "sub": "最大误差: ±0.12°", "color": root.cyan },
                        { "name": "俯仰角误差", "value": "±0.04°", "sub": "最大误差: ±0.10°", "color": root.cyan },
                        { "name": "平均置信度评分", "value": "94.5 / 100", "sub": "", "color": root.green }
                    ]
                }

                StatisticsMetricPanel {
                    title: "综合处置效果评估"
                    iconText: "◉"
                    iconColor: root.green
                    progressMode: true
                    cardBg: root.cardBg
                    panelBg: root.panelBg
                    cardBorder: root.cardBorder
                    textPrimary: root.textPrimary
                    textSecondary: root.textSecondary
                    textMuted: root.textMuted
                    items: [
                        { "name": "探测成功率", "value": "96.5%", "sub": "96.5", "color": root.green },
                        { "name": "空域覆盖率", "value": "92.3%", "sub": "92.3", "color": root.cyan },
                        { "name": "漏判率", "value": "2.1%", "sub": "2.1", "color": root.yellow },
                        { "name": "误报率", "value": "1.4%", "sub": "1.4", "color": root.red }
                    ]
                }
            }

            StatisticsChartCard {
                title: "目标探测空间位置分布图 (方位角 × 俯仰角)"
                preferredHeight: 412
                canvasMode: "scatter"
                scatterRows: root.scatterRows
                cardBg: root.cardBg
                cardBorder: root.cardBorder
                textPrimary: root.textPrimary
                textSecondary: root.textSecondary
                textMuted: root.textMuted
                cyan: root.cyan
                purple: root.purple
            }

            StatisticsChartCard {
                title: "测量数据时序动态演变图"
                preferredHeight: 360
                canvasMode: "line"
                seriesRows: root.seriesRows
                cardBg: root.cardBg
                cardBorder: root.cardBorder
                textPrimary: root.textPrimary
                textSecondary: root.textSecondary
                textMuted: root.textMuted
                cyan: root.cyan
                purple: root.purple
            }

            StatisticsChartCard {
                title: "不同批次/不同传感器通道探测置信度对比"
                preferredHeight: 360
                canvasMode: "bar"
                channelRows: root.channelRows
                barColors: root.barColors
                cardBg: root.cardBg
                cardBorder: root.cardBorder
                textPrimary: root.textPrimary
                textSecondary: root.textSecondary
                textMuted: root.textMuted
                cyan: root.cyan
                purple: root.purple
            }

            StatisticsChartCard {
                title: "多工况测试条件影响规律综合分析图"
                preferredHeight: 430
                canvasMode: "radar"
                radarRows: root.radarRows
                cardBg: root.cardBg
                cardBorder: root.cardBorder
                textPrimary: root.textPrimary
                textSecondary: root.textSecondary
                textMuted: root.textMuted
                cyan: root.cyan
                purple: root.purple
            }
        }
    }
}
