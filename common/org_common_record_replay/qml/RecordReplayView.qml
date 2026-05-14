import QtQuick 2.14
import QtQuick.Controls 2.14
import QtQuick.Layouts 1.14
import UITheme 1.0

Rectangle {
    id: root
    color: Theme.cardBg
    radius: 0
    property var dialogHost: null
    readonly property int rowControlHeight: 26

    readonly property bool controllerReady: typeof recordReplayController !== "undefined" && recordReplayController !== null
    readonly property var currentSession: controllerReady ? (recordReplayController.selectedSessionInfo || ({})) : ({})
    readonly property int sessionCount: controllerReady && recordReplayController.sessionList ? recordReplayController.sessionList.length : 0
    readonly property bool hasSelectedSession: controllerReady
                                               && recordReplayController.selectedSessionIndex >= 0
                                               && currentSession
                                               && (currentSession.directory || "").toString().trim().length > 0
    readonly property int currentRtspFrameCount: Number((currentSession && currentSession.rtspFrameCount) || 0)
    readonly property int currentRtspTopicCount: Number((currentSession && currentSession.rtspTopicCount) || 0)
    readonly property string currentRtspTopicSummary: (currentSession && currentSession.rtspTopicSummaryText) ? currentSession.rtspTopicSummaryText : ""
    readonly property var currentRtspRows: (currentSession && currentSession.rtspTopicRows) ? currentSession.rtspTopicRows : []
    readonly property bool hasRtspSessionStats: currentRtspFrameCount > 0 || currentRtspTopicCount > 0
    readonly property bool rtspAvailable: controllerReady ? recordReplayController.rtspAvailable : false
    property bool syncingTopicChecks: false
    property bool replaySeekDragging: false
    property double replaySeekVisualProgress: 0.0

    function clamp01(value) {
        return Math.max(0.0, Math.min(1.0, Number(value || 0.0)))
    }

    function updateReplaySeekFromPointer(mouseX, trackWidth, commit) {
        if (!controllerReady) return
        if (!recordReplayController.replaying) return
        if (trackWidth <= 0) return
        var progress = clamp01(mouseX / trackWidth)
        replaySeekVisualProgress = progress
        if (commit) {
            recordReplayController.seekReplayByProgress(progress)
        }
    }

    function parseTopicTokens(text) {
        if (!text || text.trim() === "") {
            return ["*"]
        }
        var pieces = text.split(/[;,\s]+/)
        var out = []
        for (var i = 0; i < pieces.length; ++i) {
            var token = (pieces[i] || "").trim().toLowerCase()
            if (token === "") continue
            out.push(token)
        }
        if (out.length === 0) out.push("*")
        return out
    }

    function setCheckStateByFilter() {
        if (!controllerReady) return
        syncingTopicChecks = true
        var tokens = parseTopicTokens(recordReplayController.topicFilterText)
        var allChecked = tokens.indexOf("*") >= 0
        allTopicCheck.checked = allChecked
        netTopicCheck.checked = allChecked || tokens.indexOf("net/") >= 0
        serialTopicCheck.checked = allChecked || tokens.indexOf("serial/") >= 0
        canTopicCheck.checked = allChecked || tokens.indexOf("can/") >= 0
        rtspTopicCheck.checked = rtspAvailable && (allChecked || tokens.indexOf("rtsp/") >= 0)
        replayTopicCheck.checked = allChecked || tokens.indexOf("record_replay/") >= 0
        syncingTopicChecks = false
    }

    function applyFilterFromChecks() {
        if (!controllerReady || syncingTopicChecks) return
        if (allTopicCheck.checked) {
            recordReplayController.topicFilterText = "*"
            return
        }
        var parts = []
        if (netTopicCheck.checked) parts.push("net/")
        if (serialTopicCheck.checked) parts.push("serial/")
        if (canTopicCheck.checked) parts.push("can/")
        if (rtspAvailable && rtspTopicCheck.checked) parts.push("rtsp/")
        if (replayTopicCheck.checked) parts.push("record_replay/")
        recordReplayController.topicFilterText = parts.length > 0 ? parts.join(";") : "*"
    }

    Component.onCompleted: {
        setCheckStateByFilter()
        replaySeekVisualProgress = controllerReady ? recordReplayController.replayProgress : 0
    }

    Connections {
        target: controllerReady ? recordReplayController : null
        function onTopicFilterTextChanged() {
            setCheckStateByFilter()
        }
        function onRtspAvailableChanged() {
            setCheckStateByFilter()
            if (!rtspAvailable) {
                applyFilterFromChecks()
            }
        }
        function onRuntimeChanged() {
            if (!replaySeekDragging && controllerReady) {
                replaySeekVisualProgress = recordReplayController.replayProgress
            }
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: Theme.spacingNormal
        spacing: Theme.spacingSmall

        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            color: "#163A5D"
            border.color: "#2D5A83"
            radius: Theme.radiusNormal

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: Theme.spacingSmall
                spacing: Theme.spacingSmall

                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: rowControlHeight + 10
                    color: "#1A456B"
                    border.color: "#2D5A83"
                    radius: 2

                    RowLayout {
                        anchors.fill: parent
                        anchors.margins: 5
                        spacing: 8

                        Text { color: Theme.textPrimary; text: "状态:" }
                        Text {
                            id: stateText
                            Layout.preferredWidth: 72
                            Layout.alignment: Qt.AlignVCenter
                            text: controllerReady ? recordReplayController.recordingStateText : "未开始"
                            color: !controllerReady ? Theme.textSecondary
                                                   : (recordReplayController.recording ? "#82D4A7"
                                                                                      : (recordReplayController.replaying ? "#F0CC6A" : Theme.textSecondary))
                            verticalAlignment: Text.AlignVCenter
                        }

                        DarkTextField {
                            id: recordDirField
                            Layout.fillWidth: true
                            Layout.preferredHeight: rowControlHeight
                            Layout.alignment: Qt.AlignVCenter
                            text: controllerReady ? recordReplayController.recordTargetDir : ""
                            onEditingFinished: {
                                if (controllerReady) {
                                    recordReplayController.recordTargetDir = text.trim()
                                }
                            }
                        }

                        DarkButton {
                            Layout.preferredWidth: 88
                            Layout.preferredHeight: rowControlHeight
                            Layout.alignment: Qt.AlignVCenter
                            text: "选择目录"
                            onClicked: {
                                if (!controllerReady) return
                                var chosen = recordReplayController.chooseRecordDirectory()
                                if (chosen && chosen !== "") {
                                    recordDirField.text = chosen
                                    recordReplayController.recordTargetDir = chosen
                                }
                            }
                        }

                        DarkButton {
                            Layout.preferredWidth: 88
                            Layout.preferredHeight: rowControlHeight
                            Layout.alignment: Qt.AlignVCenter
                            text: "开始记录"
                            enabled: controllerReady && !recordReplayController.recording && !recordReplayController.replaying
                            onClicked: if (controllerReady) recordReplayController.startRecord()
                        }

                        DarkButton {
                            Layout.preferredWidth: 88
                            Layout.preferredHeight: rowControlHeight
                            Layout.alignment: Qt.AlignVCenter
                            text: "结束记录"
                            enabled: controllerReady && recordReplayController.recording
                            onClicked: if (controllerReady) recordReplayController.stopRecord()
                        }
                    }
                }

                Text {
                    Layout.fillWidth: true
                    color: Theme.textSecondary
                    text: controllerReady
                          ? ("磁盘: " + recordReplayController.diskSummary
                             + "    |    记录时长: " + recordReplayController.recordingElapsedText
                             + "    |    当前分块: " + (recordReplayController.activeRecordFile || ""))
                          : ""
                    elide: Text.ElideRight
                }

                Text {
                    Layout.fillWidth: true
                    color: "#8EC5F8"
                    text: controllerReady ? ("反馈: " + recordReplayController.operationFeedback) : ""
                    elide: Text.ElideRight
                }

                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: rowControlHeight + 10
                    color: "#1A456B"
                    border.color: "#2D5A83"
                    radius: 2

                    RowLayout {
                        anchors.fill: parent
                        anchors.margins: 5
                        spacing: 8

                        Text { color: Theme.textPrimary; text: "文件列表:" }

                        DarkComboBox {
                            id: sessionCombo
                            Layout.fillWidth: true
                            Layout.preferredHeight: rowControlHeight
                            Layout.alignment: Qt.AlignVCenter
                            model: controllerReady ? recordReplayController.sessionList : []
                            textRole: "sessionId"
                            currentIndex: controllerReady ? recordReplayController.selectedSessionIndex : -1
                            onActivated: {
                                if (controllerReady) {
                                    recordReplayController.selectedSessionIndex = currentIndex
                                }
                            }
                        }

                        DarkButton {
                            Layout.preferredWidth: 70
                            Layout.preferredHeight: rowControlHeight
                            Layout.alignment: Qt.AlignVCenter
                            text: "上一条"
                            enabled: controllerReady && sessionCount > 1 && !recordReplayController.recording && !recordReplayController.replaying
                            onClicked: if (controllerReady) recordReplayController.selectPreviousSession()
                        }

                        DarkButton {
                            Layout.preferredWidth: 70
                            Layout.preferredHeight: rowControlHeight
                            Layout.alignment: Qt.AlignVCenter
                            text: "下一条"
                            enabled: controllerReady && sessionCount > 1 && !recordReplayController.recording && !recordReplayController.replaying
                            onClicked: if (controllerReady) recordReplayController.selectNextSession()
                        }

                        DarkButton {
                            Layout.preferredWidth: 64
                            Layout.preferredHeight: rowControlHeight
                            Layout.alignment: Qt.AlignVCenter
                            text: "删除"
                            enabled: controllerReady && hasSelectedSession && !recordReplayController.recording && !recordReplayController.replaying
                            onClicked: if (controllerReady) recordReplayController.removeSelectedSession()
                        }

                        DarkButton {
                            Layout.preferredWidth: 88
                            Layout.preferredHeight: rowControlHeight
                            Layout.alignment: Qt.AlignVCenter
                            text: "打开目录"
                            enabled: controllerReady && hasSelectedSession
                            onClicked: if (controllerReady) recordReplayController.openSelectedSessionDirectory()
                        }

                        DarkButton {
                            Layout.preferredWidth: 72
                            Layout.preferredHeight: rowControlHeight
                            Layout.alignment: Qt.AlignVCenter
                            text: "刷新"
                            onClicked: if (controllerReady) recordReplayController.refreshSessionList()
                        }
                    }
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 2

                    Text { color: Theme.textPrimary; text: "文件名: " + (currentSession.sessionId || "") }
                    Text { color: Theme.textPrimary; text: "记录数: " + (currentSession.recordCount || 0) }
                    Text {
                        color: Theme.textPrimary
                        text: "时间段: " + (currentSession.startTime || "") + " - " + (currentSession.endTime || "")
                    }
                    Text {
                        color: Theme.textPrimary
                        text: "总时长: " + (currentSession.durationText || "0:00:00")
                              + "    |    总字节: " + (currentSession.payloadBytesText || "0 B")
                              + "    |    分块: " + (currentSession.chunkCount || 0)
                    }

                    Text {
                        visible: hasRtspSessionStats
                        color: Theme.textPrimary
                        text: "RTSP帧数: " + currentRtspFrameCount
                              + "    |    RTSP主题: " + currentRtspTopicCount
                    }

                    Text {
                        visible: hasRtspSessionStats && currentRtspTopicSummary.length > 0
                        color: Theme.textSecondary
                        text: "RTSP摘要: " + currentRtspTopicSummary
                        wrapMode: Text.NoWrap
                        elide: Text.ElideRight
                    }

                    Rectangle {
                        visible: hasRtspSessionStats && currentRtspRows.length > 0
                        Layout.fillWidth: true
                        Layout.preferredHeight: 24 + Math.min(currentRtspRows.length, 6) * 22
                        color: "#1A456B"
                        border.color: "#2D5A83"
                        radius: 2

                        ColumnLayout {
                            anchors.fill: parent
                            anchors.margins: 4
                            spacing: 2

                            RowLayout {
                                Layout.fillWidth: true
                                spacing: 8
                                Text { Layout.fillWidth: true; color: Theme.textPrimary; text: "RTSP主题" }
                                Text { Layout.preferredWidth: 72; horizontalAlignment: Text.AlignRight; color: Theme.textPrimary; text: "帧数" }
                                Text { Layout.preferredWidth: 110; horizontalAlignment: Text.AlignRight; color: Theme.textPrimary; text: "字节" }
                            }

                            Repeater {
                                model: Math.min(currentRtspRows.length, 6)
                                delegate: RowLayout {
                                    Layout.fillWidth: true
                                    spacing: 8

                                    readonly property var rowData: currentRtspRows[index]

                                    Text {
                                        Layout.fillWidth: true
                                        color: Theme.textSecondary
                                        text: rowData && rowData.topic ? rowData.topic : ""
                                        elide: Text.ElideRight
                                    }
                                    Text {
                                        Layout.preferredWidth: 72
                                        horizontalAlignment: Text.AlignRight
                                        color: Theme.textSecondary
                                        text: rowData && rowData.frameCount !== undefined ? rowData.frameCount : 0
                                    }
                                    Text {
                                        Layout.preferredWidth: 110
                                        horizontalAlignment: Text.AlignRight
                                        color: Theme.textSecondary
                                        text: rowData && rowData.payloadBytesText ? rowData.payloadBytesText : "0 B"
                                    }
                                }
                            }
                        }
                    }
                }

                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: rowControlHeight + 10
                    color: "#1A456B"
                    border.color: "#2D5A83"
                    radius: 2

                    RowLayout {
                        anchors.fill: parent
                        anchors.margins: 5
                        spacing: 8

                        Text { color: Theme.textPrimary; text: "重演选项:" }

                        DarkTextField {
                            Layout.fillWidth: true
                            Layout.minimumWidth: 150
                            Layout.preferredHeight: rowControlHeight
                            Layout.alignment: Qt.AlignVCenter
                            placeholderText: "开始时间"
                            text: controllerReady ? recordReplayController.replayStartText : ""
                            onEditingFinished: if (controllerReady) recordReplayController.replayStartText = text.trim()
                        }

                        Text { color: Theme.textSecondary; text: "-" }

                        DarkTextField {
                            Layout.fillWidth: true
                            Layout.minimumWidth: 150
                            Layout.preferredHeight: rowControlHeight
                            Layout.alignment: Qt.AlignVCenter
                            placeholderText: "结束时间"
                            text: controllerReady ? recordReplayController.replayEndText : ""
                            onEditingFinished: if (controllerReady) recordReplayController.replayEndText = text.trim()
                        }

                        DarkCheckBox {
                            id: rangeCheck
                            text: "指定时间段"
                            checked: false
                            height: rowControlHeight
                            Layout.alignment: Qt.AlignVCenter
                        }

                        DarkButton {
                            Layout.preferredWidth: 132
                            Layout.preferredHeight: rowControlHeight
                            Layout.alignment: Qt.AlignVCenter
                            text: "截取时间段数据"
                            enabled: controllerReady && !recordReplayController.recording && !recordReplayController.replaying
                            onClicked: if (controllerReady) recordReplayController.clipSelectedRange()
                        }

                        Item { Layout.fillWidth: true }
                    }
                }

                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: rowControlHeight * 2 + 16
                    color: "#1A456B"
                    border.color: "#2D5A83"
                    radius: 2

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 5
                        spacing: 6

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 8

                            Text { color: Theme.textPrimary; text: "重演控制:" }

                            DarkComboBox {
                                Layout.preferredWidth: 140
                                Layout.preferredHeight: rowControlHeight
                                Layout.alignment: Qt.AlignVCenter
                                model: [controllerReady ? recordReplayController.replayModeText : "当前文件"]
                            }

                            DarkButton {
                                Layout.preferredWidth: 42
                                Layout.preferredHeight: rowControlHeight
                                Layout.alignment: Qt.AlignVCenter
                                text: "<<"
                                enabled: controllerReady && sessionCount > 1 && !recordReplayController.recording && !recordReplayController.replaying
                                onClicked: if (controllerReady) recordReplayController.selectPreviousSession()
                            }

                            DarkButton {
                                Layout.preferredWidth: 88
                                Layout.preferredHeight: rowControlHeight
                                Layout.alignment: Qt.AlignVCenter
                                text: controllerReady && recordReplayController.replaying ? "停止" : "重演"
                                enabled: controllerReady && !recordReplayController.recording
                                onClicked: {
                                    if (!controllerReady) return
                                    if (recordReplayController.replaying) {
                                        recordReplayController.stopReplay()
                                    } else {
                                        recordReplayController.startReplay(rangeCheck.checked)
                                    }
                                }
                            }

                            DarkButton {
                                Layout.preferredWidth: 42
                                Layout.preferredHeight: rowControlHeight
                                Layout.alignment: Qt.AlignVCenter
                                text: ">>"
                                enabled: controllerReady && sessionCount > 1 && !recordReplayController.recording && !recordReplayController.replaying
                                onClicked: if (controllerReady) recordReplayController.selectNextSession()
                            }

                            DarkButton {
                                Layout.preferredWidth: 32
                                Layout.preferredHeight: rowControlHeight
                                Layout.alignment: Qt.AlignVCenter
                                text: "-"
                                onClicked: if (controllerReady) recordReplayController.decreaseReplaySpeed()
                            }

                            Text {
                                Layout.preferredWidth: 56
                                horizontalAlignment: Text.AlignHCenter
                                color: Theme.textPrimary
                                text: controllerReady ? (recordReplayController.replaySpeed.toFixed(2) + "x") : "1.00x"
                            }

                            DarkButton {
                                Layout.preferredWidth: 32
                                Layout.preferredHeight: rowControlHeight
                                Layout.alignment: Qt.AlignVCenter
                                text: "+"
                                onClicked: if (controllerReady) recordReplayController.increaseReplaySpeed()
                            }

                            Item { Layout.fillWidth: true }
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 8

                            Text {
                                color: Theme.textSecondary
                                text: controllerReady ? ("重演时长: " + recordReplayController.replayElapsedText) : ""
                                Layout.alignment: Qt.AlignVCenter
                            }

                            Text {
                                color: Theme.textSecondary
                                text: "进度:"
                                Layout.alignment: Qt.AlignVCenter
                            }

                            Rectangle {
                                id: replaySeekTrack
                                Layout.fillWidth: true
                                Layout.minimumWidth: 140
                                Layout.preferredHeight: 10
                                radius: 2
                                color: "#123652"
                                border.color: "#2E618A"

                                Rectangle {
                                    anchors.left: parent.left
                                    anchors.verticalCenter: parent.verticalCenter
                                    width: parent.width * (replaySeekDragging ? replaySeekVisualProgress : (controllerReady ? recordReplayController.replayProgress : 0))
                                    height: parent.height
                                    radius: 2
                                    color: "#49A7E8"
                                }

                                Rectangle {
                                    width: 12
                                    height: 12
                                    radius: 6
                                    color: controllerReady && recordReplayController.replaying ? "#9CD7FF" : "#66829B"
                                    border.color: "#0E2B45"
                                    anchors.verticalCenter: parent.verticalCenter
                                    x: Math.max(0, Math.min(parent.width - width, (parent.width * (replaySeekDragging ? replaySeekVisualProgress : (controllerReady ? recordReplayController.replayProgress : 0))) - width / 2))
                                }

                                MouseArea {
                                    anchors.fill: parent
                                    enabled: controllerReady && recordReplayController.replaying
                                    hoverEnabled: enabled
                                    cursorShape: enabled ? Qt.PointingHandCursor : Qt.ArrowCursor

                                    onPressed: {
                                        replaySeekDragging = true
                                        root.updateReplaySeekFromPointer(mouse.x, replaySeekTrack.width, false)
                                    }

                                    onPositionChanged: {
                                        if (replaySeekDragging) {
                                            root.updateReplaySeekFromPointer(mouse.x, replaySeekTrack.width, false)
                                        }
                                    }

                                    onReleased: {
                                        if (!replaySeekDragging) return
                                        root.updateReplaySeekFromPointer(mouse.x, replaySeekTrack.width, true)
                                        replaySeekDragging = false
                                    }

                                    onCanceled: {
                                        replaySeekDragging = false
                                        if (controllerReady) {
                                            replaySeekVisualProgress = recordReplayController.replayProgress
                                        }
                                    }
                                }
                            }

                            Text {
                                Layout.preferredWidth: 48
                                horizontalAlignment: Text.AlignRight
                                color: Theme.textPrimary
                                text: controllerReady
                                      ? (replaySeekDragging
                                         ? (Math.round(root.clamp01(replaySeekVisualProgress) * 100) + "%")
                                         : recordReplayController.replayProgressText)
                                      : "0%"
                            }

                            Text {
                                Layout.preferredWidth: 72
                                elide: Text.ElideRight
                                color: Theme.textPrimary
                                text: controllerReady ? recordReplayController.recordingStateText : "未开始"
                            }
                        }
                    }
                }

                Rectangle {
                    id: topicFilterPanel
                    Layout.fillWidth: true
                    Layout.preferredHeight: rowControlHeight + 14
                    color: "#1A456B"
                    border.color: "#2D5A83"
                    radius: 2

                    RowLayout {
                        anchors.fill: parent
                        anchors.margins: 5
                        spacing: 10

                        Text {
                            color: Theme.textPrimary
                            text: "Topic过滤："
                            verticalAlignment: Text.AlignVCenter
                            Layout.alignment: Qt.AlignVCenter
                        }

                        Item {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            Layout.alignment: Qt.AlignVCenter

                            Flickable {
                                anchors.fill: parent
                                clip: true
                                contentWidth: topicRow.width
                                contentHeight: height
                                flickableDirection: Flickable.HorizontalFlick
                                boundsBehavior: Flickable.StopAtBounds
                                interactive: contentWidth > width

                                Row {
                                    id: topicRow
                                    x: 0
                                    y: Math.max(0, (parent.height - height) / 2)
                                    spacing: 14

                                    DarkCheckBox {
                                        id: allTopicCheck
                                        text: "全部"
                                        checked: true
                                        height: rowControlHeight
                                        onClicked: {
                                            if (checked) {
                                                syncingTopicChecks = true
                                                netTopicCheck.checked = true
                                                serialTopicCheck.checked = true
                                                canTopicCheck.checked = true
                                                rtspTopicCheck.checked = true
                                                replayTopicCheck.checked = true
                                                syncingTopicChecks = false
                                            }
                                            applyFilterFromChecks()
                                        }
                                    }

                                    DarkCheckBox {
                                        id: netTopicCheck
                                        text: "NET (net/)"
                                        height: rowControlHeight
                                        onClicked: {
                                            if (!checked) allTopicCheck.checked = false
                                            applyFilterFromChecks()
                                        }
                                    }

                                    DarkCheckBox {
                                        id: serialTopicCheck
                                        text: "SERIAL (serial/)"
                                        height: rowControlHeight
                                        onClicked: {
                                            if (!checked) allTopicCheck.checked = false
                                            applyFilterFromChecks()
                                        }
                                    }

                                    DarkCheckBox {
                                        id: canTopicCheck
                                        text: "CAN (can/)"
                                        height: rowControlHeight
                                        onClicked: {
                                            if (!checked) allTopicCheck.checked = false
                                            applyFilterFromChecks()
                                        }
                                    }

                                    DarkCheckBox {
                                        id: rtspTopicCheck
                                        text: "RTSP (rtsp/)"
                                        visible: rtspAvailable
                                        width: visible ? implicitWidth : 0
                                        height: visible ? rowControlHeight : 0
                                        onClicked: {
                                            if (!checked) allTopicCheck.checked = false
                                            applyFilterFromChecks()
                                        }
                                    }

                                    DarkCheckBox {
                                        id: replayTopicCheck
                                        text: "REPLAY (record_replay/)"
                                        height: rowControlHeight
                                        onClicked: {
                                            if (!checked) allTopicCheck.checked = false
                                            applyFilterFromChecks()
                                        }
                                    }
                                }

                                ScrollBar.horizontal: ScrollBar {
                                    policy: ScrollBar.AsNeeded
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}
