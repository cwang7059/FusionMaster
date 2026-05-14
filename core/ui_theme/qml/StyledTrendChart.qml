import QtQuick 2.14
import QtQuick.Controls 2.14
import QtQuick.Layouts 1.14
import UITheme 1.0

Rectangle {
    id: root
    color: Theme.inputBg
    border.color: Theme.cardBorder
    border.width: 1
    radius: Theme.radiusNormal

    property string titleText: ""
    property var xLabels: []
    property var seriesList: []
    property string suffixText: ""
    property real yMaxOverride: -1
    property int yTickCount: 4
    property int xTickCount: 4

    function maxSeriesValue() {
        if (yMaxOverride > 0) {
            return yMaxOverride
        }

        var maxValue = 1
        if (!seriesList) {
            return maxValue
        }

        for (var i = 0; i < seriesList.length; ++i) {
            var series = seriesList[i]
            if (!series || !series.values) {
                continue
            }

            for (var j = 0; j < series.values.length; ++j) {
                maxValue = Math.max(maxValue, Number(series.values[j] || 0))
            }
        }

        return maxValue
    }

    function valueToY(value, top, height, maxValue) {
        if (maxValue <= 0) {
            return top + height
        }
        var ratio = Number(value || 0) / maxValue
        return top + height - ratio * height
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: Theme.spacingSmall
        spacing: Theme.spacingSmall

        Text {
            text: root.titleText
            color: Theme.textPrimary
            font.pixelSize: Theme.fontSizeNormal
            font.weight: Font.DemiBold
            visible: text.length > 0
        }

        Canvas {
            id: chartCanvas
            Layout.fillWidth: true
            Layout.fillHeight: true
            antialiasing: true

            onPaint: {
                var ctx = getContext("2d")
                ctx.reset()

                var w = width
                var h = height
                var left = 44
                var right = 10
                var top = 8
                var bottom = 28
                var plotW = Math.max(1, w - left - right)
                var plotH = Math.max(1, h - top - bottom)
                var maxValue = root.maxSeriesValue()
                var yTicks = Math.max(1, root.yTickCount)

                ctx.fillStyle = Theme.inputBg
                ctx.fillRect(0, 0, w, h)

                ctx.strokeStyle = "rgba(255,255,255,0.08)"
                ctx.lineWidth = 1
                for (var gy = 0; gy <= yTicks; ++gy) {
                    var gyPos = top + gy * (plotH / yTicks)
                    ctx.beginPath()
                    ctx.moveTo(left, gyPos)
                    ctx.lineTo(left + plotW, gyPos)
                    ctx.stroke()
                }

                var xGridTicks = Math.max(1, root.xTickCount)
                for (var gx = 0; gx <= xGridTicks; ++gx) {
                    var gxPos = left + gx * (plotW / xGridTicks)
                    ctx.beginPath()
                    ctx.moveTo(gxPos, top)
                    ctx.lineTo(gxPos, top + plotH)
                    ctx.stroke()
                }

                ctx.strokeStyle = "rgba(255,255,255,0.2)"
                ctx.beginPath()
                ctx.moveTo(left, top)
                ctx.lineTo(left, top + plotH)
                ctx.lineTo(left + plotW, top + plotH)
                ctx.stroke()

                if (root.seriesList) {
                    for (var si = 0; si < root.seriesList.length; ++si) {
                        var entry = root.seriesList[si]
                        if (!entry || !entry.values || entry.values.length <= 0) {
                            continue
                        }

                        var values = entry.values
                        var count = values.length
                        var step = count > 1 ? (plotW / (count - 1)) : 0
                        var lineColor = String(entry.color || Theme.accent)

                        ctx.strokeStyle = lineColor
                        ctx.lineWidth = 2
                        ctx.beginPath()
                        for (var vi = 0; vi < count; ++vi) {
                            var x = left + vi * step
                            var y = root.valueToY(values[vi], top, plotH, maxValue)
                            if (vi === 0) {
                                ctx.moveTo(x, y)
                            } else {
                                ctx.lineTo(x, y)
                            }
                        }
                        ctx.stroke()
                    }
                }

                ctx.fillStyle = Theme.textSecondary
                ctx.font = "11px Consolas"
                ctx.textAlign = "right"
                for (var ly = 0; ly <= yTicks; ++ly) {
                    var yValue = Math.round(maxValue * (yTicks - ly) / yTicks)
                    var yTextPos = top + ly * (plotH / yTicks) + 4
                    ctx.fillText(String(yValue), left - 6, yTextPos)
                }

                ctx.textAlign = "center"
                var labelCount = Math.max(1, root.xTickCount)
                labelCount = Math.min(labelCount, root.xLabels ? root.xLabels.length : 0)
                if (labelCount > 0) {
                    for (var lx = 0; lx < labelCount; ++lx) {
                        var index = Math.round((root.xLabels.length - 1) * lx / Math.max(1, labelCount - 1))
                        var tx = left + plotW * lx / Math.max(1, labelCount - 1)
                        var label = String(root.xLabels[index] || "")
                        ctx.fillText(label, tx, top + plotH + 17)
                    }
                }
            }

            onWidthChanged: requestPaint()
            onHeightChanged: requestPaint()
        }

        RowLayout {
            Layout.alignment: Qt.AlignRight
            spacing: Theme.spacingSmall

            Repeater {
                model: root.seriesList ? root.seriesList : []

                RowLayout {
                    spacing: 5

                    Rectangle {
                        width: 12
                        height: 2
                        color: modelData && modelData.color ? modelData.color : Theme.accent
                    }

                    Text {
                        text: modelData && modelData.name ? modelData.name : ""
                        color: Theme.textSecondary
                        font.pixelSize: Theme.fontSizeSmall
                    }
                }
            }

            Text {
                text: root.suffixText
                color: Theme.textSecondary
                font.pixelSize: Theme.fontSizeSmall
                visible: text.length > 0
            }
        }
    }

    onSeriesListChanged: chartCanvas.requestPaint()
    onXLabelsChanged: chartCanvas.requestPaint()
    onYMaxOverrideChanged: chartCanvas.requestPaint()
    onYTickCountChanged: chartCanvas.requestPaint()
    onXTickCountChanged: chartCanvas.requestPaint()
}
