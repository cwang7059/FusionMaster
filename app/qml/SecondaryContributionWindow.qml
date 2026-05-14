import QtQuick 2.14
import QtQuick.Controls 2.14
import QtQuick.Layouts 1.14
import QtQuick.Window 2.14
import UITheme 1.0

Window {
    id: root
    transientParent: null
    flags: Qt.Window
    width: 1280
    height: 760
    title: "副屏窗口"
    color: Theme.windowBg

    property var windowManagerService
    property var secondaryTopContributions: []
    property var secondaryLeftContributions: []
    property var secondaryCenterContributions: []
    property var secondaryRightContributions: []
    property var secondaryBottomContributions: []

    visible: windowManagerService
             ? (windowManagerService.secondaryWindowEnabled && windowManagerService.availableScreenCount > 1)
             : false

    function moveToSecondaryScreen() {
        if (Qt.application.screens.length <= 1) {
            return
        }

        const target = Qt.application.screens[1]
        if (!target) {
            return
        }

        screen = target
        x = target.virtualX
        y = target.virtualY
    }

    onVisibleChanged: {
        if (visible) {
            Qt.callLater(moveToSecondaryScreen)
        }
    }

    StyledCard {
        anchors.fill: parent
        anchors.margins: Theme.spacingNormal

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: Theme.spacingNormal
            spacing: Theme.spacingSmall

            Text {
                text: "副屏组件区域"
                color: Theme.textPrimary
                font.pixelSize: Theme.fontSizeTitle
            }

            ScrollView {
                Layout.fillWidth: true
                Layout.preferredHeight: 150
                visible: root.windowManagerService ? root.windowManagerService.isRegionVisible("top") : true
                clip: true

                Flow {
                    width: parent.width
                    spacing: Theme.spacingSmall

                    Repeater {
                        model: root.secondaryTopContributions
                        delegate: ContributionCard {
                            contribution: modelData
                            width: 300
                        }
                    }
                }
            }

            RowLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                spacing: Theme.spacingSmall

                Rectangle {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    visible: root.windowManagerService ? root.windowManagerService.isRegionVisible("left") : true
                    radius: Theme.radiusNormal
                    color: Qt.rgba(1, 1, 1, 0.04)
                    border.color: Qt.rgba(1, 1, 1, 0.1)

                    ListView {
                        anchors.fill: parent
                        anchors.margins: Theme.spacingSmall
                        spacing: Theme.spacingSmall
                        clip: true
                        model: root.secondaryLeftContributions

                        delegate: ContributionCard {
                            contribution: modelData
                            width: ListView.view ? ListView.view.width : 0
                        }
                    }
                }

                Rectangle {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    visible: root.windowManagerService ? root.windowManagerService.isRegionVisible("center") : true
                    radius: Theme.radiusNormal
                    color: Qt.rgba(1, 1, 1, 0.04)
                    border.color: Qt.rgba(1, 1, 1, 0.1)

                    ListView {
                        anchors.fill: parent
                        anchors.margins: Theme.spacingSmall
                        spacing: Theme.spacingSmall
                        clip: true
                        model: root.secondaryCenterContributions

                        delegate: ContributionCard {
                            contribution: modelData
                            width: ListView.view ? ListView.view.width : 0
                        }
                    }
                }

                Rectangle {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    visible: root.windowManagerService ? root.windowManagerService.isRegionVisible("right") : true
                    radius: Theme.radiusNormal
                    color: Qt.rgba(1, 1, 1, 0.04)
                    border.color: Qt.rgba(1, 1, 1, 0.1)

                    ListView {
                        anchors.fill: parent
                        anchors.margins: Theme.spacingSmall
                        spacing: Theme.spacingSmall
                        clip: true
                        model: root.secondaryRightContributions

                        delegate: ContributionCard {
                            contribution: modelData
                            width: ListView.view ? ListView.view.width : 0
                        }
                    }
                }
            }

            ScrollView {
                Layout.fillWidth: true
                Layout.preferredHeight: 150
                visible: root.windowManagerService ? root.windowManagerService.isRegionVisible("bottom") : true
                clip: true

                Flow {
                    width: parent.width
                    spacing: Theme.spacingSmall

                    Repeater {
                        model: root.secondaryBottomContributions
                        delegate: ContributionCard {
                            contribution: modelData
                            width: 300
                        }
                    }
                }
            }
        }
    }
}
