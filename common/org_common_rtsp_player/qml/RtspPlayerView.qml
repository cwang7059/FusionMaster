import QtQuick 2.14
import QtQuick.Controls 2.14
import QtQuick.Layouts 1.14
import UITheme 1.0
import RtspPlayer 1.0

Rectangle {
    id: root
    color: Theme.windowBg
    clip: true

    readonly property var streamRows: RtspPlayerController.streams || []

    function submitAddStream(streamIdText, urlText) {
        RtspPlayerController.requestAddStream(urlText, streamIdText)
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: Theme.spacingNormal
        spacing: Theme.spacingSmall

        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: Theme.spacingNormal

            Rectangle {
                Layout.preferredWidth: 280
                Layout.fillHeight: true
                radius: Theme.radiusNormal
                color: Theme.cardBg
                border.color: Theme.cardBorder

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: Theme.spacingSmall
                    spacing: Theme.spacingSmall

                    Text {
                        text: "\u89c6\u9891\u5217\u8868"
                        color: Theme.textPrimary
                        font.pixelSize: Theme.fontSizeLarge
                        font.bold: true
                    }

                    ListView {
                        id: streamListView
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        clip: true
                        spacing: Theme.spacingSmall
                        model: root.streamRows

                        delegate: Rectangle {
                            property var rowData: modelData || ({})
                            property string rowId: String(rowData.id || "")
                            property bool selected: rowId === String(RtspPlayerController.selectedStreamId || "")
                            property bool canRemove: !RtspPlayerController.replayActive && rowId.length > 0

                            width: ListView.view.width
                            height: 72
                            radius: Theme.radiusNormal
                            color: selected ? Qt.rgba(59 / 255, 130 / 255, 246 / 255, 0.18) : Theme.inputBg
                            border.color: selected ? Theme.accent : Theme.inputBorder
                            border.width: 1

                            RowLayout {
                                anchors.fill: parent
                                anchors.margins: Theme.spacingSmall
                                spacing: Theme.spacingSmall

                                Item {
                                    Layout.fillWidth: true
                                    Layout.fillHeight: true

                                    Column {
                                        anchors.fill: parent
                                        spacing: 4

                                        Text {
                                            text: rowData.title || rowId || "\u672a\u547d\u540d\u6d41"
                                            color: Theme.textPrimary
                                            font.bold: true
                                        }

                                        Text {
                                            text: String(rowData.url || "--")
                                            color: Theme.textSecondary
                                            font.pixelSize: 12
                                            elide: Text.ElideMiddle
                                            width: parent.width
                                        }
                                    }

                                    MouseArea {
                                        anchors.fill: parent
                                        onClicked: RtspPlayerController.selectedStreamId = rowId
                                    }
                                }

                                StyledButton {
                                    Layout.alignment: Qt.AlignVCenter
                                    Layout.preferredWidth: 52
                                    text: "\u5220\u9664"
                                    enabled: canRemove
                                    opacity: enabled ? 1.0 : 0.5
                                    onClicked: RtspPlayerController.requestRemoveStream(rowId)
                                }
                            }
                        }
                    }

                    Text {
                        Layout.fillWidth: true
                        visible: root.streamRows.length === 0
                        text: "\u8fd8\u6ca1\u6709\u6536\u5230 RTSP \u7f16\u7801\u5305\uff0c\u6536\u5230\u540e\u4f1a\u81ea\u52a8\u51fa\u73b0\u5728\u8fd9\u91cc\u3002"
                        color: Theme.textSecondary
                        wrapMode: Text.Wrap
                    }
                }
            }

            ColumnLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                spacing: Theme.spacingSmall

                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: RtspPlayerController.addStreamStatusText.length > 0 ? 76 : 52
                    radius: Theme.radiusNormal
                    color: Theme.cardBg
                    border.color: Theme.cardBorder

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: Theme.spacingSmall
                        spacing: 6

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 10

                            Text {
                                text: "\u89c6\u9891\u64ad\u653e"
                                color: Theme.textPrimary
                                font.pixelSize: Theme.fontSizeLarge
                                font.bold: true
                            }

                            Text {
                                text: RtspPlayerController.selectedStreamId || "\u672a\u9009\u62e9\u6d41"
                                color: Theme.textSecondary
                            }

                            Text {
                                text: RtspPlayerController.selectedTopic || "--"
                                color: Theme.textSecondary
                                elide: Text.ElideLeft
                                Layout.preferredWidth: 220
                                Layout.maximumWidth: 220
                            }

                            StyledInput {
                                id: streamIdField
                                Layout.preferredWidth: 150
                                placeholderText: "stream_id\uff08\u53ef\u9009\uff09"
                            }

                            StyledInput {
                                id: rtspUrlField
                                Layout.fillWidth: true
                                placeholderText: "rtsp://127.0.0.1:8554/live/cam4"
                                onAccepted: root.submitAddStream(streamIdField.text, text)
                            }

                            StyledButton {
                                id: addStreamButton
                                Layout.preferredWidth: 96
                                text: "\u6dfb\u52a0\u6d41"
                                onClicked: root.submitAddStream(streamIdField.text, rtspUrlField.text)
                            }

                            StyledButton {
                                Layout.preferredWidth: 118
                                text: RtspPlayerController.preferPacketIntegrity
                                      ? "\u6a21\u5f0f: \u5c11\u4e22\u5305"
                                      : "\u6a21\u5f0f: \u4f4e\u5ef6\u8fdf"
                                onClicked: RtspPlayerController.togglePacketHandlingMode()
                            }
                        }

                        Text {
                            Layout.fillWidth: true
                            visible: RtspPlayerController.addStreamStatusText.length > 0
                            text: RtspPlayerController.addStreamStatusText
                            color: Theme.textSecondary
                            wrapMode: Text.Wrap
                            font.pixelSize: 12
                        }
                    }
                }

                Rectangle {
                    id: playerSurface
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    radius: Theme.radiusNormal
                    color: Theme.inputBg
                    border.color: Theme.cardBorder

                    RtspVideoItem {
                        id: videoOutput
                        anchors.fill: parent
                        anchors.margins: 1
                        controller: RtspPlayerController
                        emptyText: RtspPlayerController.statusText
                    }

                    Text {
                        anchors.centerIn: parent
                        width: parent.width - Theme.spacingLarge * 2
                        visible: !videoOutput.hasFrame
                        text: RtspPlayerController.statusText
                        color: Theme.textSecondary
                        wrapMode: Text.Wrap
                        horizontalAlignment: Text.AlignHCenter
                    }
                }

                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 116
                    radius: Theme.radiusNormal
                    color: Theme.cardBg
                    border.color: Theme.cardBorder

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: Theme.spacingSmall
                        spacing: 4

                        Text {
                            Layout.fillWidth: true
                            text: RtspPlayerController.statusText
                            color: Theme.textPrimary
                            font.pixelSize: Theme.fontSizeNormal
                        }

                        Text {
                            Layout.fillWidth: true
                            text: "Codec: " + RtspPlayerController.selectedCodecText
                                  + "    |    \u5206\u8fa8\u7387: " + RtspPlayerController.selectedResolutionText
                                  + "    |    \u6700\u8fd1\u65f6\u95f4\u8f74: " + RtspPlayerController.lastFrameTimeText
                            color: Theme.textSecondary
                            wrapMode: Text.Wrap
                        }

                        Text {
                            Layout.fillWidth: true
                            text: "\u6536\u5230\u5305: " + RtspPlayerController.receivedPacketCount
                                  + "    |    \u89e3\u7801\u5e27: " + RtspPlayerController.decodedFrameCount
                                  + "    |    \u89e3\u7801 FPS: " + RtspPlayerController.decodedFps
                                  + (RtspPlayerController.waitingForKeyFrame
                                     ? "    |    \u7b49\u5f85\u5173\u952e\u5e27"
                                     : "")
                            color: Theme.textSecondary
                            wrapMode: Text.Wrap
                        }

                        Text {
                            Layout.fillWidth: true
                            text: (RtspPlayerController.preferPacketIntegrity
                                   ? "\u6a21\u5f0f: \u5c11\u4e22\u5305"
                                   : "\u6a21\u5f0f: \u4f4e\u5ef6\u8fdf")
                                  + "    |    \u6392\u961f\u5305: " + RtspPlayerController.selectedQueuedPacketCount
                                  + "    |    \u5df2\u4e22\u5305: " + RtspPlayerController.selectedDroppedPacketCount
                            color: Theme.textSecondary
                            wrapMode: Text.Wrap
                        }
                    }
                }
            }
        }
    }
}
