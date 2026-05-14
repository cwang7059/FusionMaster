import QtQuick 2.14
import QtQuick.Controls 2.14
import QtQuick.Layouts 1.14
import QtQuick.Window 2.14
import UITheme 1.0

ApplicationWindow {
    id: rootWindow
    width: Math.min(1280, Screen.desktopAvailableWidth > 0 ? Screen.desktopAvailableWidth : 1280)
    height: Math.min(760, Screen.desktopAvailableHeight > 0 ? Screen.desktopAvailableHeight : 760)
    minimumWidth: Math.min(980, Screen.desktopAvailableWidth > 0 ? Screen.desktopAvailableWidth : 980)
    minimumHeight: Math.min(620, Screen.desktopAvailableHeight > 0 ? Screen.desktopAvailableHeight : 620)
    visible: true
    title: "试验数据融合分析平台"
    flags: Qt.Window | Qt.FramelessWindowHint
    color: Theme.windowBg
    property int resizeBorderWidth: 6
    property bool manualResizeActive: false
    property string manualResizeEdge: ""
    property real resizePressX: 0
    property real resizePressY: 0
    property real resizeWindowX: 0
    property real resizeWindowY: 0
    property real resizeWindowWidth: 0
    property real resizeWindowHeight: 0
    property bool windowMaximized: false
    property bool windowStateSyncPending: false
    property rect normalWindowGeometry: Qt.rect(0, 0, 0, 0)
    property bool hasNormalWindowGeometry: false
    property bool realtimePauseApplied: false

    property var uiTopContributions: []
    property var uiLeftContributions: []
    property var uiCenterContributions: []
    property var uiRightContributions: []
    property var uiBottomContributions: []
    property var uiSecondaryTopContributions: []
    property var uiSecondaryLeftContributions: []
    property var uiSecondaryCenterContributions: []
    property var uiSecondaryRightContributions: []
    property var uiSecondaryBottomContributions: []
    property string currentTimeText: ""
    property bool topRowVisible: true
    property bool topLogoVisible: true
    property bool topTitleVisible: true
    property bool topNavVisible: true
    property bool topWindowButtonsVisible: true
    property bool topTimeVisible: true
    property int currentTabIndex: 0
    property var businessTabContributions: []
    property var topNavModel: []
    property int topNavItemCount: topNavModel && topNavModel.length !== undefined ? topNavModel.length : 0

    function ensureCurrentTabIndexValid() {
        if (topNavItemCount <= 0) {
            if (currentTabIndex !== 0) {
                currentTabIndex = 0
            }
            return
        }

        if (currentTabIndex < 0 || currentTabIndex >= topNavItemCount) {
            currentTabIndex = 0
        }
    }

    function contributionText(value) {
        return String(value || "").trim()
    }

    function contributionSurface(item) {
        return contributionText(item && item.surface ? item.surface : "panel").toLowerCase()
    }

    function contributionVisible(item) {
        const contributionId = item ? (item.id || "") : ""
        return windowManager
                ? windowManager.isContributionVisible(contributionId)
                : true
    }

    function contributionOrder(item) {
        const navOrder = Number(item && item.navOrder !== undefined ? item.navOrder : -1)
        if (!isNaN(navOrder) && navOrder >= 0) {
            return navOrder
        }

        const order = Number(item && item.order !== undefined ? item.order : 0)
        return isNaN(order) ? 0 : order
    }

    function contributionLabel(item) {
        return contributionText((item && item.navText) || (item && item.title) || (item && item.pluginName) || "")
    }

    function filterBusinessTabContributions() {
        if (typeof uiContributionsModel === "undefined" || !uiContributionsModel) {
            return []
        }

        const tabs = uiContributionsModel.filter(function(item) {
            const itemScreenIndex = Math.max(0, Number((item && item.screenIndex) || 0))
            return contributionSurface(item) === "main_tab"
                    && itemScreenIndex === 0
                    && contributionVisible(item)
        })

        tabs.sort(function(lhs, rhs) {
            const lhsOrder = contributionOrder(lhs)
            const rhsOrder = contributionOrder(rhs)
            if (lhsOrder !== rhsOrder) {
                return lhsOrder - rhsOrder
            }
            return contributionLabel(lhs).localeCompare(contributionLabel(rhs))
        })

        return tabs
    }

    function buildTopNavModel(items) {
        return (items || []).map(function(item) {
            return {
                "id": item.id || "",
                "icon": contributionText(item.navIcon),
                "text": contributionLabel(item),
                "order": contributionOrder(item)
            }
        })
    }

    function refreshBusinessTabs() {
        businessTabContributions = filterBusinessTabContributions()
        topNavModel = buildTopNavModel(businessTabContributions)
        ensureCurrentTabIndexValid()
    }

    function filterUiContributions(regionName) {
        return filterUiContributionsByScreen(regionName, 0)
    }

    function filterUiContributionsByScreen(regionName, screenIndex) {
        if (typeof uiContributionsModel === "undefined" || !uiContributionsModel) {
            return []
        }

        const targetRegion = (regionName || "center").toLowerCase()
        const targetScreenIndex = Math.max(0, Number(screenIndex || 0))

        return uiContributionsModel.filter(function(item) {
            const itemRegion = (item.region || "center").toLowerCase()
            const itemSurface = contributionSurface(item)
            const itemScreenIndex = Math.max(0, Number(item.screenIndex || 0))
            const regionVisible = windowManager ? windowManager.isRegionVisible(itemRegion) : true
            return itemRegion === targetRegion
                    && itemSurface === "panel"
                    && itemScreenIndex === targetScreenIndex
                    && regionVisible
                    && contributionVisible(item)
        })
    }

    function refreshUiContributions() {
        refreshBusinessTabs()

        uiTopContributions = filterUiContributionsByScreen("top", 0)
        uiLeftContributions = filterUiContributionsByScreen("left", 0)
        uiCenterContributions = filterUiContributionsByScreen("center", 0)
        uiRightContributions = filterUiContributionsByScreen("right", 0)
        uiBottomContributions = filterUiContributionsByScreen("bottom", 0)

        uiSecondaryTopContributions = filterUiContributionsByScreen("top", 1)
        uiSecondaryLeftContributions = filterUiContributionsByScreen("left", 1)
        uiSecondaryCenterContributions = filterUiContributionsByScreen("center", 1)
        uiSecondaryRightContributions = filterUiContributionsByScreen("right", 1)
        uiSecondaryBottomContributions = filterUiContributionsByScreen("bottom", 1)
    }

    function refreshTopRowVisibility() {
        if (!windowManager) {
            topRowVisible = true
            topLogoVisible = true
            topTitleVisible = true
            topNavVisible = topNavItemCount > 0
            topWindowButtonsVisible = true
            topTimeVisible = true
            return
        }

        topRowVisible = windowManager.titleBarVisible
                && windowManager.isContributionVisible("app.top.row")
        topLogoVisible = windowManager.isContributionVisible("app.top.logo")
        topTitleVisible = windowManager.isContributionVisible("app.top.title")
        topNavVisible = windowManager.isContributionVisible("app.top.nav")
                && topNavItemCount > 0
        topWindowButtonsVisible = windowManager.isContributionVisible("app.top.window_buttons")
        topTimeVisible = windowManager.isContributionVisible("app.top.time")
    }

    function resolveAvailableGeometryAtPoint(px, py) {
        if (windowManager && windowManager.resolveAvailableGeometryAt) {
            const g = windowManager.resolveAvailableGeometryAt(Math.round(px), Math.round(py))
            if (g && Number(g.width) > 0 && Number(g.height) > 0) {
                return Qt.rect(Number(g.x), Number(g.y), Number(g.width), Number(g.height))
            }
        }

        if (rootWindow.screen && rootWindow.screen.availableGeometry) {
            return rootWindow.screen.availableGeometry
        }
        return null
    }

    function isWindowProbablyMaximized() {
        if (rootWindow.visibility === Window.Maximized || rootWindow.visibility === Window.FullScreen) {
            return true
        }

        const centerX = rootWindow.x + rootWindow.width / 2
        const centerY = rootWindow.y + rootWindow.height / 2
        const g = resolveAvailableGeometryAtPoint(centerX, centerY)
        if (!g) {
            return windowMaximized
        }

        const edgeTolerance = 24
        const sizeTolerance = 40
        return Math.abs(rootWindow.x - g.x) <= edgeTolerance
                && Math.abs(rootWindow.y - g.y) <= edgeTolerance
                && Math.abs(rootWindow.width - g.width) <= sizeTolerance
                && Math.abs(rootWindow.height - g.height) <= sizeTolerance
    }

    function syncWindowMaximizedState() {
        windowMaximized = isWindowProbablyMaximized()
    }

    function scheduleWindowStateSync() {
        if (windowStateSyncPending) {
            return
        }
        windowStateSyncPending = true
        windowStateSyncTimer.restart()
    }

    function captureNormalWindowGeometry() {
        if (windowMaximized || rootWindow.visibility === Window.Minimized) {
            return
        }
        normalWindowGeometry = Qt.rect(rootWindow.x, rootWindow.y, rootWindow.width, rootWindow.height)
        hasNormalWindowGeometry = true
    }

    function pickCurrentScreenAvailableGeometry() {
        const centerX = hasNormalWindowGeometry
                ? (normalWindowGeometry.x + normalWindowGeometry.width / 2)
                : (rootWindow.x + rootWindow.width / 2)
        const centerY = hasNormalWindowGeometry
                ? (normalWindowGeometry.y + normalWindowGeometry.height / 2)
                : (rootWindow.y + rootWindow.height / 2)

        return resolveAvailableGeometryAtPoint(centerX, centerY)
    }

    function maximizeWithoutFlicker() {
        captureNormalWindowGeometry()
        const g = pickCurrentScreenAvailableGeometry()
        if (!g || g.width <= 0 || g.height <= 0) {
            rootWindow.showMaximized()
            windowMaximized = true
            return
        }

        rootWindow.x = g.x
        rootWindow.y = g.y
        rootWindow.width = g.width
        rootWindow.height = g.height
        windowMaximized = true
    }

    function restoreFromCustomMaximize() {
        if (hasNormalWindowGeometry) {
            rootWindow.x = normalWindowGeometry.x
            rootWindow.y = normalWindowGeometry.y
            rootWindow.width = normalWindowGeometry.width
            rootWindow.height = normalWindowGeometry.height
        } else {
            rootWindow.showNormal()
        }
        windowMaximized = false
    }

    function toggleMaximizeRestore() {
        if (windowMaximized) {
            restoreFromCustomMaximize()
        } else {
            maximizeWithoutFlicker()
        }
        scheduleWindowStateSync()
    }

    function applyRealtimePause(paused) {
        if (realtimePauseApplied === paused) {
            return
        }

        if (typeof busMonitorController !== "undefined"
                && busMonitorController
                && typeof busMonitorController.setRealtimePaused === "function") {
            busMonitorController.setRealtimePaused(paused)
        }

        if (typeof logViewerController !== "undefined"
                && logViewerController
                && typeof logViewerController.setRealtimePaused === "function") {
            logViewerController.setRealtimePaused(paused)
        }

        realtimePauseApplied = paused
    }

    function markResizeActivity() {
        applyRealtimePause(true)
        resizeIdleTimer.restart()
    }

    function canManualResize() {
        return !windowMaximized
                && rootWindow.visibility !== Window.Maximized
                && rootWindow.visibility !== Window.FullScreen
    }

    function edgeToQtEdges(edge) {
        var edges = 0
        if (edge.indexOf("l") >= 0) {
            edges |= Qt.LeftEdge
        }
        if (edge.indexOf("r") >= 0) {
            edges |= Qt.RightEdge
        }
        if (edge.indexOf("t") >= 0) {
            edges |= Qt.TopEdge
        }
        if (edge.indexOf("b") >= 0) {
            edges |= Qt.BottomEdge
        }
        return edges
    }

    function beginManualResize(edge, area, mouse) {
        if (mouse.button !== Qt.LeftButton || !canManualResize()) {
            manualResizeActive = false
            return
        }

        if (rootWindow.startSystemResize) {
            var systemEdges = edgeToQtEdges(edge)
            if (systemEdges !== 0) {
                var started = rootWindow.startSystemResize(systemEdges)
                if (started === undefined || started) {
                    manualResizeActive = false
                    return
                }
            }
        }

        const globalPressPos = area.mapToGlobal(mouse.x, mouse.y)
        resizePressX = globalPressPos.x
        resizePressY = globalPressPos.y
        resizeWindowX = rootWindow.x
        resizeWindowY = rootWindow.y
        resizeWindowWidth = rootWindow.width
        resizeWindowHeight = rootWindow.height
        manualResizeEdge = edge
        manualResizeActive = true
    }

    function updateManualResize(area, mouse) {
        if (!manualResizeActive) {
            return
        }

        if (!(mouse.buttons & Qt.LeftButton)) {
            endManualResize()
            return
        }

        const globalPos = area.mapToGlobal(mouse.x, mouse.y)
        const dx = globalPos.x - resizePressX
        const dy = globalPos.y - resizePressY
        var nextX = resizeWindowX
        var nextY = resizeWindowY
        var nextWidth = resizeWindowWidth
        var nextHeight = resizeWindowHeight

        if (manualResizeEdge.indexOf("l") >= 0) {
            nextX = resizeWindowX + dx
            nextWidth = resizeWindowWidth - dx
        }
        if (manualResizeEdge.indexOf("r") >= 0) {
            nextWidth = resizeWindowWidth + dx
        }
        if (manualResizeEdge.indexOf("t") >= 0) {
            nextY = resizeWindowY + dy
            nextHeight = resizeWindowHeight - dy
        }
        if (manualResizeEdge.indexOf("b") >= 0) {
            nextHeight = resizeWindowHeight + dy
        }

        if (nextWidth < rootWindow.minimumWidth) {
            if (manualResizeEdge.indexOf("l") >= 0) {
                nextX -= (rootWindow.minimumWidth - nextWidth)
            }
            nextWidth = rootWindow.minimumWidth
        }
        if (nextHeight < rootWindow.minimumHeight) {
            if (manualResizeEdge.indexOf("t") >= 0) {
                nextY -= (rootWindow.minimumHeight - nextHeight)
            }
            nextHeight = rootWindow.minimumHeight
        }

        rootWindow.x = Math.round(nextX)
        rootWindow.y = Math.round(nextY)
        rootWindow.width = Math.round(nextWidth)
        rootWindow.height = Math.round(nextHeight)
    }

    function endManualResize() {
        if (!manualResizeActive) {
            return
        }
        manualResizeActive = false
        manualResizeEdge = ""
        captureNormalWindowGeometry()
    }

    Component.onCompleted: {
        refreshUiContributions()
        refreshTopRowVisibility()
        ensureCurrentTabIndexValid()
        currentTimeText = Qt.formatDateTime(new Date(), "HH:mm:ss")
        syncWindowMaximizedState()
        applyRealtimePause(false)
    }

    onCurrentTabIndexChanged: ensureCurrentTabIndexValid()

    onTopNavItemCountChanged: {
        ensureCurrentTabIndexValid()
        refreshTopRowVisibility()
    }

    onVisibilityChanged: {
        scheduleWindowStateSync()
    }
    onWidthChanged: {
        scheduleWindowStateSync()
        markResizeActivity()
    }
    onHeightChanged: {
        scheduleWindowStateSync()
        markResizeActivity()
    }
    onXChanged: scheduleWindowStateSync()
    onYChanged: scheduleWindowStateSync()

    Timer {
        id: windowStateSyncTimer
        interval: 0
        repeat: false
        running: false
        onTriggered: {
            windowStateSyncPending = false
            syncWindowMaximizedState()
        }
    }

    Timer {
        interval: 1000
        repeat: true
        running: true
        onTriggered: {
            currentTimeText = Qt.formatDateTime(new Date(), "HH:mm:ss")
        }
    }

    Timer {
        id: resizeIdleTimer
        interval: 180
        repeat: false
        running: false
        onTriggered: applyRealtimePause(false)
    }

    Connections {
        target: windowManager

        function onLayoutChanged() {
            refreshUiContributions()
            refreshTopRowVisibility()
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 0
        spacing: 0

        AppTopBar {
            Layout.fillWidth: true
            hostWindow: rootWindow
            windowManagerService: windowManager
            rowVisible: topRowVisible
            logoVisible: topLogoVisible
            titleVisible: topTitleVisible
            navVisible: topNavVisible
            windowButtonsVisible: topWindowButtonsVisible
            timeVisible: false
            navItems: topNavModel
            currentNavIndex: currentTabIndex
            maximized: windowMaximized
            timeText: currentTimeText

            onNavClicked: function(index) { currentTabIndex = index }
            onToggleMaximizeRequested: toggleMaximizeRestore()
        }

        AppMainContent {
            Layout.fillWidth: true
            Layout.fillHeight: true
            windowManagerService: windowManager
            activeTabIndex: rootWindow.currentTabIndex
            discoveredPluginsTextValue: discoveredPluginsText
            selectedPluginsTextValue: selectedPluginsText
            runtimeStatesTextValue: runtimeStatesText
            scanWarningsTextValue: scanWarningsText
            skippedPluginsTextValue: skippedPluginsText
            lifecycleWarningsTextValue: lifecycleWarningsText
            businessTabs: businessTabContributions
            uiContributionsData: uiContributionsModel
            topContributions: uiTopContributions
            leftContributions: uiLeftContributions
            centerContributions: uiCenterContributions
            rightContributions: uiRightContributions
            bottomContributions: uiBottomContributions

            onBusinessPureUiRequested: if (windowManager) {
                windowManager.menuBarVisible = false
                windowManager.toolBarVisible = false
                windowManager.statusBarVisible = false
                windowManager.setRegionVisible("top", false)
                windowManager.setRegionVisible("left", false)
                windowManager.setRegionVisible("right", false)
                windowManager.setRegionVisible("bottom", false)
            }
        }

        AppStatusBar {
            Layout.fillWidth: true
            visible: windowManager ? windowManager.statusBarVisible : true
            statusText: windowManager ? windowManager.statusText : "历史数据融合分析"
        }
    }

    AppResizeHandles {
        enabled: canManualResize()
        borderWidth: resizeBorderWidth
        beginResizeHandler: beginManualResize
        updateResizeHandler: updateManualResize
        endResizeHandler: endManualResize
    }

    SecondaryContributionWindow {
        id: secondaryWindow
        windowManagerService: windowManager
        secondaryTopContributions: uiSecondaryTopContributions
        secondaryLeftContributions: uiSecondaryLeftContributions
        secondaryCenterContributions: uiSecondaryCenterContributions
        secondaryRightContributions: uiSecondaryRightContributions
        secondaryBottomContributions: uiSecondaryBottomContributions
    }
}
