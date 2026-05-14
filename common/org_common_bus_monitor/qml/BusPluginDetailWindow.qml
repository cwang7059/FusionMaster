import QtQuick 2.14
import QtQuick.Controls 2.14
import QtQuick.Layouts 1.14
import QtQuick.Window 2.14
import UITheme 1.0

Window {
    id: root
    width: 1060
    height: 620
    minimumWidth: 920
    minimumHeight: 520
    visible: false
    color: Theme.windowBg
    flags: Qt.Dialog | Qt.FramelessWindowHint
    title: String(pluginName || "").trim() === ""
           ? "关注插件"
           : ("关注插件: " + pluginName)

    property string pluginName: ""
    property bool receiveEstimated: false
    property var publishRows: []
    property var receiveRows: []
    property var timeLabels: []
    property var publishCountSeries: []
    property var receiveCountSeries: []
    property var publishBytesSeries: []
    property var receiveBytesSeries: []

    function openWindow() {
        if (visible) {
            return
        }
        visible = true
        raise()
        requestActivate()
    }

    Rectangle {
        anchors.fill: parent
        color: Theme.windowBg

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: Theme.spacingSmall
            spacing: Theme.spacingSmall

            RowLayout {
                Layout.fillWidth: true
                spacing: Theme.spacingSmall

                Text {
                    Layout.fillWidth: true
                    text: root.title
                    color: Theme.textPrimary
                    font.pixelSize: Theme.fontSizeTitle
                    font.bold: true
                }

                DarkButton {
                    Layout.preferredWidth: 88
                    text: "关闭"
                    onClicked: root.visible = false
                }
            }

            Text {
                Layout.fillWidth: true
                visible: root.receiveEstimated
                color: Theme.textSecondary
                text: "说明：接收统计目前按总线可观测数据估算（不含订阅方精确标记）"
                font.pixelSize: Theme.fontSizeNormal
            }

            RowLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                spacing: Theme.spacingSmall

                ColumnLayout {
                    Layout.preferredWidth: 380
                    Layout.fillHeight: true
                    spacing: Theme.spacingSmall

                    BusTopicDetailTable {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        titleText: "发布详细信息"
                        rowsModel: root.publishRows
                    }

                    BusTopicDetailTable {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        titleText: "接收详细信息"
                        rowsModel: root.receiveRows
                    }
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    spacing: Theme.spacingSmall

                    BusTrendChart {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        titleText: "消息计数"
                        timeLabels: root.timeLabels
                        publishSeries: root.publishCountSeries
                        receiveSeries: root.receiveCountSeries
                        valueSuffix: "单位：条"
                    }

                    BusTrendChart {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        titleText: "消息长度(B)"
                        timeLabels: root.timeLabels
                        publishSeries: root.publishBytesSeries
                        receiveSeries: root.receiveBytesSeries
                        valueSuffix: "单位：字节"
                    }
                }
            }
        }
    }
}
