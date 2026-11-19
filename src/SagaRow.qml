import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// The heading above a group of phases ("The Infinity Saga").
Item {
    id: row
    required property bool shown
    required property string name

    width: ListView.view.width
    height: shown ? 52 : 0
    visible: shown

    RowLayout {
        x: Ui.inset(row.width)
        width: row.width - 2 * x
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 10
        spacing: 12
        Label {
            text: qsTr(row.name)
            font.pixelSize: 12
            font.weight: Font.Bold
            font.letterSpacing: 2
            font.capitalization: Font.AllUppercase
            color: Ui.muted
        }
        Rectangle { Layout.fillWidth: true; implicitHeight: 1; color: Ui.line }
    }
}
