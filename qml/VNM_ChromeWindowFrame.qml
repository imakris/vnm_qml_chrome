import QtQuick

Item {
    id: frame
    objectName: "chrome_window_frame"

    property VNM_ChromeTheme theme: VNM_ChromeTheme {}
    property bool frame_visible: true
    property real frame_width: 1
    property bool top_edge_visible: true
    property bool bottom_edge_visible: true
    property bool left_edge_visible: true
    property bool right_edge_visible: true
    readonly property real effective_frame_width:
        isFinite(frame_width) ? Math.max(0, frame_width) : 0
    readonly property real horizontal_line_width:
        Math.min(effective_frame_width, Math.max(0, height))
    readonly property real vertical_line_width:
        Math.min(effective_frame_width, Math.max(0, width))
    readonly property bool effective_frame_visible:
        frame_visible
        && effective_frame_width > 0
        && theme.window_frame_border.a > 0

    z: 1000
    visible: effective_frame_visible
    enabled: false

    Rectangle {
        objectName: "chrome_window_frame_top"

        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        height: frame.horizontal_line_width
        color: frame.theme.window_frame_border
        visible: frame.effective_frame_visible
            && frame.top_edge_visible
            && height > 0
    }

    Rectangle {
        objectName: "chrome_window_frame_bottom"

        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        height: frame.horizontal_line_width
        color: frame.theme.window_frame_border
        visible: frame.effective_frame_visible
            && frame.bottom_edge_visible
            && height > 0
    }

    Rectangle {
        objectName: "chrome_window_frame_left"

        anchors.left: parent.left
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        width: frame.vertical_line_width
        color: frame.theme.window_frame_border
        visible: frame.effective_frame_visible
            && frame.left_edge_visible
            && width > 0
    }

    Rectangle {
        objectName: "chrome_window_frame_right"

        anchors.right: parent.right
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        width: frame.vertical_line_width
        color: frame.theme.window_frame_border
        visible: frame.effective_frame_visible
            && frame.right_edge_visible
            && width > 0
    }
}
