import QtQuick
import QtQuick.Window
import VNM_Chrome

Window {
    id: root

    width: 720
    height: 420
    visible: true
    flags: Qt.Window | Qt.FramelessWindowHint
    title: "VNM Chrome Example"
    color: "#1b1d20"

    readonly property real device_pixel_ratio: root.screen
        ? root.screen.devicePixelRatio
        : 1
    readonly property real frame_width: 1
    readonly property color frame_border: chrome_theme.window_frame_border
    readonly property real frame_inset: visibility !== Window.FullScreen
        && frame_width > 0
        && frame_border.a > 0
            ? frame_width
            : 0
    readonly property real content_inset: native_window_frame.active ? 0 : frame_inset
    readonly property rect content_rect: VNM_chrome_geometry.snapped_logical_rect(
        Qt.rect(
            content_inset,
            content_inset,
            Math.max(0, width  - 2 * content_inset),
            Math.max(0, height - 2 * content_inset)),
        device_pixel_ratio)

    VNM_ChromeTheme {
        id: chrome_theme

        titlebar: "#202124"
        titlebar_content_border: "#35363a"
        window_frame_border: "#35363a"
    }

    function toggle_window_maximized() {
        if (visibility === Window.Maximized) {
            showNormal()
        }
        else {
            showMaximized()
        }
    }

    VNM_NativeWindowFrame {
        id: native_window_frame

        window: root
        frame_visible: root.visibility !== Window.FullScreen
            && root.frame_inset > 0
        frame_width: root.frame_width
        frame_color: root.frame_border
    }

    Rectangle {
        anchors.fill: parent
        color: root.frame_inset > 0
            ? root.frame_border
            : root.color
        visible: root.visibility !== Window.FullScreen
            && !native_window_frame.active
        z: -100
    }

    VNM_ChromeSideResizeLayer {
        anchors.fill: parent
        resize_enabled: root.visibility === Window.Windowed
        resize_border_width: 6
        onResize_requested: (edges) =>
            VNM_system_window.start_system_resize(root, edges)
    }

    VNM_ChromeBottomResizeLayer {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        height: 6
        resize_enabled: root.visibility === Window.Windowed
        resize_border_width: 6
        onResize_requested: (edges) =>
            VNM_system_window.start_system_resize(root, edges)
    }

    Item {
        x: root.content_rect.x
        y: root.content_rect.y
        width: root.content_rect.width
        height: root.content_rect.height
        clip: true

        VNM_ChromeTitleBar {
            id: titlebar

            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            height: 32
            title: root.title
            active: root.active
            maximized: root.visibility === Window.Maximized
            resize_enabled: root.visibility === Window.Windowed
            resize_border_width: 6
            theme: chrome_theme

            onMove_requested: VNM_system_window.start_system_move(root)
            onResize_requested: (edges) =>
                VNM_system_window.start_system_resize(root, edges)
            onMinimize_requested: root.showMinimized()
            onMaximize_toggle_requested: root.toggle_window_maximized()
            onClose_requested: root.close()
        }

        Rectangle {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: titlebar.bottom
            anchors.bottom: parent.bottom
            color: "#263238"
        }
    }
}
