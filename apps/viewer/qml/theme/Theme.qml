pragma Singleton
import QtQuick

QtObject {
    id: theme

    // ── Theme ──────────────────────────────────────────────────────────
    property string mode: "dark"
    readonly property bool isDark: mode === "dark"
    function toggle() {
        mode = isDark ? "light" : "dark";
    }

    // ── Typography ─────────────────────────────────────────────────────
    readonly property string fontFamily: "Menlo"
    readonly property string fontFamilyFallback: "Monaco"
    readonly property int fontSizeTiny: 9
    readonly property int fontSizeSmall: 10
    readonly property int fontSizeNormal: 12
    readonly property int fontSizeMed: 13
    readonly property int fontSizeLarge: 16
    readonly property int fontSizeTitle: 20
    readonly property int fontSizeHero: 44

    // ── Spacing ────────────────────────────────────────────────────────
    readonly property int radius: 12
    readonly property int radiusSmall: 6
    readonly property int radiusLarge: 18
    readonly property int padding: 14
    readonly property int paddingSmall: 8
    readonly property int gap: 14
    readonly property int gapSmall: 8

    // ── Animation ──────────────────────────────────────────────────────
    readonly property int durFast: 120
    readonly property int durNorm: 200
    readonly property int durSlow: 320

    // ── Color tokens ───────────────────────────────────────────────────
    readonly property color bg: isDark ? "#07080f" : "#f4f5f8"
    readonly property color cardBg: isDark ? Qt.rgba(0.07, 0.078, 0.118, 0.72) : Qt.rgba(1, 1, 1,
                                                                                         0.82)

    readonly property color cardBgSolid: isDark ? "#14161f" : "#ffffff"
    readonly property color cardBorder: isDark ? Qt.rgba(1, 1, 1, 0.06) : Qt.rgba(0.06, 0.09, 0.16,
                                                                                  0.08)

    readonly property color cardBorderHi: isDark ? Qt.rgba(1, 1, 1, 0.12) : Qt.rgba(0.06, 0.09, 0.16,
                                                                                    0.16)
    readonly property color headerBg: isDark ? Qt.rgba(0.04, 0.043, 0.07, 0.85) : Qt.rgba(1, 1, 1,
                                                                                          0.85)

    readonly property color hover: isDark ? Qt.rgba(1, 1, 1, 0.04) : Qt.rgba(0.06, 0.09, 0.16, 0.04)
    readonly property color hoverStrong: isDark ? Qt.rgba(1, 1, 1, 0.08) : Qt.rgba(0.06, 0.09, 0.16,
                                                                                   0.08)
    readonly property color divider: isDark ? Qt.rgba(1, 1, 1, 0.06) : Qt.rgba(0.06, 0.09, 0.16,
                                                                               0.08)

    readonly property color text: isDark ? "#e5e9f5" : "#1e1e2e"
    readonly property color textDim: isDark ? "#6b7390" : "#6b7488"
    readonly property color textStrong: isDark ? "#ffffff" : "#000000"

    readonly property color accent: isDark ? "#7aa2ff" : "#1d5bd9"
    readonly property color accentSoft: isDark ? Qt.rgba(0.48, 0.635, 1.0, 0.16) : Qt.rgba(0.114, 0.357,
                                                                                           0.851, 0.12)
    readonly property color accentGlow: isDark ? Qt.rgba(0.48, 0.635, 1.0, 0.30) : Qt.rgba(0.114, 0.357,
                                                                                           0.851, 0.22)
    readonly property color accentAlt: isDark ? "#86efac" : "#2e7d32"
    readonly property color warning: isDark ? "#fdba74" : "#d97706"
    readonly property color danger: isDark ? "#f87171" : "#c62828"

    readonly property color idle: isDark ? "#2c3142" : "#c0c5d2"
    readonly property color qHigh: isDark ? "#7aa2ff" : "#1d5bd9"
    readonly property color qLow: isDark ? "#86efac" : "#2e7d32"
    readonly property color qDisk: isDark ? "#fdba74" : "#d97706"
    readonly property color qTape: isDark ? "#c4a5fc" : "#7b1fa2"
    readonly property color qPrint: isDark ? "#fde68a" : "#b08800"

    readonly property color scanColor: accent

    // ── PID color via golden angle (readonly var for AOT compatibility) ─
    readonly property var pidColor: function (pid) {
        var hue = ((pid * 137) % 360) / 360.0;
        var s = isDark ? 0.65 : 0.55;
        var l = isDark ? 0.60 : 0.45;
        return Qt.hsla(hue, s, l, 1.0);
    }

    // ── Color with alpha helper (readonly var = AOT compatible) ────────
    readonly property var withAlpha: function (c, a) {
        return Qt.rgba(c.r, c.g, c.b, a);
    }
}
