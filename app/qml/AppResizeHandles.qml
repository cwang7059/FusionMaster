import QtQuick 2.14

Item {
    id: root
    anchors.fill: parent
    z: 10000
    visible: enabled

    property bool enabled: true
    property int borderWidth: 6
    property var beginResizeHandler: null
    property var updateResizeHandler: null
    property var endResizeHandler: null

    MouseArea {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        height: root.borderWidth
        hoverEnabled: true
        acceptedButtons: Qt.LeftButton
        cursorShape: Qt.SizeVerCursor
        onPressed: function(mouse) { if (root.beginResizeHandler) { root.beginResizeHandler("t", this, mouse) } }
        onPositionChanged: function(mouse) { if (root.updateResizeHandler) { root.updateResizeHandler(this, mouse) } }
        onReleased: if (root.endResizeHandler) { root.endResizeHandler() }
        onCanceled: if (root.endResizeHandler) { root.endResizeHandler() }
    }

    MouseArea {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        height: root.borderWidth
        hoverEnabled: true
        acceptedButtons: Qt.LeftButton
        cursorShape: Qt.SizeVerCursor
        onPressed: function(mouse) { if (root.beginResizeHandler) { root.beginResizeHandler("b", this, mouse) } }
        onPositionChanged: function(mouse) { if (root.updateResizeHandler) { root.updateResizeHandler(this, mouse) } }
        onReleased: if (root.endResizeHandler) { root.endResizeHandler() }
        onCanceled: if (root.endResizeHandler) { root.endResizeHandler() }
    }

    MouseArea {
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        width: root.borderWidth
        hoverEnabled: true
        acceptedButtons: Qt.LeftButton
        cursorShape: Qt.SizeHorCursor
        onPressed: function(mouse) { if (root.beginResizeHandler) { root.beginResizeHandler("l", this, mouse) } }
        onPositionChanged: function(mouse) { if (root.updateResizeHandler) { root.updateResizeHandler(this, mouse) } }
        onReleased: if (root.endResizeHandler) { root.endResizeHandler() }
        onCanceled: if (root.endResizeHandler) { root.endResizeHandler() }
    }

    MouseArea {
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        width: root.borderWidth
        hoverEnabled: true
        acceptedButtons: Qt.LeftButton
        cursorShape: Qt.SizeHorCursor
        onPressed: function(mouse) { if (root.beginResizeHandler) { root.beginResizeHandler("r", this, mouse) } }
        onPositionChanged: function(mouse) { if (root.updateResizeHandler) { root.updateResizeHandler(this, mouse) } }
        onReleased: if (root.endResizeHandler) { root.endResizeHandler() }
        onCanceled: if (root.endResizeHandler) { root.endResizeHandler() }
    }

    MouseArea {
        anchors.left: parent.left
        anchors.top: parent.top
        width: root.borderWidth + 2
        height: root.borderWidth + 2
        hoverEnabled: true
        acceptedButtons: Qt.LeftButton
        cursorShape: Qt.SizeFDiagCursor
        onPressed: function(mouse) { if (root.beginResizeHandler) { root.beginResizeHandler("tl", this, mouse) } }
        onPositionChanged: function(mouse) { if (root.updateResizeHandler) { root.updateResizeHandler(this, mouse) } }
        onReleased: if (root.endResizeHandler) { root.endResizeHandler() }
        onCanceled: if (root.endResizeHandler) { root.endResizeHandler() }
    }

    MouseArea {
        anchors.right: parent.right
        anchors.top: parent.top
        width: root.borderWidth + 2
        height: root.borderWidth + 2
        hoverEnabled: true
        acceptedButtons: Qt.LeftButton
        cursorShape: Qt.SizeBDiagCursor
        onPressed: function(mouse) { if (root.beginResizeHandler) { root.beginResizeHandler("tr", this, mouse) } }
        onPositionChanged: function(mouse) { if (root.updateResizeHandler) { root.updateResizeHandler(this, mouse) } }
        onReleased: if (root.endResizeHandler) { root.endResizeHandler() }
        onCanceled: if (root.endResizeHandler) { root.endResizeHandler() }
    }

    MouseArea {
        anchors.left: parent.left
        anchors.bottom: parent.bottom
        width: root.borderWidth + 2
        height: root.borderWidth + 2
        hoverEnabled: true
        acceptedButtons: Qt.LeftButton
        cursorShape: Qt.SizeBDiagCursor
        onPressed: function(mouse) { if (root.beginResizeHandler) { root.beginResizeHandler("bl", this, mouse) } }
        onPositionChanged: function(mouse) { if (root.updateResizeHandler) { root.updateResizeHandler(this, mouse) } }
        onReleased: if (root.endResizeHandler) { root.endResizeHandler() }
        onCanceled: if (root.endResizeHandler) { root.endResizeHandler() }
    }

    MouseArea {
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        width: root.borderWidth + 2
        height: root.borderWidth + 2
        hoverEnabled: true
        acceptedButtons: Qt.LeftButton
        cursorShape: Qt.SizeFDiagCursor
        onPressed: function(mouse) { if (root.beginResizeHandler) { root.beginResizeHandler("br", this, mouse) } }
        onPositionChanged: function(mouse) { if (root.updateResizeHandler) { root.updateResizeHandler(this, mouse) } }
        onReleased: if (root.endResizeHandler) { root.endResizeHandler() }
        onCanceled: if (root.endResizeHandler) { root.endResizeHandler() }
    }
}
