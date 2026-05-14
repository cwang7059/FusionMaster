import QtQuick 2.14
import QtQuick.Controls 2.14
import QtQuick.Layouts 1.14
import UITheme 1.0
import DeviceConsole 1.0

Rectangle {
    id: root
    color: Theme.windowBg
    clip: true

    property string currentDeviceId: String(DeviceConsole.selectedDeviceId || "")
    property var currentDevice: normalizeMap(DeviceConsole.selectedDevice)
    property var currentDevicePanels: DeviceConsole.selectedDevicePanels || []
    property var serialPortRows: []
    property string selectedSerialPortName: ""
    readonly property color sectionBackground: Theme.inputBg
    readonly property color sectionBorder: Theme.inputBorder
    readonly property color summaryBackground: Qt.rgba(47 / 255, 127 / 255, 182 / 255, 0.14)
    readonly property bool hasDedicatedDevicePage: currentDeviceId !== ""
                                                && currentDevicePanels.length > 0
                                                && !!DeviceConsole.selectedDevicePage
                                                && DeviceConsole.selectedDevicePage !== ""
    // Keep all device pages embedded in the shared detail shell so eddy current
    // follows the same title/transport/panel layout as encoder and fast mirror.
    readonly property bool useDedicatedDevicePage: false
    property bool switchingVisual: false
    property bool waitingDedicatedPageReady: false

    function normalizeMap(value) {
        return value || ({})
    }

    function transportsOf(device) {
        var value = device && device.transports ? device.transports : []
        return value || []
    }

    function actionsOf(device) {
        var value = device && device.actions ? device.actions : []
        return value || []
    }

    function localizedCommonText(value, fallbackText) {
        if (value === undefined || value === null || value === "") {
            return fallbackText || "-"
        }

        if (value === true) {
            return "\u662f"
        }

        if (value === false) {
            return "\u5426"
        }

        var text = String(value)
        var lower = text.toLowerCase()

        if (lower === "unknown") {
            return "\u672a\u77e5"
        }
        if (lower === "idle") {
            return "\u7a7a\u95f2"
        }
        if (lower === "none") {
            return "\u65e0"
        }
        if (lower === "connected") {
            return "\u5df2\u8fde\u63a5"
        }
        if (lower === "disconnected") {
            return "\u672a\u8fde\u63a5"
        }
        if (lower === "true") {
            return "\u662f"
        }
        if (lower === "false") {
            return "\u5426"
        }
        if (lower === "serial") {
            return "\u4e32\u53e3"
        }
        if (lower === "udp") {
            return "UDP"
        }
        if (lower === "tcp") {
            return "TCP"
        }
        if (lower === "can") {
            return "CAN"
        }

        return text
    }

    function hasTransport(device, transportName) {
        var values = transportsOf(device)
        for (var i = 0; i < values.length; ++i) {
            if (String(values[i]) === transportName) {
                return true
            }
        }
        return false
    }

    function serialDefaultBaudRateOf(device) {
        var caps = device && device.capabilities ? device.capabilities : {}
        var serial = caps && caps.serial ? caps.serial : {}
        var value = serial ? serial.defaultBaudRate : 0
        var parsed = parseInt(String(value || "0"), 10)
        return isNaN(parsed) || parsed <= 0 ? 115200 : parsed
    }

    function serialPortNameAt(index) {
        if (!serialPortRows || index < 0 || index >= serialPortRows.length) {
            return ""
        }
        var row = serialPortRows[index] || ({})
        return String(row.portName || "")
    }

    function serialPortIndexByName(portName) {
        var name = String(portName || "").trim()
        if (!name.length || !serialPortRows) return -1
        for (var i = 0; i < serialPortRows.length; ++i) {
            if (String((serialPortRows[i] || {}).portName || "") === name) {
                return i
            }
        }
        return -1
    }

    function refreshSerialPorts() {
        var keepName = selectedSerialPortName
        var rows = []
        if (typeof DeviceConsole !== "undefined" && DeviceConsole && DeviceConsole.serialPorts) {
            var fetched = DeviceConsole.serialPorts()
            rows = fetched && fetched.length ? fetched : []
        }
        serialPortRows = rows

        if (!serialPortRows.length) {
            selectedSerialPortName = ""
            return
        }

        var idx = serialPortIndexByName(keepName)
        if (idx < 0) idx = 0
        selectedSerialPortName = serialPortNameAt(idx)
    }

    function localizedTransportName(name) {
        var text = String(name || "")
        var lower = text.toLowerCase()

        if (lower === "serial") {
            return "\u4e32\u53e3"
        }
        if (lower === "udp") {
            return "UDP"
        }
        if (lower === "tcp") {
            return "TCP"
        }
        if (lower === "can") {
            return "CAN"
        }
        if (lower === "none" || lower === "") {
            return "\u672a\u914d\u7f6e"
        }

        return text
    }

    function localizedTransportSummary(device) {
        var values = transportsOf(device)
        if (values.length === 0) {
            return "\u672a\u914d\u7f6e"
        }

        var result = []
        for (var i = 0; i < values.length; ++i) {
            result.push(localizedTransportName(values[i]))
        }

        return result.join(" / ")
    }

    function connectionText(device) {
        return device && device.connected ? "\u5728\u7ebf" : "\u79bb\u7ebf"
    }

    function connectionColor(device) {
        return device && device.connected ? "#82D4A7" : Theme.textSecondary
    }

    function deviceKeyOf(device) {
        return [
            String((device && device.deviceType) || ""),
            String((device && device.modelId) || ""),
            String((device && device.id) || ""),
            String((device && device.title) || "")
        ].join(" ").toLowerCase()
    }

    function localizedDeviceName(device) {
        var titleValue = String((device && device.title) || "")
        var typeValue = deviceKeyOf(device)

        if (/[\u4E00-\u9FFF]/.test(titleValue)) {
            return titleValue
        }

        if (typeValue.indexOf("turntable") >= 0 || typeValue.indexOf("\u8f6c\u53f0") >= 0) {
            return "\u8f6c\u53f0"
        }
        if (typeValue.indexOf("encoder") >= 0
                || typeValue.indexOf("55aa") >= 0
                || typeValue.indexOf("\u7f16\u7801\u5668") >= 0) {
            return "\u7f16\u7801\u5668"
        }
        if (typeValue.indexOf("eddy_current") >= 0
                || typeValue.indexOf("eddy current") >= 0
                || typeValue.indexOf("\u6da1\u6d41") >= 0) {
            return "\u7535\u6da1\u6d41"
        }
        if (typeValue.indexOf("fast_mirror") >= 0
                || typeValue.indexOf("fast mirror") >= 0
                || typeValue.indexOf("mirror") >= 0
                || typeValue.indexOf("\u5feb\u53cd\u955c") >= 0) {
            return "\u5feb\u53cd\u955c"
        }
        if (typeValue.indexOf("receiver") >= 0
                || typeValue.indexOf("rcv") >= 0
                || typeValue.indexOf("\u63a5\u6536\u673a") >= 0) {
            return "\u63a5\u6536\u673a"
        }
        return "\u8bbe\u5907"
    }

    function currentDeviceTitle() {
        if (root.currentDeviceId === "") {
            return "\u8bbe\u5907\u8be6\u60c5"
        }

        var localized = localizedDeviceName(root.currentDevice)
        return localized === "\u8bbe\u5907" ? root.currentDeviceId : localized
    }

    function devicePanelTitle(panel) {
        if (!panel) {
            return "\u672A\u547D\u540D\u9762\u677F"
        }

        var title = String(panel.title || "")
        if (title !== "") {
            return title
        }

        var panelId = String(panel.id || "")
        return panelId !== "" ? panelId : "\u672A\u547D\u540D\u9762\u677F"
    }

    function localizedSnapshotKey(key) {
        var text = String(key || "")
        var labels = {
            "status.connected": "\u8fde\u63a5\u72b6\u6001",
            "status.linkType": "\u94fe\u8def\u7c7b\u578b",
            "status.modeText": "\u8fd0\u884c\u6a21\u5f0f",
            "status.modeCode": "\u6a21\u5f0f\u4ee3\u7801",
            "status.imageText": "\u56fe\u50cf\u72b6\u6001",
            "status.selfCheckCode": "\u81ea\u68c0\u7801",
            "status.lastChecksumOk": "\u6821\u9a8c\u72b6\u6001",
            "status.faultBitsHex": "\u6545\u969c\u4f4d",
            "status.selfCheckFault": "\u81ea\u68c0\u6545\u969c",
            "status.inputVoltageFault": "\u7535\u538b\u6545\u969c",
            "status.driveOutputFault": "\u8f93\u51fa\u6545\u969c",
            "status.commTimeout": "\u901a\u8baf\u8d85\u65f6",
            "status.checksumFault": "\u6821\u9a8c\u6545\u969c",
            "status.motorDisabled": "\u7535\u673a\u72b6\u6001",
            "status.locked": "\u9501\u5b9a\u72b6\u6001",
            "runtime.azimuthAngle": "\u65b9\u4f4d\u89d2",
            "runtime.elevationAngle": "\u4fef\u4ef0\u89d2",
            "runtime.azimuthRate": "\u65b9\u4f4d\u89d2\u901f\u5ea6",
            "runtime.elevationRate": "\u4fef\u4ef0\u89d2\u901f\u5ea6",
            "runtime.rollRate": "\u6eda\u8f6c\u89d2\u901f\u5ea6",
            "runtime.currentXAngle": "X \u8f74\u5f53\u524d\u89d2\u5ea6",
            "runtime.currentYAngle": "Y \u8f74\u5f53\u524d\u89d2\u5ea6",
            "command.guideX": "X \u8f74\u5f15\u5bfc\u503c",
            "command.guideY": "Y \u8f74\u5f15\u5bfc\u503c",
            "command.lastModeText": "\u6700\u8fd1\u4e0b\u53d1\u6a21\u5f0f",
            "encoder.protocolText": "\u7f16\u7801\u5668\u534f\u8bae",
            "encoder.serialNumber": "\u7f16\u7801\u5668\u5e8f\u5217\u53f7",
            "encoder.versionText": "\u7f16\u7801\u5668\u7248\u672c",
            "encoder.streaming.mode": "\u6d41\u6a21\u5f0f",
            "encoder.resolution": "\u5206\u8fa8\u7387",
            "encoder.outer.angle": "\u5916\u5708\u89d2\u5ea6",
            "encoder.inner.angle": "\u5185\u5708\u89d2\u5ea6",
            "encoder.outer.sinAd": "\u5916\u5708\u6b63\u5f26 AD",
            "encoder.outer.cosAd": "\u5916\u5708\u4f59\u5f26 AD",
            "encoder.inner.sinAd": "\u5185\u5708\u6b63\u5f26 AD",
            "encoder.inner.cosAd": "\u5185\u5708\u4f59\u5f26 AD",
            "transport.lastTxFrameHex": "\u6700\u8fd1\u53d1\u9001\u5e27",
            "transport.lastRxFrameHex": "\u6700\u8fd1\u63a5\u6536\u5e27",
            "transport.lastError": "\u6700\u8fd1\u9519\u8bef"
        }

        return labels[text] || text
    }

    function snapshotEntries() {
        return DeviceConsole.selectedSnapshotEntries || []
    }

    function beginSwitchVisual() {
        if (root.currentDeviceId === "") {
            root.switchingVisual = false
            root.waitingDedicatedPageReady = false
            return
        }
        root.switchingVisual = true
        root.waitingDedicatedPageReady = root.hasDedicatedDevicePage
        switchVisualMinTimer.restart()
    }

    function maybeEndSwitchVisual() {
        if (switchVisualMinTimer.running) {
            return
        }
        if (!root.waitingDedicatedPageReady) {
            root.switchingVisual = false
        }
    }

    onCurrentDeviceIdChanged: {
        refreshSerialPorts()
        beginSwitchVisual()
    }
    onHasDedicatedDevicePageChanged: {
        root.waitingDedicatedPageReady = root.hasDedicatedDevicePage && root.currentDeviceId !== ""
        if (!root.waitingDedicatedPageReady) {
            maybeEndSwitchVisual()
        }
    }
    Component.onCompleted: refreshSerialPorts()

    Timer {
        id: switchVisualMinTimer
        interval: 260
        repeat: false
        onTriggered: root.maybeEndSwitchVisual()
    }

    ScrollView {
        id: detailScrollView
        anchors.fill: parent
        clip: true
        visible: !root.useDedicatedDevicePage

        Column {
            width: detailScrollView.availableWidth
            spacing: Theme.spacingNormal

            Rectangle {
                width: parent.width
                visible: root.currentDeviceId === ""
                color: "transparent"
                implicitHeight: emptyColumn.implicitHeight + Theme.spacingLarge * 2

                Column {
                    id: emptyColumn
                    anchors.centerIn: parent
                    spacing: Theme.spacingSmall

                    Text {
                        text: "\u8bbe\u5907\u8be6\u60c5\u89c6\u56fe"
                        color: Theme.textPrimary
                        font.pixelSize: Theme.fontSizeLarge
                        horizontalAlignment: Text.AlignHCenter
                    }

                    Text {
                        width: 360
                        text: "\u5728\u5de6\u4fa7\u9009\u62e9\u8bbe\u5907\u540e\uff0c\u8fd9\u91cc\u4f1a\u663e\u793a\u5f53\u524d\u8bbe\u5907\u7684\u8be6\u60c5\u5185\u5bb9\u3002"
                        color: Theme.textSecondary
                        font.pixelSize: 12
                        horizontalAlignment: Text.AlignHCenter
                        wrapMode: Text.Wrap
                    }
                }
            }

            Item {
                width: parent.width
                visible: root.currentDeviceId !== ""
                implicitHeight: detailColumn.implicitHeight

                Column {
                    id: detailColumn
                    width: parent.width
                    spacing: Theme.spacingNormal

                    Rectangle {
                        width: parent.width
                        color: root.summaryBackground
                        border.color: Theme.accent
                        border.width: 1
                        implicitHeight: summaryColumn.implicitHeight + Theme.spacingNormal * 2

                        ColumnLayout {
                            id: summaryColumn
                            anchors.fill: parent
                            anchors.margins: Theme.spacingNormal
                            spacing: Theme.spacingSmall

                            RowLayout {
                                Layout.fillWidth: true

                                Text {
                                    Layout.fillWidth: true
                                    text: root.currentDeviceTitle()
                                    color: Theme.textPrimary
                                    font.pixelSize: Theme.fontSizeLarge
                                    font.bold: true
                                    elide: Text.ElideRight
                                }

                                Text {
                                    text: root.connectionText(root.currentDevice)
                                    color: root.connectionColor(root.currentDevice)
                                    font.pixelSize: 12
                                }

                                StyledButton {
                                    text: "\u5237\u65b0\u72b6\u6001"
                                    onClicked: DeviceConsole.refreshNow()
                                }

                                StyledButton {
                                    text: "\u65ad\u5f00\u8fde\u63a5"
                                    enabled: root.currentDeviceId !== ""
                                    onClicked: DeviceConsole.disconnectCurrent()
                                }
                            }


                        }
                    }

                    Rectangle {
                        width: parent.width
                        visible: root.hasTransport(root.currentDevice, "serial")
                        color: root.sectionBackground
                        border.color: root.sectionBorder
                        border.width: 1
                        implicitHeight: serialColumn.implicitHeight + Theme.spacingNormal * 2

                        ColumnLayout {
                            id: serialColumn
                            anchors.fill: parent
                            anchors.margins: Theme.spacingNormal
                            spacing: Theme.spacingSmall

                            Text {
                                text: "\u4e32\u53e3\u8fde\u63a5"
                                color: Theme.textPrimary
                                font.pixelSize: Theme.fontSizeNormal
                            }

                            RowLayout {
                                Layout.fillWidth: true

                                StyledComboBox {
                                    id: portNameCombo
                                    Layout.fillWidth: true
                                    model: root.serialPortRows
                                    textRole: "displayText"
                                    currentIndex: root.serialPortIndexByName(root.selectedSerialPortName)
                                    onActivated: root.selectedSerialPortName = root.serialPortNameAt(currentIndex)
                                }

                                StyledButton {
                                    text: "\u5237\u65b0"
                                    onClicked: root.refreshSerialPorts()
                                }
                            }

                            RowLayout {
                                Layout.fillWidth: true

                                StyledInput {
                                    id: baudRateField
                                    Layout.fillWidth: true
                                    text: String(root.serialDefaultBaudRateOf(root.currentDevice))
                                    placeholderText: "\u6ce2\u7279\u7387"
                                }

                                StyledButton {
                                    text: "\u8fde\u63a5"
                                    enabled: root.currentDeviceId !== "" && root.selectedSerialPortName.length > 0
                                    onClicked: DeviceConsole.connectSerial(
                                                   root.selectedSerialPortName,
                                                   parseInt(baudRateField.text || String(root.serialDefaultBaudRateOf(root.currentDevice))))
                                }
                            }
                        }
                    }

                    Rectangle {
                        width: parent.width
                        visible: root.hasTransport(root.currentDevice, "udp")
                        color: root.sectionBackground
                        border.color: root.sectionBorder
                        border.width: 1
                        implicitHeight: udpColumn.implicitHeight + Theme.spacingNormal * 2

                        ColumnLayout {
                            id: udpColumn
                            anchors.fill: parent
                            anchors.margins: Theme.spacingNormal
                            spacing: Theme.spacingSmall

                            Text {
                                text: "\u7f51\u7edc\u8fde\u63a5\uff08UDP\uff09"
                                color: Theme.textPrimary
                                font.pixelSize: Theme.fontSizeNormal
                            }

                            StyledInput {
                                id: remoteAddressField
                                Layout.fillWidth: true
                                placeholderText: "\u8fdc\u7aef\u5730\u5740"
                            }

                            RowLayout {
                                Layout.fillWidth: true

                                StyledInput {
                                    id: remotePortField
                                    Layout.fillWidth: true
                                    placeholderText: "\u8fdc\u7aef\u7aef\u53e3"
                                }

                                StyledInput {
                                    id: localPortField
                                    Layout.fillWidth: true
                                    placeholderText: "\u672c\u5730\u7aef\u53e3"
                                }
                            }

                            StyledButton {
                                text: "\u8fde\u63a5"
                                enabled: root.currentDeviceId !== ""
                                onClicked: DeviceConsole.connectUdp(
                                               remoteAddressField.text,
                                               parseInt(remotePortField.text || "0"),
                                               "",
                                               parseInt(localPortField.text || "0"),
                                               1)
                            }
                        }
                    }

                    Rectangle {
                        width: parent.width
                        visible: !root.useDedicatedDevicePage
                        color: root.sectionBackground
                        border.color: root.sectionBorder
                        border.width: 1
                        implicitHeight: devicePageColumn.implicitHeight + Theme.spacingNormal * 2

                        Column {
                            id: devicePageColumn
                            anchors.left: parent.left
                            anchors.right: parent.right
                            anchors.top: parent.top
                            anchors.margins: Theme.spacingNormal
                            spacing: Theme.spacingSmall

                            Text {
                                width: parent.width
                                text: "\u8bbe\u5907\u4e13\u7528\u9875\u9762"
                                color: Theme.textPrimary
                                font.pixelSize: Theme.fontSizeNormal
                                font.bold: true
                            }

                            Flow {
                                width: parent.width
                                visible: root.currentDevicePanels.length > 1
                                spacing: Theme.spacingSmall

                                Repeater {
                                    model: root.currentDevicePanels

                                    delegate: Rectangle {
                                        property var panelEntry: modelData || ({})
                                        property string panelId: String(panelEntry.id || "")
                                        readonly property bool selected: panelId === String(DeviceConsole.selectedDevicePanelId || "")
                                        implicitWidth: panelTitle.implicitWidth + 24
                                        implicitHeight: 30
                                        radius: 6
                                        color: selected ? Theme.accent : Theme.inputBg
                                        border.color: selected ? Theme.cardBorder : Theme.inputBorder
                                        border.width: 1

                                        Text {
                                            id: panelTitle
                                            anchors.centerIn: parent
                                            text: root.devicePanelTitle(panelEntry)
                                            color: selected ? Theme.accentText : Theme.textSecondary
                                            font.pixelSize: 12
                                        }

                                        MouseArea {
                                            anchors.fill: parent
                                            onClicked: DeviceConsole.selectedDevicePanelId = panelId
                                        }
                                    }
                                }
                            }

                            Loader {
                                id: embeddedDevicePageLoader
                                width: parent.width
                                height: item
                                        ? Math.max(
                                              0,
                                              item.implicitHeight > 0
                                                  ? item.implicitHeight
                                                  : item.childrenRect.height)
                                        : 0
                                active: !root.useDedicatedDevicePage
                                        && root.currentDeviceId !== ""
                                        && root.hasDedicatedDevicePage
                                asynchronous: true
                                source: active ? DeviceConsole.selectedDevicePage : ""
                                onStatusChanged: {
                                    if (status === Loader.Loading) {
                                        root.waitingDedicatedPageReady = true
                                        root.switchingVisual = true
                                        return
                                    }
                                    if (status === Loader.Ready
                                            || status === Loader.Error
                                            || !active) {
                                        root.waitingDedicatedPageReady = false
                                        root.maybeEndSwitchVisual()
                                    }
                                }
                            }

                            Text {
                                width: parent.width
                                visible: embeddedDevicePageLoader.status === Loader.Error
                                text: "\u8bbe\u5907\u4e13\u7528\u9875\u9762\u52a0\u8f7d\u5931\u8d25\uff0c\u8bf7\u68c0\u67e5 QML \u9875\u9762\u8d44\u6e90\u662f\u5426\u5b8c\u6574\u3002"
                                color: Theme.textSecondary
                                wrapMode: Text.Wrap
                            }

                            Text {
                                width: parent.width
                                visible: !DeviceConsole.selectedDevicePage || DeviceConsole.selectedDevicePage === ""
                                text: "\u5f53\u524d\u8bbe\u5907\u6682\u65e0\u4e13\u7528\u9875\uff0c\u53f3\u4fa7\u663e\u793a\u7684\u662f\u901a\u7528\u8bbe\u5907\u5185\u5bb9\u3002"
                                color: Theme.textSecondary
                                wrapMode: Text.Wrap
                            }
                        }
                    }
                }
            }
        }
    }

    Rectangle {
        id: switchMask
        anchors.fill: parent
        visible: root.switchingVisual && !root.useDedicatedDevicePage
        z: 1000
        color: Qt.rgba(7 / 255, 11 / 255, 20 / 255, 0.38)
        opacity: visible ? 1 : 0

        Behavior on opacity {
            NumberAnimation {
                duration: 150
                easing.type: Easing.OutCubic
            }
        }

        Rectangle {
            anchors.centerIn: parent
            radius: Theme.radiusLarge
            color: Qt.rgba(17 / 255, 24 / 255, 39 / 255, 0.92)
            border.color: Theme.cardBorder
            border.width: 1
            implicitWidth: switchRow.implicitWidth + 24
            implicitHeight: switchRow.implicitHeight + 16

            RowLayout {
                id: switchRow
                anchors.centerIn: parent
                spacing: Theme.spacingSmall

                BusyIndicator {
                    running: root.switchingVisual
                    visible: running
                    width: 18
                    height: 18
                }

                Text {
                    text: "正在切换设备..."
                    color: Theme.textPrimary
                    font.pixelSize: Theme.fontSizeNormal
                }
            }
        }
    }

    Loader {
        anchors.fill: parent
        anchors.margins: Theme.spacingNormal
        active: root.useDedicatedDevicePage
        visible: active
        source: active ? DeviceConsole.selectedDevicePage : ""
    }
}
