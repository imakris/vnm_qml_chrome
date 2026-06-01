import QtQuick

Item {
    id: layer
    objectName: "bottom_resize_layer"

    property bool resize_enabled: true
    property int resize_border_width: 6

    signal resize_requested(int edges)

    z: 30

    VNM_ChromeResizeArea {
        objectName: "bottom_resize_area"
        anchors.left: parent.left
        anchors.leftMargin: layer.resize_border_width
        anchors.right: parent.right
        anchors.rightMargin: layer.resize_border_width
        anchors.bottom: parent.bottom
        height: layer.resize_border_width
        enabled: layer.resize_enabled
        edges: Qt.BottomEdge
        cursorShape: Qt.SizeVerCursor
        z: 1

        onResize_requested: (edges) => layer.resize_requested(edges)
    }

    VNM_ChromeResizeArea {
        objectName: "bottom_left_resize_area"
        anchors.left: parent.left
        anchors.bottom: parent.bottom
        width: layer.resize_border_width
        height: layer.resize_border_width
        enabled: layer.resize_enabled
        edges: Qt.LeftEdge | Qt.BottomEdge
        cursorShape: Qt.SizeBDiagCursor
        z: 2

        onResize_requested: (edges) => layer.resize_requested(edges)
    }

    VNM_ChromeResizeArea {
        objectName: "bottom_right_resize_area"
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        width: layer.resize_border_width
        height: layer.resize_border_width
        enabled: layer.resize_enabled
        edges: Qt.RightEdge | Qt.BottomEdge
        cursorShape: Qt.SizeFDiagCursor
        z: 2

        onResize_requested: (edges) => layer.resize_requested(edges)
    }
}
