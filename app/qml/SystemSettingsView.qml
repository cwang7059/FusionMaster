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
    readonly property color purple: "#9B7CFF"
    readonly property color blue: "#2F7DF6"
    readonly property color orange: "#F59E0B"

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
                    text: "系统管理"
                    color: root.textPrimary
                    font.pixelSize: 26
                    font.bold: true
                }

                Text {
                    Layout.fillWidth: true
                    text: "系统配置、用户管理与安全设置"
                    color: root.textSecondary
                    font.pixelSize: 16
                }
            }

            GridLayout {
                Layout.fillWidth: true
                columns: root.width < 1040 ? 1 : 2
                columnSpacing: 16
                rowSpacing: 16

                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 240
                    radius: 6
                    color: root.cardBg
                    border.color: root.cardBorder
                    border.width: 1

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 18
                        spacing: 14

                        SettingsSectionTitle { iconText: "▣"; iconColor: root.cyan; titleText: "数据库配置"; textColor: root.textPrimary }

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 7
                            Text { text: "数据库地址"; color: root.textSecondary; font.pixelSize: 13 }
                            Rectangle {
                                Layout.fillWidth: true
                                Layout.preferredHeight: 38
                                radius: 4
                                color: root.panelBg
                                border.color: root.cardBorder
                                border.width: 1
                                Text {
                                    anchors.left: parent.left
                                    anchors.verticalCenter: parent.verticalCenter
                                    anchors.leftMargin: 12
                                    text: "192.168.1.100:5432"
                                    color: root.textPrimary
                                    font.pixelSize: 14
                                }
                            }
                        }

                        SettingsStatusLine { label: "连接状态"; value: "正常连接"; labelColor: root.textSecondary; valueColor: root.green; strongColorA: root.green; strongColorB: root.cyan; pulse: true }

                        RowLayout {
                            Layout.fillWidth: true
                            Text { Layout.fillWidth: true; text: "数据表结构"; color: root.textSecondary; font.pixelSize: 13 }
                            Text { text: "查看数据字典设计文档"; color: root.cyan; font.pixelSize: 13; font.underline: true }
                        }
                    }
                }

                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 240
                    radius: 6
                    color: root.cardBg
                    border.color: root.cardBorder
                    border.width: 1

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 18
                        spacing: 16

                        SettingsSectionTitle { iconText: "◫"; iconColor: root.purple; titleText: "系统性能监控"; textColor: root.textPrimary }
                        SettingsStatusLine { label: "CPU 使用率"; value: "23%"; labelColor: root.textSecondary; valueColor: root.cyan; strongColorA: root.green; strongColorB: root.cyan }
                        SettingsStatusLine { label: "内存使用率"; value: "45%"; labelColor: root.textSecondary; valueColor: root.cyan; strongColorA: root.green; strongColorB: root.cyan }
                        SettingsStatusLine { label: "网络时延"; value: "12ms"; labelColor: root.textSecondary; valueColor: root.green; strongColorA: root.green; strongColorB: root.cyan }
                        SettingsStatusLine { label: "运行环境"; value: "兼容良好"; labelColor: root.textSecondary; valueColor: root.green; strongColorA: root.green; strongColorB: root.cyan }
                    }
                }

                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 220
                    radius: 6
                    color: root.cardBg
                    border.color: root.cardBorder
                    border.width: 1

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 18
                        spacing: 16

                        SettingsSectionTitle { iconText: "◇"; iconColor: root.green; titleText: "安全与权限"; textColor: root.textPrimary }
                        SettingsStatusLine { label: "数据加密"; value: "已启用"; labelColor: root.textSecondary; valueColor: root.green; strongColorA: root.green; strongColorB: root.cyan }
                        SettingsStatusLine { label: "访问日志"; value: "查看"; labelColor: root.textSecondary; valueColor: root.cyan; strongColorA: root.green; strongColorB: root.cyan }
                        SettingsStatusLine { label: "备份策略"; value: "每日 02:00"; labelColor: root.textSecondary; valueColor: root.textSecondary; strongColorA: root.green; strongColorB: root.cyan }
                        SettingsStatusLine { label: "上次备份"; value: "今日 02:00"; labelColor: root.textSecondary; valueColor: root.textSecondary; strongColorA: root.green; strongColorB: root.cyan }
                    }
                }

                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 220
                    radius: 6
                    color: root.cardBg
                    border.color: root.cardBorder
                    border.width: 1

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 18
                        spacing: 16

                        SettingsSectionTitle { iconText: "◉"; iconColor: root.blue; titleText: "用户管理"; textColor: root.textPrimary }
                        SettingsStatusLine { label: "当前在线用户"; value: "3"; labelColor: root.textSecondary; valueColor: root.cyan; strongColorA: root.green; strongColorB: root.cyan }
                        SettingsStatusLine { label: "注册用户总数"; value: "12"; labelColor: root.textSecondary; valueColor: root.textSecondary; strongColorA: root.green; strongColorB: root.cyan }
                        PrototypeActionButton {
                            Layout.fillWidth: true
                            text: "用户权限配置"
                            preferredWidth: 100
                            fill: "#2563EB"
                            stroke: Qt.rgba(1, 1, 1, 0.12)
                            textFill: "#FFFFFF"
                            strongText: true
                        }
                    }
                }

                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 230
                    radius: 6
                    color: root.cardBg
                    border.color: root.cardBorder
                    border.width: 1

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 18
                        spacing: 12

                        SettingsSectionTitle { iconText: "△"; iconColor: root.yellow; titleText: "通知与告警"; textColor: root.textPrimary }

                        Repeater {
                            model: [
                                { "text": "数据异常告警", "checked": true },
                                { "text": "系统性能告警", "checked": true },
                                { "text": "备份完成通知", "checked": false },
                                { "text": "分析任务完成通知", "checked": true }
                            ]

                            CheckBox {
                                id: notificationCheck
                                Layout.fillWidth: true
                                text: modelData.text
                                checked: modelData.checked
                                contentItem: Text {
                                    text: notificationCheck.text
                                    color: root.textSecondary
                                    font.pixelSize: 13
                                    leftPadding: notificationCheck.indicator.width + notificationCheck.spacing
                                    verticalAlignment: Text.AlignVCenter
                                }
                            }
                        }
                    }
                }

                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 230
                    radius: 6
                    color: root.cardBg
                    border.color: root.cardBorder
                    border.width: 1

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 18
                        spacing: 12

                        SettingsSectionTitle { iconText: "▤"; iconColor: root.orange; titleText: "导出与报告"; textColor: root.textPrimary }

                        Repeater {
                            model: ["导出系统配置", "生成运维报告", "查看历史日志"]

                            PrototypeActionButton {
                                Layout.fillWidth: true
                                text: modelData
                                preferredWidth: 100
                                fill: root.panelBg
                                stroke: root.cardBorder
                                textFill: root.textSecondary
                            }
                        }
                    }
                }
            }
        }
    }
}
