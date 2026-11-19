import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import QtQml

ApplicationWindow {
    id: win
    width: 1040
    height: 780
    minimumWidth: 640
    minimumHeight: 480
    visible: true
    title: "Multiwersum"
    color: Ui.background

    Material.theme: Material.Dark
    Material.accent: theme.accent
    Material.roundedScale: theme.radius

    readonly property string mode: backend.mode
    readonly property var activePerson: backend.profiles[backend.activeProfile] || ({ name: "", count: 0, percent: 0, color: "#888" })
    property bool helpVisible: false

    onModeChanged: Ui.current = -1

    function moveCurrent(direction) {
        const from = Ui.current < 0 ? (direction > 0 ? -1 : list.count) : Ui.current
        const row = backend.nextRow(from, direction)
        if (row >= 0) {
            Ui.current = row
            list.positionViewAtIndex(row, ListView.Contain)
        }
    }
    function currentItem() { return backend.itemAt(Ui.current) }
    function toggleCurrent(second) {
        const id = currentItem()
        if (id === "") { moveCurrent(1); return }
        const person = mode === "compare" ? (second ? backend.compareB : backend.compareA) : backend.activeProfile
        backend.toggleWatched(id, person)
    }
    function openRename(index) {
        renameDialog.person = index
        renameDialog.open()
    }
    function focusList() { list.forceActiveFocus() }

    // ------------------------------------------------------------ keyboard
    Shortcut { sequence: "?"; context: Qt.ApplicationShortcut; onActivated: win.helpVisible = !win.helpVisible }
    Shortcut { sequence: "Q"; context: Qt.ApplicationShortcut; onActivated: Qt.quit() }
    Shortcut { sequences: [StandardKey.Undo]; context: Qt.ApplicationShortcut; onActivated: backend.undo() }
    Shortcut { sequences: ["Ctrl+Shift+Z", "Ctrl+Y"]; context: Qt.ApplicationShortcut; onActivated: backend.redo() }
    Shortcut { sequences: ["/", StandardKey.Find]; context: Qt.ApplicationShortcut; onActivated: { search.forceActiveFocus(); search.selectAll() } }
    Shortcut {
        sequence: "Escape"; context: Qt.ApplicationShortcut
        onActivated: {
            if (win.helpVisible) win.helpVisible = false
            else if (search.text !== "") { search.text = ""; win.focusList() }
            else win.focusList()
        }
    }
    Shortcut { sequence: "1"; context: Qt.ApplicationShortcut; onActivated: backend.setMode("list") }
    Shortcut { sequence: "2"; context: Qt.ApplicationShortcut; onActivated: backend.setMode("compare") }
    Shortcut { sequence: "3"; context: Qt.ApplicationShortcut; onActivated: backend.setMode("queue") }
    Shortcut { sequence: "["; context: Qt.ApplicationShortcut; onActivated: backend.selectProfile((backend.activeProfile + backend.profiles.length - 1) % backend.profiles.length) }
    Shortcut { sequence: "]"; context: Qt.ApplicationShortcut; onActivated: backend.selectProfile((backend.activeProfile + 1) % backend.profiles.length) }
    Shortcut { sequences: ["Down", "J"]; context: Qt.ApplicationShortcut; onActivated: win.moveCurrent(1) }
    Shortcut { sequences: ["Up", "K"]; context: Qt.ApplicationShortcut; onActivated: win.moveCurrent(-1) }
    Shortcut { sequence: "Space"; context: Qt.ApplicationShortcut; onActivated: win.toggleCurrent(false) }
    Shortcut { sequence: "Shift+Space"; context: Qt.ApplicationShortcut; enabled: win.mode === "compare"; onActivated: win.toggleCurrent(true) }
    Shortcut {
        sequences: ["Return", "+"]; context: Qt.ApplicationShortcut; enabled: win.mode !== "queue"
        onActivated: { const id = win.currentItem(); if (id !== "") backend.toggleQueue(id) }
    }
    Shortcut { sequence: "T"; context: Qt.ApplicationShortcut; enabled: win.mode === "queue"; onActivated: win.queueOp("top") }
    Shortcut { sequence: "Ctrl+Up"; context: Qt.ApplicationShortcut; enabled: win.mode === "queue"; onActivated: win.queueOp("up") }
    Shortcut { sequence: "Ctrl+Down"; context: Qt.ApplicationShortcut; enabled: win.mode === "queue"; onActivated: win.queueOp("down") }
    Shortcut { sequences: ["Delete", "Backspace"]; context: Qt.ApplicationShortcut; enabled: win.mode === "queue"; onActivated: win.queueOp("remove") }
    Shortcut { sequence: "N"; context: Qt.ApplicationShortcut; onActivated: backend.queueNext() }
    Shortcut { sequence: "F"; context: Qt.ApplicationShortcut; onActivated: backend.cycleFilter() }
    Shortcut { sequence: "C"; context: Qt.ApplicationShortcut; enabled: win.mode !== "queue"; onActivated: backend.setAllOpen(backend.allCollapsed) }
    Shortcut { sequence: "Left"; context: Qt.ApplicationShortcut; enabled: win.mode !== "queue"; onActivated: backend.setSectionOpen(backend.sectionAt(Ui.current), false) }
    Shortcut { sequence: "Right"; context: Qt.ApplicationShortcut; enabled: win.mode !== "queue"; onActivated: backend.setSectionOpen(backend.sectionAt(Ui.current), true) }
    Shortcut { sequence: "R"; context: Qt.ApplicationShortcut; onActivated: win.openRename(backend.activeProfile) }
    Shortcut { sequence: "P"; context: Qt.ApplicationShortcut; onActivated: { const i = backend.addProfile(); if (i >= 0) win.openRename(i) } }

    function queueOp(op) {
        const id = currentItem()
        if (id === "") return
        backend.moveInQueue(id, op)
        // In the queue view rows are only titles, so the moved title's new row
        // is its new place in the queue.
        Ui.current = op === "remove" ? Math.min(Ui.current, list.count - 1) : list.indexOfItem(id)
        if (Ui.current >= 0) list.positionViewAtIndex(Ui.current, ListView.Contain)
    }

    // Right-to-left languages mirror the window; the Translator sets the
    // application's layout direction.
    Item {
        anchors.fill: parent
        LayoutMirroring.enabled: Qt.application.layoutDirection === Qt.RightToLeft
        LayoutMirroring.childrenInherit: true

        ColumnLayout {
            anchors.fill: parent
            spacing: 0

            // -------------------------------------------------------- header
            Rectangle {
                Layout.fillWidth: true
                implicitHeight: head.implicitHeight + 32
                color: Ui.surface

                Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: Ui.line }

                ColumnLayout {
                    id: head
                    x: Ui.inset(parent.width)
                    y: 16
                    width: parent.width - 2 * x
                    spacing: 12

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 8
                        Label {
                            text: "Multiwersum"
                            font.pixelSize: 26
                            font.weight: Font.Black
                            font.letterSpacing: 1.5
                            font.capitalization: Font.AllUppercase
                            color: Ui.text
                        }
                        Item { Layout.fillWidth: true }
                        Repeater {
                            model: [
                                { key: "list", label: qsTr("List") },
                                { key: "compare", label: qsTr("Compare") },
                                { key: "queue", label: qsTr("Queue") }
                            ]
                            Chip {
                                required property var modelData
                                text: modelData.label
                                count: modelData.key === "queue" ? String(backend.queueCount) : ""
                                checked: backend.mode === modelData.key
                                enabled: modelData.key !== "compare" || backend.profiles.length > 1
                                onClicked: backend.setMode(modelData.key)
                            }
                        }
                        Chip { text: "?"; onClicked: win.helpVisible = true; ToolTip.visible: hovered; ToolTip.text: qsTr("Keyboard shortcuts") }
                    }

                    Label {
                        visible: backend.error !== ""
                        text: backend.error
                        color: "#ff6b6b"
                        wrapMode: Text.Wrap
                        Layout.fillWidth: true
                    }

                    Flow {
                        Layout.fillWidth: true
                        spacing: 8
                        Repeater {
                            model: backend.profiles
                            PersonChip {
                                required property var modelData
                                required property int index
                                name: modelData.name
                                initial: modelData.initial
                                tint: modelData.color
                                percent: modelData.percent
                                selected: win.mode !== "compare" && index === backend.activeProfile
                                onClicked: backend.selectProfile(index)
                                onDoubleClicked: win.openRename(index)
                            }
                        }
                        Chip {
                            visible: backend.profiles.length < backend.maxProfiles
                            text: qsTr("+ Add person")
                            subtle: true
                            onClicked: { const i = backend.addProfile(); if (i >= 0) win.openRename(i) }
                        }
                        Chip {
                            text: qsTr("Rename")
                            subtle: true
                            onClicked: win.openRename(backend.activeProfile)
                        }
                    }

                    // The list view's progress for the selected person.
                    RowLayout {
                        visible: win.mode === "list"
                        Layout.fillWidth: true
                        spacing: 12
                        Rectangle {
                            Layout.fillWidth: true
                            implicitHeight: 10
                            radius: Ui.small
                            color: Ui.line
                            Rectangle {
                                height: parent.height
                                radius: parent.radius
                                width: parent.width * win.activePerson.percent / 100
                                color: win.activePerson.color
                                Behavior on width { NumberAnimation { duration: 250; easing.type: Easing.OutCubic } }
                            }
                        }
                        Label {
                            text: qsTr("%1 / %2 · %3%").arg(win.activePerson.count).arg(backend.total).arg(win.activePerson.percent)
                            font.family: Ui.mono
                            color: Ui.text
                        }
                    }

                    // Who is compared with whom.
                    RowLayout {
                        visible: win.mode === "compare"
                        Layout.fillWidth: true
                        spacing: 12
                        ComboBox {
                            Layout.fillWidth: true
                            model: backend.profiles
                            textRole: "name"
                            currentIndex: backend.compareA
                            onActivated: (i) => backend.setCompare(i, backend.compareB)
                            Material.foreground: (backend.profiles[backend.compareA] || {}).color || Ui.text
                        }
                        Label { text: qsTr("vs"); font.pixelSize: 18; font.weight: Font.Black; color: Ui.muted }
                        ComboBox {
                            Layout.fillWidth: true
                            model: backend.profiles
                            textRole: "name"
                            currentIndex: backend.compareB
                            onActivated: (i) => backend.setCompare(backend.compareA, i)
                            Material.foreground: (backend.profiles[backend.compareB] || {}).color || Ui.text
                        }
                    }

                    Label {
                        visible: win.mode === "queue"
                        text: backend.queueSeen > 0
                              ? qsTr("%1 · %2").arg(qsTr("%n in the queue", "", backend.queueCount)).arg(qsTr("%n seen by everyone", "", backend.queueSeen))
                              : qsTr("%n in the queue", "", backend.queueCount)
                        color: Ui.muted
                    }

                    // Filters, queue actions, and search.
                    Flow {
                        Layout.fillWidth: true
                        spacing: 8

                        Repeater {
                            model: win.mode === "list" ? [
                                { key: "all", label: qsTr("Everything") },
                                { key: "film", label: qsTr("Films") },
                                { key: "series", label: qsTr("Series") },
                                { key: "todo", label: qsTr("Not watched") }
                            ] : win.mode === "compare" ? [
                                { key: "all", label: qsTr("Everything"), n: backend.total },
                                { key: "a", label: qsTr("Only %1").arg((backend.profiles[backend.compareA] || {}).name), n: backend.stats.a },
                                { key: "b", label: qsTr("Only %1").arg((backend.profiles[backend.compareB] || {}).name), n: backend.stats.b },
                                { key: "both", label: qsTr("Both"), n: backend.stats.both },
                                { key: "none", label: qsTr("Neither"), n: backend.stats.none }
                            ] : []
                            Chip {
                                required property var modelData
                                text: modelData.label
                                count: modelData.n !== undefined ? String(modelData.n) : ""
                                checked: backend.filter === modelData.key
                                onClicked: backend.setFilter(modelData.key)
                            }
                        }
                        Chip {
                            visible: win.mode === "queue"
                            text: qsTr("+ Next unwatched MCU title")
                            enabled: backend.hasNext
                            onClicked: backend.queueNext()
                        }
                        Chip {
                            visible: win.mode === "queue" && backend.queueSeen > 0
                            text: qsTr("Remove what everyone has seen")
                            count: String(backend.queueSeen)
                            onClicked: backend.queueClean()
                        }
                        TextField {
                            id: search
                            width: Math.min(280, head.width)
                            placeholderText: qsTr("Search titles")
                            onTextChanged: backend.setQuery(text)
                            Keys.onReturnPressed: win.focusList()
                            Keys.onDownPressed: { win.focusList(); win.moveCurrent(1) }
                        }
                        Chip {
                            visible: win.mode !== "queue"
                            subtle: true
                            text: backend.allCollapsed ? qsTr("Expand all") : qsTr("Collapse all")
                            onClicked: backend.setAllOpen(backend.allCollapsed)
                        }
                    }
                }
            }

            // ---------------------------------------------------------- list
            ListView {
                id: list
                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true
                focus: true
                model: backend.rows
                topMargin: 4
                bottomMargin: 32
                boundsBehavior: Flickable.StopAtBounds
                ScrollBar.vertical: ScrollBar {}

                function indexOfItem(id) {
                    for (let r = 0; r < count; ++r)
                        if (backend.itemAt(r) === id) return r
                    return -1
                }

                delegate: DelegateChooser {
                    role: "kind"
                    DelegateChoice { roleValue: "saga"; delegate: SagaRow {} }
                    DelegateChoice { roleValue: "section"; delegate: SectionRow {} }
                    DelegateChoice { roleValue: "item"; delegate: TitleRow {} }
                    DelegateChoice { roleValue: "queued"; delegate: QueuedRow {} }
                }

                Label {
                    anchors.centerIn: parent
                    width: Math.min(parent.width - 48, 460)
                    visible: win.mode === "queue" && backend.queueCount === 0
                    horizontalAlignment: Text.AlignHCenter
                    wrapMode: Text.Wrap
                    color: Ui.muted
                    text: qsTr("The queue is empty. Add titles with + in the List or Compare view, or press N to add the next MCU title nobody has seen.")
                }
            }
        }

        HelpOverlay {
            anchors.fill: parent
            visible: win.helpVisible
            onClose: win.helpVisible = false
        }
    }

    // ---------------------------------------------------------- rename
    Dialog {
        id: renameDialog
        property int person: 0
        property bool armed: false
        anchors.centerIn: parent
        width: Math.min(420, win.width - 48)
        modal: true
        title: qsTr("Person")
        standardButtons: Dialog.Save | Dialog.Cancel
        // Popups don't inherit the window's Material settings, so the dialog
        // states them and draws its own corners to match the windows.
        Material.theme: Material.Dark
        Material.accent: theme.accent
        Material.roundedScale: theme.radius
        Overlay.modal: Rectangle { color: Qt.rgba(0, 0, 0, 0.6) }
        background: Rectangle {
            color: Ui.surface
            radius: theme.radius
            border.color: Ui.line
        }
        footer: DialogButtonBox {
            background: Item {}
        }
        header: Label {
            text: renameDialog.title
            padding: 24
            bottomPadding: 8
            font.pixelSize: 18
            font.weight: Font.Black
            color: Ui.text
        }
        onAboutToShow: {
            armed = false
            nameField.text = (backend.profiles[person] || {}).name || ""
            nameField.forceActiveFocus()
            nameField.selectAll()
        }
        onAccepted: backend.renameProfile(person, nameField.text)
        onClosed: win.focusList()

        ColumnLayout {
            anchors.fill: parent
            spacing: 16
            TextField {
                id: nameField
                Layout.fillWidth: true
                placeholderText: qsTr("Name")
                maximumLength: 24
                onAccepted: renameDialog.accept()
            }
            Button {
                visible: backend.profiles.length > 1
                flat: !renameDialog.armed
                highlighted: renameDialog.armed
                Material.accent: "#e5484d"
                text: renameDialog.armed
                      ? qsTr("Remove %1 and their ticks").arg((backend.profiles[renameDialog.person] || {}).name)
                      : qsTr("Remove this person…")
                onClicked: {
                    if (!renameDialog.armed) { renameDialog.armed = true; return }
                    backend.removeProfile(renameDialog.person)
                    renameDialog.close()
                }
            }
        }
    }
}
