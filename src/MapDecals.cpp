/***************************************************************************
 *   Copyright (C) 2026 by Mudlet Developers                               *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#include "MapDecals.h"

#include "RoomAppearance.h"
#include "TMapLabel.h"

#include <QFontMetricsF>
#include <QPainter>
#include <QtMath>

#include <algorithm>
#include <cmath>

namespace MapDecals {

namespace {
// Clear texels keep color's hue, so that filtering does not fringe what is drawn with black
QImage clearImage(const QSize& size, QColor color)
{
    QImage image(size, QImage::Format_ARGB32);
    color.setAlpha(0);
    image.fill(color);
    return image;
}

QColor withAlpha(QColor color, const int alpha)
{
    color.setAlpha(alpha);
    return color;
}

// Black, white, a grey or nearly so: what a label is given when nobody picked a color for it
bool neutral(const QColor& color)
{
    return color.hsvSaturationF() < 0.2f || color.valueF() < 0.12f;
}

void ring(QPainter& painter, const QPointF& center, const qreal radius, const QColor& color, const qreal width, const Qt::PenStyle style = Qt::SolidLine)
{
    QPen pen(color, width);
    pen.setStyle(style);
    if (style == Qt::CustomDashLine) {
        pen.setDashPattern({2.5, 1.8});
    }
    painter.setPen(pen);
    painter.setBrush(Qt::NoBrush);
    painter.drawEllipse(center, radius, radius);
}
} // namespace

QImage highlight(const MapStyle::HighlightShape shape, const QColor& color, const QColor& centerColor, const bool round, const int size)
{
    QImage image = clearImage(QSize(size, size), color);
    QPainter painter(&image);
    painter.setRenderHint(QPainter::Antialiasing);
    const qreal radius = size / 2.0;
    const QPointF center(radius, radius);
    const auto fillRadial = [&](const QGradientStops& stops) {
        QRadialGradient gradient(center, radius);
        gradient.setStops(stops);
        painter.setPen(Qt::NoPen);
        painter.setBrush(gradient);
        painter.drawEllipse(center, radius, radius);
    };
    switch (shape) {
    case MapStyle::HighlightShape::Disc:
        fillRadial(RoomAppearance::highlightStops(color, centerColor));
        break;
    case MapStyle::HighlightShape::Glow:
        fillRadial({{0.0, withAlpha(color, 170)}, {0.35, withAlpha(color, 140)}, {0.65, withAlpha(color, 60)}, {1.0, withAlpha(color, 0)}});
        break;
    case MapStyle::HighlightShape::DashedEllipse:
        ring(painter, center, radius * 0.86, color, size * 0.04, Qt::CustomDashLine);
        break;
    case MapStyle::HighlightShape::DoubleEllipse:
        ring(painter, center, radius * 0.92, color, size * 0.025);
        ring(painter, center, radius * 0.76, color, size * 0.025);
        break;
    case MapStyle::HighlightShape::Halo:
        fillRadial({{0.0, withAlpha(color, 0)}, {0.5, withAlpha(color, 40)}, {0.75, withAlpha(color, 210)}, {0.88, withAlpha(color, 120)}, {1.0, withAlpha(color, 0)}});
        break;
    case MapStyle::HighlightShape::Cell:
        painter.setPen(Qt::NoPen);
        painter.setBrush(withAlpha(color, 200));
        if (round) {
            painter.drawEllipse(center, radius, radius);
        } else {
            painter.drawRect(image.rect());
        }
        break;
    case MapStyle::HighlightShape::Rings:
        fillRadial({{0.0, withAlpha(color, 0)}, {0.6, withAlpha(color, 50)}, {0.9, withAlpha(color, 30)}, {1.0, withAlpha(color, 0)}});
        ring(painter, center, radius * 0.92, color, size * 0.03);
        ring(painter, center, radius * 0.68, color, size * 0.03);
        break;
    }
    painter.end();
    return image;
}

bool highlightOnGround(const MapStyle::HighlightShape shape)
{
    switch (shape) {
    case MapStyle::HighlightShape::Halo:
    case MapStyle::HighlightShape::Glow:
        return true;
    case MapStyle::HighlightShape::Disc:
    case MapStyle::HighlightShape::DashedEllipse:
    case MapStyle::HighlightShape::DoubleEllipse:
    case MapStyle::HighlightShape::Cell:
    case MapStyle::HighlightShape::Rings:
        return false;
    }
    return false;
}

bool highlightFillsRoom(const MapStyle::HighlightShape shape)
{
    return shape == MapStyle::HighlightShape::Cell;
}

QGradientStops thinPlayerRingStops(const QColor& color)
{
    return {{0.0, withAlpha(color, 0)}, {0.78, withAlpha(color, 0)}, {0.83, withAlpha(color, 255)}, {0.93, withAlpha(color, 255)}, {1.0, withAlpha(color, 0)}};
}

QImage blockSheen(const bool round, const int size)
{
    QImage image = clearImage(QSize(size, size), Qt::white);
    QPainter painter(&image);
    painter.setRenderHint(QPainter::Antialiasing);
    // Drawn with its first row to the map's north
    QLinearGradient gradient(QPointF(0.0, 0.0), QPointF(size, size));
    gradient.setStops({{0.0, QColor(255, 255, 255, 80)}, {0.45, QColor(255, 255, 255, 0)}, {0.55, QColor(0, 0, 0, 0)}, {1.0, QColor(0, 0, 0, 60)}});
    painter.setPen(Qt::NoPen);
    painter.setBrush(gradient);
    if (round) {
        painter.drawEllipse(QRectF(image.rect()));
    } else {
        painter.drawRect(image.rect());
    }
    painter.end();
    return image;
}

QImage blockShadow(const bool round, const int size)
{
    QImage image = clearImage(QSize(size, size), Qt::black);
    QPainter painter(&image);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(0, 0, 0, 16));
    // Layers each a little smaller than the last, so that it darkens toward its middle as a blur would
    constexpr int layers = 14;
    for (int layer = 0; layer < layers; ++layer) {
        const qreal inset = size * (0.02 + 0.016 * layer);
        const QRectF rect = QRectF(image.rect()).adjusted(inset, inset, -inset, -inset);
        if (round) {
            painter.drawEllipse(rect);
        } else {
            painter.drawRoundedRect(rect, size * 0.08, size * 0.08);
        }
    }
    painter.end();
    return image;
}

QImage themedLabel(const TMapLabel& label, const MapStyle& style)
{
    const QSize canvas = label.pix.size();
    if (!style.themedMapLabels || label.text.isEmpty() || canvas.isEmpty()) {
        return QImage();
    }
    QFont font = label.font;
    if (font.family().isEmpty() && !style.labelFontFamily.isEmpty()) {
        font.setFamily(style.labelFontFamily);
    }
    const bool ownTextColor = !neutral(label.fgColor) || !style.mapLabelColor.isValid();
    const QColor textColor = ownTextColor ? label.fgColor : withAlpha(style.mapLabelColor, label.fgColor.alpha());
    const bool box = !neutral(label.bgColor) && label.bgColor.alpha() > 0;
    const bool outline = !neutral(label.outlineColor) && label.outlineColor != label.fgColor;
    const bool halo = !box && !outline && style.mapLabelHalo.isValid();

    // Centered on the label's own picture, as the 2D map's label dialog centers it in the box drawn for it, so the
    // label keeps its place and size; only the text shrinks, to fit the box in the style's font and below its limit
    constexpr int flags = Qt::AlignCenter;
    static const QImage metricsDevice(1, 1, QImage::Format_ARGB32);
    const QRectF bounds = QFontMetricsF(font, &metricsDevice).boundingRect(QRectF(QPointF(), QSizeF(canvas)), flags, label.text);
    const qreal margin = halo ? 2.0 : 1.0;
    qreal shrink = 1.0;
    if (bounds.width() > 0.0 && bounds.height() > 0.0) {
        shrink = std::min({1.0, (canvas.width() - 2.0 * margin) / bounds.width(), (canvas.height() - 2.0 * margin) / bounds.height()});
        const qreal labelHeight = std::abs(label.size.height());
        if (style.mapLabelMaxHeight > 0.0f && !label.noScaling && labelHeight > 0.0) {
            shrink = qMin(shrink, style.mapLabelMaxHeight * canvas.height() / labelHeight / bounds.height());
        }
    }
    if (shrink < 1.0) {
        if (font.pointSizeF() > 0.0) {
            font.setPointSizeF(qMax(1.0, font.pointSizeF() * shrink));
        } else {
            font.setPixelSize(qMax(1, qFloor(font.pixelSize() * shrink)));
        }
    }

    QImage image = clearImage(canvas, textColor);
    if (box) {
        image.fill(label.bgColor);
    }
    QPainter painter(&image);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::TextAntialiasing);
    painter.setFont(font);
    const QRectF textRect(image.rect());
    if (halo) {
        painter.setPen(style.mapLabelHalo);
        for (const QPointF& offset : {QPointF(-2.0, 0.0), QPointF(2.0, 0.0), QPointF(0.0, -2.0), QPointF(0.0, 2.0), QPointF(-1.5, -1.5), QPointF(1.5, -1.5), QPointF(-1.5, 1.5), QPointF(1.5, 1.5)}) {
            painter.drawText(textRect.translated(offset), flags, label.text);
        }
    }
    if (outline) {
        painter.setPen(label.outlineColor);
        for (const QPointF& offset : {QPointF(-1.0, 0.0), QPointF(1.0, 0.0), QPointF(0.0, -1.0), QPointF(0.0, 1.0)}) {
            painter.drawText(textRect.translated(offset), flags, label.text);
        }
    }
    painter.setPen(textColor);
    painter.drawText(textRect, flags, label.text);
    painter.end();
    image.setDevicePixelRatio(label.pix.devicePixelRatio());
    return image;
}

} // namespace MapDecals
