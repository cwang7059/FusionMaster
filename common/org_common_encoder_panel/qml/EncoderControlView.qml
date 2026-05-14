import QtQuick 2.14
import QtQuick.Controls 2.14
import QtQuick.Layouts 1.14
import UITheme 1.0
import DeviceConsole 1.0
import "qrc:/org.common.device.console/qml/" as Common

StyledCard {
    property bool isEncoderDevice: (typeof DeviceConsole !== "undefined" && DeviceConsole)
                                   && (DeviceConsole.selectedDeviceId === "encoder"
                                       || DeviceConsole.selectedDeviceId === "encoder55aa")
    property var snapshot: mapOrEmpty((typeof DeviceConsole !== "undefined" && DeviceConsole)
                                      ? DeviceConsole.selectedSnapshot
                                      : ({}))
    property var feedback: isEncoderDevice
                           ? mapOrEmpty((typeof DeviceConsole !== "undefined" && DeviceConsole)
                                        ? DeviceConsole.actionFeedback
                                        : ({}))
                           : ({})
    property var state: isEncoderDevice ? ({
        connected: stateValueOf("status.connected", false),
        protocol: stateValueOf("encoder.protocolText", "unknown"),
        serialNumber: stateValueOf("encoder.serialNumber", ""),
        versionText: stateValueOf("encoder.versionText", ""),
        streamingMode: stateValueOf("encoder.streaming.mode", "idle"),
        resolution: stateValueOf("encoder.resolution", ""),
        outerAngle: stateValueOf("encoder.outer.angle", ""),
        innerAngle: stateValueOf("encoder.inner.angle", ""),
        outerSinAd: stateValueOf("encoder.outer.sinAd", ""),
        outerCosAd: stateValueOf("encoder.outer.cosAd", ""),
        innerSinAd: stateValueOf("encoder.inner.sinAd", ""),
        innerCosAd: stateValueOf("encoder.inner.cosAd", ""),
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
                Text { text: state.connected ? "已连接" : "未连接"; color: state.connected ? "#82D4A7" : Theme.textPrimary }

                Text { text: "协议类型:"; color: Theme.textSecondary }
                Text { text: state.protocol || "-"; color: Theme.textPrimary }

                Text { text: "设备序列号:"; color: Theme.textSecondary }
                Text { text: state.serialNumber || "-"; color: Theme.textPrimary }

                Text { text: "内部版本号:"; color: Theme.textSecondary }
                Text { text: state.versionText || "-"; color: Theme.textPrimary }

                Text { text: "流模式状态:"; color: Theme.textSecondary }
                Text { text: state.streamingMode || "-"; color: Theme.textPrimary }

                Text { text: "分辨率:"; color: Theme.textSecondary }
                Text { text: state.resolution || "-"; color: Theme.textPrimary }

                Text { text: "外圈角度:"; color: Theme.textSecondary }
                Text { text: state.outerAngle !== "" ? Number(state.outerAngle).toFixed(4) + "°" : "-"; color: Theme.accent }

                Text { text: "内圈角度:"; color: Theme.textSecondary }
                Text { text: state.innerAngle !== "" ? Number(state.innerAngle).toFixed(4) + "°" : "-"; color: Theme.accent }

                Text { text: "外圈 SIN AD:"; color: Theme.textSecondary }
                Text { text: state.outerSinAd || "-"; color: Theme.textPrimary }

                Text { text: "外圈 COS AD:"; color: Theme.textSecondary }
                Text { text: state.outerCosAd || "-"; color: Theme.textPrimary }

                Text { text: "内圈 SIN AD:"; color: Theme.textSecondary }
                Text { text: state.innerSinAd || "-"; color: Theme.textPrimary }

                Text { text: "内圈 COS AD:"; color: Theme.textSecondary }
                Text { text: state.innerCosAd || "-"; color: Theme.textPrimary }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            height: 1
            color: Theme.inputBorder
            Layout.topMargin: Theme.spacingSmall
            Layout.bottomMargin: Theme.spacingSmall
        }

        ColumnLayout {
            id: operationCol
            Layout.fillWidth: true
            spacing: Theme.spacingMedium

            Text {
                text: "编码器操作 (55AA 协议)"
                color: Theme.textPrimary
                font.bold: true
                Layout.topMargin: 8
            }

            // --- 功能行: 基础读取 ---
            RowLayout {
                Layout.fillWidth: true
                spacing: Theme.spacingNormal
                Text { text: "身份信息查询:"; color: Theme.textPrimary; Layout.preferredWidth: 140 }
                StyledButton {
                    text: "读取版本"
                    Layout.preferredWidth: 180
                    onClicked: DeviceConsole.invokeCurrent("encoder55aa.read.version")
                }
                StyledButton {
                    text: "读取协议"
                    Layout.preferredWidth: 180
                    onClicked: DeviceConsole.invokeCurrent("encoder55aa.read.protocol")
                }
                Item { Layout.fillWidth: true }
            }

            // --- 功能行: 参数读取 ---
            RowLayout {
                Layout.fillWidth: true
                spacing: Theme.spacingNormal
                Text { text: "参数配置查询:"; color: Theme.textPrimary; Layout.preferredWidth: 140 }
                StyledButton {
                    text: "读取分辨率"
                    Layout.preferredWidth: 180
                    onClicked: DeviceConsole.invokeCurrent("encoder55aa.read.resolution")
                }
                Item { Layout.fillWidth: true }
            }

            // --- 功能行: 采样读取 ---
            RowLayout {
                Layout.fillWidth: true
                spacing: Theme.spacingNormal
                Text { text: "IPS 采样查询:"; color: Theme.textPrimary; Layout.preferredWidth: 140 }
                StyledButton {
                    text: "读取外编 IPS"
                    Layout.preferredWidth: 180
                    onClicked: DeviceConsole.invokeCurrent("encoder55aa.read.outer.ips")
                }
                StyledButton {
                    text: "读取内编 IPS"
                    Layout.preferredWidth: 180
                    onClicked: DeviceConsole.invokeCurrent("encoder55aa.read.inner.ips")
                }
                Item { Layout.fillWidth: true }
            }
        }

        Rectangle { Layout.fillWidth: true; height: 1; color: Theme.inputBorder; Layout.topMargin: 8; Layout.bottomMargin: 8 }

        Common.CommandHistoryPanel {
            deviceId: DeviceConsole.selectedDeviceId
            Layout.fillWidth: true
            Layout.preferredHeight: 300
        }
    }
}
