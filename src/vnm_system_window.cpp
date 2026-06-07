#include "vnm_qml_chrome/vnm_system_window.h"

#include <QWindow>

namespace {

bool is_valid_resize_edges(Qt::Edges edges)
{
    switch (edges.toInt()) {
        case int(Qt::LeftEdge):
        case int(Qt::RightEdge):
        case int(Qt::TopEdge):
        case int(Qt::BottomEdge):
        case int(Qt::LeftEdge  | Qt::TopEdge):
        case int(Qt::RightEdge | Qt::TopEdge):
        case int(Qt::LeftEdge  | Qt::BottomEdge):
        case int(Qt::RightEdge | Qt::BottomEdge):
            return true;

        default:
            return false;
    }
}

} // namespace

vnm_qml_chrome::System_window::System_window(QObject* parent)
:
    QObject(parent)
{
}

bool vnm_qml_chrome::System_window::start_system_move(QWindow* window) const
{
    if (!window) {
        return false;
    }

    return window->startSystemMove();
}

bool vnm_qml_chrome::System_window::start_system_resize(
    QWindow* window,
    int      edges) const
{
    if (!window) {
        return false;
    }

    const Qt::Edges qt_edges = Qt::Edges::fromInt(edges);
    if (!is_valid_resize_edges(qt_edges)) {
        return false;
    }

    return window->startSystemResize(qt_edges);
}
