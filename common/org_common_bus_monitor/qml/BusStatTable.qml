import QtQuick 2.14
import QtQuick.Controls 2.14
import QtQuick.Layouts 1.14
import UITheme 1.0

Rectangle {
    id: root
    radius: Theme.radiusNormal
    color: Theme.inputBg
    border.color: Theme.inputBorder

    property string titleText: ""
    property string nameHeader: "名称"
    property var statsModel: []

    property int nameColumnWidth: 170
    property int pubStateColumnWidth: 84
    property int pubCountColumnWidth: 58
    property int pubLengthColumnWidth: 70
    property int recvStateColumnWidth: 84
    property int recvCountColumnWidth: 58

    property int minNameColumnWidth: 120
    property int minStateColumnWidth: 72
    property int minCountColumnWidth: 52
    property int minLengthColumnWidth: 60
    property int minAutoColumnWidth: 60
    property int resizeHandleWidth: 8

    property color gridColor: Theme.inputBorder
    property color headerColor: Theme.accentWeak
    property color rowEvenColor: Theme.cardBg
    property color rowOddColor: Theme.inputBg
    property bool enableRowDoubleClick: false

    signal rowDoubleClicked(var rowData)

    function minWidthForKey(columnKey) {
        if (columnKey === "name") return minNameColumnWidth
        if (columnKey === "pubState" || columnKey === "recvState") return minStateColumnWidth
        if (columnKey === "pubCount" || columnKey === "recvCount") return minCountColumnWidth
        return minLengthColumnWidth
    }

    function widthForKey(columnKey) {
        if (columnKey === "name") return nameColumnWidth
        if (columnKey === "pubState") return pubStateColumnWidth
        if (columnKey === "pubCount") return pubCountColumnWidth
        if (columnKey === "pubLength") return pubLengthColumnWidth
        if (columnKey === "recvState") return recvStateColumnWidth
        if (columnKey === "recvCount") return recvCountColumnWidth
        return 0
    }

    function setWidthForKey(columnKey, value) {
        var nextValue = Math.max(1, Math.floor(value))
        if (columnKey === "name") nameColumnWidth = nextValue
        else if (columnKey === "pubState") pubStateColumnWidth = nextValue
        else if (columnKey === "pubCount") pubCountColumnWidth = nextValue
        else if (columnKey === "pubLength") pubLengthColumnWidth = nextValue
        else if (columnKey === "recvState") recvStateColumnWidth = nextValue
        else if (columnKey === "recvCount") recvCountColumnWidth = nextValue
    }

    function fixedWidthSum() {
        return Math.max(minNameColumnWidth, nameColumnWidth)
             + Math.max(minStateColumnWidth, pubStateColumnWidth)
             + Math.max(minCountColumnWidth, pubCountColumnWidth)
             + Math.max(minLengthColumnWidth, pubLengthColumnWidth)
             + Math.max(minStateColumnWidth, recvStateColumnWidth)
             + Math.max(minCountColumnWidth, recvCountColumnWidth)
    }

    function setColumnWidth(columnKey, targetWidth, totalWidth) {
        var minWidth = minWidthForKey(columnKey)
        var currentWidth = Math.max(minWidth, widthForKey(columnKey))
        var fixedWithout = fixedWidthSum() - currentWidth
        var maxWidth = Math.max(minWidth, Math.floor(totalWidth) - minAutoColumnWidth - fixedWithout)
        var nextWidth = Math.max(minWidth, Math.min(Math.floor(targetWidth), maxWidth))
        setWidthForKey(columnKey, nextWidth)
    }

    function computeColumns(totalWidth) {
        var cols = {
            name: Math.max(minNameColumnWidth, nameColumnWidth),
            pubState: Math.max(minStateColumnWidth, pubStateColumnWidth),
            pubCount: Math.max(minCountColumnWidth, pubCountColumnWidth),
            pubLength: Math.max(minLengthColumnWidth, pubLengthColumnWidth),
            recvState: Math.max(minStateColumnWidth, recvStateColumnWidth),
            recvCount: Math.max(minCountColumnWidth, recvCountColumnWidth),
            recvLength: 0
        }

        var width = Math.max(0, Math.floor(totalWidth))
        var over = (cols.name + cols.pubState + cols.pubCount + cols.pubLength + cols.recvState + cols.recvCount + minAutoColumnWidth) - width

        function shrinkColumn(key, minValue) {
            if (over <= 0) return
            var canShrink = cols[key] - minValue
            if (canShrink <= 0) return
            var delta = Math.min(canShrink, over)
            cols[key] -= delta
            over -= delta
        }

        var guard = 0
        while (over > 0 && guard < 12) {
            shrinkColumn("name", minNameColumnWidth)
            shrinkColumn("pubState", minStateColumnWidth)
            shrinkColumn("pubCount", minCountColumnWidth)
            shrinkColumn("pubLength", minLengthColumnWidth)
            shrinkColumn("recvState", minStateColumnWidth)
            shrinkColumn("recvCount", minCountColumnWidth)
            guard += 1
        }

        cols.recvLength = Math.max(0, width - (cols.name + cols.pubState + cols.pubCount + cols.pubLength + cols.recvState + cols.recvCount))
        return cols
    }

    function boundaryX(columnKey, cols) {
        if (columnKey === "name") return cols.name
        if (columnKey === "pubState") return cols.name + cols.pubState
        if (columnKey === "pubCount") return cols.name + cols.pubState + cols.pubCount
        if (columnKey === "pubLength") return cols.name + cols.pubState + cols.pubCount + cols.pubLength
        if (columnKey === "recvState") return cols.name + cols.pubState + cols.pubCount + cols.pubLength + cols.recvState
        if (columnKey === "recvCount") return cols.name + cols.pubState + cols.pubCount + cols.pubLength + cols.recvState + cols.recvCount
        return 0
    }

    function stateColor(activeOrCount) {
        if (typeof activeOrCount === "boolean") {
            return activeOrCount ? Theme.success : Theme.textSecondary
        }
        return Number(activeOrCount || 0) > 0 ? Theme.success : Theme.textSecondary
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: Theme.spacingSmall
        spacing: Theme.spacingSmall

        Text {
            text: root.titleText
            color: Theme.textPrimary
            font.bold: true
        }

        Rectangle {
            id: header
            Layout.fillWidth: true
            Layout.preferredHeight: 30
            color: root.headerColor
            border.color: root.gridColor
            border.width: 1

            Row {
                id: headerRow
                anchors.fill: parent
                spacing: 0
                property var columns: root.computeColumns(width)

                Rectangle {
                    width: headerRow.columns.name
                    height: parent.height
                    color: "transparent"
                    border.color: root.gridColor
                    border.width: 1
                    Text { anchors.centerIn: parent; text: root.nameHeader; color: Theme.textPrimary; font.bold: true }
                }
                Rectangle {
                    width: headerRow.columns.pubState
                    height: parent.height
                    color: "transparent"
                    border.color: root.gridColor
                    border.width: 1
                    Text { anchors.centerIn: parent; text: "发布状态"; color: Theme.textPrimary; font.bold: true }
                }
                Rectangle {
                    width: headerRow.columns.pubCount
                    height: parent.height
                    color: "transparent"
                    border.color: root.gridColor
                    border.width: 1
                    Text { anchors.centerIn: parent; text: "计数"; color: Theme.textPrimary; font.bold: true }
                }
                Rectangle {
                    width: headerRow.columns.pubLength
                    height: parent.height
                    color: "transparent"
                    border.color: root.gridColor
                    border.width: 1
                    Text { anchors.centerIn: parent; text: "长度"; color: Theme.textPrimary; font.bold: true }
                }
                Rectangle {
                    width: headerRow.columns.recvState
                    height: parent.height
                    color: "transparent"
                    border.color: root.gridColor
                    border.width: 1
                    Text { anchors.centerIn: parent; text: "接收状态"; color: Theme.textPrimary; font.bold: true }
                }
                Rectangle {
                    width: headerRow.columns.recvCount
                    height: parent.height
                    color: "transparent"
                    border.color: root.gridColor
                    border.width: 1
                    Text { anchors.centerIn: parent; text: "计数"; color: Theme.textPrimary; font.bold: true }
                }
                Rectangle {
                    width: headerRow.columns.recvLength
                    height: parent.height
                    color: "transparent"
                    border.color: root.gridColor
                    border.width: 1
                    Text { anchors.centerIn: parent; text: "长度"; color: Theme.textPrimary; font.bold: true }
                }
            }

            Repeater {
                model: ["name", "pubState", "pubCount", "pubLength", "recvState", "recvCount"]

                Rectangle {
                    id: resizeHandle
                    width: root.resizeHandleWidth
                    height: header.height
                    x: root.boundaryX(modelData, headerRow.columns) - width / 2
                    y: 0
                    z: 4
                    color: mouseArea.containsMouse ? Theme.accent : "transparent"

                    property real pressHeaderX: 0
                    property real pressColumnWidth: 0

                    MouseArea {
                        id: mouseArea
                        anchors.fill: parent
                        hoverEnabled: true
                        acceptedButtons: Qt.LeftButton
                        cursorShape: Qt.SplitHCursor

                        onPressed: {
                            resizeHandle.pressHeaderX = mapToItem(header, mouse.x, mouse.y).x
                            resizeHandle.pressColumnWidth = root.widthForKey(modelData)
                        }

                        onPositionChanged: {
                            if (!(mouse.buttons & Qt.LeftButton)) {
                                return
                            }
                            var currentHeaderX = mapToItem(header, mouse.x, mouse.y).x
                            var delta = currentHeaderX - resizeHandle.pressHeaderX
                            root.setColumnWidth(modelData, resizeHandle.pressColumnWidth + delta, header.width)
                        }
                    }
                }
            }
        }

        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true

            ListView {
                id: listView
                anchors.fill: parent
                clip: true
                model: root.statsModel

                delegate: Rectangle {
                    width: listView.width
                    height: 32
                    color: index % 2 === 0 ? root.rowEvenColor : root.rowOddColor
                    border.color: root.gridColor
                    border.width: 1

                    property var rowData: modelData

                    MouseArea {
                        anchors.fill: parent
                        acceptedButtons: Qt.LeftButton
                        onDoubleClicked: {
                            if (root.enableRowDoubleClick) {
                                root.rowDoubleClicked(rowData)
                            }
                        }
                    }

                    Row {
                        id: dataRow
                        anchors.fill: parent
                        spacing: 0
                        property var columns: root.computeColumns(width)

                        Rectangle {
                            width: dataRow.columns.name
                            height: parent.height
                            color: "transparent"
                            border.color: root.gridColor
                            border.width: 1
                            Text {
                                anchors.fill: parent
                                anchors.leftMargin: 6
                                anchors.rightMargin: 6
                                verticalAlignment: Text.AlignVCenter
                                color: Theme.textPrimary
                                text: rowData.name || ""
                                elide: Text.ElideRight
                            }
                        }

                        Rectangle {
                            width: dataRow.columns.pubState
                            height: parent.height
                            color: "transparent"
                            border.color: root.gridColor
                            border.width: 1
                            Rectangle {
                                anchors.centerIn: parent
                                width: 12
                                height: 12
                                radius: 6
                                color: root.stateColor(rowData.publishActive)
                            }
                        }

                        Rectangle {
                            width: dataRow.columns.pubCount
                            height: parent.height
                            color: "transparent"
                            border.color: root.gridColor
                            border.width: 1
                            Text {
                                anchors.centerIn: parent
                                color: Theme.textPrimary
                                text: String(rowData.publishCount || 0)
                            }
                        }

                        Rectangle {
                            width: dataRow.columns.pubLength
                            height: parent.height
                            color: "transparent"
                            border.color: root.gridColor
                            border.width: 1
                            Text {
                                anchors.centerIn: parent
                                color: Theme.textPrimary
                                text: String(rowData.publishBytes || 0)
                            }
                        }

                        Rectangle {
                            width: dataRow.columns.recvState
                            height: parent.height
                            color: "transparent"
                            border.color: root.gridColor
                            border.width: 1
                            Rectangle {
                                anchors.centerIn: parent
                                width: 12
                                height: 12
                                radius: 6
                                color: root.stateColor(rowData.receiveActive)
                            }
                        }

                        Rectangle {
                            width: dataRow.columns.recvCount
                            height: parent.height
                            color: "transparent"
                            border.color: root.gridColor
                            border.width: 1
                            Text {
                                anchors.centerIn: parent
                                color: Theme.textPrimary
                                text: String(rowData.receiveCount || 0)
                            }
                        }

                        Rectangle {
                            width: dataRow.columns.recvLength
                            height: parent.height
                            color: "transparent"
                            border.color: root.gridColor
                            border.width: 1
                            Text {
                                anchors.centerIn: parent
                                color: Theme.textPrimary
                                text: String(rowData.receiveBytes || 0)
                            }
                        }
                    }
                }

                ScrollBar.vertical: ScrollBar {
                    policy: ScrollBar.AsNeeded
                }
            }

            Text {
                anchors.centerIn: parent
                visible: listView.count === 0
                color: Theme.textSecondary
                text: "暂无数据"
            }
        }
    }
}
