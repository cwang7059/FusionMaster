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
    property var topContributions: []
    property var leftContributions: []
    property var centerContributions: []
    property var rightContributions: []
    property var bottomContributions: []

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

    StackLayout {
        anchors.fill: parent
        currentIndex: Math.max(0, Math.min(root.activeTabIndex, 5))

        SystemOverviewView {
            Layout.fillWidth: true
            Layout.fillHeight: true
        }

        DataManagementView {
            Layout.fillWidth: true
            Layout.fillHeight: true
        }

        FusionAnalysisView {
            Layout.fillWidth: true
            Layout.fillHeight: true
        }

        AlgorithmOptimizationView {
            Layout.fillWidth: true
            Layout.fillHeight: true
        }

        StatisticsView {
            Layout.fillWidth: true
            Layout.fillHeight: true
        }

        SystemSettingsView {
            Layout.fillWidth: true
            Layout.fillHeight: true
        }
    }
}
