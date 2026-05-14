import QtQuick 2.14
import QtQuick.Layouts 1.14

Rectangle {
    id: root

    property string title: ""
    property int preferredHeight: 360
    property string canvasMode: "line"
    property var seriesRows: []
    property var channelRows: []
    property var scatterRows: []
    property var radarRows: []
    property var barColors: ["#00D7FF", "#2F7DF6", "#9B7CFF", "#EC4899", "#F59E0B"]
    property color cardBg: "#0B192A"
    property color cardBorder: "#16314C"
    property color textPrimary: "#F8FBFF"
    property color textSecondary: "#A7B7C8"
    property color textMuted: "#6F849A"
    property color cyan: "#00D7FF"
    property color purple: "#9B7CFF"

    Layout.fillWidth: true
    Layout.preferredHeight: preferredHeight
    radius: 6
    color: cardBg
    border.color: cardBorder
    border.width: 1

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 18
        spacing: 12

        Text {
            Layout.fillWidth: true
            text: root.title
            color: root.textPrimary
            font.pixelSize: 18
            font.bold: true
            elide: Text.ElideRight
        }

        Canvas {
            Layout.fillWidth: true
            Layout.fillHeight: true
            onWidthChanged: requestPaint()
            onHeightChanged: requestPaint()
            onPaint: {
                var ctx = getContext("2d")
                ctx.reset()
                ctx.clearRect(0, 0, width, height)
                if (root.canvasMode === "scatter") {
                    root.drawScatter(ctx, width, height)
                } else if (root.canvasMode === "bar") {
                    root.drawBar(ctx, width, height)
                } else if (root.canvasMode === "radar") {
                    root.drawRadar(ctx, width, height)
                } else {
                    root.drawLine(ctx, width, height)
                }
            }
        }
    }

    function drawGrid(ctx, left, top, chartW, chartH, xSteps, ySteps) {
        ctx.strokeStyle = "rgba(255,255,255,0.08)"
        ctx.lineWidth = 1
        for (var i = 0; i <= ySteps; ++i) {
            var y = top + i * chartH / ySteps
            ctx.beginPath()
            ctx.moveTo(left, y)
            ctx.lineTo(left + chartW, y)
            ctx.stroke()
        }
        for (var j = 0; j <= xSteps; ++j) {
            var x = left + j * chartW / xSteps
            ctx.beginPath()
            ctx.moveTo(x, top)
            ctx.lineTo(x, top + chartH)
            ctx.stroke()
        }
    }

    function drawScatter(ctx, w, h) {
        var left = 54
        var right = 22
        var top = 14
        var bottom = 38
        var chartW = Math.max(1, w - left - right)
        var chartH = Math.max(1, h - top - bottom)
        drawGrid(ctx, left, top, chartW, chartH, 8, 5)

        ctx.font = "12px Segoe UI"
        ctx.fillStyle = root.textMuted
        ctx.fillText("方位角", left + chartW - 42, top + chartH + 28)
        ctx.fillText("俯仰角", 8, top + 12)

        for (var i = 0; i < root.scatterRows.length; ++i) {
            var p = root.scatterRows[i]
            var x = left + Number(p.azimuth || 0) / 360 * chartW
            var y = top + chartH - Number(p.elevation || 0) / 90 * chartH
            var r = 3 + Math.max(0, Number(p.confidence || 0) - 60) / 40 * 4
            ctx.fillStyle = "rgba(0,215,255,0.62)"
            ctx.beginPath()
            ctx.arc(x, y, r, 0, Math.PI * 2)
            ctx.fill()
        }
    }

    function drawLine(ctx, w, h) {
        var left = 54
        var right = 24
        var top = 14
        var bottom = 38
        var chartW = Math.max(1, w - left - right)
        var chartH = Math.max(1, h - top - bottom)
        drawGrid(ctx, left, top, chartW, chartH, 7, 5)

        drawSeriesLine(ctx, root.seriesRows, "confidence", root.cyan, left, top, chartW, chartH)
        drawSeriesLine(ctx, root.seriesRows, "detection", root.purple, left, top, chartW, chartH)

        ctx.font = "12px Segoe UI"
        ctx.fillStyle = root.textMuted
        for (var i = 0; i < root.seriesRows.length; i += 2) {
            var x = left + i * chartW / Math.max(1, root.seriesRows.length - 1)
            ctx.fillText(root.seriesRows[i].time, x - 12, top + chartH + 24)
        }
        drawLegend(ctx, left + chartW - 210, 10, [["置信度评分", root.cyan], ["探测成功率", root.purple]])
    }

    function drawSeriesLine(ctx, rows, field, color, left, top, chartW, chartH) {
        ctx.strokeStyle = color
        ctx.lineWidth = 3
        ctx.beginPath()
        for (var i = 0; i < rows.length; ++i) {
            var value = Number(rows[i][field] || 0)
            var x = left + i * chartW / Math.max(1, rows.length - 1)
            var y = top + (100 - value) / 15 * chartH
            if (i === 0) {
                ctx.moveTo(x, y)
            } else {
                ctx.lineTo(x, y)
            }
        }
        ctx.stroke()
    }

    function drawBar(ctx, w, h) {
        var left = 54
        var right = 24
        var top = 14
        var bottom = 40
        var chartW = Math.max(1, w - left - right)
        var chartH = Math.max(1, h - top - bottom)
        drawGrid(ctx, left, top, chartW, chartH, 4, 4)

        var groupW = chartW / root.channelRows.length
        var barW = Math.min(18, groupW / 7)
        for (var i = 0; i < root.channelRows.length; ++i) {
            var row = root.channelRows[i]
            var values = row.values || []
            var startX = left + i * groupW + groupW * 0.17
            for (var j = 0; j < values.length; ++j) {
                var value = Number(values[j] || 0)
                var bh = (value - 80) / 20 * chartH
                ctx.fillStyle = root.barColors[j]
                ctx.fillRect(startX + j * (barW + 5), top + chartH - bh, barW, bh)
            }
            ctx.fillStyle = root.textMuted
            ctx.font = "12px Segoe UI"
            ctx.fillText(row.channel, left + i * groupW + groupW * 0.26, top + chartH + 24)
        }
        drawLegend(ctx, left + chartW - 330, 10, [["批次1", root.barColors[0]], ["批次2", root.barColors[1]], ["批次3", root.barColors[2]], ["批次4", root.barColors[3]], ["批次5", root.barColors[4]]])
    }

    function drawRadar(ctx, w, h) {
        var cx = w / 2
        var cy = h / 2 + 8
        var radius = Math.min(w, h) * 0.36
        var count = root.radarRows.length

        ctx.strokeStyle = "rgba(255,255,255,0.12)"
        ctx.lineWidth = 1
        for (var ring = 1; ring <= 5; ++ring) {
            ctx.beginPath()
            for (var i = 0; i < count; ++i) {
                var angle = -Math.PI / 2 + i * Math.PI * 2 / count
                var x = cx + Math.cos(angle) * radius * ring / 5
                var y = cy + Math.sin(angle) * radius * ring / 5
                if (i === 0) {
                    ctx.moveTo(x, y)
                } else {
                    ctx.lineTo(x, y)
                }
            }
            ctx.closePath()
            ctx.stroke()
        }

        ctx.font = "12px Segoe UI"
        ctx.fillStyle = root.textMuted
        for (var axis = 0; axis < count; ++axis) {
            var axisAngle = -Math.PI / 2 + axis * Math.PI * 2 / count
            var ax = cx + Math.cos(axisAngle) * radius
            var ay = cy + Math.sin(axisAngle) * radius
            ctx.beginPath()
            ctx.moveTo(cx, cy)
            ctx.lineTo(ax, ay)
            ctx.stroke()
            ctx.fillText(root.radarRows[axis].subject, cx + Math.cos(axisAngle) * (radius + 16) - 28, cy + Math.sin(axisAngle) * (radius + 16) + 4)
        }

        ctx.beginPath()
        for (var j = 0; j < count; ++j) {
            var row = root.radarRows[j]
            var a = -Math.PI / 2 + j * Math.PI * 2 / count
            var r = radius * Number(row.value || 0) / 100
            var px = cx + Math.cos(a) * r
            var py = cy + Math.sin(a) * r
            if (j === 0) {
                ctx.moveTo(px, py)
            } else {
                ctx.lineTo(px, py)
            }
        }
        ctx.closePath()
        ctx.fillStyle = "rgba(0,215,255,0.34)"
        ctx.fill()
        ctx.strokeStyle = root.cyan
        ctx.lineWidth = 2
        ctx.stroke()
    }

    function drawLegend(ctx, x, y, items) {
        ctx.font = "12px Segoe UI"
        var offset = 0
        for (var i = 0; i < items.length; ++i) {
            ctx.fillStyle = items[i][1]
            ctx.fillRect(x + offset, y, 10, 10)
            ctx.fillStyle = root.textSecondary
            ctx.fillText(items[i][0], x + offset + 15, y + 10)
            offset += ctx.measureText(items[i][0]).width + 38
        }
    }
}
