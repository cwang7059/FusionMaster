import QtQuick 2.14
import QtQuick.Controls 2.14
import QtQuick.Layouts 1.14
import QtQuick.Window 2.14
import UITheme 1.0

Rectangle {
    id: root
    color: Theme.cardBg
    radius: 0

    readonly property bool controllerReady: typeof busMonitorController !== "undefined" && busMonitorController !== null
    readonly property bool busReady: typeof messageBusService !== "undefined" && messageBusService !== null

    property var pluginStatsModel: []
    property var topicStatsModel: []
    property int busTotalBytes: 0
    property int busTotalCount: 0

    property bool recordRunning: controllerReady ? busMonitorController.busRecordRunning : false
    property string recordPath: controllerReady ? (busMonitorController.busRecordDefaultTarget || "") : ""

    property string detailPluginName: ""
    property bool detailReceiveEstimated: false
    property var detailPublishRows: []
    property var detailReceiveRows: []
    property var detailTimeLabels: []
    property var detailPublishCountSeries: []
    property var detailReceiveCountSeries: []
    property var detailPublishBytesSeries: []
    property var detailReceiveBytesSeries: []

    property string topicDetailName: ""
    property bool topicDetailReceiveEstimated: false
    property var topicDetailPublishRows: []
    property var topicDetailReceiveRows: []
    property var topicDetailTimeLabels: []
    property var topicDetailPublishCountSeries: []
    property var topicDetailReceiveCountSeries: []
    property var topicDetailPublishBytesSeries: []
    property var topicDetailReceiveBytesSeries: []

    function inferSender(source, topic) {
        var src = String(source || "").trim()
        var srcLower = src.toLowerCase()
        var isRuntimePlaceholder = (src === ""
                || srcLower === "runtime"
                || srcLower === "runtime_bus"
                || srcLower === "message_bus"
                || srcLower === "bus"
                || src === "\u8FD0\u884C\u65F6"
                || src === "\u8FD0\u884C\u65F6\u603B\u7EBF"
                || src === "\u6D88\u606F\u603B\u7EBF")
        if (!isRuntimePlaceholder) {
            return src
        }

        var t = String(topic || "").trim().toLowerCase()
        if (t.indexOf("net/") === 0) return "org_common_net_receiver"
        if (t.indexOf("serial/") === 0) return "org_common_serial_receiver"
        if (t.indexOf("can/") === 0) return "org_common_can_receiver"
        if (t.indexOf("rtsp/") === 0) return "org_common_rtsp_receiver"
        if (t.indexOf("ui/") === 0) return "window_manager/app"
        if (t.indexOf("record_replay/") === 0) return "org_common_record_replay"
        return "unknown"
    }

    function rebuildStats() {
        if (!controllerReady || !busMonitorController.busMessages) {
            pluginStatsModel = []
            topicStatsModel = []
            busTotalBytes = 0
            busTotalCount = 0
            return
        }

        var records = busMonitorController.busMessages
        var pluginMap = {}
        var topicMap = {}
        var pluginRows = []
        var topicRows = []
        var totalBytes = 0

        var nowMs = Date.now()
        var activeWindowMs = 3000

        for (var i = 0; i < records.length; ++i) {
            var rec = records[i] || {}
            var topic = rec.topic || "(unknown)"
            var sender = inferSender(rec.source || "", topic)
            var size = Number(rec.size || 0)
            var tsMs = Number(rec.timestampMs || 0)
            totalBytes += size

            if (!pluginMap[sender]) {
                pluginMap[sender] = {
                    name: sender,
                    publishCount: 0,
                    publishBytes: 0,
                    receiveCount: 0,
                    receiveBytes: 0,
                    publishActive: false,
                    receiveActive: false,
                    lastSeenMs: 0
                }
                pluginRows.push(pluginMap[sender])
            }

            if (!topicMap[topic]) {
                topicMap[topic] = {
                    name: topic,
                    publishCount: 0,
                    publishBytes: 0,
                    receiveCount: 0,
                    receiveBytes: 0,
                    publishActive: false,
                    receiveActive: false,
                    lastSeenMs: 0
                }
                topicRows.push(topicMap[topic])
            }

            pluginMap[sender].publishCount += 1
            pluginMap[sender].publishBytes += size
            pluginMap[sender].receiveCount += 1
            pluginMap[sender].receiveBytes += size
            if (tsMs > pluginMap[sender].lastSeenMs) {
                pluginMap[sender].lastSeenMs = tsMs
            }

            topicMap[topic].publishCount += 1
            topicMap[topic].publishBytes += size
            topicMap[topic].receiveCount += 1
            topicMap[topic].receiveBytes += size
            if (tsMs > topicMap[topic].lastSeenMs) {
                topicMap[topic].lastSeenMs = tsMs
            }
        }

        for (var p = 0; p < pluginRows.length; ++p) {
            var prow = pluginRows[p]
            var pActive = prow.lastSeenMs > 0 && (nowMs - prow.lastSeenMs) <= activeWindowMs
            prow.publishActive = pActive
            prow.receiveActive = pActive
        }

        for (var t = 0; t < topicRows.length; ++t) {
            var trow = topicRows[t]
            var tActive = trow.lastSeenMs > 0 && (nowMs - trow.lastSeenMs) <= activeWindowMs
            trow.publishActive = tActive
            trow.receiveActive = tActive
        }

        pluginRows.sort(function(a, b) { return b.publishCount - a.publishCount })
        topicRows.sort(function(a, b) { return b.publishCount - a.publishCount })

        pluginStatsModel = pluginRows
        topicStatsModel = topicRows
        busTotalBytes = totalBytes
        busTotalCount = records.length
    }

    function topicRowsFromMap(mapObject) {
        var rows = []
        var keys = Object.keys(mapObject)
        for (var i = 0; i < keys.length; ++i) {
            var topic = keys[i]
            rows.push({
                topic: topic,
                count: Number(mapObject[topic].count || 0),
                bytes: Number(mapObject[topic].bytes || 0)
            })
        }
        rows.sort(function(a, b) { return b.count - a.count })
        return rows
    }

    function pluginRowsFromMap(mapObject) {
        var rows = []
        var keys = Object.keys(mapObject)
        for (var i = 0; i < keys.length; ++i) {
            var plugin = keys[i]
            rows.push({
                plugin: plugin,
                count: Number(mapObject[plugin].count || 0),
                bytes: Number(mapObject[plugin].bytes || 0)
            })
        }
        rows.sort(function(a, b) { return b.count - a.count })
        return rows
    }

    function topicMatchesPrefix(topic, prefix) {
        var t = String(topic || "")
        var p = String(prefix || "").trim()
        if (p === "") {
            return true
        }
        return t.indexOf(p) === 0
    }

    function ensureCountRow(mapObject, key) {
        if (!mapObject[key]) {
            mapObject[key] = { count: 0, bytes: 0 }
        }
        return mapObject[key]
    }

    function ensureBucket(timeBuckets, label) {
        if (!timeBuckets[label]) {
            timeBuckets[label] = {
                publishCount: 0,
                publishBytes: 0,
                receiveCount: 0,
                receiveBytes: 0
            }
        }
        return timeBuckets[label]
    }

    function toTimeLabel(timestampText, timestampMs) {
        if (timestampText && String(timestampText).length >= 19) {
            return String(timestampText).substring(11, 19)
        }

        var ms = Number(timestampMs || 0)
        if (ms <= 0) {
            return "unknown"
        }

        var dt = new Date(ms)
        function pad(v) { return (v < 10 ? "0" : "") + v }
        return pad(dt.getHours()) + ":" + pad(dt.getMinutes()) + ":" + pad(dt.getSeconds())
    }

    function getSubscriptionStats() {
        if (!busReady || typeof messageBusService.subscriptionStats !== "function") {
            return []
        }
        var snapshot = messageBusService.subscriptionStats()
        return snapshot || []
    }

    function openPluginDetail(pluginName) {
        if (!controllerReady || !busMonitorController || !busMonitorController.busMessages) {
            return
        }
        if (!pluginName || pluginName.trim() === "") {
            return
        }

        var targetPlugin = pluginName.trim()
        var records = busMonitorController.busMessages
        var subscriptions = getSubscriptionStats()
        var publishMap = {}
        var receiveMap = {}
        var timeBuckets = {}
        var subscribedPrefixes = {}
        var subscribeAll = false

        for (var s = 0; s < subscriptions.length; ++s) {
            var sub = subscriptions[s] || {}
            var owner = String(sub.owner || "").trim()
            if (owner !== targetPlugin) {
                continue
            }
            var prefix = String(sub.topicPrefix || "").trim()
            if (prefix === "") {
                subscribeAll = true
            } else {
                subscribedPrefixes[prefix] = true
            }
        }

        var prefixList = Object.keys(subscribedPrefixes)

        for (var i = 0; i < records.length; ++i) {
            var rec = records[i] || {}
            var topic = rec.topic || "(unknown)"
            var sender = inferSender(rec.source || "", topic)
            var size = Math.max(0, Number(rec.size || 0))
            var label = toTimeLabel(rec.timestamp || "", rec.timestampMs || 0)

            if (sender === targetPlugin) {
                var pub = ensureCountRow(publishMap, topic)
                pub.count += 1
                pub.bytes += size

                var pubBucket = ensureBucket(timeBuckets, label)
                pubBucket.publishCount += 1
                pubBucket.publishBytes += size
            }

            var matchedReceive = subscribeAll
            if (!matchedReceive) {
                for (var p = 0; p < prefixList.length; ++p) {
                    if (topicMatchesPrefix(topic, prefixList[p])) {
                        matchedReceive = true
                        break
                    }
                }
            }

            if (matchedReceive) {
                var recv = ensureCountRow(receiveMap, topic)
                recv.count += 1
                recv.bytes += size

                var recvBucket = ensureBucket(timeBuckets, label)
                recvBucket.receiveCount += 1
                recvBucket.receiveBytes += size
            }
        }

        var publishRows = topicRowsFromMap(publishMap)
        var receiveRows = topicRowsFromMap(receiveMap)
        var labels = Object.keys(timeBuckets).sort()
        var pubCountSeries = []
        var recvCountSeries = []
        var pubBytesSeries = []
        var recvBytesSeries = []

        for (var m = 0; m < labels.length; ++m) {
            var bucket = timeBuckets[labels[m]]
            pubCountSeries.push(Number(bucket.publishCount || 0))
            recvCountSeries.push(Number(bucket.receiveCount || 0))
            pubBytesSeries.push(Number(bucket.publishBytes || 0))
            recvBytesSeries.push(Number(bucket.receiveBytes || 0))
        }

        detailPluginName = targetPlugin
        detailReceiveEstimated = false
        detailPublishRows = publishRows
        detailReceiveRows = receiveRows
        detailTimeLabels = labels
        detailPublishCountSeries = pubCountSeries
        detailReceiveCountSeries = recvCountSeries
        detailPublishBytesSeries = pubBytesSeries
        detailReceiveBytesSeries = recvBytesSeries

        busPluginDetailWindow.openWindow()
    }

    function openTopicDetail(topicName) {
        if (!controllerReady || !busMonitorController || !busMonitorController.busMessages) {
            return
        }
        if (!topicName || topicName.trim() === "") {
            return
        }

        var targetTopic = topicName.trim()
        var records = busMonitorController.busMessages
        var subscriptions = getSubscriptionStats()
        var publishMap = {}
        var receiveMap = {}
        var receiverPluginSet = {}
        var timeBuckets = {}

        for (var s = 0; s < subscriptions.length; ++s) {
            var sub = subscriptions[s] || {}
            var owner = String(sub.owner || "").trim()
            if (owner === "") {
                owner = "unknown"
            }
            if (topicMatchesPrefix(targetTopic, sub.topicPrefix || "")) {
                receiverPluginSet[owner] = true
            }
        }

        var receiverPlugins = Object.keys(receiverPluginSet)

        for (var i = 0; i < records.length; ++i) {
            var rec = records[i] || {}
            var topic = rec.topic || "(unknown)"
            if (topic !== targetTopic) {
                continue
            }

            var sender = inferSender(rec.source || "", topic)
            var size = Math.max(0, Number(rec.size || 0))
            var label = toTimeLabel(rec.timestamp || "", rec.timestampMs || 0)
            var pub = ensureCountRow(publishMap, sender)
            pub.count += 1
            pub.bytes += size

            var bucket = ensureBucket(timeBuckets, label)
            bucket.publishCount += 1
            bucket.publishBytes += size

            for (var r = 0; r < receiverPlugins.length; ++r) {
                var recvPlugin = receiverPlugins[r]
                var recv = ensureCountRow(receiveMap, recvPlugin)
                recv.count += 1
                recv.bytes += size
                bucket.receiveCount += 1
                bucket.receiveBytes += size
            }
        }

        if (Object.keys(receiveMap).length === 0) {
            for (var x = 0; x < receiverPlugins.length; ++x) {
                ensureCountRow(receiveMap, receiverPlugins[x])
            }
        }

        var publishRows = pluginRowsFromMap(publishMap)
        var receiveRows = pluginRowsFromMap(receiveMap)
        var labels = Object.keys(timeBuckets).sort()
        var pubCountSeries = []
        var recvCountSeries = []
        var pubBytesSeries = []
        var recvBytesSeries = []

        for (var k = 0; k < labels.length; ++k) {
            var b = timeBuckets[labels[k]]
            pubCountSeries.push(Number(b.publishCount || 0))
            recvCountSeries.push(Number(b.receiveCount || 0))
            pubBytesSeries.push(Number(b.publishBytes || 0))
            recvBytesSeries.push(Number(b.receiveBytes || 0))
        }

        topicDetailName = targetTopic
        topicDetailReceiveEstimated = false
        topicDetailPublishRows = publishRows
        topicDetailReceiveRows = receiveRows
        topicDetailTimeLabels = labels
        topicDetailPublishCountSeries = pubCountSeries
        topicDetailReceiveCountSeries = recvCountSeries
        topicDetailPublishBytesSeries = pubBytesSeries
        topicDetailReceiveBytesSeries = recvBytesSeries

        busTopicDetailWindow.openWindow()
    }

    function metricsText() {
        if (!busReady) {
            return "MessageBus unavailable"
        }
        return messageBusService.metricsSummary()
    }

    Component.onCompleted: {
        rebuildStats()
        if (controllerReady && busMonitorController.busRecordDefaultTarget) {
            recordPath = busMonitorController.busRecordDefaultTarget
        }
    }

    Connections {
        target: controllerReady ? busMonitorController : null

        function onBusMessagesChanged() {
            rebuildStats()
            if (busPluginDetailWindow.visible && String(detailPluginName || "").trim() !== "") {
                openPluginDetail(detailPluginName)
            }
            if (busTopicDetailWindow.visible && String(topicDetailName || "").trim() !== "") {
                openTopicDetail(topicDetailName)
            }
        }
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
                text: "消息总线"
                color: Theme.textPrimary
                font.pixelSize: Theme.fontSizeLarge
                font.bold: true
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 44
            color: Theme.inputBg
            border.color: Theme.inputBorder
            radius: 2

            RowLayout {
                anchors.fill: parent
                anchors.margins: 6
                spacing: 8

                Text {
                    text: "状态:"
                    color: Theme.textPrimary
                }

                Text {
                    Layout.preferredWidth: 72
                    color: recordRunning ? Theme.success : Theme.textSecondary
                    text: controllerReady ? busMonitorController.busRecordState : "未开始"
                    verticalAlignment: Text.AlignVCenter
                }

                DarkTextField {
                    id: recordPathField
                    Layout.fillWidth: true
                    text: root.recordPath
                    onTextChanged: root.recordPath = text
                }

                DarkButton {
                    Layout.preferredWidth: 96
                    text: "选择目录"
                    onClicked: {
                        if (controllerReady && typeof busMonitorController.pickRecordDirectory === "function") {
                            var path = busMonitorController.pickRecordDirectory()
                            if (path) {
                                recordPathField.text = path
                            }
                        }
                    }
                }

                DarkButton {
                    Layout.preferredWidth: 96
                    text: "开始记录"
                    enabled: controllerReady && !recordRunning
                    onClicked: {
                        if (controllerReady && typeof busMonitorController.startBusRecord === "function") {
                            busMonitorController.startBusRecord(recordPathField.text.trim())
                        }
                    }
                }

                DarkButton {
                    Layout.preferredWidth: 96
                    text: "结束记录"
                    enabled: controllerReady && recordRunning
                    onClicked: {
                        if (controllerReady && typeof busMonitorController.stopBusRecord === "function") {
                            busMonitorController.stopBusRecord()
                        }
                    }
                }

                DarkButton {
                    Layout.preferredWidth: 96
                    text: "清空计数"
                    onClicked: {
                        if (controllerReady && typeof busMonitorController.clear === "function") {
                            busMonitorController.clear()
                        }
                    }
                }
            }
        }

        Text {
            Layout.fillWidth: true
            color: Theme.textSecondary
            text: "消息总数: " + busTotalCount
                  + "    |    总字节: " + busTotalBytes
                  + "    |    " + metricsText()
        }

        Text {
            Layout.fillWidth: true
            visible: controllerReady
            color: Theme.textSecondary
            elide: Text.ElideRight
            text: recordRunning
                  ? ("记录文件: " + (busMonitorController.busRecordFilePath || ""))
                  : ("记录标识: " + recordPathField.text)
        }

        Text {
            Layout.fillWidth: true
            color: Theme.textSecondary
            text: "提示：双击“插件统计”或“主题统计”中的任意行，可查看详情窗口。"
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: Theme.spacingSmall

            BusStatTable {
                Layout.fillWidth: true
                Layout.fillHeight: true
                titleText: "插件统计"
                nameHeader: "插件"
                statsModel: pluginStatsModel
                nameColumnWidth: 170
                enableRowDoubleClick: true
                onRowDoubleClicked: openPluginDetail(rowData.name || "")
            }

            BusStatTable {
                Layout.fillWidth: true
                Layout.fillHeight: true
                titleText: "主题统计"
                nameHeader: "主题"
                statsModel: topicStatsModel
                nameColumnWidth: 170
                enableRowDoubleClick: true
                onRowDoubleClicked: openTopicDetail(rowData.name || "")
            }
        }
    }

    BusPluginDetailWindow {
        id: busPluginDetailWindow
        transientParent: root.Window.window
        pluginName: detailPluginName
        receiveEstimated: detailReceiveEstimated
        publishRows: detailPublishRows
        receiveRows: detailReceiveRows
        timeLabels: detailTimeLabels
        publishCountSeries: detailPublishCountSeries
        receiveCountSeries: detailReceiveCountSeries
        publishBytesSeries: detailPublishBytesSeries
        receiveBytesSeries: detailReceiveBytesSeries
    }

    BusTopicDetailWindow {
        id: busTopicDetailWindow
        transientParent: root.Window.window
        topicName: topicDetailName
        receiveEstimated: topicDetailReceiveEstimated
        publishRows: topicDetailPublishRows
        receiveRows: topicDetailReceiveRows
        timeLabels: topicDetailTimeLabels
        publishCountSeries: topicDetailPublishCountSeries
        receiveCountSeries: topicDetailReceiveCountSeries
        publishBytesSeries: topicDetailPublishBytesSeries
        receiveBytesSeries: topicDetailReceiveBytesSeries
    }
}
