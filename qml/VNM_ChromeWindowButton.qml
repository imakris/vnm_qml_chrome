import QtQuick
import QtQuick.Controls.Basic as Basic

Basic.Button {
    id: control

    property VNM_ChromeTheme theme: VNM_ChromeTheme {}
    property color background_color: "transparent"
    property color hover_color: theme.titlebar_button_hover
    property color pressed_color: theme.titlebar_button_pressed
    default property alias icon_content: icon_slot.data

    padding: 0
    hoverEnabled: true
    flat: true

    contentItem: Item {
        id: icon_slot
    }

    background: Rectangle {
        color: control.pressed
            ? control.pressed_color
            : (control.hovered
                ? control.hover_color
                : control.background_color)
    }
}
