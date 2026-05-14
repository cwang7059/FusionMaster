import QtQuick 2.14
import QtQuick.Layouts 1.14
import UITheme 1.0

Rectangle {
    property var contribution: ({})
    implicitHeight: 180
    radius: Theme.radiusNormal
    color: Theme.cardBg
    border.color: Qt.rgba(1, 1, 1, 0.12)

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: Theme.spacingSmall
        spacing: Theme.spacingSmall

        Text {
            Layout.fillWidth: true
            text: (contribution.title || contribution.id || "未命名组件") + " [" + (contribution.pluginName || "") + "]"
            color: Theme.textPrimary
            font.pixelSize: Theme.fontSizeNormal
            elide: Text.ElideRight
        }

        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true

            Loader {
                id: contributionLoader
                anchors.fill: parent
                source: contribution.qmlSource || ""
            }

            Rectangle {
                anchors.fill: parent
                visible: (contribution.qmlSource || "") === ""
                         || contributionLoader.status === Loader.Error
                color: Qt.rgba(1, 1, 1, 0.03)
                border.color: Qt.rgba(1, 1, 1, 0.08)
                radius: Theme.radiusNormal

                Text {
                    anchors.centerIn: parent
                    color: Theme.textSecondary
                    text: (contribution.qmlSource || "") === ""
                          ? "未配置 qmlSource"
                          : "组件加载失败: " + (contribution.qmlSource || "")
                    horizontalAlignment: Text.AlignHCenter
                    wrapMode: Text.WordWrap
                }
            }
        }
    }
}

