import QtQuick 2.14
import QtQuick.Controls 2.14
import UITheme 1.0

ComboBox {
    id: root

    property color bgColor: Theme.inputBg
    property color borderColor: Theme.inputBorder
    property color textColor: Theme.textPrimary

    contentItem: Text {
        text: root.displayText
        color: root.textColor
        leftPadding: 10
        rightPadding: 26
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight
        font.pixelSize: 14
    }

    indicator: Text {
        text: "\u25BE"
        color: root.textColor
        anchors.verticalCenter: parent.verticalCenter
        anchors.right: parent.right
        anchors.rightMargin: 8
        font.pixelSize: 12
    }

    background: Rectangle {
        radius: 2
        color: root.bgColor
        border.color: root.borderColor
    }

    popup: Popup {
        y: root.height + 2
        width: root.width
        implicitHeight: Math.min(contentItem.implicitHeight, 280)
        padding: 0

        contentItem: ListView {
            clip: true
            model: root.popup.visible ? root.delegateModel : null
            currentIndex: root.highlightedIndex
            ScrollIndicator.vertical: ScrollIndicator { }
        }

        background: Rectangle {
            color: Theme.cardBg
            border.color: Theme.inputBorder
            radius: 2
        }
    }

    delegate: ItemDelegate {
        width: root.width
        text: modelData
        highlighted: root.highlightedIndex === index

        contentItem: Text {
            text: modelData
            color: Theme.textPrimary
            verticalAlignment: Text.AlignVCenter
            elide: Text.ElideRight
            leftPadding: 10
            rightPadding: 10
            font.pixelSize: 14
        }

        background: Rectangle {
            color: highlighted ? Theme.accent : Theme.cardBg
        }
    }
}
