import QtQuick
import QtQuick.Controls

// Whether one person has watched a title. Shows a check, or the person's
// initial when several people share a row.
Rectangle {
    id: box
    property bool on: false
    property color tint: "white"
    property string letter: ""
    property string person: ""
    signal clicked()

    implicitWidth: 28
    implicitHeight: 28
    radius: Math.min(4, theme.radius)
    color: on ? tint : "transparent"
    border.width: 2
    border.color: on ? tint : (letter !== "" ? tint : "#5a5a66")

    Label {
        anchors.centerIn: parent
        text: box.letter !== "" ? box.letter : "✓"
        visible: box.letter !== "" || box.on
        font.pixelSize: box.letter !== "" ? 14 : 16
        font.weight: Font.Black
        color: box.on ? "white" : box.tint
    }

    MouseArea {
        anchors.fill: parent
        cursorShape: Qt.PointingHandCursor
        onClicked: box.clicked()
    }
    HoverHandler { id: hover }
    ToolTip.visible: hover.hovered && person !== ""
    ToolTip.delay: 500
    ToolTip.text: person
}
