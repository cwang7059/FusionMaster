import QtQuick 2.14
import QtQuick.Controls 2.14
import QtQuick.Layouts 1.14
import UITheme 1.0
import DeviceConsole 1.0
import "qrc:/org.common.device.console/qml/" as Common

StyledCard {
    id: root
    property bool isEddyCurrentDevice: DeviceConsole.selectedDeviceId === "eddy_current"
    property var snapshot: DeviceConsole.selectedSnapshot || ({})
    property var state: isEddyCurrentDevice ? ({
        connected: valueOf("status.connected", false),
        serialNumber: valueOf("eddy.serialNumber", ""),
        versionText: valueOf("eddy.versionText", ""),
        frequency: valueOf("eddy.frequency", ""),
        protocolType: valueOf("eddy.protocolType", 0),
        protocolText: valueOf("eddy.protocolText", ""),
        uploadMode: valueOf("eddy.uploadMode", "idle"),
        adValues: valueOf("eddy.adValues", []),
        displacementValues: valueOf("eddy.displacementValues", []),
        rangeValues: valueOf("eddy.range", []),
        gainValues: valueOf("eddy.gain", valueOf("eddy.fitParams", [])),
        zeroValues: valueOf("eddy.zero", valueOf("eddy.interpolationParams", [])),
        phaseValues: valueOf("eddy.phaseValues", valueOf("eddy.phase", [])),
        overRange: valueOf("eddy.status.overRange", false),
        power: valueOf("eddy.status.power", ""),
        temperature: valueOf("eddy.status.temperature", ""),
        lastCommand: valueOf("eddy.last.command", ""),
        lastPayloadHex: valueOf("eddy.last.payloadHex", ""),
        lastError: valueOf("transport.lastError", "")
    }) : ({})

    readonly property color successColor: "#82D4A7"
    readonly property color errorColor: "#FF7A7A"
    readonly property int labelWidth: 140
    readonly property int fieldWidth: 160
    readonly property int pairFieldWidth: 120

    implicitHeight: contentCol.implicitHeight + Theme.spacingNormal * 2
    width: parent ? parent.width : implicitWidth

    function valueOf(key, fallbackValue) {
        var value = snapshot[key]
        return value === undefined || value === null ? fallbackValue : value
    }

    function listValue(values, index, fallbackValue) {
        if (values instanceof Array && index >= 0 && index < values.length) {
            var value = values[index]
            return value === undefined || value === null ? fallbackValue : value
        }
        return fallbackValue
    }

    function formatNumber(value, digits, suffix, fallbackValue) {
        var fallbackText = fallbackValue === undefined ? "-" : fallbackValue
        if (value === undefined || value === null || value === "") return fallbackText

        var number = Number(value)
        if (isNaN(number)) return String(value)

        var text = number.toFixed(digits === undefined ? 3 : digits)
        return suffix ? (text + suffix) : text
    }

    function formatPair(values, digits, suffix) {
        return "CH1 " + formatNumber(listValue(values, 0, null), digits, suffix, "-")
             + "  CH2 " + formatNumber(listValue(values, 1, null), digits, suffix, "-")
    }

    function protocolLabel(value) {
        var number = Number(value)
        if (!isNaN(number)) {
            return number === 1 ? "RKX" : "标准"
        }

        var text = String(value || "").trim()
        return text.length ? text : "-"
    }

    function protocolLabelFromState() {
        var text = String(state.protocolText || "").trim()
        if (text.length && text !== "0" && text !== "1") return text
        return protocolLabel(state.protocolType)
    }

    function uploadModeLabel(value) {
        var text = String(value || "").toLowerCase()
        if (text === "ad") return "AD 上传"
        if (text === "displacement") return "位移上传"
        return "停止"
    }

    function parseFloatPair(text1, text2) {
        var first = Number(text1)
        var second = Number(text2)
        return [isNaN(first) ? 0.0 : first, isNaN(second) ? 0.0 : second]
    }

    function parseIntPair(text1, text2) {
        var first = parseInt(text1 || "0", 10)
        var second = parseInt(text2 || "0", 10)
        return [isNaN(first) ? 0 : first, isNaN(second) ? 0 : second]
    }

    function parseIntValue(text, fallbackValue) {
        var value = parseInt(text || "", 10)
        return isNaN(value) ? fallbackValue : value
    }

    function syncFieldText(field, value) {
        if (!field || field.activeFocus) return

        var nextText = value === undefined || value === null ? "" : String(value)
        if (field.text !== nextText) field.text = nextText
    }

    function comboIndexByValue(combo, key, expectedValue) {
        if (!combo) return -1

        var model = combo.model || []
        for (var index = 0; index < model.length; ++index) {
            var item = model[index]
            if (item && item[key] === expectedValue) return index
        }
        return -1
    }

    function syncComboSelection(combo, key, expectedValue) {
        if (!combo || combo.popup.visible) return

        var index = comboIndexByValue(combo, key, expectedValue)
        if (index >= 0 && combo.currentIndex !== index) combo.currentIndex = index
    }

    function syncFormFields() {
        syncFieldText(frequencyField, state.frequency)
        syncFieldText(rangeMinField, listValue(state.rangeValues, 0, ""))
        syncFieldText(rangeMaxField, listValue(state.rangeValues, 1, ""))
        syncFieldText(serialField, state.serialNumber)
        syncFieldText(gain1Field, listValue(state.gainValues, 0, ""))
        syncFieldText(gain2Field, listValue(state.gainValues, 1, ""))
        syncFieldText(zero1Field, listValue(state.zeroValues, 0, ""))
        syncFieldText(zero2Field, listValue(state.zeroValues, 1, ""))
        syncFieldText(phase1Field, listValue(state.phaseValues, 0, ""))
        syncFieldText(phase2Field, listValue(state.phaseValues, 1, ""))
        syncComboSelection(protocolCombo, "value", Number(state.protocolType))

        var uploadMode = String(state.uploadMode || "").toLowerCase()
        if (uploadMode === "ad" || uploadMode === "displacement")
            syncComboSelection(uploadModeCombo, "value", uploadMode)
    }

    function selectedUploadAction() {
        var model = uploadModeCombo.model || []
        var item = model[uploadModeCombo.currentIndex]
        return item && item.action ? item.action : "eddy55aa.start.ad_upload"
    }

    function selectedProtocolType() {
        var model = protocolCombo.model || []
        var item = model[protocolCombo.currentIndex]
        return item && item.value !== undefined ? item.value : 0
    }

    function invokeSequence(commands) {
        for (var index = 0; index < commands.length; ++index) {
            commandQueue.append(commands[index])
        }
        if (!queueTimer.running) queueTimer.start()
    }

    function queryAll() {
        if (!isEddyCurrentDevice) return

        invokeSequence([
            { action: "eddy55aa.read.version" },
            { action: "eddy55aa.read.serial" },
            { action: "eddy55aa.read.frequency" },
            { action: "eddy55aa.read.range" },
            { action: "eddy55aa.read.gain" },
            { action: "eddy55aa.read.zero" },
            { action: "eddy55aa.read.protocol" },
            { action: "eddy55aa.read.phase" }
        ])
    }

    function startUpload() {
        if (!isEddyCurrentDevice) return

        var periodMs = parseInt(feedbackPeriodField.text || "1000", 10)
        if (isNaN(periodMs)) periodMs = 1000

        invokeSequence([
            {
                action: "eddy55aa.set.feedback_period",
                args: { seconds: Math.max(1, Math.round(periodMs / 1000)) }
            },
            { action: selectedUploadAction() }
        ])
    }

    onSnapshotChanged: syncFormFields()
    Component.onCompleted: syncFormFields()

    ListModel {
        id: commandQueue
    }

    Timer {
        id: queueTimer
        interval: 140
        repeat: true
        onTriggered: {
            if (!commandQueue.count) {
                stop()
                return
            }

            var command = commandQueue.get(0)
            commandQueue.remove(0)
            DeviceConsole.invokeCurrent(command.action, command.args || {})
        }
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
                Text {
                    text: state.connected ? "已连接" : "未连接"
                    color: state.connected ? root.successColor : Theme.textPrimary
                }

                Text { text: "版本信息:"; color: Theme.textSecondary }
                Text { text: state.versionText || "-"; color: Theme.textPrimary }

                Text { text: "设备序列号:"; color: Theme.textSecondary }
                Text { text: state.serialNumber || "-"; color: Theme.textPrimary }

                Text { text: "当前频率:"; color: Theme.textSecondary }
                Text { text: root.formatNumber(state.frequency, 0, " Hz", "-"); color: Theme.textPrimary }

                Text { text: "协议类型:"; color: Theme.textSecondary }
                Text { text: root.protocolLabelFromState(); color: Theme.textPrimary }

                Text { text: "上传模式:"; color: Theme.textSecondary }
                Text {
                    text: root.uploadModeLabel(state.uploadMode)
                    color: state.uploadMode === "idle" ? Theme.textPrimary : root.successColor
                }

                Text { text: "供电状态:"; color: Theme.textSecondary }
                Text { text: root.formatNumber(state.power, 0, "", "-"); color: Theme.textPrimary }

                Text { text: "内部温度:"; color: Theme.textSecondary }
                Text { text: root.formatNumber(state.temperature, 0, " °C", "-"); color: Theme.textPrimary }

                Text { text: "标定量程:"; color: Theme.textSecondary }
                Text { text: root.formatPair(state.rangeValues, 3, ""); color: Theme.textPrimary }

                Text { text: "超程告警:"; color: Theme.textSecondary }
                Text {
                    text: state.overRange ? "超程" : "正常"
                    color: state.overRange ? root.errorColor : Theme.textPrimary
                }

                Text { text: "增益系数:"; color: Theme.textSecondary }
                Text { text: root.formatPair(state.gainValues, 0, ""); color: Theme.textPrimary }

                Text { text: "零位偏移:"; color: Theme.textSecondary }
                Text { text: root.formatPair(state.zeroValues, 0, ""); color: Theme.textPrimary }

                Text { text: "相位参数:"; color: Theme.textSecondary }
                Text { text: root.formatPair(state.phaseValues, 0, ""); color: Theme.textPrimary }

                Text { text: "AD 通道:"; color: Theme.textSecondary }
                Text { text: root.formatPair(state.adValues, 0, ""); color: Theme.accent }

                Text { text: "位移通道:"; color: Theme.textSecondary }
                Text { text: root.formatPair(state.displacementValues, 3, ""); color: Theme.accent }

                Text { text: "最近命令:"; color: Theme.textSecondary }
                Text {
                    text: state.lastCommand || "-"
                    color: Theme.textPrimary
                    font.family: "Consolas, Monaco, monospace"
                }
            }
        }

        Text {
            Layout.fillWidth: true
            text: "最近载荷: " + (state.lastPayloadHex || "-")
            color: Theme.textSecondary
            font.family: "Consolas, Monaco, monospace"
            wrapMode: Text.Wrap
        }

        Text {
            Layout.fillWidth: true
            text: state.lastError ? ("最近错误: " + state.lastError) : "最近错误: 无"
            color: state.lastError ? root.errorColor : Theme.textSecondary
            wrapMode: Text.Wrap
        }

        Rectangle {
            Layout.fillWidth: true
            height: 1
            color: Theme.inputBorder
            Layout.topMargin: Theme.spacingSmall
            Layout.bottomMargin: Theme.spacingSmall
        }

        Text {
            text: "电涡流专用操作"
            color: Theme.textPrimary
            font.bold: true
        }

        Text {
            Layout.fillWidth: true
            text: "采用与编码器、快反镜一致的右侧卡片结构，保留 55AA 电涡流的查询、上传和参数写入能力。"
            color: Theme.textSecondary
            wrapMode: Text.Wrap
        }

        ColumnLayout {
            id: operationCol
            Layout.fillWidth: true
            spacing: Theme.spacingMedium

            Text {
                text: "查询操作"
                color: Theme.textPrimary
                font.bold: true
                Layout.topMargin: 8
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: Theme.spacingNormal

                Text {
                    text: "基础查询:"
                    color: Theme.textPrimary
                    Layout.preferredWidth: root.labelWidth
                }
                StyledButton { text: "读取全部"; onClicked: root.queryAll() }
                StyledButton { text: "版本"; onClicked: DeviceConsole.invokeCurrent("eddy55aa.read.version") }
                StyledButton { text: "协议"; onClicked: DeviceConsole.invokeCurrent("eddy55aa.read.protocol") }
                StyledButton { text: "序列号"; onClicked: DeviceConsole.invokeCurrent("eddy55aa.read.serial") }
                Item { Layout.fillWidth: true }
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: Theme.spacingNormal

                Text {
                    text: "参数查询:"
                    color: Theme.textPrimary
                    Layout.preferredWidth: root.labelWidth
                }
                StyledButton { text: "频率"; onClicked: DeviceConsole.invokeCurrent("eddy55aa.read.frequency") }
                StyledButton { text: "量程"; onClicked: DeviceConsole.invokeCurrent("eddy55aa.read.range") }
                StyledButton { text: "增益"; onClicked: DeviceConsole.invokeCurrent("eddy55aa.read.gain") }
                StyledButton { text: "零位"; onClicked: DeviceConsole.invokeCurrent("eddy55aa.read.zero") }
                StyledButton { text: "相位"; onClicked: DeviceConsole.invokeCurrent("eddy55aa.read.phase") }
                Item { Layout.fillWidth: true }
            }

            Rectangle {
                Layout.fillWidth: true
                height: 1
                color: Theme.inputBorder
                Layout.topMargin: 4
                Layout.bottomMargin: 4
            }

            Text {
                text: "实时上传"
                color: Theme.textPrimary
                font.bold: true
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: Theme.spacingNormal

                Text {
                    text: "采样控制:"
                    color: Theme.textPrimary
                    Layout.preferredWidth: root.labelWidth
                }

                StyledComboBox {
                    id: uploadModeCombo
                    Layout.preferredWidth: root.fieldWidth
                    model: [
                        { text: "AD 上传", value: "ad", action: "eddy55aa.start.ad_upload" },
                        { text: "位移上传", value: "displacement", action: "eddy55aa.start.displacement_upload" }
                    ]
                    textRole: "text"
                }

                StyledInput {
                    id: feedbackPeriodField
                    Layout.preferredWidth: root.fieldWidth
                    text: "1000"
                    placeholderText: "反馈周期(ms)"
                }

                StyledButton {
                    text: "启动上传"
                    onClicked: root.startUpload()
                }

                StyledButton {
                    text: "停止上传"
                    onClicked: DeviceConsole.invokeCurrent("eddy55aa.stop.upload")
                }

                Item { Layout.fillWidth: true }
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: Theme.spacingNormal

                Text {
                    text: "实时反馈:"
                    color: Theme.textPrimary
                    Layout.preferredWidth: root.labelWidth
                }

                Text {
                    text: "AD " + root.formatPair(state.adValues, 0, "")
                    color: Theme.textSecondary
                }

                Text {
                    text: "位移 " + root.formatPair(state.displacementValues, 3, "")
                    color: Theme.textSecondary
                }

                Text {
                    text: "当前模式: " + root.uploadModeLabel(state.uploadMode)
                    color: state.uploadMode === "idle" ? Theme.textSecondary : root.successColor
                    Layout.fillWidth: true
                    horizontalAlignment: Text.AlignRight
                }
            }

            Rectangle {
                Layout.fillWidth: true
                height: 1
                color: Theme.inputBorder
                Layout.topMargin: 4
                Layout.bottomMargin: 4
            }

            Text {
                text: "参数设置"
                color: Theme.textPrimary
                font.bold: true
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: Theme.spacingNormal

                Text {
                    text: "频率设置:"
                    color: Theme.textPrimary
                    Layout.preferredWidth: root.labelWidth
                }
                StyledInput {
                    id: frequencyField
                    Layout.preferredWidth: root.fieldWidth
                    placeholderText: "频率(Hz)"
                }
                StyledButton { text: "读取"; onClicked: DeviceConsole.invokeCurrent("eddy55aa.read.frequency") }
                StyledButton {
                    text: "写入"
                    onClicked: DeviceConsole.invokeCurrent("eddy55aa.write.frequency", {
                        frequency: root.parseIntValue(frequencyField.text, 0)
                    })
                }
                Text {
                    text: "当前: " + root.formatNumber(state.frequency, 0, " Hz", "-")
                    color: Theme.textSecondary
                    Layout.fillWidth: true
                    elide: Text.ElideRight
                }
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: Theme.spacingNormal

                Text {
                    text: "量程设置:"
                    color: Theme.textPrimary
                    Layout.preferredWidth: root.labelWidth
                }
                StyledInput {
                    id: rangeMinField
                    Layout.preferredWidth: root.pairFieldWidth
                    placeholderText: "最小值"
                }
                StyledInput {
                    id: rangeMaxField
                    Layout.preferredWidth: root.pairFieldWidth
                    placeholderText: "最大值"
                }
                StyledButton { text: "读取"; onClicked: DeviceConsole.invokeCurrent("eddy55aa.read.range") }
                StyledButton {
                    text: "写入"
                    onClicked: DeviceConsole.invokeCurrent("eddy55aa.write.range", {
                        values: root.parseFloatPair(rangeMinField.text, rangeMaxField.text)
                    })
                }
                Text {
                    text: "当前: " + root.formatPair(state.rangeValues, 3, "")
                    color: Theme.textSecondary
                    Layout.fillWidth: true
                    elide: Text.ElideRight
                }
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: Theme.spacingNormal

                Text {
                    text: "设备序列号:"
                    color: Theme.textPrimary
                    Layout.preferredWidth: root.labelWidth
                }
                StyledInput {
                    id: serialField
                    Layout.preferredWidth: root.fieldWidth
                    placeholderText: "序列号"
                }
                StyledButton { text: "读取"; onClicked: DeviceConsole.invokeCurrent("eddy55aa.read.serial") }
                StyledButton {
                    text: "写入"
                    onClicked: DeviceConsole.invokeCurrent("eddy55aa.write.serial", {
                        serialNumber: root.parseIntValue(serialField.text, 0)
                    })
                }
                Text {
                    text: "当前: " + (state.serialNumber || "-")
                    color: Theme.textSecondary
                    Layout.fillWidth: true
                    elide: Text.ElideRight
                }
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: Theme.spacingNormal

                Text {
                    text: "协议选择:"
                    color: Theme.textPrimary
                    Layout.preferredWidth: root.labelWidth
                }
                StyledComboBox {
                    id: protocolCombo
                    Layout.preferredWidth: root.fieldWidth
                    model: [
                        { text: "标准协议", value: 0 },
                        { text: "RKX 协议", value: 1 }
                    ]
                    textRole: "text"
                }
                StyledButton { text: "读取"; onClicked: DeviceConsole.invokeCurrent("eddy55aa.read.protocol") }
                StyledButton {
                    text: "写入"
                    onClicked: DeviceConsole.invokeCurrent("eddy55aa.write.protocol", {
                        protocolType: root.selectedProtocolType()
                    })
                }
                Text {
                    text: "当前: " + root.protocolLabelFromState()
                    color: Theme.textSecondary
                    Layout.fillWidth: true
                    elide: Text.ElideRight
                }
            }

            Rectangle {
                Layout.fillWidth: true
                height: 1
                color: Theme.inputBorder
                Layout.topMargin: 4
                Layout.bottomMargin: 4
            }

            Text {
                text: "通道校准"
                color: Theme.textPrimary
                font.bold: true
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: Theme.spacingNormal

                Text {
                    text: "增益系数:"
                    color: Theme.textPrimary
                    Layout.preferredWidth: root.labelWidth
                }
                StyledInput {
                    id: gain1Field
                    Layout.preferredWidth: root.pairFieldWidth
                    placeholderText: "CH1"
                }
                StyledInput {
                    id: gain2Field
                    Layout.preferredWidth: root.pairFieldWidth
                    placeholderText: "CH2"
                }
                StyledButton { text: "读取"; onClicked: DeviceConsole.invokeCurrent("eddy55aa.read.gain") }
                StyledButton {
                    text: "写入"
                    onClicked: DeviceConsole.invokeCurrent("eddy55aa.write.gain", {
                        values: root.parseFloatPair(gain1Field.text, gain2Field.text)
                    })
                }
                Text {
                    text: "当前: " + root.formatPair(state.gainValues, 0, "")
                    color: Theme.textSecondary
                    Layout.fillWidth: true
                    elide: Text.ElideRight
                }
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: Theme.spacingNormal

                Text {
                    text: "零位偏移:"
                    color: Theme.textPrimary
                    Layout.preferredWidth: root.labelWidth
                }
                StyledInput {
                    id: zero1Field
                    Layout.preferredWidth: root.pairFieldWidth
                    placeholderText: "CH1"
                }
                StyledInput {
                    id: zero2Field
                    Layout.preferredWidth: root.pairFieldWidth
                    placeholderText: "CH2"
                }
                StyledButton { text: "读取"; onClicked: DeviceConsole.invokeCurrent("eddy55aa.read.zero") }
                StyledButton {
                    text: "写入"
                    onClicked: DeviceConsole.invokeCurrent("eddy55aa.write.zero", {
                        values: root.parseFloatPair(zero1Field.text, zero2Field.text)
                    })
                }
                Text {
                    text: "当前: " + root.formatPair(state.zeroValues, 0, "")
                    color: Theme.textSecondary
                    Layout.fillWidth: true
                    elide: Text.ElideRight
                }
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: Theme.spacingNormal

                Text {
                    text: "相位参数:"
                    color: Theme.textPrimary
                    Layout.preferredWidth: root.labelWidth
                }
                StyledInput {
                    id: phase1Field
                    Layout.preferredWidth: root.pairFieldWidth
                    placeholderText: "CH1"
                }
                StyledInput {
                    id: phase2Field
                    Layout.preferredWidth: root.pairFieldWidth
                    placeholderText: "CH2"
                }
                StyledButton { text: "读取"; onClicked: DeviceConsole.invokeCurrent("eddy55aa.read.phase") }
                StyledButton {
                    text: "写入"
                    onClicked: DeviceConsole.invokeCurrent("eddy55aa.write.phase", {
                        values: root.parseIntPair(phase1Field.text, phase2Field.text)
                    })
                }
                Text {
                    text: "当前: " + root.formatPair(state.phaseValues, 0, "")
                    color: Theme.textSecondary
                    Layout.fillWidth: true
                    elide: Text.ElideRight
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            height: 1
            color: Theme.inputBorder
            Layout.topMargin: 8
            Layout.bottomMargin: 8
        }

        Common.CommandHistoryPanel {
            deviceId: root.isEddyCurrentDevice ? DeviceConsole.selectedDeviceId : ""
            Layout.fillWidth: true
            Layout.preferredHeight: 300
        }
    }
}
