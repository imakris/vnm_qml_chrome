import QtQuick

VNM_TitleBarRoundButton {
    id: control

    property string primary_label: "DE"
    property string secondary_label: "EN"
    property bool primary_active: true
    property string primary_tooltip_text: "Switch language"
    property string secondary_tooltip_text: "Switch language"

    signal toggle_requested()

    label: primary_active ? primary_label : secondary_label
    tooltip_text: primary_active ? secondary_tooltip_text : primary_tooltip_text

    onClicked: toggle_requested()
}
