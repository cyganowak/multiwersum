pragma Singleton
import QtQuick

// The look every component shares, and the list row the keyboard is on.
QtObject {
    readonly property color background: "#0e0e10"
    readonly property color surface: "#16161a"
    readonly property color raised: "#1e1e24"
    readonly property color line: "#2b2b33"
    readonly property color text: "#ececf1"
    readonly property color muted: "#8b8b96"
    readonly property color done: "#3fb27f"
    readonly property int small: Math.min(4, theme.radius)
    readonly property string mono: "monospace"

    property int current: -1

    function inset(width) { return Math.max(16, (width - 960) / 2) }

    // Reading translator.language makes every binding that calls these
    // re-render when the Omarchy language changes.
    function typeLabel(type) {
        translator.language
        return type === "film" ? qsTr("film") : type === "series" ? qsTr("series")
             : type === "animated" ? qsTr("animated") : qsTr("special")
    }
    function titleText(title, season) {
        translator.language
        return season > 0 ? qsTr("%1 – season %2").arg(title).arg(season) : title
    }
}
