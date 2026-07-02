import QtQuick
import CuteSim.Viewer

Rectangle {
    id: card
    color: Theme.cardBg
    radius: Theme.radius
    border.width: 1
    border.color: Theme.cardBorder
    antialiasing: true

    property color accentStripe: "transparent"
    property int stripeHeight: 3

    Rectangle {
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        height: card.stripeHeight
        radius: card.radius
        color: card.accentStripe
        visible: card.accentStripe !== "transparent"
        Rectangle {
            anchors.fill: parent
            anchors.topMargin: parent.height / 2
            color: parent.color
        }
    }

    Rectangle {
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.margins: 1
        height: 1
        color: Theme.isDark ? Qt.rgba(1, 1, 1, 0.04) : Qt.rgba(1, 1, 1, 0.7)
        opacity: card.accentStripe === "transparent" ? 1 : 0
    }
}
