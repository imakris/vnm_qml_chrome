#include "vnm_qml_chrome/vnm_qml_chrome_runtime.h"

#include "vnm_qml_chrome/vnm_chrome_geometry.h"

#include <QColor>
#include <QGuiApplication>
#include <QMetaMethod>
#include <QPointF>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QQuickItem>
#include <QResource>
#include <QSignalSpy>
#include <QString>
#include <QtTest/QTest>

#include <cmath>
#include <limits>
#include <memory>

namespace {

QString component_error_string(const QQmlComponent& component)
{
    QStringList out;
    const auto errors = component.errors();
    for (const QQmlError& error : errors) {
        out.push_back(error.toString());
    }
    return out.join(QStringLiteral("\n"));
}

void collect_descendants(QObject* object, QList<QObject*>& out)
{
    const auto object_children = object->children();
    for (QObject* child : object_children) {
        if (!out.contains(child)) {
            out.push_back(child);
            collect_descendants(child, out);
        }
    }

    auto* item = qobject_cast<QQuickItem*>(object);
    if (!item) {
        return;
    }

    const auto child_items = item->childItems();
    for (QQuickItem* child : child_items) {
        if (!out.contains(child)) {
            out.push_back(child);
            collect_descendants(child, out);
        }
    }
}

QObject* find_descendant(QObject* root, const QString& object_name)
{
    if (root->objectName() == object_name) {
        return root;
    }

    QList<QObject*> descendants;
    collect_descendants(root, descendants);
    for (QObject* object : descendants) {
        if (object->objectName() == object_name) {
            return object;
        }
    }
    return nullptr;
}

std::unique_ptr<QObject> create_qml_object(
    QQmlEngine& engine,
    const char* qml_source,
    const char* resource_path)
{
    QQmlComponent component(&engine);
    component.setData(qml_source, QUrl(QString::fromUtf8(resource_path)));
    if (!component.isReady()) {
        qWarning().noquote() << component_error_string(component);
        return nullptr;
    }

    return std::unique_ptr<QObject>(component.create());
}

bool has_property(QObject* object, const char* property_name)
{
    return object->property(property_name).isValid();
}

bool has_signal(QObject* object, const char* signal_signature)
{
    const QByteArray normalized = QMetaObject::normalizedSignature(signal_signature);
    return object->metaObject()->indexOfSignal(normalized.constData()) >= 0;
}

QColor object_color(QObject* object, const char* property_name)
{
    return object->property(property_name).value<QColor>();
}

bool nearly_equal(qreal actual, qreal expected)
{
    return std::abs(actual - expected) <= 0.000001;
}

bool rect_nearly_equal(
    const QRectF& actual,
    const QRectF& expected)
{
    return
        nearly_equal(actual.x(),      expected.x())      &&
        nearly_equal(actual.y(),      expected.y())      &&
        nearly_equal(actual.width(),  expected.width())  &&
        nearly_equal(actual.height(), expected.height());
}

} // namespace

class Vnm_chrome_contract_tests : public QObject
{
    Q_OBJECT

private slots:
    void geometry_helpers_normalize_invalid_dpr()
    {
        using vnm_qml_chrome::normalized_device_pixel_ratio;

        QCOMPARE(normalized_device_pixel_ratio(1.25), 1.25);
        QCOMPARE(normalized_device_pixel_ratio(0.0),  1.0);
        QCOMPARE(normalized_device_pixel_ratio(-2.0), 1.0);
        QCOMPARE(
            normalized_device_pixel_ratio(std::numeric_limits<qreal>::infinity()),
            1.0);
        QCOMPARE(
            normalized_device_pixel_ratio(std::numeric_limits<qreal>::quiet_NaN()),
            1.0);
    }

    void geometry_helpers_snap_custom_frame_rect_to_physical_pixels()
    {
        using vnm_qml_chrome::rect_has_snapped_physical_edges;
        using vnm_qml_chrome::snapped_logical_edge;
        using vnm_qml_chrome::snapped_logical_rect;

        constexpr qreal dpr                  = 1.25;
        constexpr qreal resize_border_width  = 6.0;
        constexpr qreal titlebar_height      = 32.0;
        constexpr qreal content_border_width = 1.0 / dpr;

        const QRectF unsnapped_terminal_rect(
            resize_border_width + content_border_width,
            titlebar_height     + content_border_width,
            1894.4,
            1040.4);

        QVERIFY(!rect_has_snapped_physical_edges(unsnapped_terminal_rect, dpr));
        QVERIFY(nearly_equal(snapped_logical_edge(6.8, dpr), 7.2));

        const QRectF snapped_terminal_rect = snapped_logical_rect(
            unsnapped_terminal_rect,
            dpr);
        const QRectF expected_terminal_rect(7.2, 32.8, 1894.4, 1040.8);

        QVERIFY(rect_has_snapped_physical_edges(snapped_terminal_rect, dpr));
        QVERIFY2(
            rect_nearly_equal(snapped_terminal_rect, expected_terminal_rect),
            "Custom-frame terminal content must snap off the half physical pixel.");
    }

