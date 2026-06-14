#include "vnm_qml_chrome/vnm_system_window.h"

#include <QSize>
#include <QWindow>

#include <cmath>

#ifdef Q_OS_WIN
#include <dwmapi.h>
#include <windows.h>
#endif

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

bool nearly_equal(qreal lhs, qreal rhs)
{
    return std::abs(lhs - rhs) <= 0.001;
}

#ifdef Q_OS_WIN

HWND window_hwnd(QWindow* window)
{
    if (!window) {
        return nullptr;
    }

    HWND hwnd = reinterpret_cast<HWND>(window->winId());
    return hwnd && IsWindow(hwnd) ? hwnd : nullptr;
}

QSize dwm_window_physical_size(HWND hwnd)
{
    RECT bounds{};
    const HRESULT result = DwmGetWindowAttribute(
        hwnd,
        DWMWA_EXTENDED_FRAME_BOUNDS,
        &bounds,
        sizeof(bounds));
    if (FAILED(result)) {
        return QSize();
    }

    const int width  = bounds.right - bounds.left;
    const int height = bounds.bottom - bounds.top;
    if (width <= 0 || height <= 0) {
        return QSize();
    }

    return QSize(width, height);
}

#endif

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

QSize vnm_qml_chrome::System_window::native_window_physical_size(
    QWindow* window,
    qreal    logical_width,
    qreal    logical_height) const
{
#ifdef Q_OS_WIN
    if (window
        && nearly_equal(logical_width,  window->width())
        && nearly_equal(logical_height, window->height())) {
        const QSize native_size = dwm_window_physical_size(window_hwnd(window));
        if (native_size.isValid()) {
            return native_size;
        }
    }
#else
    Q_UNUSED(window);
#endif

    Q_UNUSED(logical_width);
    Q_UNUSED(logical_height);
    return QSize();
}
