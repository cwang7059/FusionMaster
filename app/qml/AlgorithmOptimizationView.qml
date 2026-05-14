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

    readonly property var algorithms: [
        { "name": "相关性分析", "scenario": "多变量耦合分析", "color": "#0EA5E9", "selected": false },
        { "name": "回归建模", "scenario": "连续趋势预测", "color": "#3B82F6", "selected": true },
        { "name": "聚类分析", "scenario": "无监督航迹分类", "color": "#9B7CFF", "selected": false },
        { "name": "分类识别", "scenario": "目标定性识别", "color": "#EC4899", "selected": false },
        { "name": "时间序列分析", "scenario": "时域动态演变", "color": "#F59E0B", "selected": true }
    ]
    readonly property var factors: [
        { "factor": "探测距离", "score": 85.4, "direction": "负相关", "color": root.red, "trend": [20, 35, 48, 65, 78, 85] },
        { "factor": "能见度", "score": 78.2, "direction": "正相关", "color": root.green, "trend": [15, 28, 42, 58, 70, 78] },
        { "factor": "目标类型", "score": 72.6, "direction": "非线性", "color": root.textSecondary, "trend": [25, 38, 50, 62, 68, 73] },
        { "factor": "天气条件", "score": 68.9, "direction": "正相关", "color": root.green, "trend": [18, 32, 45, 55, 64, 69] },
        { "factor": "机动性", "score": 65.3, "direction": "负相关", "color": root.red, "trend": [22, 35, 48, 56, 62, 65] },
        { "factor": "传感器通道", "score": 58.7, "direction": "正相关", "color": root.green, "trend": [12, 25, 38, 46, 54, 59] }
    ]
    readonly property var confidenceTrend: [
        { "distance": 500, "confidence": 98.5 },
        { "distance": 1000, "confidence": 96.2 },
        { "distance": 1500, "confidence": 92.8 },
        { "distance": 2000, "confidence": 88.4 },
        { "distance": 2500, "confidence": 82.1 },
        { "distance": 3000, "confidence": 75.3 },
        { "distance": 3500, "confidence": 68.9 },
        { "distance": 4000, "confidence": 62.5 }
    ]

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
                    text: "算法优选与模型辨识"
                    color: root.textPrimary
                    font.pixelSize: 26
                    font.bold: true
                }

                Text {
                    Layout.fillWidth: true
                    text: "AI自主优选算法与影响因素归因分析"
                    color: root.textSecondary
                    font.pixelSize: 16
                }
            }

            Rectangle {
                Layout.fillWidth: true
                implicitHeight: featureRow.implicitHeight + 30
                radius: 6
                color: "#082744"
                border.color: "#1E6BA8"
                border.width: 1

                RowLayout {
                    id: featureRow
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.top: parent.top
                    anchors.margins: 15
                    spacing: 12

                    Text {
                        text: "✦"
                        color: root.cyan
                        font.pixelSize: 24
                    }

                    Text {
                        text: "环境特征智能识别"
                        color: root.textPrimary
                        font.pixelSize: 18
                        font.bold: true
                    }

                    Text {
                        text: "当前数据特征:"
                        color: root.textSecondary
                        font.pixelSize: 14
                    }

                    Repeater {
                        model: [
                            { "text": "目标距离大", "fg": root.cyan, "bg": "#07364A" },
                            { "text": "低能见度", "fg": root.yellow, "bg": "#4A3B05" },
                            { "text": "高机动性", "fg": root.purple, "bg": "#2C2354" }
                        ]

                        Rectangle {
                            Layout.preferredWidth: chipText.implicitWidth + 24
                            Layout.preferredHeight: 30
                            radius: 4
                            color: modelData.bg
                            border.color: modelData.fg
                            border.width: 1

                            Text {
                                id: chipText
                                anchors.centerIn: parent
                                text: modelData.text
                                color: modelData.fg
                                font.pixelSize: 13
                                font.bold: true
                            }
                        }
                    }

                    Item { Layout.fillWidth: true }
                }
            }

            Rectangle {
                Layout.fillWidth: true
                implicitHeight: algorithmColumn.implicitHeight + 32
                radius: 6
                color: root.cardBg
                border.color: root.cardBorder
                border.width: 1

                ColumnLayout {
                    id: algorithmColumn
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.top: parent.top
                    anchors.margins: 16
                    spacing: 16

                    Text {
                        text: "预置算法库状态"
                        color: root.textPrimary
                        font.pixelSize: 20
                        font.bold: true
                    }

                    GridLayout {
                        Layout.fillWidth: true
                        columns: root.width < 1160 ? 3 : 5
                        columnSpacing: 14
                        rowSpacing: 14

                        Repeater {
                            model: root.algorithms

                            Rectangle {
                                Layout.fillWidth: true
                                Layout.preferredHeight: 164
                                radius: 6
                                color: modelData.selected ? "#082744" : root.panelBg
                                border.color: modelData.selected ? root.cyan : root.cardBorder
                                border.width: modelData.selected ? 2 : 1

                                ColumnLayout {
                                    anchors.fill: parent
                                    anchors.margins: 14
                                    spacing: 8

                                    Rectangle {
                                        Layout.preferredWidth: 48
                                        Layout.preferredHeight: 48
                                        radius: 8
                                        color: modelData.color

                                        Text {
                                            anchors.centerIn: parent
                                            text: "◌"
                                            color: "#FFFFFF"
                                            font.pixelSize: 24
                                            font.bold: true
                                        }
                                    }

                                    Text {
                                        Layout.fillWidth: true
                                        text: modelData.name
                                        color: root.textPrimary
                                        font.pixelSize: 16
                                        font.bold: true
                                        elide: Text.ElideRight
                                    }

                                    Text {
                                        Layout.fillWidth: true
                                        text: "适用场景:"
                                        color: root.textMuted
                                        font.pixelSize: 12
                                    }

                                    Text {
                                        Layout.fillWidth: true
                                        text: modelData.scenario
                                        color: root.textSecondary
                                        font.pixelSize: 12
                                        elide: Text.ElideRight
                                    }

                                    Text {
                                        Layout.fillWidth: true
                                        visible: modelData.selected
                                        text: "✦ 已选中"
                                        color: root.cyan
                                        font.pixelSize: 12
                                        font.bold: true
                                    }
                                }
                            }
                        }
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        implicitHeight: matchRow.implicitHeight + 20
                        radius: 4
                        color: "#07364A"
                        border.color: "#1E6BA8"
                        border.width: 1

                        RowLayout {
                            id: matchRow
                            anchors.left: parent.left
                            anchors.right: parent.right
                            anchors.top: parent.top
                            anchors.margins: 10
                            spacing: 10

                            Text {
                                Layout.fillWidth: true
                                text: "系统已根据当前测试条件自动匹配最佳算法: 【回归建模】+【时间序列分析】"
                                color: root.cyan
                                font.pixelSize: 14
                                font.bold: true
                                elide: Text.ElideRight
                            }

                            Text {
                                text: "查看算法特征描述与输出指标界定"
                                color: root.cyan
                                font.pixelSize: 13
                                font.underline: true
                            }
                        }
                    }
                }
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 344
                radius: 6
                color: root.cardBg
                border.color: root.cardBorder
                border.width: 1

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 18
                    spacing: 14

                    Text {
                        Layout.fillWidth: true
                        text: "试验结果显著影响因素贡献度排序"
                        color: root.textPrimary
                        font.pixelSize: 18
                        font.bold: true
                    }

                    Row {
                        Layout.fillWidth: true
                        height: 34
                        PrototypeTableCell { width: parent.width * 0.18; height: 34; value: "因素名称"; textColor: root.textSecondary; rowColor: root.panelBg }
                        PrototypeTableCell { width: parent.width * 0.32; height: 34; value: "贡献度分数"; textColor: root.textSecondary; rowColor: root.panelBg }
                        PrototypeTableCell { width: parent.width * 0.18; height: 34; value: "敏感性方向"; textColor: root.textSecondary; rowColor: root.panelBg }
                        PrototypeTableCell { width: parent.width * 0.32; height: 34; value: "影响趋势拟合曲线"; textColor: root.textSecondary; rowColor: root.panelBg }
                    }

                    Repeater {
                        model: root.factors

                        Row {
                            Layout.fillWidth: true
                            height: 39
                            property color rowColor: index % 2 === 0 ? root.cardBg : "#0A1726"

                            PrototypeTableCell { width: parent.width * 0.18; height: 39; value: modelData.factor; textColor: root.textSecondary; rowColor: parent.rowColor }

                            Rectangle {
                                width: parent.width * 0.32
                                height: 39
                                color: parent.rowColor
                                border.color: "#102B44"
                                border.width: 1

                                RowLayout {
                                    anchors.fill: parent
                                    anchors.leftMargin: 12
                                    anchors.rightMargin: 12
                                    spacing: 10

                                    Rectangle {
                                        Layout.fillWidth: true
                                        Layout.preferredHeight: 8
                                        radius: 4
                                        color: root.panelBg

                                        Rectangle {
                                            width: parent.width * Number(modelData.score || 0) / 100
                                            height: parent.height
                                            radius: 4
                                            color: "#0EA5E9"
                                        }
                                    }

                                    Text {
                                        text: Number(modelData.score).toFixed(1)
                                        color: root.cyan
                                        font.pixelSize: 13
                                        font.bold: true
                                    }
                                }
                            }

                            Rectangle {
                                width: parent.width * 0.18
                                height: 39
                                color: parent.rowColor
                                border.color: "#102B44"
                                border.width: 1

                                Rectangle {
                                    anchors.centerIn: parent
                                    width: directionText.implicitWidth + 20
                                    height: 24
                                    radius: 3
                                    color: modelData.direction === "正相关" ? "#073D31" : (modelData.direction === "负相关" ? "#4B1722" : "#263242")

                                    Text {
                                        id: directionText
                                        anchors.centerIn: parent
                                        text: modelData.direction
                                        color: modelData.color
                                        font.pixelSize: 12
                                        font.bold: true
                                    }
                                }
                            }

                            Rectangle {
                                width: parent.width * 0.32
                                height: 39
                                color: parent.rowColor
                                border.color: "#102B44"
                                border.width: 1

                                Canvas {
                                    anchors.left: parent.left
                                    anchors.verticalCenter: parent.verticalCenter
                                    anchors.leftMargin: 14
                                    width: 128
                                    height: 28
                                    property var values: modelData.trend || []
                                    onPaint: {
                                        var ctx = getContext("2d")
                                        ctx.reset()
                                        ctx.clearRect(0, 0, width, height)
                                        ctx.strokeStyle = root.cyan
                                        ctx.lineWidth = 2
                                        ctx.beginPath()
                                        for (var i = 0; i < values.length; ++i) {
                                            var x = i * width / Math.max(1, values.length - 1)
                                            var y = height - (Number(values[i]) / 100 * height)
                                            if (i === 0) ctx.moveTo(x, y)
                                            else ctx.lineTo(x, y)
                                        }
                                        ctx.stroke()
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
                    spacing: 10

                    Text {
                        Layout.fillWidth: true
                        text: "探测距离对置信度的影响趋势分析"
                        color: root.textPrimary
                        font.pixelSize: 18
                        font.bold: true
                    }

                    Canvas {
                        id: confidenceChart
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

                            var left = 54
                            var right = 24
                            var top = 18
                            var bottom = 42
                            var chartW = Math.max(1, w - left - right)
                            var chartH = Math.max(1, h - top - bottom)

                            ctx.strokeStyle = "rgba(255,255,255,0.08)"
                            ctx.lineWidth = 1
                            ctx.font = "12px sans-serif"
                            ctx.fillStyle = root.textMuted
                            for (var gy = 0; gy <= 5; ++gy) {
                                var yy = top + gy * chartH / 5
                                ctx.beginPath()
                                ctx.moveTo(left, yy)
                                ctx.lineTo(left + chartW, yy)
                                ctx.stroke()
                                ctx.fillText(String(100 - gy * 8), 18, yy + 4)
                            }

                            ctx.strokeStyle = root.cyan
                            ctx.lineWidth = 3
                            ctx.beginPath()
                            for (var i = 0; i < root.confidenceTrend.length; ++i) {
                                var item = root.confidenceTrend[i]
                                var x = left + i * chartW / Math.max(1, root.confidenceTrend.length - 1)
                                var y = top + (100 - Number(item.confidence || 0)) / 40 * chartH
                                if (i === 0) ctx.moveTo(x, y)
                                else ctx.lineTo(x, y)
                            }
                            ctx.stroke()

                            ctx.fillStyle = root.cyan
                            for (var j = 0; j < root.confidenceTrend.length; ++j) {
                                var pt = root.confidenceTrend[j]
                                var px = left + j * chartW / Math.max(1, root.confidenceTrend.length - 1)
                                var py = top + (100 - Number(pt.confidence || 0)) / 40 * chartH
                                ctx.beginPath()
                                ctx.arc(px, py, 4, 0, Math.PI * 2)
                                ctx.fill()
                                if (j % 2 === 0) {
                                    ctx.fillStyle = root.textMuted
                                    ctx.fillText(String(pt.distance), px - 14, top + chartH + 24)
                                    ctx.fillStyle = root.cyan
                                }
                            }
                        }
                    }

                    Text {
                        Layout.fillWidth: true
                        text: "拟合模型: 指数衰减回归 | R² = 0.9823 | p < 0.001"
                        color: root.textSecondary
                        font.pixelSize: 13
                        horizontalAlignment: Text.AlignHCenter
                    }
                }
            }
        }
    }
}
