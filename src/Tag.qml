import QtQuick
import QtQuick.Controls

// A small uppercase label for a title's kind or release status.
Rectangle {
    id: tag
    property alias text: label.text
    property bool filled: false

    implicitWidth: label.implicitWidth + 10
    implicitHeight: label.implicitHeight + 4
    radius: Math.min(3, theme.radius)
    color: filled ? theme.accent : "transparent"
    border.width: filled ? 0 : 1
    border.color: Ui.line

    Label {
        id: label
        anchors.centerIn: parent
        font.pixelSize: 10
        font.family: Ui.mono
        font.letterSpacing: 0.6
        font.capitalization: Font.AllUppercase
        color: tag.filled ? theme.accentForeground : Ui.muted
    }
}
