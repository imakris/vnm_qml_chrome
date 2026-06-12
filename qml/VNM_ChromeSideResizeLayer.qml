import QtQuick

Item {
    id: layer
    objectName: "side_resize_layer"

    property bool resize_enabled: true
    property real resize_border_width: 6

    signal resize_requested(int edges)

    function resize_edges_for_y(base_edges, local_y) {
        const corner_height = Math.max(0, Math.min(resize_border_width, height / 2))
        if (local_y <= corner_height) {
            return base_edges | Qt.TopEdge
        }
        if (local_y >= height - corner_height) {
            return base_edges | Qt.BottomEdge
        }
        return base_edges
    }

    function resize_cursor_for_y(base_edges, local_y) {
        const resolved_edges = resize_edges_for_y(base_edges, local_y)
        const left_side = (base_edges & Qt.LeftEdge) !== 0

        if ((resolved_edges & Qt.TopEdge) !== 0) {
            return left_side ? Qt.SizeFDiagCursor : Qt.SizeBDiagCursor
        }
        if ((resolved_edges & Qt.BottomEdge) !== 0) {
            return left_side ? Qt.SizeBDiagCursor : Qt.SizeFDiagCursor
        }
        return Qt.SizeHorCursor
    }

    z: 10

    VNM_ChromeResizeArea {
        objectName: "left_resize_area"
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        width: layer.resize_border_width
        enabled: layer.resize_enabled
        edges: Qt.LeftEdge
        resolve_edges: (mouse) => layer.resize_edges_for_y(Qt.LeftEdge, mouse.y)
        hoverEnabled: true
        cursorShape: containsMouse
            ? layer.resize_cursor_for_y(Qt.LeftEdge, mouseY)
            : Qt.SizeHorCursor

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
        resolve_edges: (mouse) => layer.resize_edges_for_y(Qt.RightEdge, mouse.y)
        hoverEnabled: true
        cursorShape: containsMouse
            ? layer.resize_cursor_for_y(Qt.RightEdge, mouseY)
            : Qt.SizeHorCursor

        onResize_requested: (edges) => layer.resize_requested(edges)
    }
}
