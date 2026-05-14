import QtQuick 2.14
import QtQuick.Layouts 1.14

Item {
    id: root

    property string pageTitle: ""
    property string pageSubtitle: ""
    property var rows: [
        { "name": "任务队列", "value": "12", "state": "运行中" },
        { "name": "数据通道", "value": "6", "state": "正常" },
        { "name": "处理策略", "value": "4", "state": "可用" }
    ]

    readonly property color pageBg: "#071321"
    readonly property color cardBg: "#0B192A"
    readonly property color cardBorder: "#16314C"
    readonly property color textPrimary: "#F8FBFF"
    readonly property color textSecondary: "#A7B7C8"
    readonly property color cyan: "#00D7FF"
    readonly property color green: "#00D38A"

    Rectangle {
        anchors.fill: parent
        color: root.pageBg
    }

    ColumnLayout {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.margins: 24
        spacing: 24

        ColumnLayout {
            Layout.fillWidth: true
            spacing: 8

            Text {
                Layout.fillWidth: true
                text: root.pageTitle
                color: root.textPrimary
                font.pixelSize: 26
                font.bold: true
            }

            Text {
                Layout.fillWidth: true
                text: root.pageSubtitle
                color: root.textSecondary
                font.pixelSize: 16
            }
        }

        GridLayout {
            Layout.fillWidth: true
            columns: root.width < 980 ? 1 : 3
            columnSpacing: 16
            rowSpacing: 16

            Repeater {
                model: root.rows

                delegate: Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 126
                    radius: 6
                    color: root.cardBg
                    border.color: root.cardBorder
                    border.width: 1

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 20
                        spacing: 12

                        Text {
                            Layout.fillWidth: true
                            text: modelData.name
                            color: root.textSecondary
                            font.pixelSize: 14
                        }

                        RowLayout {
                            Layout.fillWidth: true

                            Text {
                                Layout.fillWidth: true
                                text: modelData.value
                                color: root.textPrimary
                                font.pixelSize: 32
                                font.bold: true
                            }

                            Text {
                                text: modelData.state
                                color: root.green
                                font.pixelSize: 14
                                font.bold: true
                            }
                        }
                    }
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 322
            radius: 6
            color: root.cardBg
            border.color: root.cardBorder
            border.width: 1

            Canvas {
                anchors.fill: parent
                anchors.margins: 24

                onPaint: {
                    var ctx = getContext("2d")
                    var w = width
                    var h = height

                    ctx.reset()
                    ctx.fillStyle = root.cardBg
                    ctx.fillRect(0, 0, w, h)

                    ctx.strokeStyle = "rgba(255,255,255,0.06)"
                    ctx.lineWidth = 1
                    for (var i = 0; i <= 5; ++i) {
                        var y = i * h / 5
                        ctx.beginPath()
                        ctx.moveTo(0, y)
                        ctx.lineTo(w, y)
                        ctx.stroke()
                    }
                    for (var j = 0; j <= 8; ++j) {
                        var x = j * w / 8
                        ctx.beginPath()
                        ctx.moveTo(x, 0)
                        ctx.lineTo(x, h)
                        ctx.stroke()
                    }

                    ctx.strokeStyle = root.cyan
                    ctx.lineWidth = 3
                    ctx.beginPath()
                    ctx.moveTo(w * 0.04, h * 0.72)
                    ctx.lineTo(w * 0.18, h * 0.52)
                    ctx.lineTo(w * 0.32, h * 0.61)
                    ctx.lineTo(w * 0.48, h * 0.34)
                    ctx.lineTo(w * 0.65, h * 0.43)
                    ctx.lineTo(w * 0.82, h * 0.24)
                    ctx.lineTo(w * 0.96, h * 0.36)
                    ctx.stroke()
                }
            }
        }
    }
}
