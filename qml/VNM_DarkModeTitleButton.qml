import QtQuick

VNM_TitleBarRoundButton {
    id: control

    property bool dark_mode: false
    property string dark_label: "D"
    property string light_label: "L"
    property string dark_tooltip_text: "Switch to dark mode"
    property string light_tooltip_text: "Switch to light mode"

    signal toggle_requested()

    label: dark_mode ? light_label : dark_label
    tooltip_text: dark_mode ? light_tooltip_text : dark_tooltip_text

    onClicked: toggle_requested()
}