    void geometry_singleton_exposes_snapping_contract()
    {
        QQmlEngine engine;
        QVERIFY(vnm_init_qml_chrome_runtime(engine));

        static const char qml_source[] = R"(
import QtQuick
import VNM_Chrome

Item {
    function snapped_edge() {
        return VNM_chrome_geometry.snapped_logical_edge(6.8, 1.25)
    }

    function snapped_rect() {
        return VNM_chrome_geometry.snapped_logical_rect(
            Qt.rect(6.8, 32.8, 1894.4, 1040.4),
            1.25)
    }

    function invalid_dpr() {
        return VNM_chrome_geometry.normalized_device_pixel_ratio(0)
    }

    function rect_snapped() {
        return VNM_chrome_geometry.rect_has_snapped_physical_edges(snapped_rect(), 1.25)
    }
}
)";

        std::unique_ptr<QObject> root = create_qml_object(
            engine, qml_source, "qrc:/tests/geometry_singleton_contract.qml");
        QVERIFY(root != nullptr);

        QVariant snapped_edge;
        QVERIFY(QMetaObject::invokeMethod(
            root.get(),
            "snapped_edge",
            Q_RETURN_ARG(QVariant, snapped_edge)));
        QVERIFY(nearly_equal(snapped_edge.toReal(), 7.2));

        QVariant snapped_rect;
        QVERIFY(QMetaObject::invokeMethod(
            root.get(),
            "snapped_rect",
            Q_RETURN_ARG(QVariant, snapped_rect)));
        QVERIFY(rect_nearly_equal(
            snapped_rect.toRectF(),
            QRectF(7.2, 32.8, 1894.4, 1040.8)));

        QVariant invalid_dpr;
        QVERIFY(QMetaObject::invokeMethod(
            root.get(),
            "invalid_dpr",
            Q_RETURN_ARG(QVariant, invalid_dpr)));
        QCOMPARE(invalid_dpr.toReal(), 1.0);

