#pragma once

#include <QObject>
#include <QRectF>
#include <QtGlobal>

namespace vnm_qml_chrome {

/**
 * @brief Return a usable device-pixel ratio for logical-to-physical snapping.
 *
 * Non-finite and non-positive values are treated as DPR 1.0.
 */
qreal normalized_device_pixel_ratio(qreal device_pixel_ratio);

/**
 * @brief Snap a logical coordinate to the nearest integer physical pixel.
 */
qreal snapped_logical_edge(
    qreal logical_edge,
    qreal device_pixel_ratio);

/**
 * @brief Snap all rectangle edges to the nearest integer physical pixels.
 */
QRectF snapped_logical_rect(
    const QRectF& logical_rect,
    qreal         device_pixel_ratio);

/**
 * @brief Return whether every rectangle edge lands on an integer physical pixel.
 */
bool rect_has_snapped_physical_edges(
    const QRectF& logical_rect,
    qreal         device_pixel_ratio);

class Chrome_geometry : public QObject
{
    Q_OBJECT

public:
    explicit Chrome_geometry(QObject* parent = nullptr);

    Q_INVOKABLE qreal normalized_device_pixel_ratio(qreal device_pixel_ratio) const;

    Q_INVOKABLE qreal snapped_logical_edge(
        qreal logical_edge,
        qreal device_pixel_ratio) const;

    Q_INVOKABLE QRectF snapped_logical_rect(
        const QRectF& logical_rect,
        qreal         device_pixel_ratio) const;

    Q_INVOKABLE bool rect_has_snapped_physical_edges(
        const QRectF& logical_rect,
        qreal         device_pixel_ratio) const;
};

} // namespace vnm_qml_chrome
