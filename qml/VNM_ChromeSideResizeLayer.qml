import QtQuick

Item {
    id: layer
    objectName: "side_resize_layer"

    property bool resize_enabled: true
    property real resize_border_width: 6

    signal resize_requested(int edges)

    z: 10

    VNM_ChromeResizeArea {
        objectName: "left_resize_area"
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        width: layer.resize_border_width
        enabled: layer.resize_enabled
        edges: Qt.LeftEdge
        cursorShape: Qt.SizeHorCursor

        onResize_requested: (edges) => layer.resize_requested(edges)
    }

    VNM_ChromeResizeArea {
        objectName: "right_resize_area"
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        width: layer.resize_border_width
        enabled: layer.resize_enabled
        edges: Qt.RightEdge
        cursorShape: Qt.SizeHorCursor

        onResize_requested: (edges) => layer.resize_requested(edges)
    }
}
