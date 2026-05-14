import QtQuick 2.14
import UITheme 1.0

StyledTrendChart {
    id: root

    // 保持兼容旧字段命名
    property var timeLabels: []
    property var publishSeries: []
    property var receiveSeries: []
    property string valueSuffix: ""

    xLabels: timeLabels
    suffixText: valueSuffix
    seriesList: [
        {
            "name": "发布",
            "color": "#E0625A",
            "values": publishSeries
        },
        {
            "name": "接收",
            "color": "#88D46B",
            "values": receiveSeries
        }
    ]
}
