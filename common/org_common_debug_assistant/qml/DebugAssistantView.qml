import QtQuick 2.14
import QtQuick.Controls 2.14
import QtQuick.Layouts 1.14
import UITheme 1.0

Rectangle {
    id: root
    color: Theme.cardBg
    radius: 0
    clip: true

    property string busMonitorSource: ""
    property string recordReplaySource: ""
    property string logViewerSource: ""

    function resolvePanelSource(pluginName) {
        if (typeof uiContributionsModel === "undefined" || !uiContributionsModel) {
            return ""
        }

        for (var i = 0; i < uiContributionsModel.length; ++i) {
            var item = uiContributionsModel[i]
            if (!item) {
                continue
            }
            if ((item.pluginName || "") !== pluginName) {
                continue
            }

            var source = String(item.qmlSource || "")
            if (source !== "") {
                return source
            }
        }
        return ""
    }

    function refreshSources() {
        busMonitorSource = resolvePanelSource("org_common_bus_monitor")
        recordReplaySource = resolvePanelSource("org_common_record_replay")
        logViewerSource = resolvePanelSource("org_common_log_viewer")
    }

    Component.onCompleted: refreshSources()

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 0
        spacing: 0

        SplitView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            orientation: Qt.Vertical

            handle: Item {
                implicitHeight: 1
                implicitWidth: parent ? parent.width : 0

                Rectangle {
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.leftMargin: Theme.spacingNormal
                    anchors.rightMargin: Theme.spacingNormal
                    anchors.verticalCenter: parent.verticalCenter
                    height: 1
                    color: Theme.inputBorder
                }
            }

            Item {
                SplitView.fillWidth: true
                SplitView.fillHeight: true
                SplitView.preferredHeight: 430

                ColumnLayout {
                    anchors.fill: parent
                    spacing: 0

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 36
                        visible: root.recordReplaySource !== ""
                        color: Theme.inputBg
                        border.color: Theme.inputBorder

                        RowLayout {
                            anchors.fill: parent
                            anchors.margins: 5
                            spacing: 8

                            Text {
                                color: Theme.textPrimary
                                text: "消息总线监视"
                                font.pixelSize: Theme.fontSizeLarge
                                font.bold: true
                            }

                            Item { Layout.fillWidth: true }

                            Button {
                                Layout.preferredWidth: 110
                                text: "记录重演"
                                visible: root.recordReplaySource !== ""
                                onClicked: recordReplayDialog.open()

                                contentItem: Text {
                                    text: parent.text
                                    color: Theme.accentText
                                    horizontalAlignment: Text.AlignHCenter
                                    verticalAlignment: Text.AlignVCenter
                                    font.pixelSize: 14
                                }

                                background: Rectangle {
                                    radius: 2
                                    color: parent.down ? Theme.accentPressed : (parent.hovered ? Theme.accent : Theme.accentWeak)
                                    border.color: Theme.inputBorder
                                }
                            }
                        }
                    }

                    Loader {
                        id: topLoader
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        active: true
                        source: root.busMonitorSource
                    }
                }

                Text {
                    anchors.centerIn: parent
                    visible: topLoader.status !== Loader.Ready
                    color: Theme.textSecondary
                    text: "\u6D88\u606F\u603B\u7EBF\u76D1\u89C6\u63D2\u4EF6\u754C\u9762\u672A\u52A0\u8F7D"
                }
            }

            Item {
                SplitView.fillWidth: true
                SplitView.fillHeight: true
                SplitView.minimumHeight: 220
                SplitView.preferredHeight: 340

                Loader {
                    id: logLoader
                    anchors.fill: parent
                    active: true
                    source: root.logViewerSource
                }

                Text {
                    anchors.centerIn: parent
                    visible: logLoader.status !== Loader.Ready
                    color: Theme.textSecondary
                    text: "\u65E5\u5FD7\u663E\u793A\u63D2\u4EF6\u754C\u9762\u672A\u52A0\u8F7D"
                }
            }
        }
    }

    Popup {
        id: recordReplayDialog
        modal: false
        focus: true
        dim: false
        property real minDialogWidth: 900
        property real minDialogHeight: 520
        property real resizeMargin: 8
        property real rsMouseX: 0
        property real rsMouseY: 0
        property real rsX: 0
        property real rsY: 0
        property real rsW: 0
        property real rsH: 0
        property bool rsLeft: false
        property bool rsRight: false
        property bool rsTop: false
        property bool rsBottom: false
        padding: 0
        closePolicy: Popup.CloseOnEscape
        x: (root.width - width) / 2
        y: (root.height - height) / 2
        width: Math.max(minDialogWidth, Math.min(root.width - 80, 980))
        height: Math.max(minDialogHeight, Math.min(root.height - 70, 620))

        function beginResize(area, mouse, left, right, top, bottom) {
            var p = area.mapToItem(root, mouse.x, mouse.y)
            rsMouseX = p.x
            rsMouseY = p.y
            rsX = x
            rsY = y
            rsW = width
            rsH = height
            rsLeft = left
            rsRight = right
            rsTop = top
            rsBottom = bottom
        }

        function updateResize(area, mouse) {
            var p = area.mapToItem(root, mouse.x, mouse.y)
            var dx = p.x - rsMouseX
            var dy = p.y - rsMouseY

            var left = rsX
            var right = rsX + rsW
            var top = rsY
            var bottom = rsY + rsH

            if (rsLeft) {
                left += dx
            }
            if (rsRight) {
                right += dx
            }
            if (rsTop) {
                top += dy
            }
            if (rsBottom) {
                bottom += dy
            }

            if (rsLeft) {
                left = Math.max(0, Math.min(left, right - minDialogWidth))
            }
            if (rsRight) {
                right = Math.min(root.width, Math.max(right, left + minDialogWidth))
            }
            if (rsTop) {
                top = Math.max(0, Math.min(top, bottom - minDialogHeight))
            }
            if (rsBottom) {
                bottom = Math.min(root.height, Math.max(bottom, top + minDialogHeight))
            }

            x = left
            y = top
            width = right - left
            height = bottom - top
        }

        background: Rectangle {
            color: Theme.cardBg
            border.color: Theme.inputBorder
            radius: 2
        }

        contentItem: Item {
            width: recordReplayDialog.availableWidth
            height: recordReplayDialog.availableHeight
            clip: true

            ColumnLayout {
                anchors.fill: parent
                spacing: 0

                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 34
                    color: Theme.inputBg
                    border.color: Theme.inputBorder

                    MouseArea {
                        id: dragArea
                        anchors.fill: parent
                        cursorShape: Qt.SizeAllCursor
                        property real pressRootX: 0
                        property real pressRootY: 0
                        property real dialogX: 0
                        property real dialogY: 0

                        onPressed: function(mouse) {
                            var p = dragArea.mapToItem(root, mouse.x, mouse.y)
                            pressRootX = p.x
                            pressRootY = p.y
                            dialogX = recordReplayDialog.x
                            dialogY = recordReplayDialog.y
                        }

                        onPositionChanged: function(mouse) {
                            if (!(mouse.buttons & Qt.LeftButton)) {
                                return
                            }
                            var p = dragArea.mapToItem(root, mouse.x, mouse.y)
                            var nx = dialogX + p.x - pressRootX
                            var ny = dialogY + p.y - pressRootY
                            var maxX = Math.max(0, root.width - recordReplayDialog.width)
                            var maxY = Math.max(0, root.height - recordReplayDialog.height)
                            recordReplayDialog.x = Math.max(0, Math.min(maxX, nx))
                            recordReplayDialog.y = Math.max(0, Math.min(maxY, ny))
                        }
                    }

                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 10
                        anchors.rightMargin: 8
                        spacing: 6

                        Text {
                            text: "记录重演"
                            color: Theme.textPrimary
                            font.pixelSize: Theme.fontSizeLarge
                            font.bold: true
                        }

                        Item { Layout.fillWidth: true }

                        Button {
                            Layout.preferredWidth: 24
                            Layout.preferredHeight: 24
                            text: "\u00D7"
                            onClicked: recordReplayDialog.close()

                            contentItem: Text {
                                text: parent.text
                                color: Theme.accentText
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                                font.pixelSize: 16
                                font.bold: true
                            }

                            background: Rectangle {
                                radius: 2
                                color: parent.down ? Qt.rgba(239 / 255, 68 / 255, 68 / 255, 0.78)
                                                   : (parent.hovered ? Qt.rgba(239 / 255, 68 / 255, 68 / 255, 0.4) : Theme.accent)
                                border.color: Theme.inputBorder
                            }
                        }
                    }
                }

                Loader {
                    id: recordReplayLoader
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    active: recordReplayDialog.visible
                    source: root.recordReplaySource

                    onLoaded: {
                        if (!item) {
                            return
                        }
                        if (item.hasOwnProperty("dialogHost")) {
                            item.dialogHost = recordReplayDialog
                        }
                        // 直接绑定到 Loader 尺寸，确保弹窗缩放时内容同步缩放
                        item.width = Qt.binding(function() { return recordReplayLoader.width })
                        item.height = Qt.binding(function() { return recordReplayLoader.height })
                    }
                }
            }
        }

        MouseArea {
            anchors.left: parent.left
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            width: recordReplayDialog.resizeMargin
            hoverEnabled: true
            cursorShape: Qt.SizeHorCursor
            onPressed: function(mouse) { recordReplayDialog.beginResize(this, mouse, true, false, false, false) }
            onPositionChanged: function(mouse) {
                if (pressed) {
                    recordReplayDialog.updateResize(this, mouse)
                }
            }
        }

        MouseArea {
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            width: recordReplayDialog.resizeMargin
            hoverEnabled: true
            cursorShape: Qt.SizeHorCursor
            onPressed: function(mouse) { recordReplayDialog.beginResize(this, mouse, false, true, false, false) }
            onPositionChanged: function(mouse) {
                if (pressed) {
                    recordReplayDialog.updateResize(this, mouse)
                }
            }
        }

        MouseArea {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            height: recordReplayDialog.resizeMargin
            hoverEnabled: true
            cursorShape: Qt.SizeVerCursor
            onPressed: function(mouse) { recordReplayDialog.beginResize(this, mouse, false, false, true, false) }
            onPositionChanged: function(mouse) {
                if (pressed) {
                    recordReplayDialog.updateResize(this, mouse)
                }
            }
        }

        MouseArea {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            height: recordReplayDialog.resizeMargin
            hoverEnabled: true
            cursorShape: Qt.SizeVerCursor
            onPressed: function(mouse) { recordReplayDialog.beginResize(this, mouse, false, false, false, true) }
            onPositionChanged: function(mouse) {
                if (pressed) {
                    recordReplayDialog.updateResize(this, mouse)
                }
            }
        }

        MouseArea {
            anchors.left: parent.left
            anchors.top: parent.top
            width: recordReplayDialog.resizeMargin
            height: recordReplayDialog.resizeMargin
            hoverEnabled: true
            cursorShape: Qt.SizeFDiagCursor
            onPressed: function(mouse) { recordReplayDialog.beginResize(this, mouse, true, false, true, false) }
            onPositionChanged: function(mouse) {
                if (pressed) {
                    recordReplayDialog.updateResize(this, mouse)
                }
            }
        }

        MouseArea {
            anchors.right: parent.right
            anchors.top: parent.top
            width: recordReplayDialog.resizeMargin
            height: recordReplayDialog.resizeMargin
            hoverEnabled: true
            cursorShape: Qt.SizeBDiagCursor
            onPressed: function(mouse) { recordReplayDialog.beginResize(this, mouse, false, true, true, false) }
            onPositionChanged: function(mouse) {
                if (pressed) {
                    recordReplayDialog.updateResize(this, mouse)
                }
            }
        }

        MouseArea {
            anchors.left: parent.left
            anchors.bottom: parent.bottom
            width: recordReplayDialog.resizeMargin
            height: recordReplayDialog.resizeMargin
            hoverEnabled: true
            cursorShape: Qt.SizeBDiagCursor
            onPressed: function(mouse) { recordReplayDialog.beginResize(this, mouse, true, false, false, true) }
            onPositionChanged: function(mouse) {
                if (pressed) {
                    recordReplayDialog.updateResize(this, mouse)
                }
            }
        }

        MouseArea {
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            width: recordReplayDialog.resizeMargin
            height: recordReplayDialog.resizeMargin
            hoverEnabled: true
            cursorShape: Qt.SizeFDiagCursor
            onPressed: function(mouse) { recordReplayDialog.beginResize(this, mouse, false, true, false, true) }
            onPositionChanged: function(mouse) {
                if (pressed) {
                    recordReplayDialog.updateResize(this, mouse)
                }
            }
        }
    }
}
