#include "vnm_qml_chrome/vnm_native_window_frame.h"

#include <QScreen>

#ifdef Q_OS_WIN
#include <windows.h>
#endif

#include <algorithm>
#include <cmath>

#ifdef Q_OS_WIN
#include <mutex>
#endif

namespace {

#ifdef Q_OS_WIN

constexpr wchar_t k_native_frame_window_class[] = L"VNM_NativeWindowFrameEdge";
constexpr int k_top_edge                        = 0;
constexpr int k_bottom_edge                     = 1;
constexpr int k_left_edge                       = 2;
constexpr int k_right_edge                      = 3;

HWND as_hwnd(void* handle)
{
    return static_cast<HWND>(handle);
}

void* from_hwnd(HWND hwnd)
{
    return static_cast<void*>(hwnd);
}

LRESULT CALLBACK native_frame_window_proc(
    HWND hwnd,
    UINT message,
    WPARAM w_param,
    LPARAM l_param)
{
    switch (message) {
        case WM_NCHITTEST:
            return HTTRANSPARENT;

        case WM_ERASEBKGND:
            return 1;

        case WM_PAINT: {
            PAINTSTRUCT paint_struct;
            HDC dc = BeginPaint(hwnd, &paint_struct);
            if (dc) {
                RECT client_rect{};
                GetClientRect(hwnd, &client_rect);

                auto* frame = reinterpret_cast<VNM_NativeWindowFrame*>(
                    GetWindowLongPtrW(hwnd, GWLP_USERDATA));
                const QColor frame_color = frame ? frame->frame_color() : QColor(Qt::black);
                HBRUSH brush = CreateSolidBrush(
                    RGB(frame_color.red(), frame_color.green(), frame_color.blue()));
                if (brush) {
                    FillRect(dc, &client_rect, brush);
                    DeleteObject(brush);
                }
            }
            EndPaint(hwnd, &paint_struct);
            return 0;
        }

        default:
            break;
    }

    return DefWindowProcW(hwnd, message, w_param, l_param);
}

ATOM ensure_native_frame_window_class()
{
    static std::once_flag register_flag;
    static ATOM window_class_atom = 0;

    std::call_once(register_flag, [] {
        WNDCLASSEXW window_class{};
        window_class.cbSize        = sizeof(window_class);
        window_class.lpfnWndProc   = native_frame_window_proc;
        window_class.hInstance     = GetModuleHandleW(nullptr);
        window_class.hCursor       = LoadCursorW(nullptr, IDC_ARROW);
        window_class.lpszClassName = k_native_frame_window_class;

        window_class_atom = RegisterClassExW(&window_class);
        if (window_class_atom == 0 && GetLastError() == ERROR_CLASS_ALREADY_EXISTS) {
            window_class_atom = 1;
        }
    });

    return window_class_atom;
}

#endif

} // namespace

VNM_NativeWindowFrame::VNM_NativeWindowFrame(QObject* parent)
    : QObject(parent)
{
}

VNM_NativeWindowFrame::~VNM_NativeWindowFrame()
{
#ifdef Q_OS_WIN
    destroy_edge_windows();
#endif
    disconnect_window();
}

QWindow* VNM_NativeWindowFrame::window() const
{
    return m_window;
}

void VNM_NativeWindowFrame::set_window(QWindow* window)
{
    if (m_window == window) {
        return;
    }

#ifdef Q_OS_WIN
    destroy_edge_windows();
#endif
    disconnect_window();

    m_window = window;
    if (m_window) {
        m_window_connections.push_back(QObject::connect(
            m_window,
            &QWindow::widthChanged,
            this,
            [this](int) { update_native_frame(); }));
        m_window_connections.push_back(QObject::connect(
            m_window,
            &QWindow::heightChanged,
            this,
            [this](int) { update_native_frame(); }));
        m_window_connections.push_back(QObject::connect(
            m_window,
            &QWindow::visibleChanged,
            this,
            [this](bool) { update_native_frame(); }));
        m_window_connections.push_back(QObject::connect(
            m_window,
            &QWindow::visibilityChanged,
            this,
            [this](QWindow::Visibility) { update_native_frame(); }));
        m_window_connections.push_back(QObject::connect(
            m_window,
            &QWindow::screenChanged,
            this,
            [this](QScreen*) { update_native_frame(); }));
        m_window_connections.push_back(QObject::connect(
            m_window,
            &QObject::destroyed,
            this,
            [this] {
#ifdef Q_OS_WIN
                for (void*& edge_window : m_edge_windows) {
                    edge_window = nullptr;
                }
#endif
                disconnect_window();
                m_window = nullptr;
                set_active(false);
                emit window_changed();
            }));
    }

    emit window_changed();
    update_native_frame();
}

