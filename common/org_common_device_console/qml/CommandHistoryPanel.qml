import QtQuick 2.14
import QtQuick.Controls 2.14
import QtQuick.Layouts 1.14
import UITheme 1.0
import DeviceConsole 1.0

Rectangle {
    id: root
    implicitHeight: 200
    color: Theme.inputBg
    border.color: Theme.inputBorder
    border.width: 1
    clip: true

    property string deviceId: ""

    ListModel {
        id: historyModel
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: Theme.spacingSmall
        spacing: Theme.spacingSmall

        RowLayout {
            Layout.fillWidth: true
            Text {
                text: "指令下发记录"
                color: Theme.textSecondary
                font.bold: true
            }
            Item { Layout.fillWidth: true }
            StyledButton {
                text: "清空"
                onClicked: historyModel.clear()
            }
        }

        ListView {
            id: listView
            Layout.fillWidth: true
            Layout.fillHeight: true
            model: historyModel
            spacing: 4
            delegate: Rectangle {
                width: listView.width
                height: contentCol.implicitHeight + 12
                color: model.ok ? Theme.sectionBg : Qt.rgba(1, 0, 0, 0.15)
                border.color: model.ok ? Theme.inputBorder : Theme.danger
                border.width: 1
                radius: 4

                ColumnLayout {
                    id: contentCol
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.top: parent.top
                    anchors.margins: 6
                    spacing: 4

                    // 第一行：Hex 帧 (主视觉，始终优先)
                    Text {
                        text: model.hex || (model.ok ? "(等待同步...)" : "(协议生成失败/拦截)")
                        color: model.ok ? Theme.accent : "#FFFFFF"
                        font.family: "Consolas, Monaco, monospace"
                        font.pixelSize: 16
                        font.bold: true
                        wrapMode: Text.Wrap
                        Layout.fillWidth: true
                    }
                    
                    // 第二行：辅助信息 [时间] 指令说明 (错误详情)
                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 8
                        
                        Text {
                            text: "[" + model.timestamp + "]"
                            color: Theme.textSecondary
                            font.pixelSize: 11
                        }
                        
                        Text {
                            text: "指令: " + model.action + (model.ok ? "" : " <font color='#FF9999'>(" + model.message + ")</font>")
                            textFormat: Text.StyledText
                            color: Theme.textSecondary
                            font.pixelSize: 11
                            Layout.fillWidth: true
                            elide: Text.ElideRight
                        }

                        Rectangle {
                            width: 36
                            height: 18
                            radius: 4
                            color: model.ok ? "#1E4632" : "#5A1A1A"
                            border.color: model.ok ? "#82D4A7" : "#FF6666"
                            border.width: 1
                            
                            Text {
                                text: model.ok ? "成功" : "失败"
                                anchors.centerIn: parent
                                color: model.ok ? "#82D4A7" : "#FFFFFF"
                                font.pixelSize: 10
                                font.bold: true
                            }
                        }
                    }
                }
            }

            ScrollBar.vertical: ScrollBar {}
        }
    }

    Connections {
        target: DeviceConsole
        function onActionFeedbackChanged() {
            var feedback = DeviceConsole.actionFeedback
            if (!feedback || feedback.deviceId !== root.deviceId) return

            // 优先使用信号自带的 hex，如果为空则回退到快照（兼容旧逻辑或异步手动刷新）
            var hex = feedback.hex || ""
            if (!hex) {
                var snapshot = DeviceConsole.selectedSnapshot
                if (snapshot) {
                    hex = snapshot["transport.lastTxHex"]
                          || snapshot["transport.lastTxFrameHex"]
                          || snapshot["tx.lastFrameHex"]
                          || snapshot["eddy.last.payloadHex"]
                          || ""
                }
            }

            var timestamp = new Date().toLocaleTimeString('zh-CN', { hour12: false })
            
            historyModel.insert(0, {
                timestamp: timestamp,
                action: feedback.action,
                ok: feedback.ok,
                message: feedback.message,
                hex: hex.toUpperCase()
            })

            if (historyModel.count > 100) {
                historyModel.remove(100)
            }
        }
    }
}
