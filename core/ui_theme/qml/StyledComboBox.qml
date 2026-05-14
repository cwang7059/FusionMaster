import QtQuick 2.14
import QtQuick.Controls 2.14
import UITheme 1.0

ComboBox {
    id: root

    property color bgColor: Theme.inputBg
    property color borderColor: Theme.inputBorder
    property color textColor: Theme.textPrimary
    property color highlightColor: Qt.rgba(59 / 255, 130 / 255, 246 / 255, 0.24)
    property color popupBgColor: Theme.cardBg

    implicitHeight: 34

    contentItem: Text {
        text: root.displayText
        color: root.enabled ? root.textColor : Qt.rgba(1, 1, 1, 0.45)
        leftPadding: 10
        rightPadding: 26
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight
        font.pixelSize: Theme.fontSizeNormal
    }

    indicator: Text {
        text: "\u25BE"
        color: root.textColor
        anchors.verticalCenter: parent.verticalCenter
        anchors.right: parent.right
        anchors.rightMargin: 10
        font.pixelSize: 12
    }

    background: Rectangle {
        radius: Theme.radiusNormal
        color: root.bgColor
        border.color: root.activeFocus ? Theme.accent : root.borderColor
        border.width: 1
    }

    popup: Popup {
        y: root.height + 2
        width: root.width
        implicitHeight: Math.min(contentItem.implicitHeight, 280)
        padding: 0

        contentItem: ListView {
            clip: true
            implicitHeight: contentHeight
            model: root.popup.visible ? root.delegateModel : null
            currentIndex: root.highlightedIndex
            ScrollIndicator.vertical: ScrollIndicator { }
        }

        background: Rectangle {
            color: root.popupBgColor
            border.color: root.borderColor
            border.width: 1
            radius: Theme.radiusNormal
        }
    }

    delegate: ItemDelegate {
        id: delegateRoot
        width: root.width
        height: 32
        text: {
            if (typeof modelData === "string") {
                return modelData
            }
            if (modelData && root.textRole !== "" && modelData[root.textRole] !== undefined) {
                return String(modelData[root.textRole])
            }
            if (model && root.textRole !== "" && model[root.textRole] !== undefined) {
                return String(model[root.textRole])
            }
            return String(modelData)
        }
        highlighted: root.highlightedIndex === index

        contentItem: Text {
            text: delegateRoot.text
            color: root.textColor
            verticalAlignment: Text.AlignVCenter
            elide: Text.ElideRight
            leftPadding: 10
            rightPadding: 10
            font.pixelSize: Theme.fontSizeNormal
        }

        background: Rectangle {
            color: highlighted ? root.highlightColor : "transparent"
            radius: Theme.radiusSmall
        }
    }
}
