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
    property var businessTabs: []
    property var uiContributionsData: []
    property var topContributions: []
    property var leftContributions: []
    property var centerContributions: []
    property var rightContributions: []
    property var bottomContributions: []
    readonly property int businessTabCount: businessTabs && businessTabs.length !== undefined ? businessTabs.length : 0

    signal businessPureUiRequested()

    function normalizedActiveTabIndex() {
        if (businessTabCount <= 0) {
            return 0
        }
        return Math.max(0, Math.min(activeTabIndex, businessTabCount - 1))
    }

    function contributionTitle(entry) {
        return String((entry && (entry.navText || entry.title || entry.pluginName)) || "未命名业务页面")
    }

    function contributionSource(entry) {
        return String((entry && entry.qmlSource) || "")
    }

    StackLayout {
        id: businessStack
        anchors.fill: parent
        visible: root.businessTabCount > 0
        currentIndex: root.normalizedActiveTabIndex()

        Repeater {
            model: root.businessTabs || []

            delegate: Item {
                Layout.fillWidth: true
                Layout.fillHeight: true

                property var tabContribution: modelData || ({})
                property string pageSource: root.contributionSource(tabContribution)

                Loader {
                    id: tabLoader
                    anchors.fill: parent
                    active: index === businessStack.currentIndex && pageSource !== ""
                    asynchronous: true
                    source: pageSource
                }

                Rectangle {
                    anchors.fill: parent
                    visible: index === businessStack.currentIndex
                             && (pageSource === ""
                                 || tabLoader.status === Loader.Loading
                                 || tabLoader.status === Loader.Error)
                    color: Theme.inputBg
                    border.color: Theme.cardBorder
                    border.width: 1

                    ColumnLayout {
                        anchors.centerIn: parent
                        width: Math.min(parent.width - Theme.spacingLarge * 2, 520)
                        spacing: Theme.spacingSmall

                        Text {
                            Layout.fillWidth: true
                            text: root.contributionTitle(tabContribution)
                            color: Theme.textPrimary
                            font.pixelSize: Theme.fontSizeLarge
                            font.bold: true
                            horizontalAlignment: Text.AlignHCenter
                            elide: Text.ElideRight
                        }

                        Text {
                            Layout.fillWidth: true
                            text: pageSource === ""
                                  ? "未配置业务页面入口"
                                  : (tabLoader.status === Loader.Error
                                     ? "业务页面加载失败: " + pageSource
                                     : "正在加载业务页面...")
                            color: Theme.textSecondary
                            font.pixelSize: Theme.fontSizeNormal
                            horizontalAlignment: Text.AlignHCenter
                            wrapMode: Text.WrapAnywhere
                        }
                    }
                }
            }
        }
    }

    Rectangle {
        anchors.fill: parent
        visible: root.businessTabCount <= 0
        color: Theme.inputBg
        border.color: Theme.cardBorder
        border.width: 1

        ColumnLayout {
            anchors.centerIn: parent
            width: Math.min(parent.width - Theme.spacingLarge * 2, 520)
            spacing: Theme.spacingSmall

            Text {
                Layout.fillWidth: true
                text: "未加载业务页面"
                color: Theme.textPrimary
                font.pixelSize: Theme.fontSizeLarge
                font.bold: true
                horizontalAlignment: Text.AlignHCenter
            }

            Text {
                Layout.fillWidth: true
                text: "请启用声明 surface = main_tab 的业务插件。"
                color: Theme.textSecondary
                font.pixelSize: Theme.fontSizeNormal
                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.Wrap
            }
        }
    }
}
