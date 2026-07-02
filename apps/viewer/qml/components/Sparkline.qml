import QtQuick
import CuteSim.Viewer

Canvas {
    id: root
    property var chartData: []
    property color lineColor: Theme.accent
    property real lineWidth: 1.2
    property bool fillUnder: true

    onChartDataChanged: requestPaint()
    onLineColorChanged: requestPaint()
    onWidthChanged:     requestPaint()
    onHeightChanged:    requestPaint()

    onPaint: {
        var ctx = getContext("2d")
        ctx.reset()
        if (!chartData || chartData.length < 2) return

        var w = width, h = height
        var minV = chartData[0], maxV = chartData[0]
        for (var i = 1; i < chartData.length; i++) {
            if (chartData[i] < minV) minV = chartData[i]
            if (chartData[i] > maxV) maxV = chartData[i]
        }
        var range = (maxV - minV) || 1

        if (fillUnder) {
            ctx.beginPath()
            for (var j = 0; j < chartData.length; j++) {
                var x = (j / (chartData.length - 1)) * w
                var y = h - ((chartData[j] - minV) / range) * (h - 2) - 1
                if (j === 0) ctx.moveTo(x, y)
                else ctx.lineTo(x, y)
            }
            ctx.lineTo(w, h)
            ctx.lineTo(0, h)
            ctx.closePath()
            var grad = ctx.createLinearGradient(0, 0, 0, h)
            grad.addColorStop(0.0, Qt.rgba(lineColor.r, lineColor.g, lineColor.b, 0.4))
            grad.addColorStop(1.0, Qt.rgba(lineColor.r, lineColor.g, lineColor.b, 0.0))
            ctx.fillStyle = grad
            ctx.fill()
        }

        ctx.beginPath()
        for (var k = 0; k < chartData.length; k++) {
            var xx = (k / (chartData.length - 1)) * w
            var yy = h - ((chartData[k] - minV) / range) * (h - 2) - 1
            if (k === 0) ctx.moveTo(xx, yy)
            else ctx.lineTo(xx, yy)
        }
        ctx.strokeStyle = lineColor
        ctx.lineWidth = lineWidth
        ctx.lineJoin = "round"
        ctx.stroke()
    }
}
