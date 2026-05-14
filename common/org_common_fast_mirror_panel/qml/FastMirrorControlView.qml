import QtQuick 2.14
import QtQuick.Controls 2.14
import QtQuick.Layouts 1.14
import UITheme 1.0
import DeviceConsole 1.0
import "qrc:/org.common.device.console/qml/" as Common

StyledCard {
    id: root
    property bool isFastMirrorDevice: (typeof DeviceConsole !== "undefined" && DeviceConsole)
                                      ? DeviceConsole.selectedDeviceId === "fast_mirror"
                                      : false
    property var snapshot: mapOrEmpty((typeof DeviceConsole !== "undefined" && DeviceConsole)
                                      ? DeviceConsole.selectedSnapshot
                                      : ({}))
    property var feedback: isFastMirrorDevice
                           ? mapOrEmpty((typeof DeviceConsole !== "undefined" && DeviceConsole)
                                        ? DeviceConsole.actionFeedback
                                        : ({}))
                           : ({})
    property var state: isFastMirrorDevice ? ({
        connected: stateValueOf("status.connected", false),
        linkType: stateValueOf("status.linkType", "none"),
        modeCode: stateValueOf("status.modeCode", 0),
        modeText: stateValueOf("status.modeText", "idle"),
        currentXAngle: stateValueOf("runtime.currentXAngle", 0),
        currentYAngle: stateValueOf("runtime.currentYAngle", 0),
        guideX: stateValueOf("command.guideX", 0),
        guideY: stateValueOf("command.guideY", 0),
        lastCommandMode: stateValueOf("command.lastModeText", "idle"),
        faultBitsHex: stateValueOf("status.faultBitsHex", "0x0000"),
        checksumOk: stateValueOf("status.lastChecksumOk", false),
        selfCheckFault: stateValueOf("status.selfCheckFault", false),
        inputVoltageFault: stateValueOf("status.inputVoltageFault", false),
        driveOutputFault: stateValueOf("status.driveOutputFault", false),
        commTimeout: stateValueOf("status.commTimeout", false),
        checksumFault: stateValueOf("status.checksumFault", false),
        motorDisabled: stateValueOf("status.motorDisabled", false),
        locked: stateValueOf("status.locked", false),
        lastTxFrameHex: stateValueOf("transport.lastTxFrameHex", ""),
        lastRxFrameHex: stateValueOf("transport.lastRxFrameHex", ""),
        lastError: stateValueOf("transport.lastError", "")
    }) : ({})
    implicitHeight: contentCol.implicitHeight + Theme.spacingNormal * 2
    width: parent ? parent.width : implicitWidth

    function stateValueOf(key, fallbackValue) {
        try {
            var map = snapshot
            if (map === undefined || map === null || typeof map !== "object") {
                map = (typeof DeviceConsole !== "undefined" && DeviceConsole && DeviceConsole.selectedSnapshot)
                      ? DeviceConsole.selectedSnapshot
                      : ({})
            }
            var value = map && typeof map === "object" ? map[key] : undefined
            if ((value === undefined || value === null)
                && typeof DeviceConsole !== "undefined"
                && DeviceConsole
                && DeviceConsole.snapshotValue) {
                value = DeviceConsole.snapshotValue(key)
            }
            return value === undefined || value === null ? fallbackValue : value
        } catch (error) {
            return fallbackValue
        }
    }

    function mapOrEmpty(value) {
        if (value === undefined || value === null) return ({})
        if (typeof value !== "object") return ({})
        return value
    }

    function modeLabel(value) {
        if (value === 0) {
            return "空闲模式"
        }
        if (value === 1) {
            return "锁零模式"
        }
        if (value === 2) {
            return "定点模式"
        }
        return "故障模式"
    }

    function sendCurrentMode() {
        return DeviceConsole.invokeCurrent("fast_mirror.send.command", {
            mode: modeCombo.model[modeCombo.currentIndex].value,
            xGuide: parseFloat(xGuideField.text || "0"),
            yGuide: parseFloat(yGuideField.text || "0")
        })
    }

    ColumnLayout {
        id: contentCol
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.margins: Theme.spacingSmall
        spacing: Theme.spacingSmall

        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.spacingLarge

            Text {
                text: "状态监控"
                color: Theme.textPrimary
                font.bold: true
                Layout.alignment: Qt.AlignTop
            }

            GridLayout {
                Layout.fillWidth: true
                columns: 4
                rowSpacing: Theme.spacingSmall
                columnSpacing: Theme.spacingMedium

                Text { text: "连接状态:"; color: Theme.textSecondary }
                Text { text: state.connected ? ("已连接 (" + state.linkType + ")") : "未连接"; color: state.connected ? "#82D4A7" : Theme.textPrimary }

                Text { text: "当前工作模式:"; color: Theme.textSecondary }
                Text {
                    text: (state && state.modeText !== undefined && state.modeText !== null)
                          ? String(state.modeText)
                          : "idle"
                    color: Theme.textPrimary
                }

                Text { text: "当前 X 轴角度:"; color: Theme.textSecondary }
                Text { text: Number(state.currentXAngle).toFixed(4) + "°"; color: Theme.textPrimary }

                Text { text: "当前 Y 轴角度:"; color: Theme.textSecondary }
                Text { text: Number(state.currentYAngle).toFixed(4) + "°"; color: Theme.textPrimary }

                Text { text: "引导 X 轴:"; color: Theme.textSecondary }
                Text { text: Number(state.guideX).toFixed(4) + "°"; color: Theme.textPrimary }

                Text { text: "引导 Y 轴:"; color: Theme.textSecondary }
                Text { text: Number(state.guideY).toFixed(4) + "°"; color: Theme.textPrimary }

                Text { text: "锁定状态:"; color: Theme.textSecondary }
                Text { text: state.locked ? "已锁定" : "未锁定"; color: state.locked ? Theme.accent : Theme.textPrimary }

                Text { text: "电机状态:"; color: Theme.textSecondary }
                Text { text: state.motorDisabled ? "已禁用" : "已使能"; color: state.motorDisabled ? Theme.danger : "#82D4A7" }

                Text { text: "故障代码:"; color: Theme.textSecondary }
                Text {
                    text: (state && state.faultBitsHex !== undefined && state.faultBitsHex !== null)
                          ? String(state.faultBitsHex)
                          : "0x0000"
                    color: text === "0x0000" ? Theme.textPrimary : Theme.danger
                }

                Text { text: "报警信息:"; color: Theme.textSecondary }
                Text { 
                    text: (function() {
                        var alarms = []
                        if (state.selfCheckFault) alarms.push("自检故障")
                        if (state.inputVoltageFault) alarms.push("输入电压故障")
                        if (state.driveOutputFault) alarms.push("驱动输出故障")
                        if (state.commTimeout) alarms.push("通信超时")
                        if (state.checksumFault) alarms.push("内部校验错误")
                        if (!state.checksumOk) alarms.push("数据包异常")
                        return alarms.length > 0 ? alarms.join(", ") : "无异常"
                    })()
                    color: text === "无异常" ? Theme.textPrimary : Theme.danger
                    elide: Text.ElideRight
                    Layout.fillWidth: true
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            height: 1
            color: Theme.inputBorder
            Layout.topMargin: Theme.spacingSmall
            Layout.bottomMargin: Theme.spacingSmall
        }

        Text {
            text: "快反镜专用操作"
            color: Theme.textPrimary
            font.bold: true
        }

        Text {
            Layout.fillWidth: true
            text: "通信约束：RS-422 串口，230400bps，8N1，无校验。"
            color: Theme.textSecondary
            wrapMode: Text.Wrap
        }

        ColumnLayout {
            id: operationCol
            Layout.fillWidth: true
            spacing: Theme.spacingMedium

            Text {
                text: "功能指令操作 (一功能一确认)"
                color: Theme.textPrimary
                font.bold: true
                Layout.topMargin: 8
            }

            // --- 功能行: 空闲 ---
            RowLayout {
                Layout.fillWidth: true
                spacing: Theme.spacingNormal
                Text { text: "空闲模式 (0x00):"; color: Theme.textPrimary; Layout.preferredWidth: 140 }
                StyledButton {
                    text: "点击进入空闲"
                    Layout.preferredWidth: 180
                    onClicked: DeviceConsole.invokeCurrent("fast_mirror.set.idle")
                }
                Item { Layout.fillWidth: true }
            }

            // --- 功能行: 锁零 ---
            RowLayout {
                Layout.fillWidth: true
                spacing: Theme.spacingNormal
                Text { text: "锁零模式 (0x01):"; color: Theme.textPrimary; Layout.preferredWidth: 140 }
                StyledButton {
                    text: "点击进入锁零"
                    Layout.preferredWidth: 180
                    onClicked: DeviceConsole.invokeCurrent("fast_mirror.set.lock_zero")
                }
                Item { Layout.fillWidth: true }
            }

            // --- 功能行: 定点 ---
            RowLayout {
                Layout.fillWidth: true
                spacing: Theme.spacingNormal
                Text { text: "定点模式 (0x02):"; color: Theme.textPrimary; Layout.preferredWidth: 140 }
                StyledInput { id: xGuideField; Layout.preferredWidth: 180; text: "0"; placeholderText: "X 轴引导值" }
                StyledInput { id: yGuideField; Layout.preferredWidth: 180; text: "0"; placeholderText: "Y 轴引导值" }
                StyledButton {
                    text: "确定下发定点"
                    onClicked: DeviceConsole.invokeCurrent("fast_mirror.set.point", {
                                   xGuide: parseFloat(xGuideField.text || "0"),
                                   yGuide: parseFloat(yGuideField.text || "0")
                               })
                }
                Item { Layout.fillWidth: true }
            }

            Rectangle { Layout.fillWidth: true; height: 1; color: Theme.inputBorder; Layout.topMargin: 4; Layout.bottomMargin: 4 }

            // --- 功能行: 原始帧 ---
            RowLayout {
                Layout.fillWidth: true
                spacing: Theme.spacingNormal
                Text { text: "数据帧测试:"; color: Theme.textPrimary; Layout.preferredWidth: 140 }
                StyledInput {
                    id: rawHexField
                    Layout.fillWidth: true
                    placeholderText: "原始十六进制指令，示例 55 AA 0A 00 ..."
                }
                StyledButton {
                    text: "发送原始帧"
                    enabled: isFastMirrorDevice
                    onClicked: DeviceConsole.sendRawHex(rawHexField.text)
                }
            }
        }

        Rectangle { Layout.fillWidth: true; height: 1; color: Theme.inputBorder; Layout.topMargin: 8; Layout.bottomMargin: 8 }

        Common.CommandHistoryPanel {
            deviceId: root.isFastMirrorDevice ? DeviceConsole.selectedDeviceId : ""
            Layout.fillWidth: true
            Layout.preferredHeight: 300
        }
    }
}