bool VNM_NativeWindowFrame::frame_visible() const
{
    return m_frame_visible;
}

void VNM_NativeWindowFrame::set_frame_visible(bool frame_visible)
{
    if (m_frame_visible == frame_visible) {
        return;
    }

    m_frame_visible = frame_visible;
    emit frame_visible_changed();
    update_native_frame();
}

qreal VNM_NativeWindowFrame::frame_width() const
{
    return m_frame_width;
}

void VNM_NativeWindowFrame::set_frame_width(qreal frame_width)
{
    const qreal normalized_frame_width = std::isfinite(frame_width)
        ? std::max<qreal>(0.0, frame_width)
        : 0.0;

    if (qFuzzyCompare(m_frame_width + 1.0, normalized_frame_width + 1.0)) {
        return;
    }

    m_frame_width = normalized_frame_width;
    emit frame_width_changed();
    update_native_frame();
}

QColor VNM_NativeWindowFrame::frame_color() const
{
    return m_frame_color;
}

void VNM_NativeWindowFrame::set_frame_color(const QColor& frame_color)
{
    if (m_frame_color == frame_color) {
        return;
    }

    m_frame_color = frame_color;
    emit frame_color_changed();
    update_native_frame();
}

bool VNM_NativeWindowFrame::active() const
{
    return m_active;
}

void VNM_NativeWindowFrame::disconnect_window()
{
    for (const QMetaObject::Connection& connection : m_window_connections) {
        QObject::disconnect(connection);
    }
    m_window_connections.clear();
}

void VNM_NativeWindowFrame::set_active(bool active)
{
    if (m_active == active) {
        return;
    }

    m_active = active;
    emit active_changed();
}

void VNM_NativeWindowFrame::update_native_frame()
{
#ifdef Q_OS_WIN
    if (!should_use_native_frame()) {
        clear_native_frame();
        set_active(false);
        return;
    }

    set_active(apply_native_frame());
#else
    set_active(false);
#endif
}

#ifdef Q_OS_WIN

bool VNM_NativeWindowFrame::should_use_native_frame() const
{
    return m_window
        && m_window->isVisible()
        && m_frame_visible
        && m_frame_width > 0.0
        && m_frame_color.isValid()
        && m_frame_color.alpha() == 255;
}

void* VNM_NativeWindowFrame::window_handle() const
{
    if (!m_window) {
        return nullptr;
    }

    HWND hwnd = reinterpret_cast<HWND>(m_window->winId());
    if (!hwnd || !IsWindow(hwnd)) {
        return nullptr;
    }

    return from_hwnd(hwnd);
}

bool VNM_NativeWindowFrame::apply_native_frame()
{
    void* handle = window_handle();
    HWND hwnd    = as_hwnd(handle);
    if (!hwnd || IsIconic(hwnd)) {
        hide_edge_windows();
        return false;
    }

    if (!ensure_edge_windows(handle)) {
        hide_edge_windows();
        return false;
    }

    position_edge_windows(handle);
    repaint_edge_windows();
    return true;
}

void VNM_NativeWindowFrame::clear_native_frame()
{
    destroy_edge_windows();
}

