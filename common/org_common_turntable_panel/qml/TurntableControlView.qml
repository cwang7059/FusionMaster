import QtQuick 2.14
import QtQuick.Controls 2.14
import QtQuick.Layouts 1.14
import UITheme 1.0
import DeviceConsole 1.0
import "qrc:/org.common.device.console/qml/" as Common

StyledCard {
    id: root
    property bool isTurntableDevice: (typeof DeviceConsole !== "undefined" && DeviceConsole)
                                     ? DeviceConsole.selectedDeviceId === "turntable"
                                     : false
    property var snapshot: mapOrEmpty((typeof DeviceConsole !== "undefined" && DeviceConsole)
                                      ? DeviceConsole.selectedSnapshot
                                      : ({}))
    property var feedback: isTurntableDevice
                           ? mapOrEmpty((typeof DeviceConsole !== "undefined" && DeviceConsole)
                                        ? DeviceConsole.actionFeedback
                                        : ({}))
                           : ({})
    property var state: isTurntableDevice ? ({
        connected: stateValueOf("status.connected", false),
        linkType: stateValueOf("status.linkType", "none"),
        modeText: stateValueOf("status.modeText", "unknown"),
        imageText: stateValueOf("status.imageText", "unknown"),
        selfCheckCode: stateValueOf("status.selfCheckCode", 0),
        checksumOk: stateValueOf("status.lastChecksumOk", false),
        azimuthAngle: stateValueOf("runtime.azimuthAngle", 0),
        elevationAngle: stateValueOf("runtime.elevationAngle", 0),
        azimuthRate: stateValueOf("runtime.azimuthRate", 0),
        elevationRate: stateValueOf("runtime.elevationRate", 0),
        rollRate: stateValueOf("runtime.rollRate", 0),
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

    readonly property int currentTemplate: (templateCombo.model && templateCombo.model[templateCombo.currentIndex]) 
                                            ? templateCombo.model[templateCombo.currentIndex].value : 1

    function sendCommand(command, azimuth, elevation, inertialAzimuth, inertialElevation, inertialRoll, reserved) {
        return DeviceConsole.invokeCurrent("turntable.send.command", {
            templateVersion: root.currentTemplate,
            command: String(command || "").trim(),
            azimuth: azimuth,
            elevation: elevation,
            inertialAzimuth: inertialAzimuth,
            inertialElevation: inertialElevation,
            inertialRoll: inertialRoll,
            reserved: String(reserved || "").trim()
        })
    }

    ColumnLayout {
        id: contentCol
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.margins: Theme.spacingNormal
        spacing: Theme.spacingMedium

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

                Text { text: "工作模式:"; color: Theme.textSecondary }
                Text {
                    text: (state && state.modeText !== undefined && state.modeText !== null)
                          ? String(state.modeText)
                          : "unknown"
                    color: Theme.textPrimary
                }

                Text { text: "图像状态:"; color: Theme.textSecondary }
                Text {
                    text: (state && state.imageText !== undefined && state.imageText !== null)
                          ? String(state.imageText)
                          : "unknown"
                    color: Theme.textPrimary
                }

                Text { text: "自检代码:"; color: Theme.textSecondary }
                Text {
                    text: (state && state.selfCheckCode !== undefined && state.selfCheckCode !== null)
                          ? String(state.selfCheckCode)
                          : "0"
                    color: Number(text) === 0 ? Theme.textPrimary : Theme.danger
                }

                Text { text: "方位角:"; color: Theme.textSecondary }
                Text { text: Number(state.azimuthAngle).toFixed(4) + "°"; color: Theme.textPrimary }

                Text { text: "俯仰角:"; color: Theme.textSecondary }
                Text { text: Number(state.elevationAngle).toFixed(4) + "°"; color: Theme.textPrimary }

                Text { text: "方位角速度:"; color: Theme.textSecondary }
                Text { text: Number(state.azimuthRate).toFixed(4) + "°/s"; color: Theme.textPrimary }

                Text { text: "俯仰角速度:"; color: Theme.textSecondary }
                Text { text: Number(state.elevationRate).toFixed(4) + "°/s"; color: Theme.textPrimary }

                Text { text: "横滚角速度:"; color: Theme.textSecondary }
                Text { text: Number(state.rollRate).toFixed(4) + "°/s"; color: Theme.textPrimary }

                Text { text: "校验状态:"; color: Theme.textSecondary }
                Text { text: state.checksumOk ? "正常" : "错误"; color: state.checksumOk ? Theme.textPrimary : Theme.danger }
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
            text: "全局设置与惯导姿态"
            color: Theme.textPrimary
            font.bold: true
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.spacingSmall

            StyledComboBox {
                id: templateCombo
                Layout.preferredWidth: 120
                model: [
                    { text: "模板 1", value: 1 },
                    { text: "模板 2", value: 2 }
                ]
                textRole: "text"
            }

            StyledInput {
                id: reservedField
                Layout.preferredWidth: 200
                placeholderText: "保留字节 (HEX)"
                text: "00 00 00 00"
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: Theme.spacingSmall
                visible: root.currentTemplate === 2

                StyledInput { id: inertialAzField; Layout.preferredWidth: 150; text: "0"; placeholderText: "惯导方位" }
                StyledInput { id: inertialElField; Layout.preferredWidth: 150; text: "0"; placeholderText: "惯导俯仰" }
                StyledInput { id: inertialRollField; Layout.preferredWidth: 150; text: "0"; placeholderText: "惯导横滚" }
                Item { Layout.fillWidth: true }
            }
        }

        Rectangle { Layout.fillWidth: true; height: 1; color: Theme.inputBorder; Layout.topMargin: 8; Layout.bottomMargin: 8 }

        Text {
            text: "功能指令操作 (一功能一确认)"
            color: Theme.textPrimary
            font.bold: true
            Layout.topMargin: 8
        }

        // --- 功能行: 机架指向 ---
        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.spacingNormal
            Text { text: "机架指向 (0x11):"; color: Theme.textPrimary; Layout.preferredWidth: 140 }
            StyledInput { id: frameAzInput; Layout.preferredWidth: 180; text: "0"; placeholderText: "目标方位角度" }
            StyledInput { id: frameElInput; Layout.preferredWidth: 180; text: "0"; placeholderText: "目标俯仰角度" }
            StyledButton {
                text: "确定下发"
                onClicked: root.sendCommand("0x11", parseFloat(frameAzInput.text), parseFloat(frameElInput.text),
                                            parseFloat(inertialAzField.text), parseFloat(inertialElField.text), parseFloat(inertialRollField.text), reservedField.text)
            }
            Item { Layout.fillWidth: true }
        }

        // --- 功能行: 大地指向 ---
        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.spacingNormal
            Text { text: "大地指向 (0x22):"; color: Theme.textPrimary; Layout.preferredWidth: 140 }
            StyledInput { id: groundAzInput; Layout.preferredWidth: 180; text: "0"; placeholderText: "目标方位角度" }
            StyledInput { id: groundElInput; Layout.preferredWidth: 180; text: "0"; placeholderText: "目标俯仰角度" }
            StyledButton {
                text: "确定下发"
                onClicked: root.sendCommand("0x22", parseFloat(groundAzInput.text), parseFloat(groundElInput.text),
                                            parseFloat(inertialAzField.text), parseFloat(inertialElField.text), parseFloat(inertialRollField.text), reservedField.text)
            }
            Item { Layout.fillWidth: true }
        }

        // --- 功能行: 角速度控制 ---
        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.spacingNormal
            Text { text: "角速度控制 (0x44):"; color: Theme.textPrimary; Layout.preferredWidth: 140 }
            StyledInput { id: rateAzInput; Layout.preferredWidth: 180; text: "0"; placeholderText: "方位角速度" }
            StyledInput { id: rateElInput; Layout.preferredWidth: 180; text: "0"; placeholderText: "俯仰角速度" }
            StyledButton {
                text: "确定下发"
                onClicked: root.sendCommand("0x44", parseFloat(rateAzInput.text), parseFloat(rateElInput.text),
                                            parseFloat(inertialAzField.text), parseFloat(inertialElField.text), parseFloat(inertialRollField.text), reservedField.text)
            }
            Item { Layout.fillWidth: true }
        }

        // --- 功能行: 跟踪控制 ---
        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.spacingNormal
            Text { text: "图像跟踪控制:"; color: Theme.textPrimary; Layout.preferredWidth: 140 }
            StyledInput { id: trackAzDev; Layout.preferredWidth: 180; text: "0"; placeholderText: "方位偏差" }
            StyledInput { id: trackElDev; Layout.preferredWidth: 180; text: "0"; placeholderText: "俯仰偏差" }
            StyledButton {
                text: "可见光 (0x66)"
                onClicked: root.sendCommand("0x66", parseFloat(trackAzDev.text), parseFloat(trackElDev.text),
                                            parseFloat(inertialAzField.text), parseFloat(inertialElField.text), parseFloat(inertialRollField.text), reservedField.text)
            }
            StyledButton {
                text: "红外 (0x88)"
                onClicked: root.sendCommand("0x88", parseFloat(trackAzDev.text), parseFloat(trackElDev.text),
                                            parseFloat(inertialAzField.text), parseFloat(inertialElField.text), parseFloat(inertialRollField.text), reservedField.text)
            }
            Item { Layout.fillWidth: true }
        }

        // --- 功能行: 系统操作 ---
        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.spacingNormal
            Text { text: "状态与使能控制:"; color: Theme.textPrimary; Layout.preferredWidth: 140 }
            StyledButton {
                text: "停止控制 (0x5A)"
                Layout.preferredWidth: 180
                onClicked: DeviceConsole.invokeCurrent("turntable.send.stop")
            }
            StyledButton {
                text: "使能控制 (0x5B)"
                Layout.preferredWidth: 180
                onClicked: DeviceConsole.invokeCurrent("turntable.send.enable")
            }
            Item { Layout.fillWidth: true }
        }

        Rectangle { Layout.fillWidth: true; height: 1; color: Theme.inputBorder; Layout.topMargin: 8; Layout.bottomMargin: 8 }

        Common.CommandHistoryPanel {
            deviceId: root.isTurntableDevice ? DeviceConsole.selectedDeviceId : ""
            Layout.fillWidth: true
            Layout.preferredHeight: 300
        }
    }
}
