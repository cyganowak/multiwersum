import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// A phase or group: its badge, name, progress per person, and a click to
// collapse or expand it.
Item {
    id: row
    required property bool shown
    required property string sectionId
    required property string badge
    required property string name
    required property string subtitle
    required property bool open
    required property bool complete
    required property var counts

    width: ListView.view.width
    height: shown ? 82 : 0
    visible: shown

    Rectangle {
        x: Ui.inset(row.width)
        y: 10
        width: row.width - 2 * x
        height: parent.height - 10
        color: hover.hovered ? "#22222a" : Ui.raised
        border.width: 1
        border.color: Ui.line

        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 12
            anchors.rightMargin: 16
            spacing: 14

            Rectangle {
                implicitWidth: Math.max(48, badgeLabel.implicitWidth + 16)
                implicitHeight: 48
                radius: Math.min(4, theme.radius)
                color: row.complete ? Ui.done : theme.accent
                Label {
                    id: badgeLabel
                    anchors.centerIn: parent
                    text: row.badge
                    font.pixelSize: row.badge.length > 2 ? 12 : 26
                    font.weight: Font.Black
                    font.letterSpacing: row.badge.length > 2 ? 1 : 0
                    color: row.complete ? "white" : theme.accentForeground
                }
            }

            ColumnLayout {
                Layout.fillWidth: true
                spacing: 2
                Label {
                    Layout.fillWidth: true
                    text: qsTr(row.name) + (row.open ? "  ▾" : "  ▸")
                    font.pixelSize: 18
                    font.weight: Font.Black
                    font.capitalization: Font.AllUppercase
                    elide: Text.ElideRight
                    color: Ui.text
                }
                Label {
                    Layout.fillWidth: true
                    text: qsTr(row.subtitle)
                    font.pixelSize: 13
                    elide: Text.ElideRight
                    color: Ui.muted
                }
            }

            ColumnLayout {
                spacing: 6
                Repeater {
                    model: row.counts
                    RowLayout {
                        required property var modelData
                        spacing: 8
                        Layout.alignment: Qt.AlignRight
                        Label {
                            text: (row.counts.length > 1 ? modelData.initial + " " : "") + modelData.count + " / " + modelData.total
                            font.family: Ui.mono
                            font.pixelSize: 12
                            color: row.counts.length > 1 ? modelData.color : Ui.text
                        }
                        Rectangle {
                            implicitWidth: 80
                            implicitHeight: 5
                            color: Ui.line
                            Rectangle {
                                height: parent.height
                                width: parent.width * modelData.count / modelData.total
                                color: modelData.color
                            }
                        }
                    }
                }
            }
        }

        HoverHandler { id: hover; cursorShape: Qt.PointingHandCursor }
        TapHandler { onTapped: backend.toggleSection(row.sectionId) }
    }
}
