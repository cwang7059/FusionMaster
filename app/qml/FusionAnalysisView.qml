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
    readonly property color blue: "#2F7DF6"
    readonly property color green: "#00D38A"
    readonly property color yellow: "#FFD23C"
    readonly property color purple: "#9B7CFF"

    readonly property var comparisonRows: [
        { "batch": "B001", "mean": "1250", "max": "3500", "min": "850", "stdDev": "245" },
        { "batch": "B002", "mean": "1320", "max": "3800", "min": "900", "stdDev": "268" },
        { "batch": "B003", "mean": "1180", "max": "3200", "min": "820", "stdDev": "221" },
        { "batch": "B004", "mean": "1290", "max": "3650", "min": "880", "stdDev": "255" },
        { "batch": "B005", "mean": "1265", "max": "3420", "min": "870", "stdDev": "238" }
    ]
    readonly property var distributionRows: [
        { "distance": "0-1000", "values": [12, 15, 10, 14, 11] },
        { "distance": "1000-2000", "values": [28, 25, 30, 27, 29] },
        { "distance": "2000-3000", "values": [35, 38, 32, 36, 34] },
        { "distance": "3000-4000", "values": [18, 16, 20, 17, 19] },
        { "distance": "4000+", "values": [7, 6, 8, 6, 7] }
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
                    text: "多批次融合分析"
                    color: root.textPrimary
                    font.pixelSize: 26
                    font.bold: true
                }

                Text {
                    Layout.fillWidth: true
                    text: "支持不少于5批次的数据融合与对比分析"
                    color: root.textSecondary
                    font.pixelSize: 16
                }
            }

            Rectangle {
                Layout.fillWidth: true
                implicitHeight: taskConfigColumn.implicitHeight + 32
                radius: 6
                color: root.cardBg
                border.color: root.cardBorder
                border.width: 1

                ColumnLayout {
                    id: taskConfigColumn
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.top: parent.top
                    anchors.margins: 16
                    spacing: 17

                    Text {
                        text: "融合任务配置"
                        color: root.textPrimary
                        font.pixelSize: 20
                        font.bold: true
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 12

                        PrototypeActionButton {
                            text: "+ 添加对比批次"
                            preferredWidth: 146
                            fill: "#0891B2"
                            stroke: Qt.rgba(1, 1, 1, 0.12)
                            textFill: "#FFFFFF"
                            strongText: true
                        }

                        Text {
                            text: "已选 5/10 批次"
                            color: root.cyan
                            font.pixelSize: 14
                        }

                        Item { Layout.fillWidth: true }
                    }

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 9

                        Text {
                            text: "测试条件对比维度"
                            color: root.textSecondary
                            font.pixelSize: 13
                        }

                        GridLayout {
                            Layout.fillWidth: true
                            columns: root.width < 1120 ? 3 : 5
                            columnSpacing: 12
                            rowSpacing: 10

                            Repeater {
                                model: ["时间差异", "地点环境", "距离与高度", "搜索/扫描工况", "目标数量差异"]

                                Rectangle {
                                    Layout.fillWidth: true
                                    Layout.preferredHeight: 38
                                    radius: 4
                                    color: root.panelBg
                                    border.color: root.cardBorder
                                    border.width: 1

                                    RowLayout {
                                        anchors.fill: parent
                                        anchors.leftMargin: 10
                                        anchors.rightMargin: 10
                                        spacing: 6

                                        CheckBox {
                                            checked: true
                                            Layout.preferredWidth: 22
                                            Layout.preferredHeight: 22
                                        }

                                        Text {
                                            Layout.fillWidth: true
                                            text: modelData
                                            color: root.textSecondary
                                            font.pixelSize: 13
                                            elide: Text.ElideRight
                                        }
                                    }
                                }
                            }
                        }
                    }

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 8

                        Text {
                            text: "计算指标配置"
                            color: root.textSecondary
                            font.pixelSize: 13
                        }

                        ComboBox {
                            Layout.preferredWidth: Math.min(460, root.width - 96)
                            Layout.preferredHeight: 38
                            model: [
                                "标准统计分析方案 (均值/方差/极值)",
                                "高级融合分析方案 (加权平均/趋势拟合)",
                                "自定义计算规则"
                            ]
                        }
                    }

                    PrototypeActionButton {
                        text: "⚡ 智能融合分析"
                        preferredWidth: 168
                        fill: "#0EA5E9"
                        stroke: "#1D9FEF"
                        textFill: "#FFFFFF"
                        strongText: true
                    }
                }
            }

            GridLayout {
                Layout.fillWidth: true
                columns: root.width < 1120 ? 1 : 3
                columnSpacing: 16
                rowSpacing: 16

                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 290
                    radius: 6
                    color: root.cardBg
                    border.color: root.cardBorder
                    border.width: 1

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 18
                        spacing: 13

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 8

                            Text {
                                text: "✓"
                                color: root.green
                                font.pixelSize: 20
                                font.bold: true
                            }

                            Text {
                                Layout.fillWidth: true
                                text: "数据质量评估概览"
                                color: root.textPrimary
                                font.pixelSize: 18
                                font.bold: true
                                elide: Text.ElideRight
                            }
                        }

                        Repeater {
                            model: [
                                { "name": "关键字段缺失", "value": "12 条", "sub": "完整率 99.8%", "color": root.green },
                                { "name": "异常值检测", "value": "5 处", "sub": "已自动隔离", "color": root.yellow },
                                { "name": "字段一致性校验", "value": "通过", "sub": "校验规则匹配度 100%", "color": root.green }
                            ]

                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: 5

                                RowLayout {
                                    Layout.fillWidth: true

                                    Text {
                                        Layout.fillWidth: true
                                        text: modelData.name
                                        color: root.textSecondary
                                        font.pixelSize: 14
                                    }

                                    Text {
                                        text: modelData.value
                                        color: modelData.color
                                        font.pixelSize: 15
                                        font.bold: true
                                    }
                                }

                                Text {
                                    Layout.fillWidth: true
                                    text: modelData.sub
                                    color: root.textMuted
                                    font.pixelSize: 12
                                }

                                Rectangle {
                                    Layout.fillWidth: true
                                    Layout.preferredHeight: 1
                                    color: "#132D47"
                                }
                            }
                        }
                    }
                }

                Rectangle {
                    Layout.columnSpan: root.width < 1120 ? 1 : 2
                    Layout.fillWidth: true
                    Layout.preferredHeight: 290
                    radius: 6
                    color: root.cardBg
                    border.color: root.cardBorder
                    border.width: 1

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 18
                        spacing: 14

                        RowLayout {
                            Layout.fillWidth: true

                            Text {
                                Layout.fillWidth: true
                                text: "融合一致性对比"
                                color: root.textPrimary
                                font.pixelSize: 18
                                font.bold: true
                            }

                            Repeater {
                                model: ["探测距离", "方位角", "俯仰角"]

                                PrototypeActionButton {
                                    text: modelData
                                    preferredWidth: 82
                                    fill: "#0891B2"
                                    stroke: Qt.rgba(1, 1, 1, 0.1)
                                    textFill: "#FFFFFF"
                                    strongText: true
                                }
                            }
                        }

                        Column {
                            Layout.fillWidth: true

                            Row {
                                width: parent.width
                                height: 34
                                PrototypeTableCell { width: parent.width * 0.2; height: 34; value: "批次"; textColor: root.textSecondary; rowColor: root.panelBg }
                                PrototypeTableCell { width: parent.width * 0.2; height: 34; value: "均值 (m)"; textColor: root.textSecondary; rowColor: root.panelBg }
                                PrototypeTableCell { width: parent.width * 0.2; height: 34; value: "最大值 (m)"; textColor: root.textSecondary; rowColor: root.panelBg }
                                PrototypeTableCell { width: parent.width * 0.2; height: 34; value: "最小值 (m)"; textColor: root.textSecondary; rowColor: root.panelBg }
                                PrototypeTableCell { width: parent.width * 0.2; height: 34; value: "标准差"; textColor: root.textSecondary; rowColor: root.panelBg }
                            }

                            Repeater {
                                model: root.comparisonRows

                                Row {
                                    width: parent.width
                                    height: 38
                                    property color rowColor: index % 2 === 0 ? root.cardBg : "#0A1726"
                                    PrototypeTableCell { width: parent.width * 0.2; height: 38; value: modelData.batch; textColor: root.cyan; rowColor: parent.rowColor; mono: true }
                                    PrototypeTableCell { width: parent.width * 0.2; height: 38; value: modelData.mean; textColor: root.textSecondary; rowColor: parent.rowColor }
                                    PrototypeTableCell { width: parent.width * 0.2; height: 38; value: modelData.max; textColor: root.textSecondary; rowColor: parent.rowColor }
                                    PrototypeTableCell { width: parent.width * 0.2; height: 38; value: modelData.min; textColor: root.textSecondary; rowColor: parent.rowColor }
                                    PrototypeTableCell { width: parent.width * 0.2; height: 38; value: modelData.stdDev; textColor: root.textSecondary; rowColor: parent.rowColor }
                                }
                            }
                        }
                    }
                }

                Rectangle {
                    Layout.columnSpan: root.width < 1120 ? 1 : 3
                    Layout.fillWidth: true
                    Layout.preferredHeight: 132
                    radius: 6
                    color: root.cardBg
                    border.color: root.cardBorder
                    border.width: 1

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 18
                        spacing: 18

                        Text {
                            Layout.fillWidth: true
                            text: "系统处理性能复核"
                            color: root.textPrimary
                            font.pixelSize: 18
                            font.bold: true
                        }

                        GridLayout {
                            Layout.fillWidth: true
                            columns: 4
                            columnSpacing: 16

                            Repeater {
                                model: [
                                    { "name": "离线分析平均耗时", "value": "235", "unit": "ms", "color": root.cyan },
                                    { "name": "离线分析最大耗时", "value": "512", "unit": "ms", "color": root.yellow },
                                    { "name": "时延状态", "value": "优", "unit": "符合归档标准", "color": root.green },
                                    { "name": "数据吞吐量", "value": "8.2k", "unit": "条/s", "color": root.purple }
                                ]

                                ColumnLayout {
                                    Layout.fillWidth: true
                                    spacing: 5

                                    Text {
                                        Layout.fillWidth: true
                                        text: modelData.name
                                        color: root.textSecondary
                                        font.pixelSize: 13
                                        elide: Text.ElideRight
                                    }

                                    Text {
                                        Layout.fillWidth: true
                                        text: modelData.value + " " + modelData.unit
                                        color: modelData.color
                                        font.pixelSize: index === 2 ? 22 : 25
                                        font.bold: true
                                        elide: Text.ElideRight
                                    }
                                }
                            }
                        }
                    }
                }
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 382
                radius: 6
                color: root.cardBg
                border.color: root.cardBorder
                border.width: 1

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 18
                    spacing: 12

                    Text {
                        Layout.fillWidth: true
                        text: "关键参数多传感器空间分布差异对比图"
                        color: root.textPrimary
                        font.pixelSize: 18
                        font.bold: true
                    }

                    Canvas {
                        id: distributionChart
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        onWidthChanged: requestPaint()
                        onHeightChanged: requestPaint()
                        onPaint: {
                            var ctx = getContext("2d")
                            var w = width
                            var h = height
                            ctx.reset()
                            ctx.clearRect(0, 0, w, h)

                            var left = 48
                            var right = 20
                            var top = 12
                            var bottom = 44
                            var chartW = Math.max(1, w - left - right)
                            var chartH = Math.max(1, h - top - bottom)

                            ctx.strokeStyle = "rgba(255,255,255,0.08)"
                            ctx.lineWidth = 1
                            ctx.font = "12px Segoe UI"
                            ctx.fillStyle = root.textMuted
                            for (var y = 0; y <= 4; ++y) {
                                var yy = top + y * chartH / 4
                                ctx.beginPath()
                                ctx.moveTo(left, yy)
                                ctx.lineTo(left + chartW, yy)
                                ctx.stroke()
                                ctx.fillText(String(40 - y * 10), 12, yy + 4)
                            }

                            var groupW = chartW / root.distributionRows.length
                            var barW = Math.min(16, groupW / 8)
                            for (var i = 0; i < root.distributionRows.length; ++i) {
                                var row = root.distributionRows[i]
                                var startX = left + i * groupW + groupW * 0.18
                                var values = row.values || []
                                for (var j = 0; j < values.length; ++j) {
                                    var value = Number(values[j] || 0)
                                    var bh = value / 40 * chartH
                                    ctx.fillStyle = root.barColors[j]
                                    ctx.fillRect(startX + j * (barW + 4), top + chartH - bh, barW, bh)
                                }
                                ctx.fillStyle = root.textMuted
                                ctx.fillText(row.distance, left + i * groupW + 4, top + chartH + 24)
                            }
                        }
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 16
                        Repeater {
                            model: ["批次1", "批次2", "批次3", "批次4", "批次5"]
                            RowLayout {
                                spacing: 6
                                Rectangle {
                                    Layout.preferredWidth: 10
                                    Layout.preferredHeight: 10
                                    radius: 2
                                    color: root.barColors[index]
                                }
                                Text {
                                    text: modelData
                                    color: root.textSecondary
                                    font.pixelSize: 12
                                }
                            }
                        }
                        Item { Layout.fillWidth: true }
                    }
                }
            }

            Rectangle {
                Layout.fillWidth: true
                implicitHeight: reportRow.implicitHeight + 32
                radius: 6
                color: root.cardBg
                border.color: root.cardBorder
                border.width: 1

                RowLayout {
                    id: reportRow
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.top: parent.top
                    anchors.margins: 16
                    spacing: 18

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 12

                        Text {
                            text: "生成综合分析报告"
                            color: root.textPrimary
                            font.pixelSize: 18
                            font.bold: true
                        }

                        Flow {
                            Layout.fillWidth: true
                            spacing: 14

                            Repeater {
                                model: ["数据分析结果", "核心影响规律", "结论与改进建议", "量化分析指标清单"]
                                CheckBox {
                                    id: reportCheck
                                    text: modelData
                                    checked: true
                                    contentItem: Text {
                                        text: reportCheck.text
                                        color: root.textSecondary
                                        font.pixelSize: 13
                                        leftPadding: reportCheck.indicator.width + reportCheck.spacing
                                        verticalAlignment: Text.AlignVCenter
                                    }
                                }
                            }
                        }
                    }

                    PrototypeActionButton {
                        text: "▤ 生成报告 (PDF/Word)"
                        preferredWidth: 194
                        fill: "#16A34A"
                        stroke: Qt.rgba(1, 1, 1, 0.12)
                        textFill: "#FFFFFF"
                        strongText: true
                    }
                }
            }
        }
    }
}
