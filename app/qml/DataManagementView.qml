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
    readonly property color red: "#FF6B7A"

    readonly property var sampleRows: [
        { "batch": "B2026-0512-001", "time": "2026-05-12 14:30:25", "target": "单体目标-001", "distance": "1250.5", "azimuth": "45.2", "elevation": "12.8", "confidence": "96.5", "status": "命中", "condition": "晴天/能见度良好" },
        { "batch": "B2026-0513-002", "time": "2026-05-13 09:15:43", "target": "无人机集群-5台", "distance": "2340.2", "azimuth": "120.5", "elevation": "8.3", "confidence": "92.1", "status": "命中", "condition": "多云/中等能见度" },
        { "batch": "B2026-0513-003", "time": "2026-05-13 11:22:17", "target": "单体目标-002", "distance": "3120.8", "azimuth": "315.7", "elevation": "15.2", "confidence": "78.3", "status": "告警", "condition": "雾天/低能见度" },
        { "batch": "B2026-0514-004", "time": "2026-05-14 10:05:33", "target": "无人机集群-10台", "distance": "1890.3", "azimuth": "225.4", "elevation": "10.1", "confidence": "88.7", "status": "命中", "condition": "晴天/高能见度" },
        { "batch": "B2026-0514-005", "time": "2026-05-14 13:45:12", "target": "单体目标-003", "distance": "4250.1", "azimuth": "89.9", "elevation": "6.5", "confidence": "65.2", "status": "漏警", "condition": "雨天/能见度差" }
    ]

    function statusColor(status) {
        if (status === "命中") {
            return root.green
        }
        if (status === "告警") {
            return root.yellow
        }
        if (status === "漏警") {
            return root.red
        }
        return root.textSecondary
    }

    function statusBg(status) {
        if (status === "命中") {
            return "#073D31"
        }
        if (status === "告警") {
            return "#4A3B05"
        }
        if (status === "漏警") {
            return "#4B1722"
        }
        return "#1F2937"
    }

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
                    text: "数据调取与管理"
                    color: root.textPrimary
                    font.pixelSize: 26
                    font.bold: true
                }

                Text {
                    Layout.fillWidth: true
                    text: "数据导入、清洗、存储、检索和导出"
                    color: root.textSecondary
                    font.pixelSize: 16
                }
            }

            Rectangle {
                Layout.fillWidth: true
                implicitHeight: filterColumn.implicitHeight + 32
                radius: 6
                color: root.cardBg
                border.color: root.cardBorder
                border.width: 1

                ColumnLayout {
                    id: filterColumn
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.top: parent.top
                    anchors.margins: 16
                    spacing: 16

                    GridLayout {
                        Layout.fillWidth: true
                        columns: root.width < 980 ? 2 : 4
                        columnSpacing: 16
                        rowSpacing: 14

                        ColumnLayout {
                            Layout.columnSpan: root.width < 980 ? 2 : 2
                            Layout.fillWidth: true
                            spacing: 8

                            Text {
                                text: "模糊搜索"
                                color: root.textSecondary
                                font.pixelSize: 13
                            }

                            Rectangle {
                                Layout.fillWidth: true
                                Layout.preferredHeight: 38
                                radius: 4
                                color: root.panelBg
                                border.color: searchInput.activeFocus ? root.cyan : root.cardBorder
                                border.width: 1

                                RowLayout {
                                    anchors.fill: parent
                                    anchors.leftMargin: 12
                                    anchors.rightMargin: 12
                                    spacing: 9

                                    Text {
                                        text: "⌕"
                                        color: root.textMuted
                                        font.pixelSize: 18
                                    }

                                    TextInput {
                                        id: searchInput
                                        Layout.fillWidth: true
                                        color: root.textPrimary
                                        selectionColor: "#0EA5E9"
                                        selectedTextColor: "#FFFFFF"
                                        font.pixelSize: 14
                                        clip: true

                                        Text {
                                            anchors.verticalCenter: parent.verticalCenter
                                            text: "输入任务标识/批次号/目标编号查询..."
                                            color: root.textMuted
                                            font.pixelSize: 14
                                            visible: searchInput.text.length === 0
                                        }
                                    }
                                }
                            }
                        }

                        PrototypeDateInputBlock {
                            title: "开始时间"
                            valueText: "2026-05-12 00:00"
                            labelColor: root.textSecondary
                            valueColor: root.textSecondary
                            panelColor: root.panelBg
                            borderColor: root.cardBorder
                        }

                        PrototypeDateInputBlock {
                            title: "结束时间"
                            valueText: "2026-05-14 23:59"
                            labelColor: root.textSecondary
                            valueColor: root.textSecondary
                            panelColor: root.panelBg
                            borderColor: root.cardBorder
                        }
                    }

                    GridLayout {
                        Layout.fillWidth: true
                        columns: root.width < 1040 ? 1 : 3
                        columnSpacing: 16
                        rowSpacing: 14

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 9

                            Text {
                                text: "探测通道筛选"
                                color: root.textSecondary
                                font.pixelSize: 13
                            }

                            Flow {
                                Layout.fillWidth: true
                                spacing: 12

                                Repeater {
                                    model: ["可见光通道", "红外通道", "激光探测通道", "多传感器融合"]

                                    CheckBox {
                                        id: channelCheck
                                        text: modelData
                                        checked: index < 2
                                        spacing: 7
                                        contentItem: Text {
                                            text: channelCheck.text
                                            color: root.textSecondary
                                            font.pixelSize: 13
                                            leftPadding: channelCheck.indicator.width + channelCheck.spacing
                                            verticalAlignment: Text.AlignVCenter
                                        }
                                    }
                                }
                            }
                        }

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 8

                            Text {
                                text: "目标类型筛选"
                                color: root.textSecondary
                                font.pixelSize: 13
                            }

                            ComboBox {
                                Layout.fillWidth: true
                                Layout.preferredHeight: 38
                                model: ["全部", "单体目标", "无人机集群 (1-10台)"]
                            }
                        }

                        Item {
                            Layout.fillWidth: true
                        }
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 10

                        PrototypeActionButton {
                            text: "查询"
                            fill: root.cyan
                            stroke: Qt.rgba(1, 1, 1, 0.12)
                            textFill: "#042236"
                            strongText: true
                        }

                        PrototypeActionButton {
                            text: "重置"
                            fill: root.panelBg
                            stroke: root.cardBorder
                            textFill: root.textSecondary
                        }

                        PrototypeActionButton {
                            text: "▾ 高级筛选"
                            preferredWidth: 122
                            fill: root.panelBg
                            stroke: root.cardBorder
                            textFill: root.textSecondary
                        }

                        Item { Layout.fillWidth: true }
                    }
                }
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 12

                PrototypeActionButton {
                    text: "⇧ 导入历史数据"
                    preferredWidth: 150
                    fill: "#2563EB"
                    stroke: Qt.rgba(1, 1, 1, 0.12)
                    textFill: "#FFFFFF"
                    strongText: true
                }

                PrototypeActionButton {
                    text: "▣ 数据标准化转换"
                    preferredWidth: 176
                    fill: "#7C3AED"
                    stroke: Qt.rgba(1, 1, 1, 0.12)
                    textFill: "#FFFFFF"
                    strongText: true
                }

                PrototypeActionButton {
                    text: "⇩ 批量导出"
                    preferredWidth: 126
                    fill: "#16A34A"
                    stroke: Qt.rgba(1, 1, 1, 0.12)
                    textFill: "#FFFFFF"
                    strongText: true
                }

                Item { Layout.fillWidth: true }
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 326
                radius: 6
                color: root.cardBg
                border.color: root.cardBorder
                border.width: 1
                clip: true

                Flickable {
                    anchors.fill: parent
                    contentWidth: 1174
                    contentHeight: tableColumn.implicitHeight
                    boundsBehavior: Flickable.StopAtBounds
                    clip: true

                    Column {
                        id: tableColumn
                        width: 1174

                        Row {
                            width: parent.width
                            height: 44

                            Repeater {
                                model: [
                                    { "label": "批次标识", "w": 146 },
                                    { "label": "时间戳", "w": 168 },
                                    { "label": "目标类型及编号", "w": 144 },
                                    { "label": "探测距离(m)", "w": 110 },
                                    { "label": "方位角(°)", "w": 96 },
                                    { "label": "俯仰角(°)", "w": 96 },
                                    { "label": "置信度评分", "w": 104 },
                                    { "label": "探测结果", "w": 92 },
                                    { "label": "试验条件", "w": 144 },
                                    { "label": "操作", "w": 74 }
                                ]

                                Rectangle {
                                    width: modelData.w
                                    height: 44
                                    color: root.panelBg
                                    border.color: "#102B44"
                                    border.width: 1

                                    Text {
                                        anchors.left: parent.left
                                        anchors.right: parent.right
                                        anchors.verticalCenter: parent.verticalCenter
                                        anchors.leftMargin: 12
                                        anchors.rightMargin: 8
                                        text: modelData.label
                                        color: root.textSecondary
                                        font.pixelSize: 12
                                        font.bold: true
                                        elide: Text.ElideRight
                                    }
                                }
                            }
                        }

                        Repeater {
                            model: root.sampleRows

                            Row {
                                width: parent.width
                                height: 48

                                property color rowColor: index % 2 === 0 ? root.cardBg : "#0A1726"

                                PrototypeTableCell { width: 146; value: modelData.batch; textColor: root.cyan; rowColor: parent.rowColor; mono: true }
                                PrototypeTableCell { width: 168; value: modelData.time; textColor: root.textSecondary; rowColor: parent.rowColor }
                                PrototypeTableCell { width: 144; value: modelData.target; textColor: root.textSecondary; rowColor: parent.rowColor }
                                PrototypeTableCell { width: 110; value: modelData.distance; textColor: root.textSecondary; rowColor: parent.rowColor }
                                PrototypeTableCell { width: 96; value: modelData.azimuth; textColor: root.textSecondary; rowColor: parent.rowColor }
                                PrototypeTableCell { width: 96; value: modelData.elevation; textColor: root.textSecondary; rowColor: parent.rowColor }
                                PrototypeTableCell { width: 104; value: modelData.confidence; textColor: root.textSecondary; rowColor: parent.rowColor }

                                Rectangle {
                                    width: 92
                                    height: 48
                                    color: rowColor
                                    border.color: "#102B44"
                                    border.width: 1

                                    Rectangle {
                                        anchors.centerIn: parent
                                        width: 48
                                        height: 24
                                        radius: 3
                                        color: root.statusBg(modelData.status)
                                        border.color: root.statusColor(modelData.status)
                                        border.width: 1

                                        Text {
                                            anchors.centerIn: parent
                                            text: modelData.status
                                            color: root.statusColor(modelData.status)
                                            font.pixelSize: 12
                                            font.bold: true
                                        }
                                    }
                                }

                                PrototypeTableCell { width: 144; value: modelData.condition; textColor: root.textSecondary; rowColor: parent.rowColor }

                                Rectangle {
                                    width: 74
                                    height: 48
                                    color: rowColor
                                    border.color: "#102B44"
                                    border.width: 1

                                    Text {
                                        anchors.centerIn: parent
                                        text: "查看"
                                        color: root.cyan
                                        font.pixelSize: 13
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
                implicitHeight: infoText.implicitHeight + 28
                radius: 6
                color: "#082744"
                border.color: "#1E6BA8"
                border.width: 1

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 16
                    anchors.rightMargin: 16
                    spacing: 12

                    Text {
                        text: "ⓘ"
                        color: "#55B8FF"
                        font.pixelSize: 22
                    }

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 4

                        Text {
                            text: "数据标准化提示"
                            color: "#55B8FF"
                            font.pixelSize: 14
                            font.bold: true
                        }

                        Text {
                            id: infoText
                            Layout.fillWidth: true
                            text: "导入的数据将自动转换为项目数据字典标准格式，包括字段映射、单位统一、编码口径对齐等步骤"
                            color: root.textSecondary
                            font.pixelSize: 13
                            wrapMode: Text.Wrap
                        }
                    }
                }
            }
        }
    }

}