        QVariant rect_snapped;
        QVERIFY(QMetaObject::invokeMethod(
            root.get(),
            "rect_snapped",
            Q_RETURN_ARG(QVariant, rect_snapped)));
        QVERIFY(rect_snapped.toBool());
    }

    void runtime_bootstrap_registers_manual_qrc_import()
    {
        QQmlEngine engine;
        QVERIFY(vnm_init_qml_chrome_runtime(engine));
        QVERIFY(vnm_init_qml_chrome_runtime(engine));

        const QString import_path = QStringLiteral("qrc:/vnm_qml_chrome/qml");
        const QResource qmldir_resource(
            QStringLiteral(":/vnm_qml_chrome/qml/VNM_Chrome/qmldir"));
        QVERIFY(qmldir_resource.isValid());
        QVERIFY(engine.importPathList().contains(import_path));
        QCOMPARE(engine.importPathList().count(import_path), qsizetype{1});
    }

    void exported_types_instantiate_and_expose_contract()
    {
        QQmlEngine engine;
        QVERIFY(vnm_init_qml_chrome_runtime(engine));

        static const char qml_source[] = R"(
import QtQuick
import VNM_Chrome

Item {
    width: 640
    height: 240

    VNM_ChromeTheme {
        id: chrome_theme
        objectName: "chrome_theme"
    }

    VNM_AnimatedMark {
        objectName: "animated_mark"
        theme: chrome_theme
        x: 0
        y: 0
    }

    VNM_ChromeWindowButton {
        objectName: "window_button"
        theme: chrome_theme
        x: 32
        y: 0
        width: 46
        height: 32
    }

    VNM_TitleBarRoundButton {
        objectName: "round_button"
        theme: chrome_theme
        x: 84
        y: 0
        label: "R"
    }

    VNM_DarkModeTitleButton {
        objectName: "dark_button"
        theme: chrome_theme
        x: 112
        y: 0
    }

    VNM_LanguageTitleButton {
        objectName: "language_button"
        theme: chrome_theme
        x: 140
        y: 0
    }

    VNM_ChromeResizeArea {
        objectName: "resize_area"
        x: 170
        y: 0
        width: 8
        height: 8
        edges: Qt.LeftEdge
    }

    VNM_ChromeSideResizeLayer {
        objectName: "side_layer"
        y: 40
        width: 240
        height: 80
    }

    VNM_ChromeBottomResizeLayer {
        objectName: "bottom_layer"
        y: 130
        width: 240
        height: 8
    }

    VNM_NativeWindowFrame {
        objectName: "native_window_frame"
        frame_visible: true
        frame_width: 2
        frame_color: "#123456"
    }

    VNM_ChromeTitleBar {
        objectName: "chrome_titlebar"
        y: 150
        width: 480
        height: 32
        title: "Contract"
        theme: chrome_theme
        leading_action_component: Component {
            VNM_DarkModeTitleButton {
                theme: chrome_theme
            }
        }
        trailing_action_component: Component {
            VNM_LanguageTitleButton {
                theme: chrome_theme
            }
        }
    }
}
)";

        std::unique_ptr<QObject> root = create_qml_object(
            engine, qml_source, "qrc:/tests/exported_types_contract.qml");
        QVERIFY(root != nullptr);
        QCoreApplication::processEvents(QEventLoop::AllEvents, 50);

        QObject* theme = find_descendant(root.get(), QStringLiteral("chrome_theme"));
        QVERIFY(theme != nullptr);
        const char* theme_properties[] = {
            "titlebar",
            "titlebar_text",
            "titlebar_button_icon",
            "titlebar_button_hover",
            "titlebar_button_pressed",
            "titlebar_close_hover",
            "titlebar_close_pressed",
            "titlebar_activity_marker",
            "titlebar_content_border",
            "window_frame_border",
            "round_button_background",
            "round_button_border",
            "round_button_text",
            "mark_grey",
            "mark_orange",
            "mark_underlay",
        };
        for (const char* property_name : theme_properties) {
            QVERIFY2(has_property(theme, property_name), property_name);
        }

        QObject* mark = find_descendant(root.get(), QStringLiteral("animated_mark"));
        QVERIFY(mark != nullptr);
        QVERIFY(has_property(mark, "theme"));
        QVERIFY(has_property(mark, "mark_size"));
        QVERIFY(has_property(mark, "move_enabled"));
        QVERIFY(has_property(mark, "move_drag_threshold"));
        QVERIFY(has_property(mark, "hover_active"));
        QVERIFY(has_property(mark, "alt_reveal_active"));
        QVERIFY(has_signal(mark, "move_requested()"));
        QVERIFY(has_signal(mark, "maximize_toggle_requested()"));

        QObject* window_button = find_descendant(root.get(), QStringLiteral("window_button"));
        QVERIFY(window_button != nullptr);
        QVERIFY(has_property(window_button, "theme"));
        QVERIFY(has_property(window_button, "background_color"));
        QVERIFY(has_property(window_button, "hover_color"));
        QVERIFY(has_property(window_button, "pressed_color"));
        QVERIFY(has_signal(window_button, "clicked()"));

        QObject* round_button = find_descendant(root.get(), QStringLiteral("round_button"));
        QVERIFY(round_button != nullptr);
        QVERIFY(has_property(round_button, "theme"));
        QVERIFY(has_property(round_button, "label"));
        QVERIFY(has_property(round_button, "tooltip_text"));
        QVERIFY(has_signal(round_button, "clicked()"));

        QObject* dark_button = find_descendant(root.get(), QStringLiteral("dark_button"));
        QVERIFY(dark_button != nullptr);
        QVERIFY(has_property(dark_button, "dark_mode"));
        QVERIFY(has_property(dark_button, "dark_label"));
        QVERIFY(has_property(dark_button, "light_label"));
        QVERIFY(has_signal(dark_button, "toggle_requested()"));

        QObject* language_button = find_descendant(root.get(), QStringLiteral("language_button"));
        QVERIFY(language_button != nullptr);
        QVERIFY(has_property(language_button, "primary_label"));
        QVERIFY(has_property(language_button, "secondary_label"));
        QVERIFY(has_property(language_button, "primary_active"));
        QVERIFY(has_signal(language_button, "toggle_requested()"));

        QObject* resize_area = find_descendant(root.get(), QStringLiteral("resize_area"));
        QVERIFY(resize_area != nullptr);
        QVERIFY(has_property(resize_area, "edges"));
        QVERIFY(has_signal(resize_area, "resize_requested(int)"));

        QObject* side_layer = find_descendant(root.get(), QStringLiteral("side_layer"));
        QVERIFY(side_layer != nullptr);
        QVERIFY(has_property(side_layer, "resize_enabled"));
        QVERIFY(has_property(side_layer, "resize_border_width"));
        QVERIFY(has_signal(side_layer, "resize_requested(int)"));

        QObject* bottom_layer = find_descendant(root.get(), QStringLiteral("bottom_layer"));
        QVERIFY(bottom_layer != nullptr);
        QVERIFY(has_property(bottom_layer, "resize_enabled"));
        QVERIFY(has_property(bottom_layer, "resize_border_width"));
        QVERIFY(has_signal(bottom_layer, "resize_requested(int)"));

        QObject* native_frame = find_descendant(root.get(), QStringLiteral("native_window_frame"));
        QVERIFY(native_frame != nullptr);
        QVERIFY(has_property(native_frame, "window"));
        QVERIFY(has_property(native_frame, "frame_visible"));
        QVERIFY(has_property(native_frame, "frame_width"));
        QVERIFY(has_property(native_frame, "frame_color"));
        QVERIFY(has_property(native_frame, "active"));
        QCOMPARE(native_frame->property("frame_visible").toBool(), true);
        QCOMPARE(native_frame->property("frame_width").toReal(), 2.0);
        QCOMPARE(
            object_color(native_frame, "frame_color"),
            QColor(QStringLiteral("#123456")));
        QCOMPARE(native_frame->property("active").toBool(), false);

        QObject* titlebar = find_descendant(root.get(), QStringLiteral("chrome_titlebar"));
        QVERIFY(titlebar != nullptr);
        const char* titlebar_properties[] = {
            "theme",
            "title",
            "active",
            "maximized",
            "resize_enabled",
            "resize_border_width",
            "device_pixel_ratio",
            "animated_mark_visible",
            "activity_marker_text",
            "leading_action_component",
            "trailing_action_component",
        };
        for (const char* property_name : titlebar_properties) {
            QVERIFY2(has_property(titlebar, property_name), property_name);
        }
        QVERIFY(has_signal(titlebar, "move_requested()"));
        QVERIFY(has_signal(titlebar, "resize_requested(int)"));
        QVERIFY(has_signal(titlebar, "minimize_requested()"));
        QVERIFY(has_signal(titlebar, "maximize_toggle_requested()"));
        QVERIFY(has_signal(titlebar, "close_requested()"));
    }

    void titlebar_creates_visible_default_mark_and_propagates_theme_override()
    {
        QQmlEngine engine;
        QVERIFY(vnm_init_qml_chrome_runtime(engine));

        static const char qml_source[] = R"(
import QtQuick
import VNM_Chrome

Item {
    width: 500
    height: 60

    VNM_ChromeTheme {
        id: custom_theme
        objectName: "custom_theme"
        titlebar: "#102030"
        titlebar_text: "#ddeeff"
    }

    VNM_ChromeTitleBar {
        objectName: "chrome_titlebar"
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        theme: custom_theme
        title: "Theme"
    }
}
)";

        std::unique_ptr<QObject> root = create_qml_object(
            engine, qml_source, "qrc:/tests/titlebar_default_mark_contract.qml");
        QVERIFY(root != nullptr);
        QCoreApplication::processEvents(QEventLoop::AllEvents, 50);

        auto* titlebar = qobject_cast<QQuickItem*>(
            find_descendant(root.get(), QStringLiteral("chrome_titlebar")));
        QVERIFY(titlebar != nullptr);
        QCOMPARE(object_color(titlebar, "color"), QColor(QStringLiteral("#102030")));

        auto* mark = qobject_cast<QQuickItem*>(
            find_descendant(root.get(), QStringLiteral("vnm_animated_mark")));
        QVERIFY(mark != nullptr);
        QVERIFY(mark->isVisible());

        QObject* custom_theme = find_descendant(root.get(), QStringLiteral("custom_theme"));
        QVERIFY(custom_theme != nullptr);
        QVERIFY(custom_theme->setProperty("titlebar", QColor(QStringLiteral("#405060"))));
        QCoreApplication::processEvents(QEventLoop::AllEvents, 50);
        QCOMPARE(object_color(titlebar, "color"), QColor(QStringLiteral("#405060")));
    }

    void inactive_titlebar_does_not_dim_child_opacity()
    {
        QQmlEngine engine;
        QVERIFY(vnm_init_qml_chrome_runtime(engine));

        static const char qml_source[] = R"(
import QtQuick
import VNM_Chrome

Item {
    width: 500
    height: 60

    VNM_ChromeTitleBar {
        objectName: "chrome_titlebar"
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        active: false
    }
}
)";

        std::unique_ptr<QObject> root = create_qml_object(
            engine, qml_source, "qrc:/tests/inactive_titlebar_opacity_contract.qml");
        QVERIFY(root != nullptr);
        QCoreApplication::processEvents(QEventLoop::AllEvents, 50);

        auto* titlebar = qobject_cast<QQuickItem*>(
            find_descendant(root.get(), QStringLiteral("chrome_titlebar")));
        QVERIFY(titlebar != nullptr);
        QCOMPARE(titlebar->opacity(), 1.0);

        auto* mark = qobject_cast<QQuickItem*>(
            find_descendant(root.get(), QStringLiteral("vnm_animated_mark")));
        QVERIFY(mark != nullptr);
        QCOMPARE(mark->opacity(), 1.0);

        auto* title_label = qobject_cast<QQuickItem*>(
            find_descendant(root.get(), QStringLiteral("title_label")));
        QVERIFY(title_label != nullptr);
        QCOMPARE(title_label->opacity(), 1.0);
    }

    void titlebar_snaps_content_inset_at_fractional_dpr()
    {
        QQmlEngine engine;
        QVERIFY(vnm_init_qml_chrome_runtime(engine));

        static const char qml_source[] = R"(
import QtQuick
import VNM_Chrome

Item {
    width: 500
    height: 60

    VNM_ChromeTitleBar {
        objectName: "chrome_titlebar"
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        height: 32
        resize_border_width: 6
        device_pixel_ratio: 1.25
    }
}
)";

        std::unique_ptr<QObject> root_object = create_qml_object(
            engine, qml_source, "qrc:/tests/titlebar_fractional_dpr_contract.qml");
        QVERIFY(root_object != nullptr);
        auto* root = qobject_cast<QQuickItem*>(root_object.get());
        QVERIFY(root != nullptr);
        root->ensurePolished();
        QCoreApplication::processEvents(QEventLoop::AllEvents, 50);

        auto* mark = qobject_cast<QQuickItem*>(
            find_descendant(root_object.get(), QStringLiteral("vnm_animated_mark")));
        QVERIFY(mark != nullptr);
        const QPointF mark_origin = mark->mapToItem(root, QPointF(0.0, 0.0));
        QVERIFY(nearly_equal(mark_origin.x(), 6.4));
        QVERIFY(vnm_qml_chrome::rect_has_snapped_physical_edges(
            QRectF(mark_origin.x(), 0.0, mark->width(), 0.0),
            1.25));

        auto* top_left_resize_area = qobject_cast<QQuickItem*>(
            find_descendant(root_object.get(), QStringLiteral("top_left_resize_area")));
        QVERIFY(top_left_resize_area != nullptr);
        QVERIFY(nearly_equal(top_left_resize_area->width(),  6.4));
        QVERIFY(nearly_equal(top_left_resize_area->height(), 6.4));
    }

    void resize_layers_preserve_fractional_resize_border_widths()
    {
        QQmlEngine engine;
        QVERIFY(vnm_init_qml_chrome_runtime(engine));

        static const char qml_source[] = R"(
import QtQuick
import VNM_Chrome

Item {
    width: 240
    height: 140

    VNM_ChromeSideResizeLayer {
        objectName: "side_layer"
        anchors.fill: parent
        resize_border_width: 5.6
    }

    VNM_ChromeBottomResizeLayer {
        objectName: "bottom_layer"
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        height: resize_border_width
        resize_border_width: 5.6
    }

    VNM_ChromeTitleBar {
        objectName: "chrome_titlebar"
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        height: 32
        resize_border_width: 5.6
        device_pixel_ratio: 1.25
    }
}
)";

        std::unique_ptr<QObject> root_object = create_qml_object(
            engine,
            qml_source,
            "qrc:/tests/fractional_resize_border_width_contract.qml");
        QVERIFY(root_object != nullptr);
        QCoreApplication::processEvents(QEventLoop::AllEvents, 50);

        auto* left_resize_area = qobject_cast<QQuickItem*>(
            find_descendant(root_object.get(), QStringLiteral("left_resize_area")));
        QVERIFY(left_resize_area != nullptr);
        QVERIFY(nearly_equal(left_resize_area->width(), 5.6));

        auto* bottom_layer = qobject_cast<QQuickItem*>(
            find_descendant(root_object.get(), QStringLiteral("bottom_layer")));
        QVERIFY(bottom_layer != nullptr);
        QVERIFY(nearly_equal(bottom_layer->height(), 5.6));

        auto* bottom_left_resize_area = qobject_cast<QQuickItem*>(
            find_descendant(root_object.get(), QStringLiteral("bottom_left_resize_area")));
        QVERIFY(bottom_left_resize_area != nullptr);
        QVERIFY(nearly_equal(bottom_left_resize_area->width(),  5.6));
        QVERIFY(nearly_equal(bottom_left_resize_area->height(), 5.6));

        auto* top_left_resize_area = qobject_cast<QQuickItem*>(
            find_descendant(root_object.get(), QStringLiteral("top_left_resize_area")));
        QVERIFY(top_left_resize_area != nullptr);
        QVERIFY(nearly_equal(top_left_resize_area->width(),  5.6));
        QVERIFY(nearly_equal(top_left_resize_area->height(), 5.6));
    }

    void titlebar_activity_marker_is_optional_and_themeable()
    {
        QQmlEngine engine;
        QVERIFY(vnm_init_qml_chrome_runtime(engine));

        static const char qml_source[] = R"(
import QtQuick
import VNM_Chrome

Item {
    width: 500
    height: 60

    VNM_ChromeTheme {
        id: custom_theme
        titlebar_activity_marker: "#112233"
    }

    VNM_ChromeTitleBar {
        objectName: "chrome_titlebar"
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        theme: custom_theme
        title: "Build"
    }
}
)";

        std::unique_ptr<QObject> root = create_qml_object(
            engine, qml_source, "qrc:/tests/titlebar_activity_marker_contract.qml");
        QVERIFY(root != nullptr);
        auto* root_item = qobject_cast<QQuickItem*>(root.get());
        QVERIFY(root_item != nullptr);
        QCoreApplication::processEvents(QEventLoop::AllEvents, 50);

        QObject* titlebar = find_descendant(root.get(), QStringLiteral("chrome_titlebar"));
        QVERIFY(titlebar != nullptr);
        auto* marker_label = qobject_cast<QQuickItem*>(
            find_descendant(root.get(), QStringLiteral("activity_marker_label")));
        QVERIFY(marker_label != nullptr);
        auto* title_label = qobject_cast<QQuickItem*>(
            find_descendant(root.get(), QStringLiteral("title_label")));
        QVERIFY(title_label != nullptr);
        QVERIFY(!marker_label->isVisible());

        const QString marker_text(QChar(0x2731));
        QVERIFY(titlebar->setProperty("activity_marker_text", marker_text));
        root_item->ensurePolished();
        QCoreApplication::processEvents(QEventLoop::AllEvents, 50);

        QVERIFY(marker_label->isVisible());
        QCOMPARE(marker_label->property("text").toString(), marker_text);
        QCOMPARE(object_color(marker_label, "color"), QColor(QStringLiteral("#112233")));
        const qreal title_x_with_marker = title_label->x();

        QVERIFY(titlebar->setProperty("activity_marker_text", QString()));
        root_item->ensurePolished();
        QCoreApplication::processEvents(QEventLoop::AllEvents, 50);

        QVERIFY(marker_label->isVisible());
        QCOMPARE(marker_label->property("text").toString(), QString());
        QVERIFY(nearly_equal(title_label->x(), title_x_with_marker));
    }

    void titlebar_title_has_margin_after_mark_without_actions()
    {
        QQmlEngine engine;
        QVERIFY(vnm_init_qml_chrome_runtime(engine));

        static const char qml_source[] = R"(
import QtQuick
import VNM_Chrome

Item {
    width: 500
    height: 60

    VNM_ChromeTitleBar {
        objectName: "chrome_titlebar"
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        title: "Logonomic"
    }
}
)";

        std::unique_ptr<QObject> root = create_qml_object(
            engine, qml_source, "qrc:/tests/titlebar_title_margin_contract.qml");
        QVERIFY(root != nullptr);
        QCoreApplication::processEvents(QEventLoop::AllEvents, 50);

        auto* animated_mark = qobject_cast<QQuickItem*>(
            find_descendant(root.get(), QStringLiteral("vnm_animated_mark")));
        auto* title_label = qobject_cast<QQuickItem*>(
            find_descendant(root.get(), QStringLiteral("title_label")));
        QVERIFY(animated_mark != nullptr);
        QVERIFY(title_label != nullptr);

        const qreal title_gap = title_label->x()
            - (animated_mark->x() + animated_mark->width());
        QVERIFY2(title_gap >= 7.5,
            "Title label must not sit directly against the animated mark.");
    }

    void animated_mark_supports_normal_hover_state_transition()
    {
        QQmlEngine engine;
        QVERIFY(vnm_init_qml_chrome_runtime(engine));

        static const char qml_source[] = R"(
import QtQuick
import VNM_Chrome

VNM_AnimatedMark {
    objectName: "animated_mark"
    mark_size: 20
}
)";

        std::unique_ptr<QObject> root = create_qml_object(
            engine, qml_source, "qrc:/tests/animated_mark_transition_contract.qml");
        QVERIFY(root != nullptr);
        QCoreApplication::processEvents(QEventLoop::AllEvents, 50);

        QCOMPARE(root->property("state").toString(), QString());
        QVERIFY(root->setProperty("hover_active", true));
        QCoreApplication::processEvents(QEventLoop::AllEvents, 50);
        QCOMPARE(root->property("state").toString(), QStringLiteral("normal_hover"));
    }

    void resize_corner_ownership_prefers_titlebar_and_bottom_layers()
    {
        QQmlEngine engine;
        QVERIFY(vnm_init_qml_chrome_runtime(engine));

        static const char qml_source[] = R"(
import QtQuick
import VNM_Chrome

Item {
    width: 240
    height: 140

    VNM_ChromeSideResizeLayer {
        objectName: "side_layer"
        anchors.fill: parent
        resize_border_width: 8
    }

    VNM_ChromeBottomResizeLayer {
        objectName: "bottom_layer"
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        height: resize_border_width
        resize_border_width: 8
    }

    VNM_ChromeTitleBar {
        objectName: "chrome_titlebar"
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        height: 32
        resize_border_width: 8
    }
}
)";

        std::unique_ptr<QObject> root_object = create_qml_object(
            engine, qml_source, "qrc:/tests/resize_corner_ownership_contract.qml");
        QVERIFY(root_object != nullptr);
        auto* root = qobject_cast<QQuickItem*>(root_object.get());
        QVERIFY(root != nullptr);
        root->ensurePolished();
        QCoreApplication::processEvents(QEventLoop::AllEvents, 50);

        auto* top_owner = root->childAt(2, 2);
        QVERIFY(top_owner != nullptr);
        QCOMPARE(top_owner->objectName(), QStringLiteral("chrome_titlebar"));

        auto* titlebar = qobject_cast<QQuickItem*>(
            find_descendant(root_object.get(), QStringLiteral("chrome_titlebar")));
        QVERIFY(titlebar != nullptr);
        auto* top_corner_owner = titlebar->childAt(2, 2);
        QVERIFY(top_corner_owner != nullptr);
        QCOMPARE(top_corner_owner->objectName(), QStringLiteral("top_left_resize_area"));

        auto* bottom_owner = root->childAt(2, 138);
        QVERIFY(bottom_owner != nullptr);
        QCOMPARE(bottom_owner->objectName(), QStringLiteral("bottom_layer"));

        auto* bottom_layer = qobject_cast<QQuickItem*>(
            find_descendant(root_object.get(), QStringLiteral("bottom_layer")));
        QVERIFY(bottom_layer != nullptr);
        auto* bottom_corner_owner = bottom_layer->childAt(2, 6);
        QVERIFY(bottom_corner_owner != nullptr);
        QCOMPARE(bottom_corner_owner->objectName(), QStringLiteral("bottom_left_resize_area"));
    }

    void resize_layers_forward_inner_resize_requests()
    {
        QQmlEngine engine;
        QVERIFY(vnm_init_qml_chrome_runtime(engine));

        static const char qml_source[] = R"(
import QtQuick
import VNM_Chrome

Item {
    width: 240
    height: 80

    VNM_ChromeSideResizeLayer {
        objectName: "side_layer"
        anchors.fill: parent
        resize_border_width: 8
    }

    VNM_ChromeBottomResizeLayer {
        objectName: "bottom_layer"
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        height: resize_border_width
        resize_border_width: 8
    }
}
)";

        std::unique_ptr<QObject> root = create_qml_object(
            engine, qml_source, "qrc:/tests/resize_forwarding_contract.qml");
        QVERIFY(root != nullptr);
        QCoreApplication::processEvents(QEventLoop::AllEvents, 50);

        QObject* side_layer = find_descendant(root.get(), QStringLiteral("side_layer"));
        QVERIFY(side_layer != nullptr);
        QObject* left_resize_area =
            find_descendant(root.get(), QStringLiteral("left_resize_area"));
        QVERIFY(left_resize_area != nullptr);
        QSignalSpy side_spy(side_layer, SIGNAL(resize_requested(int)));
        QVERIFY(side_spy.isValid());

        QVERIFY(QMetaObject::invokeMethod(
            left_resize_area, "resize_requested", Q_ARG(int, int(Qt::LeftEdge))));
        QCOMPARE(side_spy.count(), 1);
        QCOMPARE(side_spy.takeFirst().at(0).toInt(), int(Qt::LeftEdge));

        QObject* bottom_layer = find_descendant(root.get(), QStringLiteral("bottom_layer"));
        QVERIFY(bottom_layer != nullptr);
        QObject* bottom_left_resize_area =
            find_descendant(root.get(), QStringLiteral("bottom_left_resize_area"));
        QVERIFY(bottom_left_resize_area != nullptr);
        QSignalSpy bottom_spy(bottom_layer, SIGNAL(resize_requested(int)));
        QVERIFY(bottom_spy.isValid());

        QVERIFY(QMetaObject::invokeMethod(
            bottom_left_resize_area,
            "resize_requested",
            Q_ARG(int, int(Qt::LeftEdge | Qt::BottomEdge))));
        QCOMPARE(bottom_spy.count(), 1);
        QCOMPARE(bottom_spy.takeFirst().at(0).toInt(), int(Qt::LeftEdge | Qt::BottomEdge));
    }

    void titlebar_move_threshold_and_signal_are_synchronous()
    {
        QQmlEngine engine;
        QVERIFY(vnm_init_qml_chrome_runtime(engine));

        static const char qml_source[] = R"(
import QtQuick
import VNM_Chrome

Item {
    id: root

    width: 420
    height: 48
    property int move_count: 0
    property bool move_seen_before_function_return: false

    VNM_ChromeTitleBar {
        id: chrome_titlebar
        objectName: "chrome_titlebar"
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        title: "Move"

        onMove_requested: {
            root.move_count += 1
            root.move_seen_before_function_return = true
        }
    }

    QtObject {
        id: move_probe

        property real system_move_press_x: 0
        property real system_move_press_y: 0
        property bool system_move_started: false
    }

    function reset_probe() {
        root.move_count = 0
        root.move_seen_before_function_return = false
        move_probe.system_move_press_x = 0
        move_probe.system_move_press_y = 0
        move_probe.system_move_started = false
    }

    function under_threshold_move_is_ignored() {
        reset_probe()
        const mouse = {
            x: 1,
            y: 1,
            buttons: Qt.LeftButton,
            accepted: false,
        }
        chrome_titlebar.maybe_start_system_move(move_probe, mouse)
        return root.move_count === 0
            && !move_probe.system_move_started
            && mouse.accepted === false
    }

    function over_threshold_move_is_synchronous() {
        reset_probe()
        const mouse = {
            x: 3,
            y: 0,
            buttons: Qt.LeftButton,
            accepted: false,
        }
        chrome_titlebar.maybe_start_system_move(move_probe, mouse)
        return root.move_count === 1
            && root.move_seen_before_function_return
            && move_probe.system_move_started
            && mouse.accepted === true
    }
}
)";

        std::unique_ptr<QObject> root = create_qml_object(
            engine, qml_source, "qrc:/tests/titlebar_move_contract.qml");
        QVERIFY(root != nullptr);
        QCoreApplication::processEvents(QEventLoop::AllEvents, 50);

        QVariant under_threshold_result;
        QVERIFY(QMetaObject::invokeMethod(
            root.get(),
            "under_threshold_move_is_ignored",
            Q_RETURN_ARG(QVariant, under_threshold_result)));
        QVERIFY(under_threshold_result.toBool());

        QVariant over_threshold_result;
        QVERIFY(QMetaObject::invokeMethod(
            root.get(),
            "over_threshold_move_is_synchronous",
            Q_RETURN_ARG(QVariant, over_threshold_result)));
        QVERIFY(over_threshold_result.toBool());
    }
};

int main(int argc, char** argv)
{
    qputenv("QT_QPA_PLATFORM", "offscreen");
    qputenv("QT_QUICK_CONTROLS_STYLE", "Basic");

    QGuiApplication app(argc, argv);
    Vnm_chrome_contract_tests tests;
    return QTest::qExec(&tests, argc, argv);
}

#include "vnm_chrome_contract_tests.moc"
