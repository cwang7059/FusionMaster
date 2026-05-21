import QtQuick 2.14
import QtQuick.Controls 2.14
import QtQuick.Layouts 1.14
import UITheme 1.0

Rectangle {
    id: root
    color: "#071321"
    clip: true

    property var windowManagerService
    property int activeTabIndex: 0
    property string discoveredPluginsTextValue: ""
    property string selectedPluginsTextValue: ""
    property string runtimeStatesTextValue: ""
    property string scanWarningsTextValue: ""
    property string skippedPluginsTextValue: ""
    property string lifecycleWarningsTextValue: ""
    property var uiContributionsData: []
    property var mainTabContributionsData: []
    property var topContributions: []
    property var leftContributions: []
    property var centerContributions: []
    property var rightContributions: []
    property var bottomContributions: []
    readonly property int mainTabCount: mainTabContributionsData ? mainTabContributionsData.length : 0
    readonly property int activeMainTabIndex: mainTabCount > 0
            ? Math.max(0, Math.min(activeTabIndex, mainTabCount - 1))
            : -1
    readonly property var activeMainTab: activeMainTabIndex >= 0
            ? (mainTabContributionsData[activeMainTabIndex] || null)
            : null
    readonly property string activeMainTabSource: activeMainTab && activeMainTab.qmlSource
            ? activeMainTab.qmlSource
            : ""

    signal businessPureUiRequested()

    function resolveRtspPlayerQmlSource() {
        if (!uiContributionsData || uiContributionsData.length === 0) {
            return ""
        }

        for (var i = 0; i < uiContributionsData.length; ++i) {
            var item = uiContributionsData[i]
            if (!item) {
                continue
            }
            if ((item.pluginName || "") === "org_common_rtsp_player"
                    && (item.qmlSource || "") !== "") {
                return item.qmlSource
            }
        }
        return ""
    }

    function resolveDebugAssistantQmlSource() {
        if (!uiContributionsData || uiContributionsData.length === 0) {
            return ""
        }

        for (var i = 0; i < uiContributionsData.length; ++i) {
            var item = uiContributionsData[i]
            if (!item) {
                continue
            }
            if ((item.pluginName || "") === "org_common_debug_assistant"
                    && (item.qmlSource || "") !== "") {
                return item.qmlSource
            }
        }
        return ""
    }

    Item {
        anchors.fill: parent

        Loader {
            id: mainTabLoader
            anchors.fill: parent
            active: root.activeMainTabSource.length > 0
            source: root.activeMainTabSource
            visible: status === Loader.Ready

            onStatusChanged: {
                if (status === Loader.Error) {
                    console.warn("[APP] Failed to load business tab:", root.activeMainTabSource)
                }
            }
        }

        Rectangle {
            anchors.fill: parent
            color: root.color
            visible: root.mainTabCount === 0

            Text {
                anchors.centerIn: parent
                color: Theme.textSecondary
                font.pixelSize: Theme.fontSizeLarge
                text: "未加载业务页面"
            }
        }

        Rectangle {
            anchors.fill: parent
            color: root.color
            visible: root.mainTabCount > 0 && mainTabLoader.status === Loader.Error

            Column {
                anchors.centerIn: parent
                spacing: 8

                Text {
                    anchors.horizontalCenter: parent.horizontalCenter
                    color: Theme.textPrimary
                    font.pixelSize: Theme.fontSizeLarge
                    font.bold: true
                    text: "业务页面加载失败"
                }

                Text {
                    anchors.horizontalCenter: parent.horizontalCenter
                    width: Math.min(520, root.width - 80)
                    horizontalAlignment: Text.AlignHCenter
                    elide: Text.ElideMiddle
                    color: Theme.textSecondary
                    font.pixelSize: Theme.fontSizeSmall
                    text: root.activeMainTabSource
                }
            }
        }
    }
}
