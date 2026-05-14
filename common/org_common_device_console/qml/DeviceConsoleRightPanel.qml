import QtQuick 2.14
import QtQuick.Controls 2.14
import QtQuick.Layouts 1.14
import UITheme 1.0
import DeviceConsole 1.0

Item {
    id: root
    anchors.fill: parent

    property string currentDeviceId: String(DeviceConsole.selectedDeviceId || "")
    property string pendingDeviceId: ""
    readonly property bool switchingDevice: pendingDeviceId !== "" && pendingDeviceId !== currentDeviceId

    function isDeviceSelected(deviceId) {
        var targetId = String(deviceId || "")
        if (targetId === "") return false
        return targetId === currentDeviceId || targetId === pendingDeviceId
    }

    function deviceKeyOf(device) {
        return [
            String((device && device.deviceType) || ""),
            String((device && device.modelId) || ""),
            String((device && device.id) || ""),
            String((device && device.title) || "")
        ].join(" ").toLowerCase()
    }

    function deviceTitleOf(device) {
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

    function connectionText(device) {
        return device && device.connected ? "\u5728\u7ebf" : "\u79bb\u7ebf"
    }

    function connectionColor(device) {
        return device && device.connected ? Theme.success : Theme.textSecondary
    }

    function selectDevice(deviceId) {
        var targetId = String(deviceId || "")
        if (targetId === "") return
        pendingDeviceId = targetId
        if (currentDeviceId === targetId) {
            pendingDeviceId = ""
            return
        }
        DeviceConsole.selectedDeviceId = targetId
    }

    onCurrentDeviceIdChanged: {
        if (pendingDeviceId !== "" && pendingDeviceId === currentDeviceId) {
            pendingDeviceId = ""
        }
    }

    Rectangle {
        anchors.fill: parent
        color: Qt.rgba(7 / 255, 11 / 255, 20 / 255, 0.65)
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: Theme.spacingNormal
        spacing: 0

        ScrollView {
            id: panelScrollView
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true

            Column {
                width: panelScrollView.availableWidth
                spacing: Theme.spacingSmall

                Rectangle {
                    width: parent.width
                    visible: DeviceConsole.panelDevices.length === 0
                    color: "transparent"
                    implicitHeight: emptyColumn.implicitHeight + Theme.spacingLarge

                    Column {
                        id: emptyColumn
                        anchors.centerIn: parent
                        spacing: Theme.spacingSmall

                        Text {
                            text: "\u6682\u65e0\u53ef\u64cd\u4f5c\u8bbe\u5907"
                            color: Theme.textPrimary
                            font.pixelSize: Theme.fontSizeLarge
                            horizontalAlignment: Text.AlignHCenter
                        }

                        Text {
                            text: "\u8bbe\u5907\u63a5\u5165\u540e\u4f1a\u5728\u8fd9\u91cc\u4ee5\u6807\u7b7e\u5217\u8868\u7684\u5f62\u5f0f\u663e\u793a\u3002"
                            color: Theme.textSecondary
                            font.pixelSize: 12
                            horizontalAlignment: Text.AlignHCenter
                        }
                    }
                }

                Repeater {
                    model: DeviceConsole.panelDevices

                    delegate: Rectangle {
                        property var rowData: modelData || ({})
                        property string deviceId: String(rowData.id || "")
                        property bool selected: root.isDeviceSelected(deviceId)

                        width: parent.width
                        implicitHeight: 40
                        radius: Theme.radiusNormal
                        scale: selected ? 1.0 : 0.992
                        color: selected
                               ? Qt.rgba(47 / 255, 127 / 255, 182 / 255, 0.28)
                               : Theme.inputBg
                        border.color: selected ? Theme.accent : Theme.inputBorder
                        border.width: 1

                        Behavior on color {
                            ColorAnimation { duration: 130 }
                        }

                        Behavior on border.color {
                            ColorAnimation { duration: 130 }
                        }

                        Behavior on scale {
                            NumberAnimation { duration: 130; easing.type: Easing.OutCubic }
                        }

                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: Theme.spacingNormal
                            anchors.rightMargin: Theme.spacingNormal
                            spacing: Theme.spacingNormal

                            Rectangle {
                                width: 10
                                height: 10
                                radius: 5
                                color: rowData.connected ? Theme.success : Theme.danger
                            }

                            Text {
                                Layout.fillWidth: true
                                text: root.deviceTitleOf(rowData)
                                color: Theme.textPrimary
                                font.pixelSize: Theme.fontSizeNormal
                                font.bold: selected
                                elide: Text.ElideRight
                            }

                            Text {
                                text: root.connectionText(rowData)
                                color: root.connectionColor(rowData)
                                font.pixelSize: 11
                            }

                            BusyIndicator {
                                width: 14
                                height: 14
                                running: root.switchingDevice && root.pendingDeviceId === deviceId
                                visible: running
                            }
                        }

                        MouseArea {
                            anchors.fill: parent
                            onClicked: root.selectDevice(deviceId)
                        }
                    }
                }
            }
        }
    }
}
