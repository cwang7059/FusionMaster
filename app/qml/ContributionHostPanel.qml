import QtQuick 2.14
import QtQuick.Controls 2.14
import QtQuick.Layouts 1.14
import UITheme 1.0

Item {
    id: root

    property var contributions: []
    property string preferredContributionId: ""
    property string selectedContributionId: ""
    property bool showContributionMeta: true
    property bool showNavigation: true
    property string emptyTitle: "\u8bbe\u5907\u7ba1\u7406"
    property string emptyDescription: "\u5f53\u524d\u6ca1\u6709\u53ef\u663e\u793a\u7684\u8bbe\u5907\u9762\u677f\u3002"
    property var activeContribution: resolveCurrentContribution()

    signal contributionActivated(string contributionId)

    function containsContribution(contributionId) {
        var items = contributions || []
        if ((contributionId || "") === "") {
            return false
        }

        for (var i = 0; i < items.length; ++i) {
            if (((items[i] && items[i].id) || "") === contributionId) {
                return true
            }
        }
        return false
    }

    function ensureSelection() {
        var items = contributions || []
        if (items.length === 0) {
            if (selectedContributionId !== "") {
                selectedContributionId = ""
            }
            return
        }

        if (containsContribution(preferredContributionId)
                && selectedContributionId !== preferredContributionId) {
            selectedContributionId = preferredContributionId
            return
        }

        if (!containsContribution(selectedContributionId)) {
            selectedContributionId = (items[0] && items[0].id) || ""
        }
    }

    function resolveCurrentContribution() {
        var items = contributions || []
        if (items.length === 0) {
            return null
        }

        if (containsContribution(selectedContributionId)) {
            for (var i = 0; i < items.length; ++i) {
                if (((items[i] && items[i].id) || "") === selectedContributionId) {
                    return items[i]
                }
            }
        }

        if (containsContribution(preferredContributionId)) {
            for (var j = 0; j < items.length; ++j) {
                if (((items[j] && items[j].id) || "") === preferredContributionId) {
                    return items[j]
                }
            }
        }

        return items[0]
    }

    function contributionLabel(entry) {
        return String((entry && (entry.title || entry.id || entry.pluginName)) || "\u672a\u547d\u540d")
    }

    function isEntryActive(entry) {
        return String((entry && entry.id) || "") === root.selectedContributionId
    }

    function statusText(entry) {
        return isEntryActive(entry) ? "\u5728\u7ebf" : "\u79bb\u7ebf"
    }

    function statusColor(entry) {
        return isEntryActive(entry) ? Theme.success : Theme.danger
    }

    onContributionsChanged: ensureSelection()
    onPreferredContributionIdChanged: ensureSelection()
    onSelectedContributionIdChanged: {
        if (selectedContributionId !== "") {
            contributionActivated(selectedContributionId)
        }
    }

    Component.onCompleted: ensureSelection()

    ColumnLayout {
        anchors.fill: parent
        spacing: Theme.spacingNormal

        Text {
            Layout.fillWidth: true
            text: "\u8bbe\u5907\u5217\u8868"
            color: Theme.textSecondary
            font.pixelSize: Theme.fontSizeSmall
            visible: root.showNavigation && (root.contributions || []).length > 0
            leftPadding: 4
        }

        ListView {
            id: navList
            Layout.fillWidth: true
            Layout.preferredHeight: contentHeight
            Layout.maximumHeight: Math.min(contentHeight, 230)
            clip: true
            spacing: 8
            visible: root.showNavigation && (root.contributions || []).length > 0
            model: root.contributions || []

            delegate: Rectangle {
                property var entry: modelData || ({})
                property bool active: root.isEntryActive(entry)
                width: navList.width
                height: 44
                radius: Theme.radiusNormal
                border.width: 1
                border.color: active
                              ? Qt.rgba(59 / 255, 130 / 255, 246 / 255, 0.5)
                              : Qt.rgba(1, 1, 1, 0.12)
                color: active
                       ? Qt.rgba(59 / 255, 130 / 255, 246 / 255, 0.18)
                       : "transparent"

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 10
                    anchors.rightMargin: 10
                    spacing: 8

                    Rectangle {
                        width: 8
                        height: 8
                        radius: 4
                        color: root.statusColor(entry)
                    }

                    Text {
                        Layout.fillWidth: true
                        text: root.contributionLabel(entry)
                        color: active ? Theme.textPrimary : Theme.textSecondary
                        font.pixelSize: Theme.fontSizeNormal
                        elide: Text.ElideRight
                    }

                    Text {
                        text: root.statusText(entry)
                        color: root.statusColor(entry)
                        font.pixelSize: Theme.fontSizeSmall
                        font.bold: true
                    }
                }

                MouseArea {
                    anchors.fill: parent
                    onClicked: root.selectedContributionId = entry.id || ""
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            visible: root.showContributionMeta
            color: Theme.inputBg
            border.color: Theme.cardBorder
            border.width: 1
            radius: Theme.radiusNormal
            implicitHeight: metaColumn.implicitHeight + Theme.spacingNormal * 2

            ColumnLayout {
                id: metaColumn
                anchors.fill: parent
                anchors.margins: Theme.spacingNormal
                spacing: 4

                Text {
                    Layout.fillWidth: true
                    text: root.activeContribution
                          ? root.contributionLabel(root.activeContribution)
                          : root.emptyTitle
                    color: Theme.textPrimary
                    font.pixelSize: Theme.fontSizeNormal
                    font.bold: true
                    elide: Text.ElideRight
                }

                Text {
                    Layout.fillWidth: true
                    visible: root.activeContribution !== null
                    text: "\u63d2\u4ef6: " + String((root.activeContribution && root.activeContribution.pluginName) || "-")
                    color: Theme.textSecondary
                    font.pixelSize: Theme.fontSizeSmall
                    wrapMode: Text.WrapAnywhere
                }

                Text {
                    Layout.fillWidth: true
                    visible: root.activeContribution === null
                    text: root.emptyDescription
                    color: Theme.textSecondary
                    font.pixelSize: Theme.fontSizeSmall
                    wrapMode: Text.Wrap
                }
            }
        }

        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true
            property real panelOpacity: 1.0

            Behavior on panelOpacity {
                NumberAnimation {
                    duration: 140
                    easing.type: Easing.OutCubic
                }
            }

            Loader {
                id: contributionLoader
                anchors.fill: parent
                active: root.activeContribution !== null
                asynchronous: true
                opacity: parent.panelOpacity
                source: root.activeContribution ? (root.activeContribution.qmlSource || "") : ""
                onSourceChanged: parent.panelOpacity = 0.0
                onStatusChanged: {
                    if (status === Loader.Ready) {
                        parent.panelOpacity = 1.0
                    }
                }
            }

            Rectangle {
                anchors.fill: parent
                visible: root.activeContribution === null
                         || contributionLoader.status === Loader.Error
                color: Theme.inputBg
                border.color: Theme.cardBorder
                border.width: 1
                radius: Theme.radiusNormal

                Text {
                    anchors.centerIn: parent
                    width: parent.width - Theme.spacingLarge * 2
                    text: root.activeContribution === null
                          ? root.emptyDescription
                          : "\u7ec4\u4ef6\u52a0\u8f7d\u5931\u8d25: " + String(root.activeContribution.qmlSource || "")
                    color: Theme.textSecondary
                    horizontalAlignment: Text.AlignHCenter
                    wrapMode: Text.Wrap
                }
            }
        }
    }
}
