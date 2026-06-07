import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Window

Rectangle {
    id: titlebar
    objectName: "titlebar"

    property VNM_ChromeTheme theme: VNM_ChromeTheme {}
    property string title: ""
    property bool active: true
    property bool maximized: false
    property bool resize_enabled: true
    property real resize_border_width: 6
    property real device_pixel_ratio: Screen.devicePixelRatio
    property bool animated_mark_visible: true
    property string activity_marker_text: ""
    property Component leading_action_component: null
    property Component trailing_action_component: null
    readonly property real snapped_resize_border_width:
        VNM_chrome_geometry.snapped_logical_edge(
            resize_border_width,
            device_pixel_ratio)
    readonly property real content_border_width: 1
    readonly property int move_drag_threshold: 2

    signal move_requested()
    signal resize_requested(int edges)
    signal minimize_requested()
    signal maximize_toggle_requested()
    signal close_requested()

    function maybe_start_system_move(move_area, mouse) {
        if (move_area.system_move_started || !(mouse.buttons & Qt.LeftButton)) {
            return
        }

        const delta_x = mouse.x - move_area.system_move_press_x
        const delta_y = mouse.y - move_area.system_move_press_y
        if (delta_x * delta_x + delta_y * delta_y
                < move_drag_threshold * move_drag_threshold) {
            return
        }

        move_area.system_move_started = true
        titlebar.move_requested()
        mouse.accepted = true
    }

    height: 32
    color: theme.titlebar
    z: 40

    MouseArea {
        id: titlebar_move_area

        property real system_move_press_x: 0
        property real system_move_press_y: 0
        property bool system_move_started: false

        anchors.fill: parent
        acceptedButtons: Qt.LeftButton
        z: 1

        onPressed: (mouse) => {
            if (mouse.button === Qt.LeftButton) {
                system_move_press_x = mouse.x
                system_move_press_y = mouse.y
                system_move_started = false
                mouse.accepted = true
            }
        }

        onPositionChanged: (mouse) => {
            titlebar.maybe_start_system_move(titlebar_move_area, mouse)
        }

        onReleased: system_move_started = false
        onCanceled: system_move_started = false

        onDoubleClicked: (mouse) => {
            if (mouse.button === Qt.LeftButton) {
                titlebar.maximize_toggle_requested()
                mouse.accepted = true
            }
        }
    }

    RowLayout {
        id: content_layout

        anchors.fill: parent
        anchors.leftMargin: titlebar.snapped_resize_border_width
        spacing: 0
        z: 2

        VNM_AnimatedMark {
            id: animated_mark
            objectName: "vnm_animated_mark"

            theme: titlebar.theme
            mark_size: 20
            move_enabled: true
            move_drag_threshold: titlebar.move_drag_threshold
            visible: titlebar.animated_mark_visible
            Layout.preferredWidth: mark_size
            Layout.preferredHeight: mark_size
            Layout.alignment: Qt.AlignVCenter
            Layout.rightMargin: titlebar.animated_mark_visible ? 8 : 0
            onMove_requested: titlebar.move_requested()
            onMaximize_toggle_requested: titlebar.maximize_toggle_requested()
        }

        Label {
            id: activity_marker_label
            objectName: "activity_marker_label"

            readonly property bool marker_visible: titlebar.activity_marker_text.length > 0

            Layout.alignment: Qt.AlignVCenter
            Layout.preferredWidth: marker_visible ? 18 : 0
            Layout.rightMargin: marker_visible ? 6 : 0
            visible: marker_visible
            text: titlebar.activity_marker_text
            color: titlebar.theme.titlebar_activity_marker
            elide: Text.ElideRight
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            font.pointSize: 9.5
        }

        Loader {
            id: leading_action_loader
            objectName: "leading_action_loader"

            sourceComponent: titlebar.leading_action_component
            active: titlebar.leading_action_component !== null
            Layout.rightMargin: active ? 6 : 0
            Layout.alignment: Qt.AlignVCenter
        }

        Label {
            id: title_label
            objectName: "title_label"

            Layout.alignment: Qt.AlignVCenter
            Layout.fillWidth: true
            Layout.minimumWidth: 0
            Layout.rightMargin: 8
            text: titlebar.title
            color: titlebar.theme.titlebar_text
            elide: Text.ElideRight
            font.pointSize: 9.5
        }

        Loader {
            id: trailing_action_loader
            objectName: "trailing_action_loader"

            sourceComponent: titlebar.trailing_action_component
            active: titlebar.trailing_action_component !== null
            Layout.rightMargin: active ? 8 : 0
            Layout.alignment: Qt.AlignVCenter
        }

        RowLayout {
            id: titlebar_buttons
            objectName: "titlebar_buttons"

            Layout.preferredWidth: 138
            Layout.fillHeight: true
            spacing: 0

            VNM_ChromeWindowButton {
                id: minimize_button
                objectName: "minimize_button"

                theme: titlebar.theme
                Layout.preferredWidth: 46
                Layout.fillHeight: true
                onClicked: titlebar.minimize_requested()

                Canvas {
                    anchors.fill: parent
                    property color stroke_color: titlebar.theme.titlebar_button_icon

                    onStroke_colorChanged: requestPaint()
                    onPaint: {
                        const ctx = getContext("2d")
                        ctx.clearRect(0, 0, width, height)
                        ctx.strokeStyle = stroke_color
                        ctx.lineWidth = 1.3
                        ctx.lineCap = "square"

                        const size = 10
                        const x = (width - size) / 2
                        const y = (height - size) / 2 + 3
                        ctx.beginPath()
                        ctx.moveTo(x, y)
                        ctx.lineTo(x + size, y)
                        ctx.stroke()
                    }

                    onWidthChanged: requestPaint()
                    onHeightChanged: requestPaint()
                }
            }

            VNM_ChromeWindowButton {
                id: maximize_button
                objectName: "maximize_button"

                theme: titlebar.theme
                Layout.preferredWidth: 46
                Layout.fillHeight: true
                onClicked: titlebar.maximize_toggle_requested()

                Canvas {
                    anchors.fill: parent
                    property color stroke_color: titlebar.theme.titlebar_button_icon
                    property bool restored: titlebar.maximized

                    onStroke_colorChanged: requestPaint()
                    onRestoredChanged: requestPaint()
                    onPaint: {
                        const ctx = getContext("2d")
                        ctx.clearRect(0, 0, width, height)
                        ctx.strokeStyle = stroke_color
                        ctx.lineWidth = 1.3
                        ctx.lineCap = "square"

                        const size = 10
                        let x = (width - size) / 2
                        let y = (height - size) / 2
                        if (restored) {
                            x -= 1
                            y += 1
                            ctx.strokeRect(x + 3.5, y - 2.5, size - 1, size - 1)
                            ctx.strokeRect(x + 0.5, y + 0.5, size - 1, size - 1)
                        }
                        else {
                            ctx.strokeRect(x + 0.5, y + 0.5, size - 1, size - 1)
                        }
                    }

                    onWidthChanged: requestPaint()
                    onHeightChanged: requestPaint()
                }
            }

            VNM_ChromeWindowButton {
                id: close_button
                objectName: "close_button"

                theme: titlebar.theme
                hover_color: titlebar.theme.titlebar_close_hover
                pressed_color: titlebar.theme.titlebar_close_pressed
                Layout.preferredWidth: 46
                Layout.fillHeight: true
                onClicked: titlebar.close_requested()

                Canvas {
                    anchors.fill: parent
                    property color stroke_color: titlebar.theme.titlebar_button_icon

                    onStroke_colorChanged: requestPaint()
                    onPaint: {
                        const ctx = getContext("2d")
                        ctx.clearRect(0, 0, width, height)
                        ctx.strokeStyle = stroke_color
                        ctx.lineWidth = 1.3
                        ctx.lineCap = "square"

                        const size = 8
                        const x = (width - size) / 2
                        const y = (height - size) / 2
                        ctx.beginPath()
                        ctx.moveTo(x, y)
                        ctx.lineTo(x + size, y + size)
                        ctx.moveTo(x, y + size)
                        ctx.lineTo(x + size, y)
                        ctx.stroke()
                    }

                    onWidthChanged: requestPaint()
                    onHeightChanged: requestPaint()
                }
            }
        }
    }

    Rectangle {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        height: titlebar.content_border_width
        color: titlebar.theme.titlebar_content_border
        visible: titlebar.theme.titlebar_content_border.a > 0
        z: 2
    }

    VNM_ChromeResizeArea {
        objectName: "top_resize_area"
        anchors.left: parent.left
        anchors.leftMargin: titlebar.snapped_resize_border_width
        anchors.top: parent.top
        anchors.right: parent.right
        anchors.rightMargin: titlebar_buttons.width
        height: titlebar.snapped_resize_border_width
        enabled: titlebar.resize_enabled
        edges: Qt.TopEdge
        cursorShape: Qt.SizeVerCursor
        z: 4

        onResize_requested: (edges) => titlebar.resize_requested(edges)
    }

    VNM_ChromeResizeArea {
        objectName: "top_left_resize_area"
        anchors.left: parent.left
        anchors.top: parent.top
        width: titlebar.snapped_resize_border_width
        height: titlebar.snapped_resize_border_width
        enabled: titlebar.resize_enabled
        edges: Qt.LeftEdge | Qt.TopEdge
        cursorShape: Qt.SizeFDiagCursor
        z: 5

        onResize_requested: (edges) => titlebar.resize_requested(edges)
    }

    VNM_ChromeResizeArea {
        objectName: "top_right_resize_area"
        anchors.right: parent.right
        anchors.top: parent.top
        width: titlebar.snapped_resize_border_width
        height: titlebar.snapped_resize_border_width
        enabled: titlebar.resize_enabled
        edges: Qt.RightEdge | Qt.TopEdge
        cursorShape: Qt.SizeBDiagCursor
        z: 5

        onResize_requested: (edges) => titlebar.resize_requested(edges)
    }
}
