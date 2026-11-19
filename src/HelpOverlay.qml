import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// Every keyboard shortcut, shown with ?.
Rectangle {
    id: overlay
    signal close()
    color: Qt.rgba(0, 0, 0, 0.72)

    TapHandler { onTapped: overlay.close() }

    readonly property var keys: [
        ["↑ ↓  J K", qsTr("Move between titles")],
        ["Space", qsTr("Tick or untick for the selected person")],
        ["Shift+Space", qsTr("Tick for the second person when comparing")],
        ["Enter  +", qsTr("Add to or remove from the queue")],
        ["N", qsTr("Queue the next MCU title nobody has seen")],
        ["1  2  3", qsTr("List, Compare, Queue")],
        ["[  ]", qsTr("Previous or next person")],
        ["P", qsTr("Add a person")],
        ["R", qsTr("Rename the selected person")],
        ["F", qsTr("Next filter")],
        ["/  Ctrl+F", qsTr("Search")],
        ["← →", qsTr("Collapse or expand the current group")],
        ["C", qsTr("Collapse or expand everything")],
        ["T  Ctrl+↑ Ctrl+↓", qsTr("Queue: move to the top, up, down")],
        ["Delete", qsTr("Queue: remove the title")],
        ["Ctrl+Z  Ctrl+Shift+Z", qsTr("Undo and redo")],
        ["Esc", qsTr("Clear the search or close this")],
        ["?", qsTr("Show or hide shortcuts")],
        ["Q", qsTr("Quit")]
    ]

    Rectangle {
        anchors.centerIn: parent
        width: Math.min(560, parent.width - 48)
        height: Math.min(grid.implicitHeight + 96, parent.height - 48)
        radius: theme.radius
        color: Ui.surface
        border.color: Ui.line

        TapHandler {}   // keeps clicks on the card from closing it

        Label {
            id: heading
            x: 24; y: 20
            text: qsTr("Keyboard shortcuts")
            font.pixelSize: 18
            font.weight: Font.Black
            color: Ui.text
        }
        Flickable {
            anchors { left: parent.left; right: parent.right; top: heading.bottom; bottom: parent.bottom; margins: 24; topMargin: 14 }
            contentHeight: grid.implicitHeight
            clip: true
            GridLayout {
                id: grid
                width: parent.width
                columns: 2
                columnSpacing: 20
                rowSpacing: 8
                Repeater {
                    model: overlay.keys.length * 2
                    Label {
                        required property int index
                        readonly property var entry: overlay.keys[Math.floor(index / 2)]
                        text: index % 2 === 0 ? entry[0] : entry[1]
                        font.family: index % 2 === 0 ? Ui.mono : Qt.application.font.family
                        color: index % 2 === 0 ? theme.accent : Ui.text
                        Layout.fillWidth: index % 2 === 1
                        wrapMode: Text.Wrap
                    }
                }
            }
        }
    }
}
