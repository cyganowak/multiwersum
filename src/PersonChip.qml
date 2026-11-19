import QtQuick
import QtQuick.Controls

// One person: a colored initial, the name, and how much of the list they've seen.
AbstractButton {
    id: person
    property string name
    property string initial
    property color tint
    property int percent
    property bool selected

    implicitHeight: 38
    leftPadding: 6
    rightPadding: 12
    hoverEnabled: true
    focusPolicy: Qt.NoFocus

    background: Rectangle {
        radius: Math.min(4, theme.radius)
        color: person.selected ? Ui.raised : person.hovered ? "#1a1a20" : "transparent"
        border.width: person.selected ? 2 : 1
        border.color: person.selected ? person.tint : Ui.line
    }

    contentItem: Item {
        implicitWidth: row.implicitWidth
        implicitHeight: row.implicitHeight
        Row {
            id: row
            anchors.verticalCenter: parent.verticalCenter
            spacing: 8
            Rectangle {
                width: 24; height: 24
                radius: Math.min(4, theme.radius)
                color: person.tint
                Label {
                    anchors.centerIn: parent
                    text: person.initial
                    font.weight: Font.Black
                    color: "white"
                }
            }
            Label {
                anchors.verticalCenter: parent.verticalCenter
                text: person.name
                font.weight: Font.DemiBold
                color: Ui.text
            }
            Label {
                anchors.verticalCenter: parent.verticalCenter
                text: person.percent + "%"
                font.family: Ui.mono
                color: Ui.muted
            }
        }
    }

    HoverHandler { cursorShape: Qt.PointingHandCursor }
    ToolTip.visible: hovered
    ToolTip.delay: 600
    ToolTip.text: qsTr("Double-click to rename")
}
