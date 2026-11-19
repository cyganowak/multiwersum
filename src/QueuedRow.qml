import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// One title in the queue: its place, who has seen it, and controls to
// reorder or remove it.
Item {
    id: row
    required property int index
    required property bool shown
    required property string itemId
    required property string title
    required property string type
    required property string date
    required property string sectionName
    required property int season
    required property int year
    required property int queuePosition
    required property bool soon
    required property bool allSeen
    required property var marks
    readonly property bool narrow: width < 720

    width: ListView.view.width
    height: shown ? (narrow ? 96 : 66) : 0
    visible: shown

    Rectangle {
        x: Ui.inset(row.width)
        y: row.index === 0 ? 12 : 0
        width: row.width - 2 * x
        height: parent.height - y
        color: hover.hovered ? "#1b1b21" : Ui.surface
        border.width: 1
        border.color: Ui.line

        HoverHandler { id: hover }
        TapHandler { onTapped: Ui.current = row.index }

        GridLayout {
            anchors.fill: parent
            anchors.leftMargin: 12
            anchors.rightMargin: 12
            anchors.topMargin: 8
            anchors.bottomMargin: 8
            columns: row.narrow ? 2 : 3
            columnSpacing: 14
            rowSpacing: 6

            Label {
                Layout.preferredWidth: 40
                horizontalAlignment: Text.AlignRight
                text: row.queuePosition
                font.pixelSize: 26
                font.weight: Font.Black
                font.family: Ui.mono
                color: row.allSeen ? Ui.done : theme.accent
            }

            ColumnLayout {
                Layout.fillWidth: true
                spacing: 3
                Label {
                    Layout.fillWidth: true
                    text: Ui.titleText(row.title, row.season)
                    font.pixelSize: 15
                    font.weight: Font.DemiBold
                    font.strikeout: row.allSeen
                    elide: Text.ElideRight
                    color: row.allSeen ? Ui.muted : Ui.text
                }
                RowLayout {
                    spacing: 6
                    Tag { text: Ui.typeLabel(row.type) }
                    Tag {
                        visible: row.soon
                        filled: true
                        text: row.date !== "" ? qsTr("upcoming · %1").arg(row.date) : qsTr("upcoming")
                    }
                    Label {
                        text: row.year + " · " + qsTr(row.sectionName)
                        font.pixelSize: 12
                        color: Ui.muted
                        elide: Text.ElideRight
                        Layout.fillWidth: true
                    }
                }
            }

            Item { visible: row.narrow; implicitWidth: 1 }

            // Who has seen it, then the queue controls. On a narrow window
            // this group wraps under the title.
            RowLayout {
                Layout.fillWidth: row.narrow
                spacing: 14
                Row {
                    spacing: 6
                    Repeater {
                        model: row.marks
                        MarkBox {
                            required property var modelData
                            on: modelData.on
                            tint: modelData.color
                            letter: modelData.initial
                            person: modelData.name
                            onClicked: { Ui.current = row.index; backend.toggleWatched(row.itemId, modelData.profile) }
                        }
                    }
                }
                Item { Layout.fillWidth: true; visible: row.narrow }
                Row {
                    spacing: 4
                    IconButton { glyph: "⤒"; tip: qsTr("Move to the top"); enabled: row.index > 0; onClicked: backend.moveInQueue(row.itemId, "top") }
                    IconButton { glyph: "↑"; tip: qsTr("Move up"); enabled: row.index > 0; onClicked: backend.moveInQueue(row.itemId, "up") }
                    IconButton { glyph: "↓"; tip: qsTr("Move down"); enabled: row.queuePosition < backend.queueCount; onClicked: backend.moveInQueue(row.itemId, "down") }
                    IconButton { glyph: "✕"; tip: qsTr("Remove from the queue"); tint: "#e5484d"; onClicked: backend.moveInQueue(row.itemId, "remove") }
                }
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
