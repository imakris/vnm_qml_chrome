#pragma once

#include <QObject>
#include <QSize>

class QWindow;

namespace vnm_qml_chrome {

/**
 * @brief QML-facing wrapper for native QWindow move and resize operations.
 *
 * QWindow requires system move and resize requests to be made from the mouse
 * event handler that starts the operation. This singleton lets Qt 6.7 QML code
 * use the C++ API without depending on the QML Window methods added in Qt 6.8.
 * Resize edges must be one edge or two adjacent edges.
 */
class System_window : public QObject
{
    Q_OBJECT

public:
    explicit System_window(QObject* parent = nullptr);

    Q_INVOKABLE bool start_system_move(QWindow* window) const;
    Q_INVOKABLE bool start_system_resize(QWindow* window, int edges) const;
    Q_INVOKABLE QSize native_window_physical_size(
        QWindow* window,
        qreal    logical_width,
        qreal    logical_height) const;
};

} // namespace vnm_qml_chrome
