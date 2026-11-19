import QtQuick
import QtQuick.Controls

// A square button with a one-character glyph, for queue and row actions.
AbstractButton {
    id: button
    property string glyph
    property string tip
    property bool active: false
    property color tint: Ui.text

    implicitWidth: 30
    implicitHeight: 30
    hoverEnabled: true
    focusPolicy: Qt.NoFocus
    opacity: enabled ? 1 : 0.3

    background: Rectangle {
        radius: Math.min(4, theme.radius)
        color: button.active ? Ui.text : button.hovered ? Ui.raised : "transparent"
        border.width: 1
        border.color: button.active ? Ui.text : button.hovered ? button.tint : Ui.line
    }
    contentItem: Label {
        text: button.glyph
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        font.pixelSize: 15
        font.weight: Font.Bold
        color: button.active ? "#0e0e10" : button.tint
    }

    HoverHandler { cursorShape: Qt.PointingHandCursor }
    ToolTip.visible: hovered && tip !== ""
    ToolTip.delay: 500
    ToolTip.text: tip
}
