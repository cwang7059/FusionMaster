import QtQuick 2.14
import QtQuick.Controls 2.14
import QtQuick.Layouts 1.14
import UITheme 1.0
import HistoryFusionAnalysis 1.0

Rectangle {
    id: root
    color: Theme.cardBg
    radius: 0
    clip: true

    readonly property bool controllerReady: HistoryFusionController !== null
    readonly property var summary: controllerReady ? (HistoryFusionController.summary || ({})) : ({})
    readonly property int controlHeight: 28

    function pct(value) {
        return (Number(value || 0) * 100).toFixed(1) + "%"
    }

    function num(value, digits) {
        return Number(value || 0).toFixed(digits === undefined ? 1 : digits)
    }

    function statusColor(status) {
        if (status === "完成" || status === "已生成" || status === "已就绪"
                || status === "可用" || status === "通过" || status === "已登记"
                || status === "已归档" || status === "已导出") {
            return "#82D4A7"
        }
        if (status === "失败" || status === "需处理" || status === "阻断") {
            return "#F28B82"
        }
        if (status === "接口预留" || status === "待复核" || status === "待权限校验"
                || status === "降级") {
            return "#F0CC6A"
        }
        return "#8EC5F8"
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: Theme.spacingNormal
        spacing: Theme.spacingSmall

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 124
            color: Theme.inputBg
            border.color: Theme.inputBorder
            radius: 2

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 8
                spacing: 6

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 8

                    Text {
                        Layout.preferredWidth: 188
                        color: Theme.textPrimary
                        font.pixelSize: Theme.fontSizeLarge
                        font.bold: true
                        text: "试验数据智能分析工作台"
                        elide: Text.ElideRight
                    }

                    TextField {
                        Layout.preferredWidth: 220
                        Layout.preferredHeight: root.controlHeight
                        text: controllerReady ? HistoryFusionController.taskName : ""
                        enabled: controllerReady && !HistoryFusionController.running
                        placeholderText: "任务名称"
                        selectByMouse: true
                        onEditingFinished: if (controllerReady) HistoryFusionController.taskName = text
                    }

                    TextField {
                        Layout.preferredWidth: 120
                        Layout.preferredHeight: root.controlHeight
                        text: controllerReady ? HistoryFusionController.owner : ""
                        enabled: controllerReady && !HistoryFusionController.running
                        placeholderText: "责任人"
                        selectByMouse: true
                        onEditingFinished: if (controllerReady) HistoryFusionController.owner = text
                    }

                    ComboBox {
                        Layout.preferredWidth: 108
                        Layout.preferredHeight: root.controlHeight
                        enabled: controllerReady && !HistoryFusionController.running
                        model: ["文件导入", "历史数据库", "实时接收"]
                        currentIndex: controllerReady ? Math.max(0, find(HistoryFusionController.dataSourceType)) : 0
                        onActivated: if (controllerReady) HistoryFusionController.dataSourceType = currentText
                    }

                    SpinBox {
                        Layout.preferredWidth: 82
                        Layout.preferredHeight: root.controlHeight
                        from: 1
                        to: 50
                        value: controllerReady ? HistoryFusionController.minBatchRequired : 5
                        enabled: controllerReady && !HistoryFusionController.running
                        onValueChanged: if (controllerReady) HistoryFusionController.minBatchRequired = value
                    }

                    Button {
                        Layout.preferredWidth: 82
                        Layout.preferredHeight: root.controlHeight
                        text: "选择文件"
                        enabled: controllerReady && !HistoryFusionController.running
                        onClicked: {
                            var path = HistoryFusionController.chooseImportFile()
                            if (path && path.length > 0) HistoryFusionController.importFile(path)
                        }
                    }

                    Button {
                        Layout.preferredWidth: 82
                        Layout.preferredHeight: root.controlHeight
                        text: "示例数据"
                        enabled: controllerReady && !HistoryFusionController.running
                        onClicked: HistoryFusionController.loadDemoDataset()
                    }

                    Button {
                        Layout.preferredWidth: 86
                        Layout.preferredHeight: root.controlHeight
                        text: HistoryFusionController.running ? "分析中" : "开始分析"
                        enabled: controllerReady && !HistoryFusionController.running
                        onClicked: HistoryFusionController.startAnalysis()
                    }

                    Button {
                        Layout.preferredWidth: 58
                        Layout.preferredHeight: root.controlHeight
                        text: "重置"
                        enabled: controllerReady && !HistoryFusionController.running
                        onClicked: HistoryFusionController.resetTask()
                    }
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 8

                    TextField {
                        Layout.preferredWidth: 168
                        Layout.preferredHeight: root.controlHeight
                        text: controllerReady ? HistoryFusionController.filterLocation : ""
                        enabled: controllerReady && !HistoryFusionController.running
                        placeholderText: "地点筛选"
                        selectByMouse: true
                        onEditingFinished: if (controllerReady) HistoryFusionController.filterLocation = text
                    }

                    TextField {
                        Layout.preferredWidth: 168
                        Layout.preferredHeight: root.controlHeight
                        text: controllerReady ? HistoryFusionController.filterTargetType : ""
                        enabled: controllerReady && !HistoryFusionController.running
                        placeholderText: "目标类型筛选"
                        selectByMouse: true
                        onEditingFinished: if (controllerReady) HistoryFusionController.filterTargetType = text
                    }

                    TextField {
                        Layout.preferredWidth: 142
                        Layout.preferredHeight: root.controlHeight
                        text: controllerReady ? HistoryFusionController.filterResult : ""
                        enabled: controllerReady && !HistoryFusionController.running
                        placeholderText: "结果筛选"
                        selectByMouse: true
                        onEditingFinished: if (controllerReady) HistoryFusionController.filterResult = text
                    }

                    Text {
                        Layout.fillWidth: true
                        color: Theme.textSecondary
                        elide: Text.ElideRight
                        text: controllerReady
                              ? ("数据: " + (HistoryFusionController.importPath || "-")
                                 + "    记录: " + HistoryFusionController.recordCount
                                 + "    批次: " + HistoryFusionController.batchCount)
                              : ""
                    }
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 8

                    Text {
                        Layout.preferredWidth: 92
                        color: Theme.textPrimary
                        font.bold: true
                        text: controllerReady ? HistoryFusionController.taskStateText : "未加载"
                        elide: Text.ElideRight
                    }

                    ProgressBar {
                        Layout.preferredWidth: 180
                        Layout.preferredHeight: 10
                        from: 0
                        to: 1
                        value: controllerReady ? HistoryFusionController.progress : 0
                    }

                    Text {
                        Layout.preferredWidth: 44
                        color: Theme.textSecondary
                        text: controllerReady ? HistoryFusionController.progressText : "0%"
                    }

                    Text {
                        Layout.fillWidth: true
                        color: "#8EC5F8"
                        elide: Text.ElideRight
                        text: controllerReady ? HistoryFusionController.feedback : ""
                    }
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 72
            color: "#17384D"
            border.color: Theme.inputBorder
            radius: 2

            RowLayout {
                anchors.fill: parent
                anchors.margins: 8
                spacing: 10

                Repeater {
                    model: [
                        { label: "质量评分", value: num(summary.qualityScore, 1), sub: summary.qualityGrade || "-" },
                        { label: "成功率", value: pct(summary.overallSuccessRate), sub: "总体" },
                        { label: "有效批次", value: String(summary.usedBatchCount || 0), sub: "要求 " + (controllerReady ? HistoryFusionController.minBatchRequired : 5) },
                        { label: "关键因子", value: summary.keyFactor || "-", sub: "贡献最高" },
                        { label: "推荐工况", value: summary.bestCondition || "-", sub: summary.reviewRequired ? "待复核" : "可归档" },
                        { label: "发布回执", value: summary.receiptCode || "-", sub: summary.publishState || "未发布" }
                    ]

                    delegate: Rectangle {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        color: "#102F42"
                        border.color: "#31566B"
                        radius: 2

                        ColumnLayout {
                            anchors.fill: parent
                            anchors.margins: 6
                            spacing: 1

                            Text {
                                Layout.fillWidth: true
                                color: Theme.textSecondary
                                font.pixelSize: Theme.fontSizeSmall
                                text: modelData.label
                                elide: Text.ElideRight
                            }
                            Text {
                                Layout.fillWidth: true
                                color: Theme.textPrimary
                                font.pixelSize: Theme.fontSizeLarge
                                font.bold: true
                                text: modelData.value
                                elide: Text.ElideRight
                            }
                            Text {
                                Layout.fillWidth: true
                                color: "#8EC5F8"
                                font.pixelSize: Theme.fontSizeSmall
                                text: modelData.sub
                                elide: Text.ElideRight
                            }
                        }
                    }
                }
            }
        }

        TabBar {
            id: tabBar
            Layout.fillWidth: true
            Layout.preferredHeight: 34

            TabButton { text: "总览"; width: 78 }
            TabButton { text: "数据"; width: 78 }
            TabButton { text: "融合"; width: 78 }
            TabButton { text: "算法/模型"; width: 98 }
            TabButton { text: "统计"; width: 78 }
            TabButton { text: "存储"; width: 78 }
            TabButton { text: "报告"; width: 78 }
            TabButton { text: "环境"; width: 78 }
        }

        StackLayout {
            id: stack
            Layout.fillWidth: true
            Layout.fillHeight: true
            currentIndex: tabBar.currentIndex

            Item {
                SplitView {
                    anchors.fill: parent
                    orientation: Qt.Horizontal
                    handle: Rectangle { implicitWidth: 1; color: Theme.inputBorder }

                    ColumnLayout {
                        SplitView.preferredWidth: 390
                        SplitView.minimumWidth: 320
                        spacing: 6

                        AnalysisPanel {
                            Layout.fillWidth: true
                            Layout.preferredHeight: 196
                            title: "阶段状态"
                            rows: controllerReady ? HistoryFusionController.stageRows : []
                            delegateComponent: stageDelegate
                        }

                        AnalysisPanel {
                            Layout.fillWidth: true
                            Layout.preferredHeight: 206
                            title: "质量评估"
                            rows: controllerReady ? HistoryFusionController.qualityRows : []
                            delegateComponent: qualityDelegate
                        }

                        AnalysisPanel {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            title: "审计留痕"
                            rows: controllerReady ? HistoryFusionController.auditRows : []
                            delegateComponent: auditDelegate
                        }
                    }

                    ColumnLayout {
                        SplitView.fillWidth: true
                        spacing: 6

                        AnalysisPanel {
                            Layout.fillWidth: true
                            Layout.preferredHeight: 190
                            title: "报告发布状态"
                            rows: controllerReady ? HistoryFusionController.reportRows : []
                            delegateComponent: reportDelegate
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            spacing: 6

                            AnalysisPanel {
                                Layout.fillWidth: true
                                Layout.fillHeight: true
                                title: "关键因子"
                                rows: controllerReady ? HistoryFusionController.factorRows : []
                                delegateComponent: factorDelegate
                            }

                            AnalysisPanel {
                                Layout.fillWidth: true
                                Layout.fillHeight: true
                                title: "工况排序"
                                rows: controllerReady ? HistoryFusionController.conditionRows : []
                                delegateComponent: conditionDelegate
                            }
                        }
                    }
                }
            }

            Item {
                SplitView {
                    anchors.fill: parent
                    orientation: Qt.Horizontal
                    handle: Rectangle { implicitWidth: 1; color: Theme.inputBorder }

                    ColumnLayout {
                        SplitView.preferredWidth: 520
                        SplitView.minimumWidth: 380
                        spacing: 6

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 6

                            Button {
                                Layout.preferredWidth: 86
                                Layout.preferredHeight: root.controlHeight
                                text: "刷新目录"
                                enabled: controllerReady
                                onClicked: HistoryFusionController.refreshDataCatalog()
                            }

                            Button {
                                Layout.preferredWidth: 86
                                Layout.preferredHeight: root.controlHeight
                                text: "保存快照"
                                enabled: controllerReady
                                onClicked: HistoryFusionController.saveCurrentTask()
                            }

                            TextField {
                                id: archivedTaskField
                                Layout.preferredWidth: 180
                                Layout.preferredHeight: root.controlHeight
                                placeholderText: "归档任务编号"
                                selectByMouse: true
                            }

                            Button {
                                Layout.preferredWidth: 78
                                Layout.preferredHeight: root.controlHeight
                                text: "调取"
                                enabled: controllerReady && archivedTaskField.text.length > 0
                                onClicked: HistoryFusionController.loadArchivedTask(archivedTaskField.text)
                            }

                            Button {
                                Layout.preferredWidth: 86
                                Layout.preferredHeight: root.controlHeight
                                text: "导出数据"
                                enabled: controllerReady && HistoryFusionController.recordCount > 0
                                onClicked: HistoryFusionController.exportCurrentDataset()
                            }

                            Text {
                                Layout.fillWidth: true
                                color: Theme.textSecondary
                                elide: Text.ElideRight
                                text: "本地仓储: " + (controllerReady ? HistoryFusionController.storageRoot : "-")
                            }
                        }

                        AnalysisPanel {
                            Layout.fillWidth: true
                            Layout.preferredHeight: 250
                            title: "数据调取接口"
                            rows: controllerReady ? HistoryFusionController.dataAccessRows : []
                            delegateComponent: dataAccessDelegate
                        }

                        AnalysisPanel {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            title: "本地归档目录"
                            rows: controllerReady ? HistoryFusionController.catalogRows : []
                            delegateComponent: catalogDelegate
                        }
                    }

                    ColumnLayout {
                        SplitView.fillWidth: true
                        spacing: 6

                        AnalysisPanel {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            title: "调取明细记录"
                            rows: controllerReady ? HistoryFusionController.detailRows : []
                            delegateComponent: detailDelegate
                        }

                        AnalysisPanel {
                            Layout.fillWidth: true
                            Layout.preferredHeight: 160
                            title: "待发/补发队列"
                            rows: controllerReady ? HistoryFusionController.outboundRows : []
                            delegateComponent: outboundDelegate
                        }

                        AnalysisPanel {
                            Layout.fillWidth: true
                            Layout.preferredHeight: 160
                            title: "调取异常与质量标记"
                            rows: controllerReady ? HistoryFusionController.issueRows : []
                            delegateComponent: issueDelegate
                        }
                    }
                }
            }

            Item {
                ColumnLayout {
                    anchors.fill: parent
                    spacing: 6

                    AnalysisPanel {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 210
                        title: "多批次融合对比"
                        rows: controllerReady ? HistoryFusionController.batchRows : []
                        delegateComponent: batchDelegate
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        spacing: 6

                        AnalysisPanel {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            title: "关键影响因子"
                            rows: controllerReady ? HistoryFusionController.factorRows : []
                            delegateComponent: factorDelegate
                        }

                        AnalysisPanel {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            title: "推荐工况排序"
                            rows: controllerReady ? HistoryFusionController.conditionRows : []
                            delegateComponent: conditionDelegate
                        }

                        AnalysisPanel {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            title: "异常追溯"
                            rows: controllerReady ? HistoryFusionController.issueRows : []
                            delegateComponent: issueDelegate
                        }
                    }
                }
            }

            Item {
                ColumnLayout {
                    anchors.fill: parent
                    spacing: 6

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 6

                        Button {
                            Layout.preferredWidth: 126
                            Layout.preferredHeight: root.controlHeight
                            text: "刷新算法接口"
                            enabled: controllerReady
                            onClicked: HistoryFusionController.runAlgorithmOptimization()
                        }

                        Button {
                            Layout.preferredWidth: 126
                            Layout.preferredHeight: root.controlHeight
                            text: "刷新模型接口"
                            enabled: controllerReady
                            onClicked: HistoryFusionController.runModelIdentification()
                        }

                        Text {
                            Layout.fillWidth: true
                            color: "#F0CC6A"
                            elide: Text.ElideRight
                            text: "算法和模型当前按接口预留，不在本版本执行真实优化或辨识计算"
                        }
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        spacing: 6

                        AnalysisPanel {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            title: "算法自主优化接口"
                            rows: controllerReady ? HistoryFusionController.algorithmRows : []
                            delegateComponent: contractDelegate
                        }

                        AnalysisPanel {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            title: "模型辅助辨识接口"
                            rows: controllerReady ? HistoryFusionController.modelRows : []
                            delegateComponent: contractDelegate
                        }
                    }
                }
            }

            Item {
                ColumnLayout {
                    anchors.fill: parent
                    spacing: 6

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 6

                        Button {
                            Layout.preferredWidth: 108
                            Layout.preferredHeight: root.controlHeight
                            text: "刷新统计"
                            enabled: controllerReady
                            onClicked: HistoryFusionController.runStatistics()
                        }

                        Text {
                            Layout.fillWidth: true
                            color: Theme.textSecondary
                            elide: Text.ElideRight
                            text: "统计结果按任务级、批次级、目标级和时间窗口级口径组织"
                        }
                    }

                    AnalysisPanel {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        title: "试验数据统计"
                        rows: controllerReady ? HistoryFusionController.statisticsRows : []
                        delegateComponent: statisticsDelegate
                    }
                }
            }

            Item {
                RowLayout {
                    anchors.fill: parent
                    spacing: 6

                    AnalysisPanel {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        title: "数据库分层与字段"
                        rows: controllerReady ? HistoryFusionController.storageRows : []
                        delegateComponent: storageDelegate
                    }

                    AnalysisPanel {
                        Layout.preferredWidth: 430
                        Layout.fillHeight: true
                        title: "审计记录"
                        rows: controllerReady ? HistoryFusionController.auditRows : []
                        delegateComponent: auditDelegate
                    }
                }
            }

            Item {
                ColumnLayout {
                    anchors.fill: parent
                    spacing: 6

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 6

                        Button {
                            Layout.preferredWidth: 96
                            Layout.preferredHeight: root.controlHeight
                            text: "导出报告"
                            enabled: controllerReady && !HistoryFusionController.running
                                     && ((summary.conclusion || "") !== "")
                            onClicked: HistoryFusionController.exportReport()
                        }

                        Button {
                            Layout.preferredWidth: 96
                            Layout.preferredHeight: root.controlHeight
                            text: "登记发布"
                            enabled: controllerReady && !HistoryFusionController.running
                                     && ((summary.conclusion || "") !== "")
                            onClicked: HistoryFusionController.publishResult()
                        }

                        Button {
                            Layout.preferredWidth: 96
                            Layout.preferredHeight: root.controlHeight
                            text: "结果归档"
                            enabled: controllerReady && !HistoryFusionController.running
                                     && ((summary.conclusion || "") !== "")
                            onClicked: HistoryFusionController.archiveResult()
                        }

                        Button {
                            Layout.preferredWidth: 96
                            Layout.preferredHeight: root.controlHeight
                            text: "保存快照"
                            enabled: controllerReady && !HistoryFusionController.running
                                     && HistoryFusionController.recordCount > 0
                            onClicked: HistoryFusionController.saveCurrentTask()
                        }

                        Button {
                            Layout.preferredWidth: 96
                            Layout.preferredHeight: root.controlHeight
                            text: "补发队列"
                            enabled: controllerReady && !HistoryFusionController.running
                            onClicked: HistoryFusionController.retryPendingPublications()
                        }

                        Text {
                            Layout.fillWidth: true
                            color: Theme.textSecondary
                            elide: Text.ElideRight
                            text: "导出、发布、归档均写入审计；发布记录进入本地待发队列"
                        }
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        spacing: 6

                        AnalysisPanel {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            title: "报告模板与发布回执"
                            rows: controllerReady ? HistoryFusionController.reportRows : []
                            delegateComponent: reportDelegate
                        }

                        AnalysisPanel {
                            Layout.preferredWidth: 430
                            Layout.fillHeight: true
                            title: "待发/补发队列"
                            rows: controllerReady ? HistoryFusionController.outboundRows : []
                            delegateComponent: outboundDelegate
                        }
                    }
                }
            }

            Item {
                ColumnLayout {
                    anchors.fill: parent
                    spacing: 6

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 6

                        Button {
                            Layout.preferredWidth: 108
                            Layout.preferredHeight: root.controlHeight
                            text: "运行自检"
                            enabled: controllerReady
                            onClicked: HistoryFusionController.runEnvironmentSelfCheck()
                        }

                        Text {
                            Layout.fillWidth: true
                            color: Theme.textSecondary
                            elide: Text.ElideRight
                            text: "覆盖服务器部署、数据库接口、文件目录、消息链路、日志审计和版本规则"
                        }
                    }

                    AnalysisPanel {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        title: "运行环境兼容与联调校核"
                        rows: controllerReady ? HistoryFusionController.environmentRows : []
                        delegateComponent: environmentDelegate
                    }
                }
            }
        }
    }

    Component {
        id: stageDelegate
        RowLayout {
            width: ListView.view.width
            height: 26
            spacing: 8
            Text { Layout.preferredWidth: 78; color: Theme.textPrimary; text: modelData.name || ""; elide: Text.ElideRight }
            Text { Layout.preferredWidth: 54; color: statusColor(modelData.status); text: modelData.status || ""; elide: Text.ElideRight }
            Text { Layout.fillWidth: true; color: Theme.textSecondary; text: modelData.output || ""; elide: Text.ElideRight }
            Text { Layout.preferredWidth: 54; horizontalAlignment: Text.AlignRight; color: Theme.textSecondary; text: modelData.durationText || "-"; elide: Text.ElideRight }
        }
    }

    Component {
        id: qualityDelegate
        RowLayout {
            width: ListView.view.width
            height: 28
            spacing: 8
            Text { Layout.preferredWidth: 82; color: Theme.textPrimary; text: modelData.dimension || ""; elide: Text.ElideRight }
            Text { Layout.preferredWidth: 54; horizontalAlignment: Text.AlignRight; color: Theme.textPrimary; text: modelData.scoreText || "0"; elide: Text.ElideRight }
            Text { Layout.preferredWidth: 72; color: "#8EC5F8"; text: modelData.grade || ""; elide: Text.ElideRight }
            Text { Layout.fillWidth: true; color: Theme.textSecondary; text: modelData.detail || ""; elide: Text.ElideRight }
        }
    }

    Component {
        id: batchDelegate
        RowLayout {
            width: ListView.view.width
            height: 28
            spacing: 8
            Text { Layout.preferredWidth: 92; color: Theme.textPrimary; text: modelData.batchNo || ""; elide: Text.ElideRight }
            Text { Layout.preferredWidth: 42; horizontalAlignment: Text.AlignRight; color: Theme.textSecondary; text: modelData.sampleCount || 0 }
            Text { Layout.preferredWidth: 112; color: Theme.textSecondary; text: modelData.modalities || ""; elide: Text.ElideRight }
            Text { Layout.preferredWidth: 72; horizontalAlignment: Text.AlignRight; color: Theme.textPrimary; text: pct(modelData.successRate) }
            Text { Layout.preferredWidth: 70; horizontalAlignment: Text.AlignRight; color: "#8EC5F8"; text: num(modelData.qualityScore, 1) }
            Text { Layout.fillWidth: true; color: Theme.textSecondary; text: modelData.targetTypes || ""; elide: Text.ElideRight }
        }
    }

    Component {
        id: factorDelegate
        RowLayout {
            width: ListView.view.width
            height: 28
            spacing: 8
            Text { Layout.preferredWidth: 66; color: Theme.textPrimary; text: modelData.factor || ""; elide: Text.ElideRight }
            Text { Layout.fillWidth: true; color: Theme.textSecondary; text: modelData.level || ""; elide: Text.ElideRight }
            Text { Layout.preferredWidth: 58; horizontalAlignment: Text.AlignRight; color: "#8EC5F8"; text: num(modelData.contribution, 1) + "%" }
            Text { Layout.preferredWidth: 66; color: Theme.textPrimary; text: modelData.suggestion || ""; elide: Text.ElideRight }
        }
    }

    Component {
        id: conditionDelegate
        RowLayout {
            width: ListView.view.width
            height: 32
            spacing: 8
            Text { Layout.fillWidth: true; color: Theme.textSecondary; text: modelData.condition || ""; elide: Text.ElideRight }
            Text { Layout.preferredWidth: 56; horizontalAlignment: Text.AlignRight; color: Theme.textPrimary; text: num(modelData.fusionScore, 1) }
            Text { Layout.preferredWidth: 66; color: "#8EC5F8"; text: modelData.conclusion || ""; elide: Text.ElideRight }
        }
    }

    Component {
        id: issueDelegate
        RowLayout {
            width: ListView.view.width
            height: 26
            spacing: 8
            Text { Layout.preferredWidth: 88; color: Theme.textPrimary; text: modelData.batchNo || ""; elide: Text.ElideRight }
            Text { Layout.preferredWidth: 72; color: Theme.textSecondary; text: modelData.targetType || ""; elide: Text.ElideRight }
            Text { Layout.fillWidth: true; color: "#F0CC6A"; text: modelData.issue || ""; elide: Text.ElideRight }
        }
    }

    Component {
        id: dataAccessDelegate
        ColumnLayout {
            width: ListView.view.width
            height: 58
            spacing: 2
            RowLayout {
                Layout.fillWidth: true
                Text { Layout.preferredWidth: 112; color: Theme.textPrimary; font.bold: true; text: modelData.name || ""; elide: Text.ElideRight }
                Text { Layout.preferredWidth: 78; color: statusColor(modelData.status); text: modelData.status || ""; elide: Text.ElideRight }
                Text { Layout.fillWidth: true; color: Theme.textSecondary; text: modelData.control || ""; elide: Text.ElideRight }
            }
            Text { Layout.fillWidth: true; color: Theme.textSecondary; text: "输入: " + (modelData.input || ""); elide: Text.ElideRight }
            Text { Layout.fillWidth: true; color: "#8EC5F8"; text: "输出: " + (modelData.output || ""); elide: Text.ElideRight }
        }
    }

    Component {
        id: catalogDelegate
        ColumnLayout {
            width: ListView.view.width
            height: 58
            spacing: 2
            RowLayout {
                Layout.fillWidth: true
                Text { Layout.preferredWidth: 150; color: Theme.textPrimary; font.bold: true; text: modelData.taskId || ""; elide: Text.ElideRight }
                Text { Layout.preferredWidth: 74; color: statusColor(modelData.state); text: modelData.state || ""; elide: Text.ElideRight }
                Text { Layout.preferredWidth: 72; horizontalAlignment: Text.AlignRight; color: "#8EC5F8"; text: (modelData.recordCount || 0) + " 条"; elide: Text.ElideRight }
                Text { Layout.fillWidth: true; color: Theme.textSecondary; text: modelData.taskName || ""; elide: Text.ElideRight }
            }
            Text { Layout.fillWidth: true; color: Theme.textSecondary; text: "更新: " + (modelData.updatedAt || "-") + "    原因: " + (modelData.reason || "-"); elide: Text.ElideRight }
            Text { Layout.fillWidth: true; color: "#8EC5F8"; text: modelData.storagePath || ""; elide: Text.ElideRight }
        }
    }

    Component {
        id: detailDelegate
        RowLayout {
            width: ListView.view.width
            height: 30
            spacing: 8
            Text { Layout.preferredWidth: 54; color: "#8EC5F8"; text: modelData.layer || ""; elide: Text.ElideRight }
            Text { Layout.preferredWidth: 86; color: Theme.textPrimary; text: modelData.batchNo || ""; elide: Text.ElideRight }
            Text { Layout.preferredWidth: 78; color: Theme.textSecondary; text: modelData.targetType || ""; elide: Text.ElideRight }
            Text { Layout.preferredWidth: 84; color: Theme.textSecondary; text: modelData.location || ""; elide: Text.ElideRight }
            Text { Layout.preferredWidth: 64; color: Theme.textPrimary; text: modelData.resultLabel || ""; elide: Text.ElideRight }
            Text { Layout.preferredWidth: 80; horizontalAlignment: Text.AlignRight; color: Theme.textSecondary; text: modelData.distance || "-"; elide: Text.ElideRight }
            Text { Layout.preferredWidth: 72; horizontalAlignment: Text.AlignRight; color: Theme.textSecondary; text: modelData.measurement || "-"; elide: Text.ElideRight }
            Text { Layout.preferredWidth: 60; horizontalAlignment: Text.AlignRight; color: statusColor(Number(modelData.issueCount || 0) > 0 ? "待复核" : "完成"); text: modelData.qualityScore || "0"; elide: Text.ElideRight }
            Text { Layout.fillWidth: true; color: Theme.textSecondary; text: modelData.eventTime || ""; elide: Text.ElideRight }
        }
    }

    Component {
        id: outboundDelegate
        ColumnLayout {
            width: ListView.view.width
            height: 52
            spacing: 2
            RowLayout {
                Layout.fillWidth: true
                Text { Layout.preferredWidth: 134; color: Theme.textPrimary; font.bold: true; text: modelData.receiptCode || ""; elide: Text.ElideRight }
                Text { Layout.preferredWidth: 70; color: statusColor(modelData.status); text: modelData.status || ""; elide: Text.ElideRight }
                Text { Layout.preferredWidth: 54; horizontalAlignment: Text.AlignRight; color: "#8EC5F8"; text: "重试 " + (modelData.retryCount || 0); elide: Text.ElideRight }
                Text { Layout.fillWidth: true; color: Theme.textSecondary; text: modelData.target || ""; elide: Text.ElideRight }
            }
            Text { Layout.fillWidth: true; color: Theme.textSecondary; text: "任务: " + (modelData.taskId || "-") + "    创建: " + (modelData.createdAt || "-"); elide: Text.ElideRight }
        }
    }

    Component {
        id: contractDelegate
        ColumnLayout {
            width: ListView.view.width
            height: 70
            spacing: 2
            RowLayout {
                Layout.fillWidth: true
                Text { Layout.preferredWidth: 138; color: Theme.textPrimary; font.bold: true; text: modelData.name || ""; elide: Text.ElideRight }
                Text { Layout.preferredWidth: 74; color: statusColor(modelData.status); text: modelData.status || ""; elide: Text.ElideRight }
                Text { Layout.fillWidth: true; color: "#F0CC6A"; text: modelData.version || modelData.confidence || "-"; elide: Text.ElideRight }
            }
            Text { Layout.fillWidth: true; color: Theme.textSecondary; text: "输入: " + (modelData.input || ""); elide: Text.ElideRight }
            Text { Layout.fillWidth: true; color: Theme.textSecondary; text: "输出: " + (modelData.output || ""); elide: Text.ElideRight }
            Text { Layout.fillWidth: true; color: "#8EC5F8"; text: modelData.note || ""; elide: Text.ElideRight }
        }
    }

    Component {
        id: statisticsDelegate
        RowLayout {
            width: ListView.view.width
            height: 32
            spacing: 8
            Text { Layout.preferredWidth: 92; color: Theme.textPrimary; text: modelData.category || ""; elide: Text.ElideRight }
            Text { Layout.preferredWidth: 120; color: Theme.textSecondary; text: modelData.metric || ""; elide: Text.ElideRight }
            Text { Layout.preferredWidth: 96; horizontalAlignment: Text.AlignRight; color: "#8EC5F8"; text: modelData.value || ""; elide: Text.ElideRight }
            Text { Layout.preferredWidth: 104; color: Theme.textSecondary; text: modelData.dimension || ""; elide: Text.ElideRight }
            Text { Layout.preferredWidth: 70; color: Theme.textSecondary; text: modelData.chart || ""; elide: Text.ElideRight }
            Text { Layout.fillWidth: true; color: Theme.textSecondary; text: modelData.note || ""; elide: Text.ElideRight }
        }
    }

    Component {
        id: storageDelegate
        ColumnLayout {
            width: ListView.view.width
            height: 64
            spacing: 2
            RowLayout {
                Layout.fillWidth: true
                Text { Layout.preferredWidth: 104; color: Theme.textPrimary; font.bold: true; text: modelData.table || ""; elide: Text.ElideRight }
                Text { Layout.preferredWidth: 96; color: statusColor(modelData.state); text: modelData.state || ""; elide: Text.ElideRight }
                Text { Layout.fillWidth: true; color: Theme.textSecondary; text: modelData.index || ""; elide: Text.ElideRight }
            }
            Text { Layout.fillWidth: true; color: Theme.textSecondary; text: "字段: " + (modelData.fields || ""); elide: Text.ElideRight }
            Text { Layout.fillWidth: true; color: "#8EC5F8"; text: modelData.policy || ""; elide: Text.ElideRight }
        }
    }

    Component {
        id: auditDelegate
        ColumnLayout {
            width: ListView.view.width
            height: 46
            spacing: 2
            RowLayout {
                Layout.fillWidth: true
                Text { Layout.preferredWidth: 126; color: Theme.textSecondary; text: modelData.time || ""; elide: Text.ElideRight }
                Text { Layout.preferredWidth: 90; color: Theme.textPrimary; text: modelData.action || ""; elide: Text.ElideRight }
                Text { Layout.preferredWidth: 72; color: statusColor(modelData.status); text: modelData.status || ""; elide: Text.ElideRight }
                Text { Layout.fillWidth: true; color: Theme.textSecondary; text: modelData.objectId || ""; elide: Text.ElideRight }
            }
            Text { Layout.fillWidth: true; color: "#8EC5F8"; text: modelData.detail || ""; elide: Text.ElideRight }
        }
    }

    Component {
        id: reportDelegate
        RowLayout {
            width: ListView.view.width
            height: 34
            spacing: 8
            Text { Layout.preferredWidth: 128; color: Theme.textPrimary; text: modelData.section || ""; elide: Text.ElideRight }
            Text { Layout.preferredWidth: 78; color: statusColor(modelData.state); text: modelData.state || ""; elide: Text.ElideRight }
            Text { Layout.preferredWidth: 112; color: Theme.textSecondary; text: modelData.source || ""; elide: Text.ElideRight }
            Text { Layout.preferredWidth: 88; color: "#8EC5F8"; text: modelData.template || ""; elide: Text.ElideRight }
            Text { Layout.fillWidth: true; color: Theme.textSecondary; text: modelData.note || ""; elide: Text.ElideRight }
        }
    }

    Component {
        id: environmentDelegate
        ColumnLayout {
            width: ListView.view.width
            height: 58
            spacing: 2
            RowLayout {
                Layout.fillWidth: true
                Text { Layout.preferredWidth: 112; color: Theme.textPrimary; font.bold: true; text: modelData.item || ""; elide: Text.ElideRight }
                Text { Layout.preferredWidth: 76; color: statusColor(modelData.status); text: modelData.status || ""; elide: Text.ElideRight }
                Text { Layout.fillWidth: true; color: Theme.textSecondary; text: modelData.config || ""; elide: Text.ElideRight }
            }
            Text { Layout.fillWidth: true; color: Theme.textSecondary; text: modelData.requirement || ""; elide: Text.ElideRight }
            Text { Layout.fillWidth: true; color: "#8EC5F8"; text: modelData.handling || ""; elide: Text.ElideRight }
        }
    }
}