bool VNM_NativeWindowFrame::ensure_edge_windows(void* parent_window_handle)
{
    HWND parent_hwnd = as_hwnd(parent_window_handle);
    if (!parent_hwnd || ensure_native_frame_window_class() == 0) {
        return false;
    }

    HINSTANCE instance = GetModuleHandleW(nullptr);
    for (void*& edge_window : m_edge_windows) {
        HWND hwnd = as_hwnd(edge_window);
        if (hwnd && (!IsWindow(hwnd) || GetParent(hwnd) != parent_hwnd)) {
            if (IsWindow(hwnd)) {
                DestroyWindow(hwnd);
            }
            edge_window = nullptr;
            hwnd        = nullptr;
        }

        if (hwnd) {
            continue;
        }

        hwnd = CreateWindowExW(
            WS_EX_NOACTIVATE,
            k_native_frame_window_class,
            L"",
            WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS,
            0,
            0,
            0,
            0,
            parent_hwnd,
            nullptr,
            instance,
            nullptr);
        if (!hwnd) {
            destroy_edge_windows();
            return false;
        }

        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));
        edge_window = from_hwnd(hwnd);
    }

    return true;
}

void VNM_NativeWindowFrame::hide_edge_windows()
{
    for (void* edge_window : m_edge_windows) {
        HWND hwnd = as_hwnd(edge_window);
        if (hwnd && IsWindow(hwnd)) {
            ShowWindow(hwnd, SW_HIDE);
        }
    }
}

void VNM_NativeWindowFrame::destroy_edge_windows()
{
    for (void*& edge_window : m_edge_windows) {
        HWND hwnd = as_hwnd(edge_window);
        if (hwnd && IsWindow(hwnd)) {
            DestroyWindow(hwnd);
        }
        edge_window = nullptr;
    }
}

void VNM_NativeWindowFrame::position_edge_windows(void* parent_window_handle)
{
    HWND parent_hwnd = as_hwnd(parent_window_handle);
    if (!parent_hwnd || !IsWindow(parent_hwnd)) {
        hide_edge_windows();
        return;
    }

    RECT client_rect{};
    if (!GetClientRect(parent_hwnd, &client_rect)) {
        hide_edge_windows();
        return;
    }

    const int window_width  = static_cast<int>(client_rect.right - client_rect.left);
    const int window_height = static_cast<int>(client_rect.bottom - client_rect.top);
    if (window_width <= 0 || window_height <= 0) {
        hide_edge_windows();
        return;
    }

    const int width_px    = frame_width_px(parent_window_handle);
    const int edge_width  = std::min(width_px, window_width);
    const int edge_height = std::min(width_px, window_height);
    const UINT flags      = SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_SHOWWINDOW;

    SetWindowPos(
        as_hwnd(m_edge_windows[k_top_edge]),
        HWND_TOP,
        0,
        0,
        window_width,
        edge_height,
        flags);
    SetWindowPos(
        as_hwnd(m_edge_windows[k_bottom_edge]),
        HWND_TOP,
        0,
        std::max(0, window_height - edge_height),
        window_width,
        edge_height,
        flags);
    SetWindowPos(
        as_hwnd(m_edge_windows[k_left_edge]),
        HWND_TOP,
        0,
        0,
        edge_width,
        window_height,
        flags);
    SetWindowPos(
        as_hwnd(m_edge_windows[k_right_edge]),
        HWND_TOP,
        std::max(0, window_width - edge_width),
        0,
        edge_width,
        window_height,
        flags);
}

void VNM_NativeWindowFrame::repaint_edge_windows()
{
    for (void* edge_window : m_edge_windows) {
        HWND hwnd = as_hwnd(edge_window);
        if (hwnd && IsWindow(hwnd)) {
            InvalidateRect(hwnd, nullptr, TRUE);
            UpdateWindow(hwnd);
        }
    }
}

int VNM_NativeWindowFrame::frame_width_px(void* parent_window_handle) const
{
    HWND parent_hwnd = as_hwnd(parent_window_handle);
    if (!parent_hwnd || m_frame_width <= 0.0) {
        return 0;
    }

    const UINT dpi          = GetDpiForWindow(parent_hwnd);
    const qreal scale       = dpi > 0 ? static_cast<qreal>(dpi) / 96.0 : 1.0;
    const int frame_width   = static_cast<int>(std::lround(m_frame_width * scale));
    return std::max(1, frame_width);
}

#endif
