pragma Singleton
import QtQuick 2.14

QtObject {
    readonly property color windowBg: "#070B14"
    readonly property color cardBg: "#111827"
    readonly property color cardBorder: "#374151"
    readonly property color textPrimary: "#F9FAFB"
    readonly property color textSecondary: "#9CA3AF"

    readonly property color accent: "#5B7FAE"
    readonly property color accentPressed: "#4A6B93"
    readonly property color accentText: "#F8FAFC"
    readonly property color accentWeak: "#22324A"
    readonly property color success: "#10B981"
    readonly property color danger: "#EF4444"
    // Backward-compatible alias used by legacy pages.
    readonly property color error: danger

    readonly property color inputBg: "#1F2937"
    readonly property color inputBorder: "#4B5563"

    readonly property int radiusSmall: 6
    readonly property int radiusNormal: 8
    readonly property int radiusLarge: 12

    readonly property int spacingSmall: 8
    readonly property int spacingNormal: 12
    // Backward-compatible alias used by existing pages.
    readonly property int spacingMedium: spacingNormal
    readonly property int spacingLarge: 16
    readonly property int spacingXLarge: 20

    readonly property int fontSizeSmall: 12
    readonly property int fontSizeNormal: 14
    readonly property int fontSizeLarge: 18
    readonly property int fontSizeTitle: 24
}
