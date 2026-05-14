import QtQuick 2.14
import QtQuick.Controls 2.14

ComboBox {
    id: root
    implicitHeight: 34

    property color bgColor: "#102F4E"
    property color borderColor: "#2D5A83"
    property color textColor: "#EAF4FF"

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
            color: "#163A5D"
            border.color: "#2D5A83"
            radius: 2
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
            color: "#EAF4FF"
            verticalAlignment: Text.AlignVCenter
            elide: Text.ElideRight
            leftPadding: 10
            rightPadding: 10
            font.pixelSize: 14
        }

        background: Rectangle {
            color: highlighted ? "#2F7FB6" : "#163A5D"
        }
    }
}
