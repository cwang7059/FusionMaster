import QtQuick 2.14
import QtQuick.Layouts 1.14

Rectangle {
    id: root

    property string statusText: "历史数据融合分析"

    Layout.fillWidth: true
    Layout.preferredHeight: visible ? 38 : 0
    color: "#091527"
    border.color: "#162A40"
    border.width: 1

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: 24
        anchors.rightMargin: 24
        spacing: 24

        RowLayout {
            spacing: 8
            Layout.alignment: Qt.AlignVCenter

            Rectangle {
                Layout.preferredWidth: 8
                Layout.preferredHeight: 8
                radius: 4
                color: "#00D38A"

                SequentialAnimation on opacity {
                    loops: Animation.Infinite
                    NumberAnimation { from: 0.45; to: 1.0; duration: 760 }
                    NumberAnimation { from: 1.0; to: 0.45; duration: 760 }
                }
            }

            Text {
                text: "数据库连接正常"
                color: "#A7B7C8"
                font.pixelSize: 13
            }
        }

        Text {
            text: "运行环境兼容良好"
            color: "#A7B7C8"
            font.pixelSize: 13
        }

        Text {
            text: "网络通信时延: 12ms"
            color: "#A7B7C8"
            font.pixelSize: 13
        }

        Text {
            Layout.fillWidth: true
            text: root.statusText
            color: "#7F94AA"
            font.pixelSize: 13
            elide: Text.ElideRight
        }

        Text {
            text: "自动备份状态: 已于今日 02:00 完成定期备份"
            color: "#7F94AA"
            font.pixelSize: 13
            horizontalAlignment: Text.AlignRight
            elide: Text.ElideRight
        }
    }
}
