import QtQuick 2.14
import QtQuick.Controls 2.14
import QtQuick.Layouts 1.14
import UITheme 1.0
import DeviceConsole 1.0

Rectangle {
    id: root
    color: Theme.cardBg
    clip: true

    property string currentDeviceId: String(DeviceConsole.selectedDeviceId || "")
    property var currentDevice: normalizeMap(DeviceConsole.selectedDevice)
    property var serialPortRows: []
    property string selectedSerialPortName: ""
    readonly property int deviceListPaneWidth: 180

    function normalizeMap(value) {
        return value || ({})
    }

    function transportsOf(device) {
        var value = device && device.transports ? device.transports : []
        return value || []
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

        var targetIndex = serialPortIndexByName(keepName)
        if (targetIndex < 0) {
            targetIndex = 0
        }
        selectedSerialPortName = serialPortNameAt(targetIndex)
    }

    onCurrentDeviceIdChanged: refreshSerialPorts()

    Component.onCompleted: refreshSerialPorts()

    RowLayout {
        anchors.fill: parent
        anchors.margins: Theme.spacingNormal
        spacing: Theme.spacingNormal

        Rectangle {
            Layout.preferredWidth: root.deviceListPaneWidth
            Layout.minimumWidth: root.deviceListPaneWidth
            Layout.maximumWidth: root.deviceListPaneWidth
            Layout.fillHeight: true
            radius: Theme.radiusNormal
            color: "#16324a"

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: Theme.spacingSmall
                spacing: Theme.spacingSmall

                Text {
                    text: "设备列表"
                    color: Theme.textPrimary
                    font.pixelSize: Theme.fontSizeLarge
                }

                Text {
                    text: DeviceConsole.devices.length > 0
                          ? "已发现 " + DeviceConsole.devices.length + " 个设备适配器"
                          : "当前未发现设备适配器"
                    color: Theme.textSecondary
                    wrapMode: Text.Wrap
                }

                ListView {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true
                    model: DeviceConsole.devices
                    spacing: Theme.spacingSmall

                    delegate: Rectangle {
                        id: deviceDelegate
                        property var rowData: modelData || ({})
                        property string deviceId: String(rowData.id || "")
                        property string deviceTitle: String(rowData.title || rowData.id || "")
                        property bool deviceConnected: !!rowData.connected

                        width: ListView.view.width
                        height: 72
                        radius: Theme.radiusNormal
                        color: deviceId === root.currentDeviceId ? "#245377" : "#10263a"
                        border.width: 1
                        border.color: deviceId === root.currentDeviceId ? "#65b6ff" : "#214059"

                        MouseArea {
                            anchors.fill: parent
                            onClicked: DeviceConsole.selectedDeviceId = deviceDelegate.deviceId
                        }

                        Column {
                            anchors.fill: parent
                            anchors.margins: Theme.spacingSmall
                            spacing: 4

                            Text {
                                text: deviceDelegate.deviceTitle
                                color: Theme.textPrimary
                                font.bold: true
                            }

                            Text {
                                text: "ID: " + deviceDelegate.deviceId
                                color: Theme.textSecondary
                            }

                            Text {
                                text: deviceDelegate.deviceConnected ? "状态: 已连接" : "状态: 未连接"
                                color: deviceDelegate.deviceConnected ? "#82D4A7" : Theme.textSecondary
                            }
                        }
                    }
                }
            }
        }

        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: Theme.spacingNormal

            Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.preferredHeight: 520
                Layout.minimumHeight: 300
                radius: Theme.radiusNormal
                color: "#11283b"

                ScrollView {
                    anchors.fill: parent
                    anchors.margins: Theme.spacingNormal
                    clip: true
                    contentWidth: width
                    ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

                    ColumnLayout {
                        width: parent.width
                        spacing: Theme.spacingSmall

                    RowLayout {
                        Layout.fillWidth: true

                        Text {
                            text: root.currentDevice.title
                                  ? String(root.currentDevice.title)
                                  : "设备控制台"
                            color: Theme.textPrimary
                            font.pixelSize: Theme.fontSizeLarge
                            font.bold: true
                        }

                        Item { Layout.fillWidth: true }

                        Button {
                            text: "刷新状态"
                            onClicked: DeviceConsole.refreshNow()
                        }

                        Button {
                            text: "断开连接"
                            enabled: root.currentDeviceId !== ""
                            onClicked: DeviceConsole.disconnectCurrent()
                        }
                    }

                    Text {
                        Layout.fillWidth: true
                        text: root.currentDeviceId === ""
                              ? "当前没有可操作的设备。提示：core 宿主会扫描当前仓库以及同级 osgi_business_dev/plugins_business。"
                              : "模型: " + String(root.currentDevice.modelId || "")
                        color: Theme.textSecondary
                        wrapMode: Text.Wrap
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: Theme.spacingNormal

                        Rectangle {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            radius: Theme.radiusNormal
                            color: "#16324a"
                            visible: root.hasTransport(root.currentDevice, "serial")

                            ColumnLayout {
                                anchors.fill: parent
                                anchors.margins: Theme.spacingSmall
                                spacing: Theme.spacingSmall

                                Text {
                                    text: "串口连接"
                                    color: Theme.textPrimary
                                    font.bold: true
                                }

                                RowLayout {
                                    Layout.fillWidth: true

                                    ComboBox {
                                        id: portNameCombo
                                        Layout.fillWidth: true
                                        model: root.serialPortRows
                                        textRole: "displayText"
                                        currentIndex: root.serialPortIndexByName(root.selectedSerialPortName)
                                        onActivated: root.selectedSerialPortName = root.serialPortNameAt(currentIndex)
                                    }

                                    Button {
                                        text: "刷新"
                                        onClicked: root.refreshSerialPorts()
                                    }

                                    TextField {
                                        id: baudRateField
                                        Layout.preferredWidth: 120
                                        text: "115200"
                                        placeholderText: "波特率"
                                    }

                                    Button {
                                        text: "连接"
                                        enabled: root.currentDeviceId !== "" && root.selectedSerialPortName.length > 0
                                        onClicked: DeviceConsole.connectSerial(
                                                       root.selectedSerialPortName,
                                                       parseInt(baudRateField.text || "115200"))
                                    }
                                }
                            }
                        }

                        Rectangle {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            radius: Theme.radiusNormal
                            color: "#16324a"
                            visible: root.hasTransport(root.currentDevice, "udp")

                            ColumnLayout {
                                anchors.fill: parent
                                anchors.margins: Theme.spacingSmall
                                spacing: Theme.spacingSmall

                                Text {
                                    text: "UDP 连接"
                                    color: Theme.textPrimary
                                    font.bold: true
                                }

                                RowLayout {
                                    Layout.fillWidth: true

                                    TextField {
                                        id: remoteAddressField
                                        Layout.fillWidth: true
                                        placeholderText: "远端地址"
                                    }

                                    TextField {
                                        id: remotePortField
                                        Layout.preferredWidth: 90
                                        placeholderText: "远端端口"
                                    }
                                }

                                RowLayout {
                                    Layout.fillWidth: true

                                    TextField {
                                        id: localAddressField
                                        Layout.fillWidth: true
                                        placeholderText: "本地地址，可选"
                                    }

                                    TextField {
                                        id: localPortField
                                        Layout.preferredWidth: 90
                                        placeholderText: "本地端口"
                                    }

                                    Button {
                                        text: "连接"
                                        enabled: root.currentDeviceId !== ""
                                        onClicked: DeviceConsole.connectUdp(
                                                       remoteAddressField.text,
                                                       parseInt(remotePortField.text || "0"),
                                                       localAddressField.text,
                                                       parseInt(localPortField.text || "0"),
                                                       1)
                                    }
                                }
                            }
                        }
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        radius: Theme.radiusNormal
                        color: "#16324a"

                        ColumnLayout {
                            anchors.fill: parent
                            anchors.margins: Theme.spacingSmall
                            spacing: Theme.spacingSmall

                            Text {
                                text: "原始数据发送"
                                color: Theme.textPrimary
                                font.bold: true
                            }

                            RowLayout {
                                Layout.fillWidth: true
                                spacing: Theme.spacingSmall

                                TextField {
                                    id: rawHexField
                                    Layout.fillWidth: true
                                    placeholderText: "发送原始十六进制，例如 7E F5 04 77"
                                }

                                Button {
                                    text: "发送"
                                    enabled: root.currentDeviceId !== ""
                                    onClicked: DeviceConsole.sendRawHex(rawHexField.text)
                                }
                            }
                        }
                    }



                    Text {
                        Layout.fillWidth: true
                        text: "最近动作: " + String(DeviceConsole.lastActionResult || "无")
                        color: Theme.textSecondary
                        wrapMode: Text.Wrap
                    }
                }
                }
            }

        }
    }
}
