import QtQuick 2.14
import QtQuick.Controls 2.14
import QtQuick.Layouts 1.14
import UITheme 1.0

Item {
    id: root

    property var uiContributionsData: []
    property var topContributions: []
    property var leftContributions: []
    property var centerContributions: []
    property var rightContributions: []
    property var bottomContributions: []
    property string selectedContributionId: ""
    readonly property string deviceConsolePanelId: "org_common_device_console.right_panel"

    function containsContribution(items, contributionId) {
        var source = items || []
        if ((contributionId || "") === "") {
            return false
        }

        for (var i = 0; i < source.length; ++i) {
            if (((source[i] && source[i].id) || "") === contributionId) {
                return true
            }
        }

        return false
    }

    function syncSelection() {
        var items = root.rightContributions || []
        if (items.length === 0) {
            if (root.selectedContributionId !== "") {
                root.selectedContributionId = ""
            }
            return
        }

        if (containsContribution(items, root.deviceConsolePanelId)) {
            if (root.selectedContributionId !== root.deviceConsolePanelId) {
                root.selectedContributionId = root.deviceConsolePanelId
            }
            return
        }

        if (!containsContribution(items, root.selectedContributionId)) {
            root.selectedContributionId = (items[0] && items[0].id) || ""
        }
    }

    function currentDeviceContribution() {
        var items = root.rightContributions || []
        if (items.length === 0) {
            return null
        }

        if (containsContribution(items, root.selectedContributionId)) {
            for (var i = 0; i < items.length; ++i) {
                if (((items[i] && items[i].id) || "") === root.selectedContributionId) {
                    return items[i]
                }
            }
        }

        return items[0]
    }

    function rightContributionById(contributionId) {
        var items = root.rightContributions || []
        if ((contributionId || "") === "") {
            return null
        }

        for (var i = 0; i < items.length; ++i) {
            var item = items[i]
            if (((item && item.id) || "") === contributionId) {
                return item
            }
        }

        return null
    }

    function leftPanelContribution() {
        var deviceConsolePanel = rightContributionById(root.deviceConsolePanelId)
        if (deviceConsolePanel) {
            return deviceConsolePanel
        }
        return currentDeviceContribution()
    }

    function contributionLabel(item) {
        if (!item) {
            return "\u6682\u65e0\u8bbe\u5907\u9762\u677f"
        }
        return item.title || item.id || item.pluginName || "\u672a\u547d\u540d\u7ec4\u4ef6"
    }

    function regionCount(source) {
        return (source || []).length
    }

    function shouldShowDeviceConsoleDetail() {
        var contribution = root.leftPanelContribution()
        if (!contribution) {
            return false
        }
        return String(contribution.id || "") === root.deviceConsolePanelId
    }

    onRightContributionsChanged: syncSelection()
    Component.onCompleted: syncSelection()

    RowLayout {
        anchors.fill: parent
        spacing: Theme.spacingLarge

        StyledCard {
            Layout.preferredWidth: (root.rightContributions || []).length > 0 ? 260 : 0
            Layout.fillHeight: true
            visible: (root.rightContributions || []).length > 0
            clip: true

            Item {
                anchors.fill: parent
                anchors.margins: Theme.spacingNormal
                property real panelOpacity: 1.0

                Behavior on panelOpacity {
                    NumberAnimation {
                        duration: 140
                        easing.type: Easing.OutCubic
                    }
                }

                Loader {
                    id: leftPanelLoader
                    anchors.fill: parent
                    active: root.leftPanelContribution() !== null
                    asynchronous: true
                    opacity: parent.panelOpacity
                    source: root.leftPanelContribution()
                            ? String(root.leftPanelContribution().qmlSource || "")
                            : ""
                    onSourceChanged: parent.panelOpacity = 0.0
                    onStatusChanged: {
                        if (status === Loader.Ready) {
                            parent.panelOpacity = 1.0
                        }
                    }
                }

                Text {
                    anchors.centerIn: parent
                    width: parent.width - Theme.spacingLarge * 2
                    visible: !leftPanelLoader.active || leftPanelLoader.status === Loader.Error
                    text: "\u53f3\u4fa7\u8d21\u732e\u9762\u677f\u52a0\u8f7d\u5931\u8d25"
                    color: Theme.textSecondary
                    wrapMode: Text.Wrap
                    horizontalAlignment: Text.AlignHCenter
                }
            }
        }

        StyledCard {
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true

            Item {
                anchors.fill: parent
                anchors.margins: 1
                property real panelOpacity: 1.0

                Behavior on panelOpacity {
                    NumberAnimation {
                        duration: 140
                        easing.type: Easing.OutCubic
                    }
                }

                Loader {
                    id: deviceDetailLoader
                    anchors.fill: parent
                    active: root.shouldShowDeviceConsoleDetail()
                    asynchronous: true
                    opacity: parent.panelOpacity
                    source: active ? "qrc:/app/qml/qml/DeviceManagementDetailView.qml" : ""
                    onSourceChanged: parent.panelOpacity = 0.0
                    onStatusChanged: {
                        if (status === Loader.Ready) {
                            parent.panelOpacity = 1.0
                        }
                    }
                }

                ContributionHostPanel {
                    id: centerContributionHost
                    anchors.fill: parent
                    visible: !deviceDetailLoader.active && (root.centerContributions || []).length > 0
                    contributions: root.centerContributions || []
                    preferredContributionId: "history_data_fusion_analysis.panel"
                    showContributionMeta: false
                    showNavigation: (root.centerContributions || []).length > 1
                    emptyTitle: "主视图"
                    emptyDescription: "当前没有可显示的主视图。"
                }

                Rectangle {
                    anchors.fill: parent
                    color: Theme.inputBg
                    visible: !deviceDetailLoader.active && !centerContributionHost.visible
                }

                Canvas {
                    id: monitorCanvas
                    anchors.fill: parent
                    visible: !deviceDetailLoader.active && !centerContributionHost.visible

                    onPaint: {
                        var ctx = getContext("2d")
                        var w = width
                        var h = height

                        ctx.reset()
                        ctx.fillStyle = Theme.inputBg
                        ctx.fillRect(0, 0, w, h)

                        ctx.strokeStyle = "rgba(255,255,255,0.08)"
                        ctx.lineWidth = 1
                        for (var i = 0; i <= 10; ++i) {
                            var y = i * h / 10
                            ctx.beginPath()
                            ctx.moveTo(0, y)
                            ctx.lineTo(w, y)
                            ctx.stroke()
                        }
                        for (var j = 0; j <= 12; ++j) {
                            var x = j * w / 12
                            ctx.beginPath()
                            ctx.moveTo(x, 0)
                            ctx.lineTo(x, h)
                            ctx.stroke()
                        }

                        ctx.strokeStyle = "rgba(59,130,246,0.45)"
                        ctx.lineWidth = 3
                        ctx.beginPath()
                        ctx.moveTo(w * 0.08, h * 0.62)
                        ctx.lineTo(w * 0.2, h * 0.4)
                        ctx.lineTo(w * 0.33, h * 0.54)
                        ctx.lineTo(w * 0.46, h * 0.35)
                        ctx.lineTo(w * 0.61, h * 0.49)
                        ctx.lineTo(w * 0.75, h * 0.29)
                        ctx.lineTo(w * 0.92, h * 0.44)
                        ctx.stroke()

                        ctx.strokeStyle = "rgba(16,185,129,0.45)"
                        ctx.beginPath()
                        ctx.moveTo(w * 0.08, h * 0.76)
                        ctx.lineTo(w * 0.2, h * 0.67)
                        ctx.lineTo(w * 0.35, h * 0.73)
                        ctx.lineTo(w * 0.5, h * 0.56)
                        ctx.lineTo(w * 0.64, h * 0.64)
                        ctx.lineTo(w * 0.78, h * 0.52)
                        ctx.lineTo(w * 0.92, h * 0.58)
                        ctx.stroke()
                    }
                }

                Rectangle {
                    anchors.left: parent.left
                    anchors.top: parent.top
                    anchors.margins: Theme.spacingLarge
                    color: Qt.rgba(17 / 255, 24 / 255, 39 / 255, 0.88)
                    border.color: Theme.cardBorder
                    border.width: 1
                    radius: Theme.radiusNormal
                    implicitWidth: infoColumn.implicitWidth + 24
                    implicitHeight: infoColumn.implicitHeight + 20
                    visible: !deviceDetailLoader.active && !centerContributionHost.visible

                    Column {
                        id: infoColumn
                        anchors.centerIn: parent
                        spacing: 4

                        Text {
                            text: "\u8bbe\u5907\u7ba1\u7406\u89c6\u56fe"
                            color: Theme.textPrimary
                            font.pixelSize: Theme.fontSizeLarge
                            font.bold: true
                        }

                        Text {
                            text: root.contributionLabel(root.currentDeviceContribution())
                            color: Theme.textSecondary
                            font.pixelSize: Theme.fontSizeSmall
                        }
                    }
                }

                Rectangle {
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.bottom: parent.bottom
                    height: 34
                    color: Qt.rgba(17 / 255, 24 / 255, 39 / 255, 0.92)
                    border.color: Theme.cardBorder
                    border.width: 1
                    visible: !deviceDetailLoader.active && !centerContributionHost.visible

                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: Theme.spacingNormal
                        anchors.rightMargin: Theme.spacingNormal
                        spacing: Theme.spacingSmall

                        Text {
                            text: "\u8bbe\u5907\u9762\u677f " + root.regionCount(root.rightContributions)
                            color: Theme.textSecondary
                            font.pixelSize: Theme.fontSizeSmall
                        }

                        Text {
                            text: "\u4e3b\u89c6\u56fe " + root.regionCount(root.centerContributions)
                            color: Theme.textSecondary
                            font.pixelSize: Theme.fontSizeSmall
                        }

                        Item {
                            Layout.fillWidth: true
                        }

                        Text {
                            text: "\u53ef\u7528\u63d2\u4ef6 " + root.regionCount(root.uiContributionsData)
                            color: Theme.textSecondary
                            font.pixelSize: Theme.fontSizeSmall
                        }
                    }
                }
            }
        }
    }
}
