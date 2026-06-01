import QtQuick

MouseArea {
    id: control

    property int edges: 0

    signal resize_requested(int edges)

    acceptedButtons: Qt.LeftButton

    onPressed: (mouse) => {
        resize_requested(edges)
        mouse.accepted = true
    }
}
