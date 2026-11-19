import QtQuick
import QtQuick.Controls

// A flat toggle button for views, filters, and actions. `count` adds a small
// number after the label; `subtle` drops the border for secondary actions.
AbstractButton {
    id: chip
    property string count: ""
    property bool subtle: false

    implicitHeight: 34
    leftPadding: 12
    rightPadding: 12
    hoverEnabled: true
    focusPolicy: Qt.NoFocus
    opacity: enabled ? 1 : 0.4

    background: Rectangle {
        radius: Math.min(4, theme.radius)
        color: chip.checked ? theme.accent : chip.hovered ? Ui.raised : "transparent"
        border.width: chip.subtle && !chip.checked ? 0 : 1
        border.color: chip.checked ? theme.accent : Ui.line
    }

    contentItem: Item {
        implicitWidth: row.implicitWidth
        implicitHeight: row.implicitHeight
        Row {
            id: row
            anchors.verticalCenter: parent.verticalCenter
            spacing: 6
            Label {
                text: chip.text
                font.weight: Font.DemiBold
                color: chip.checked ? theme.accentForeground : chip.subtle ? Ui.muted : Ui.text
            }
            Label {
                visible: chip.count !== ""
                text: chip.count
                font.family: Ui.mono
                opacity: 0.7
                color: chip.checked ? theme.accentForeground : Ui.text
            }
        }
    }

    HoverHandler { cursorShape: Qt.PointingHandCursor }
}
