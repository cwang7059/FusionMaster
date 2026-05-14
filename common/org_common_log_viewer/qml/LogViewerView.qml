import QtQuick 2.14
import QtQuick.Controls 2.14
import QtQuick.Layouts 1.14
import UITheme 1.0

Rectangle {
    id: root
    color: Theme.cardBg
    radius: 0

    readonly property bool controllerReady: typeof logViewerController !== "undefined" && logViewerController !== null

    Component.onCompleted: {
        if (controllerReady) {
            logViewerController.activeLevel = "全部"
        }
    }

    Connections {
        target: controllerReady ? logViewerController : null

        function onFilteredLogsChanged() {
            if (logList.count > 0) {
                logList.positionViewAtEnd()
            }
        }
    }

    function normalizeToken(value) {
        var text = String(value || "").trim()
        if (text === "") {
            return "全部"
        }
        return text
    }

    function currentTag() {
        return normalizeToken(controllerReady ? logViewerController.activeTag : "")
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: Theme.spacingNormal
        spacing: Theme.spacingSmall

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 32
            radius: 2
            border.color: Theme.inputBorder
            color: Theme.inputBg

            Text {
                anchors.left: parent.left
                anchors.leftMargin: 10
                anchors.verticalCenter: parent.verticalCenter
                text: "日志管理"
                color: Theme.textPrimary
                font.pixelSize: Theme.fontSizeLarge
                font.bold: true
            }
        }

        Text {
            visible: !controllerReady
            text: "logViewerController 控制器未就绪"
            color: Theme.textSecondary
        }

        Rectangle {
            visible: controllerReady
            Layout.fillWidth: true
            Layout.fillHeight: true
            color: Theme.inputBg
            border.color: Theme.inputBorder
            radius: Theme.radiusNormal

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: Theme.spacingSmall
                spacing: Theme.spacingSmall

                RowLayout {
                    Layout.fillWidth: true
                    spacing: Theme.spacingSmall

                    Text {
                        Layout.fillWidth: true
                        color: Theme.textSecondary
                        text: "日志目录: " + (controllerReady ? logViewerController.logRootDir : "")
                              + "    |    状态: " + (controllerReady ? logViewerController.importStatus : "")
                              + "    |    刷新: " + (controllerReady ? (logViewerController.realtimePaused ? "已停止" : "运行中") : "-")
                              + "    |    总数: " + (controllerReady ? logViewerController.totalLogCount : 0)
                              + "    |    当前筛选: " + (controllerReady ? logViewerController.filteredLogCount : 0)
                        elide: Text.ElideRight
                    }
                }

                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 38
                    color: Theme.accentWeak
                    border.color: Theme.inputBorder
                    radius: 2

                    RowLayout {
                        anchors.fill: parent
                        anchors.margins: 4
                        spacing: 6

                        Text {
                            text: "标签"
                            color: Theme.textPrimary
                            Layout.alignment: Qt.AlignVCenter
                        }

                        Flickable {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            Layout.alignment: Qt.AlignVCenter
                            clip: true
                            contentWidth: tagsRow.implicitWidth
                            contentHeight: tagsRow.implicitHeight
                            flickableDirection: Flickable.HorizontalFlick
                            boundsBehavior: Flickable.StopAtBounds
                            interactive: contentWidth > width

                            Row {
                                id: tagsRow
                                x: 0
                                y: Math.max(0, (parent.height - height) / 2)
                                spacing: 6

                                Repeater {
                                    model: controllerReady ? logViewerController.tags : []

                                    Rectangle {
                                        readonly property string chipText: root.normalizeToken(modelData)
                                        height: 28
                                        width: Math.max(56, tagChipLabel.implicitWidth + 16)
                                        radius: 2
                                        color: root.currentTag() === chipText ? Theme.accent : Theme.cardBg
                                        border.color: Theme.inputBorder

                                        Text {
                                            id: tagChipLabel
                                            anchors.centerIn: parent
                                            text: chipText
                                            color: Theme.textPrimary
                                            font.pixelSize: Theme.fontSizeNormal
                                        }

                                        MouseArea {
                                            anchors.fill: parent
                                            onClicked: {
                                                if (controllerReady) {
                                                    logViewerController.activeTag = chipText
                                                }
                                            }
                                        }
                                    }
                                }
                            }

                            ScrollBar.horizontal: ScrollBar {
                                policy: ScrollBar.AsNeeded
                            }
                        }
                    }
                }

                Rectangle {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    color: Theme.cardBg
                    border.color: Theme.inputBorder

                    ListView {
                        id: logList
                        anchors.fill: parent
                        anchors.margins: 2
                        clip: true
                        spacing: 0
                        model: controllerReady ? logViewerController.filteredLogs : []

                        delegate: Rectangle {
                            width: logList.width
                            height: 28
                            color: index % 2 === 0 ? Theme.cardBg : Theme.inputBg
                            border.color: Theme.inputBorder
                            border.width: 1

                            Text {
                                anchors.fill: parent
                                anchors.leftMargin: 8
                                anchors.rightMargin: 8
                                verticalAlignment: Text.AlignVCenter
                                color: Theme.textPrimary
                                elide: Text.ElideRight
                                text: "[" + (modelData.timestamp || "") + "] "
                                      + "[" + (modelData.level || "") + "] "
                                      + "[" + (modelData.tag || "") + "] "
                                      + (modelData.message || "")
                            }
                        }

                        ScrollBar.vertical: ScrollBar {
                            policy: ScrollBar.AsNeeded
                        }
                    }
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: Theme.spacingSmall

                    DarkButton {
                        Layout.preferredWidth: 92
                        text: "导入文件"
                        onClicked: {
                            if (controllerReady) {
                                logViewerController.pickAndImportLogFile()
                            }
                        }
                    }

                    DarkButton {
                        Layout.preferredWidth: 92
                        text: "导入目录"
                        onClicked: {
                            if (controllerReady) {
                                logViewerController.pickAndImportLogDirectory()
                            }
                        }
                    }

                    DarkButton {
                        Layout.preferredWidth: 92
                        text: "默认目录"
                        onClicked: {
                            if (controllerReady) {
                                logViewerController.importLogDirectory("")
                            }
                        }
                    }

                    DarkButton {
                        Layout.preferredWidth: 92
                        text: "清空日志"
                        onClicked: {
                            if (controllerReady) {
                                logViewerController.clear()
                            }
                        }
                    }

                    DarkButton {
                        Layout.preferredWidth: 92
                        text: "停止刷新"
                        enabled: controllerReady && !logViewerController.realtimePaused
                        onClicked: {
                            if (controllerReady) {
                                logViewerController.setRealtimePaused(true)
                            }
                        }
                    }

                    DarkButton {
                        Layout.preferredWidth: 92
                        text: "开始刷新"
                        enabled: controllerReady && logViewerController.realtimePaused
                        onClicked: {
                            if (controllerReady) {
                                logViewerController.setRealtimePaused(false)
                            }
                        }
                    }

                    Item {
                        Layout.fillWidth: true
                    }

                    DarkTextField {
                        id: keywordField
                        Layout.fillWidth: true
                        placeholderText: "关键字过滤（消息）"
                        text: controllerReady ? logViewerController.keyword : ""
                        onTextChanged: {
                            if (controllerReady) {
                                logViewerController.keyword = text
                            }
                        }
                    }

                    DarkButton {
                        Layout.preferredWidth: 92
                        text: "清空检索"
                        onClicked: keywordField.text = ""
                    }
                }
            }
        }
    }
}
