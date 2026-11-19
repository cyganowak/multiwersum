import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// One film or season in the List and Compare views. In List the whole row
// ticks it for the selected person; in Compare each person has a box.
Item {
    id: row
    required property int index
    required property bool shown
    required property string itemId
    required property string title
    required property string type
    required property string date
    required property string highlight
    required property int season
    required property int year
    required property bool soon
    required property bool queued
    required property var marks
    readonly property bool single: backend.mode === "list"
    readonly property bool seen: single && marks.length > 0 && marks[0].on

    width: ListView.view.width
    height: shown ? 58 : 0
    visible: shown

    Rectangle {
        id: card
        x: Ui.inset(row.width)
        width: row.width - 2 * x
        height: parent.height
        color: hover.hovered ? "#1b1b21" : Ui.surface
        border.width: 1
        border.color: Ui.line

        Rectangle {
            anchors.fill: parent
            anchors.margins: 1
            visible: row.highlight !== ""
            gradient: Gradient {
                orientation: Gradient.Horizontal
                GradientStop { position: 0; color: Qt.alpha(row.highlight || "#000000", 0.2) }
                GradientStop { position: 0.6; color: "transparent" }
            }
        }

        HoverHandler { id: hover }
        TapHandler {
            onTapped: {
                Ui.current = row.index
                if (row.single) backend.toggleWatched(row.itemId, backend.activeProfile)
            }
        }

        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 14
            anchors.rightMargin: 12
            spacing: 12

            Repeater {
                model: row.marks
                MarkBox {
                    required property var modelData
                    on: modelData.on
                    tint: modelData.color
                    letter: row.single ? "" : modelData.initial
                    person: modelData.name
                    onClicked: { Ui.current = row.index; backend.toggleWatched(row.itemId, modelData.profile) }
                }
            }

            ColumnLayout {
                Layout.fillWidth: true
                spacing: 3
                Label {
                    Layout.fillWidth: true
                    text: Ui.titleText(row.title, row.season)
                    font.pixelSize: 15
                    font.weight: Font.DemiBold
                    font.strikeout: row.seen
                    elide: Text.ElideRight
                    color: row.seen ? Ui.muted : Ui.text
                }
                RowLayout {
                    spacing: 6
                    Tag { text: Ui.typeLabel(row.type) }
                    Tag {
                        visible: row.soon
                        filled: true
                        text: row.date !== "" ? qsTr("upcoming · %1").arg(row.date) : qsTr("upcoming")
                    }
                }
            }

            Label {
                text: row.year
                font.family: Ui.mono
                color: Ui.muted
            }
            IconButton {
                glyph: row.queued ? "✓" : "+"
                active: row.queued
                tip: row.queued ? qsTr("In the queue – click to remove") : qsTr("Add to the queue")
                onClicked: { Ui.current = row.index; backend.toggleQueue(row.itemId) }
            }
        }

        Rectangle {
            anchors.fill: parent
            color: "transparent"
            border.width: 2
            border.color: theme.accent
            visible: Ui.current === row.index
        }
    }
}
