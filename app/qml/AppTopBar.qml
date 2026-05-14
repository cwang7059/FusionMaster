import QtQuick 2.14
import QtQuick.Controls 2.14
import QtQuick.Layouts 1.14
import QtQuick.Window 2.14
import UITheme 1.0

Rectangle {
    id: root
    Layout.fillWidth: true
    Layout.preferredHeight: 66
    visible: rowVisible
    radius: 0
    border.color: "#162A40"
    border.width: 1
    color: "#091527"

    property var hostWindow
    property var windowManagerService
    property bool rowVisible: true
    property bool logoVisible: true
    property bool titleVisible: true
    property bool navVisible: true
    property bool windowButtonsVisible: true
    property bool timeVisible: true
    property var navItems: []
    property int currentNavIndex: 0
    property bool maximized: false
    property string timeText: ""
    property int windowBtnWidth: 30
    property int windowBtnHeight: 26
    property int windowBtnIconSize: 10

    property real dragPressX: 0
    property real dragPressY: 0
    property real dragWindowX: 0
    property real dragWindowY: 0
    property bool dragActive: false

    signal navClicked(int index)
    signal toggleMaximizeRequested()

    function navItemCount() {
        if (navItems && navItems.count !== undefined) {
            return navItems.count
        }
        if (navItems && navItems.length !== undefined) {
            return navItems.length
        }
        return 0
    }

    function fallbackNavItem(index) {
        const labels = [
            { "icon": "\u2302", "text": "首页总览" },
            { "icon": "\u25A3", "text": "数据调取与管理" },
            { "icon": "\u2387", "text": "多批次融合分析" },
            { "icon": "\u2699", "text": "算法优选与建模" },
            { "icon": "\u25A5", "text": "统计与可视化" },
            { "icon": "\u2699", "text": "系统管理" }
        ]
        return labels[index] || ({ "icon": "", "text": "" })
    }

    function navItemAt(index) {
        if (navItems && navItems.get) {
            return navItems.get(index) || ({ "icon": "", "text": "" })
        }
        if (navItems && navItems[index]) {
            return navItems[index]
        }
        return { "icon": "", "text": "" }
    }

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: 24
        anchors.rightMargin: 12
        anchors.topMargin: 0
        anchors.bottomMargin: 0
        spacing: 28
        z: 1

        RowLayout {
            Layout.alignment: Qt.AlignVCenter
            Layout.preferredWidth: 304
            Layout.minimumWidth: 260
            spacing: 12
            visible: logoVisible || titleVisible

            Rectangle {
                Layout.preferredWidth: 32
                Layout.preferredHeight: 32
                radius: 4
                color: "#0EA5E9"
                border.color: Qt.rgba(1, 1, 1, 0.16)
                visible: logoVisible

                Text {
                    anchors.centerIn: parent
                    text: "试"
                    color: "#FFFFFF"
                    font.pixelSize: 16
                    font.bold: true
                }
            }

            ColumnLayout {
                Layout.alignment: Qt.AlignVCenter
                visible: titleVisible
                spacing: 0

                Text {
                    Layout.fillWidth: true
                    text: "试验数据融合分析平台"
                    color: "#F8FBFF"
                    font.pixelSize: 22
                    font.bold: true
                    elide: Text.ElideRight
                }

                Text {
                    Layout.fillWidth: true
                    text: "目标探测试验数据智能分析系统"
                    color: "#16C7F5"
                    font.pixelSize: 12
                    elide: Text.ElideRight
                }
            }
        }

        Item {
            id: navSpacer
            Layout.fillWidth: true
            Layout.preferredHeight: 52
            Layout.alignment: Qt.AlignVCenter
            visible: navVisible
        }

        Column {
            id: rightControls
            spacing: 2
            visible: windowButtonsVisible || timeVisible
            Layout.alignment: Qt.AlignVCenter | Qt.AlignRight

            Row {
                id: windowButtonsRow
                visible: windowButtonsVisible
                spacing: 2

                ToolButton {
                    visible: windowManagerService ? windowManagerService.isWindowButtonVisible("minimize") : true
                    hoverEnabled: true
                    implicitWidth: windowBtnWidth
                    implicitHeight: windowBtnHeight
                    display: AbstractButton.IconOnly
                    icon.source: "qrc:/app/assets/icons/window/minimize.svg"
                    icon.width: windowBtnIconSize
                    icon.height: windowBtnIconSize
                    icon.color: Theme.textPrimary
                    text: ""
                    leftPadding: 0
                    rightPadding: 0
                    topPadding: 0
                    bottomPadding: 0
                    onClicked: if (windowManagerService) { windowManagerService.executeWindowCommand("minimize") }
                    background: Rectangle {
                        radius: Theme.radiusSmall
                        color: parent.down
                               ? Qt.rgba(91 / 255, 127 / 255, 174 / 255, 0.2)
                               : (parent.hovered
                                  ? Qt.rgba(1, 1, 1, 0.08)
                                  : "transparent")
                    }
                }

                ToolButton {
                    visible: windowManagerService
                        ? (windowManagerService.isWindowButtonVisible("maximize")
                           || windowManagerService.isWindowButtonVisible("restore"))
                        : true
                    hoverEnabled: true
                    implicitWidth: windowBtnWidth
                    implicitHeight: windowBtnHeight
                    display: AbstractButton.IconOnly
                    icon.source: maximized
                        ? "qrc:/app/assets/icons/window/restore.svg"
                        : "qrc:/app/assets/icons/window/maximize.svg"
                    icon.width: windowBtnIconSize
                    icon.height: windowBtnIconSize
                    icon.color: Theme.textPrimary
                    leftPadding: 0
                    rightPadding: 0
                    topPadding: 0
                    bottomPadding: 0
                    text: ""
                    onClicked: root.toggleMaximizeRequested()
                    background: Rectangle {
                        radius: Theme.radiusSmall
                        color: parent.down
                               ? Qt.rgba(91 / 255, 127 / 255, 174 / 255, 0.2)
                               : (parent.hovered
                                  ? Qt.rgba(1, 1, 1, 0.08)
                                  : "transparent")
                    }
                }

                ToolButton {
                    visible: windowManagerService ? windowManagerService.isWindowButtonVisible("close") : true
                    hoverEnabled: true
                    implicitWidth: windowBtnWidth
                    implicitHeight: windowBtnHeight
                    display: AbstractButton.IconOnly
                    icon.source: "qrc:/app/assets/icons/window/close.svg"
                    icon.width: windowBtnIconSize
                    icon.height: windowBtnIconSize
                    icon.color: Theme.textPrimary
                    text: ""
                    leftPadding: 0
                    rightPadding: 0
                    topPadding: 0
                    bottomPadding: 0
                    onClicked: if (windowManagerService) { windowManagerService.executeWindowCommand("close") }
                    background: Rectangle {
                        radius: Theme.radiusSmall
                        color: parent.down
                               ? Qt.rgba(239 / 255, 68 / 255, 68 / 255, 0.78)
                               : (parent.hovered
                                  ? Qt.rgba(239 / 255, 68 / 255, 68 / 255, 0.4)
                                  : "transparent")
                    }
                }
            }

            Text {
                visible: timeVisible
                text: timeText
                width: Math.max(implicitWidth, windowButtonsRow.implicitWidth)
                color: "#A7B7C8"
                font.pixelSize: 12
                font.bold: true
                font.family: "Consolas"
                horizontalAlignment: Text.AlignRight
                verticalAlignment: Text.AlignVCenter
            }
        }
    }

    Row {
        id: navOverlay
        visible: navVisible && root.navItemCount() > 0
        z: 3
        height: 52
        spacing: 6
        anchors.left: parent.left
        anchors.leftMargin: 338
        anchors.verticalCenter: parent.verticalCenter

        Repeater {
            model: root.navItemCount()

            Rectangle {
                property var navItem: root.navItemAt(index)
                property string navIcon: navItem.icon || ""
                property string navText: navItem.text || ""

                width: Math.max(92, Math.min(136, navContent.implicitWidth + 20))
                height: 38
                anchors.verticalCenter: parent.verticalCenter
                radius: 2
                border.width: 1
                border.color: index === currentNavIndex
                              ? Qt.rgba(14 / 255, 165 / 255, 233 / 255, 0.35)
                              : "transparent"
                color: index === currentNavIndex
                       ? Qt.rgba(5 / 255, 21 / 255, 37 / 255, 0.92)
                       : "transparent"

                Row {
                    id: navContent
                    anchors.centerIn: parent
                    spacing: 6

                    Text {
                        text: navIcon
                        color: index === currentNavIndex ? "#00D7FF" : "#8EA3B9"
                        font.pixelSize: 14
                        font.bold: index === currentNavIndex
                        anchors.verticalCenter: parent.verticalCenter
                    }

                    Text {
                        text: navText
                        color: index === currentNavIndex ? "#00D7FF" : "#A7B7C8"
                        font.pixelSize: 13
                        font.bold: index === currentNavIndex
                        elide: Text.ElideRight
                        width: Math.min(104, implicitWidth)
                        anchors.verticalCenter: parent.verticalCenter
                    }
                }

                MouseArea {
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: root.navClicked(index)
                }
            }
        }
    }

    MouseArea {
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        width: {
            var limit = parent.width
            if (navOverlay.visible) {
                limit = Math.min(limit, navOverlay.x - 8)
            }
            if (rightControls.visible) {
                limit = Math.min(limit, rightControls.x - 8)
            }
            return Math.max(0, limit)
        }
        z: 0
        acceptedButtons: Qt.LeftButton
        hoverEnabled: false
        preventStealing: true

        onPressed: {
            if (mouse.button !== Qt.LeftButton || !hostWindow) {
                return
            }
            if (maximized
                    || hostWindow.visibility === Window.Maximized
                    || hostWindow.visibility === Window.FullScreen) {
                dragActive = false
                return
            }
            if (hostWindow.startSystemMove) {
                var started = hostWindow.startSystemMove()
                if (started === undefined || started) {
                    dragActive = false
                    return
                }
            }
            var globalPressPos = mapToGlobal(mouse.x, mouse.y)
            dragPressX = globalPressPos.x
            dragPressY = globalPressPos.y
            dragWindowX = hostWindow.x
            dragWindowY = hostWindow.y
            dragActive = true
        }

        onPositionChanged: {
            if (!dragActive || !hostWindow) {
                return
            }
            if (!(mouse.buttons & Qt.LeftButton)) {
                dragActive = false
                return
            }
            if (maximized
                    || hostWindow.visibility === Window.Maximized
                    || hostWindow.visibility === Window.FullScreen) {
                dragActive = false
                return
            }
            var globalPos = mapToGlobal(mouse.x, mouse.y)
            hostWindow.x = dragWindowX + (globalPos.x - dragPressX)
            hostWindow.y = dragWindowY + (globalPos.y - dragPressY)
        }

        onReleased: dragActive = false
    }
}
