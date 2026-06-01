#pragma once

#include <QColor>
#include <QList>
#include <QMetaObject>
#include <QObject>
#include <QPointer>
#include <QWindow>
#include <QtGlobal>

#ifdef Q_OS_WIN
#include <array>
#endif

class VNM_NativeWindowFrame : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QWindow* window READ window WRITE set_window NOTIFY window_changed)
    Q_PROPERTY(bool frame_visible READ frame_visible WRITE set_frame_visible NOTIFY frame_visible_changed)
    Q_PROPERTY(qreal frame_width READ frame_width WRITE set_frame_width NOTIFY frame_width_changed)
    Q_PROPERTY(QColor frame_color READ frame_color WRITE set_frame_color NOTIFY frame_color_changed)
    Q_PROPERTY(bool active READ active NOTIFY active_changed)

public:
    explicit VNM_NativeWindowFrame(QObject* parent = nullptr);
    ~VNM_NativeWindowFrame() override;

    QWindow* window() const;
    void set_window(QWindow* window);

    bool frame_visible() const;
    void set_frame_visible(bool frame_visible);

    qreal frame_width() const;
    void set_frame_width(qreal frame_width);

    QColor frame_color() const;
    void set_frame_color(const QColor& frame_color);

    bool active() const;

signals:
    void window_changed();
    void frame_visible_changed();
    void frame_width_changed();
    void frame_color_changed();
    void active_changed();

private:
    void disconnect_window();
    void set_active(bool active);
    void update_native_frame();

#ifdef Q_OS_WIN
    bool should_use_native_frame() const;
    void* window_handle() const;
    bool ensure_edge_windows(void* parent_window_handle);
    bool apply_native_frame();
    void clear_native_frame();
    void hide_edge_windows();
    void destroy_edge_windows();
    void position_edge_windows(void* parent_window_handle);
    void repaint_edge_windows();
    int frame_width_px(void* parent_window_handle) const;

    std::array<void*, 4> m_edge_windows{};
#endif

    QPointer<QWindow> m_window;
    QList<QMetaObject::Connection> m_window_connections;
    bool m_frame_visible = true;
    qreal m_frame_width  = 1.0;
    QColor m_frame_color = Qt::black;
    bool m_active        = false;
};
