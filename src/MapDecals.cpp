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

#include "EnvironmentKit.h"
#include "RoomAppearance.h"
#include "TMapLabel.h"
#include "utils.h"

#include <QCoreApplication>
#include <QFontMetricsF>
#include <QPainter>
#include <QPainterPath>
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
    case MapStyle::HighlightShape::Rings:
        return true;
    case MapStyle::HighlightShape::Disc:
    case MapStyle::HighlightShape::DashedEllipse:
    case MapStyle::HighlightShape::DoubleEllipse:
    case MapStyle::HighlightShape::Cell:
        return false;
    }
    return false;
}

bool highlightFillsRoom(const MapStyle::HighlightShape shape)
{
    return shape == MapStyle::HighlightShape::Cell;
}

QImage registrationMark(const QColor& color, const int size)
{
    QImage image = clearImage(QSize(size, size), color);
    QPainter painter(&image);
    painter.setRenderHint(QPainter::Antialiasing);
    const qreal half = size / 2.0;
    const QPointF center(half, half);
    const qreal width = size * 0.035;
    ring(painter, center, half * 0.52, color, width);
    painter.setPen(QPen(color, width, Qt::SolidLine, Qt::FlatCap));
    const qreal reach = half * 0.95;
    painter.drawLine(QPointF(half - reach, half), QPointF(half + reach, half));
    painter.drawLine(QPointF(half, half - reach), QPointF(half, half + reach));
    painter.setPen(Qt::NoPen);
    painter.setBrush(color);
    painter.drawEllipse(center, width * 1.2, width * 1.2);
    painter.end();
    return image;
}

QList<qreal> spreadCallouts(const QList<qreal>& wanted, const qreal spacing, const qreal top, const qreal bottom)
{
    QList<qreal> placed = wanted;
    if (placed.isEmpty()) {
        return placed;
    }
    // Downward from the top, then back up from the bottom, so that none overlaps and all stay between top and bottom
    // where they fit there
    qreal next = top;
    for (qreal& at : placed) {
        at = std::max(at, next);
        next = at + spacing;
    }
    qreal last = bottom;
    for (auto it = placed.rbegin(); it != placed.rend(); ++it) {
        *it = std::min(*it, last);
        last = *it - spacing;
    }
    return placed;
}

QList<int> calloutLevels(const QList<int>& levels, const int shownLevel, const int most)
{
    QList<int> named;
    for (const int level : levels) {
        if (level != shownLevel && !named.contains(level)) {
            named.append(level);
        }
    }
    // Nearest first, and of two as near the one above
    std::sort(named.begin(), named.end(), [shownLevel](const int a, const int b) {
        const int nearerA = std::abs(a - shownLevel);
        const int nearerB = std::abs(b - shownLevel);
        return nearerA != nearerB ? nearerA < nearerB : a > b;
    });
    if (named.size() > std::max(most, 0)) {
        named.resize(std::max(most, 0));
    }
    std::sort(named.begin(), named.end(), [](const int a, const int b) {
        return a > b;
    });
    return named;
}

QString levelName(const int level)
{
    return level < 0
                   //: A level below ground, named in a callout by the blueprint style or on a platform by the transit style of the 3D map, as B1 for the first basement
                   ? QCoreApplication::translate("ModernGLWidget", "B%1").arg(-level)
                   //: A level at or above ground, named in a callout by the blueprint style or on a platform by the transit style of the 3D map, as L1 for the first floor up
                   : QCoreApplication::translate("ModernGLWidget", "L%1").arg(level);
}

QString levelCallout(const int level, const int rooms)
{
    return qsl("%1 · %2").arg(levelName(level), QString::number(rooms));
}

QList<qreal> stackPlatforms(const int count, const qreal anchor, const qreal spacing, const qreal top, const qreal bottom)
{
    QList<qreal> wanted;
    if (count <= 0) {
        return wanted;
    }
    const bool upward = spacing < 0.0;
    const qreal step = std::abs(spacing);
    // Listed top to bottom: upward the farthest first, downward the nearest
    for (int i = 0; i < count; ++i) {
        wanted.append(upward ? anchor - (count - i) * step : anchor + (i + 1) * step);
    }
    // Kept to their own side of the anchor, and in the view where there is room for them all
    const qreal sideTop = upward ? top : std::max(top, anchor + step);
    const qreal sideBottom = upward ? std::min(bottom, anchor - step) : bottom;
    return spreadCallouts(wanted, step, sideTop, std::max(sideBottom, sideTop + (count - 1) * step));
}

QImage bullseye(const QColor& color, const int size)
{
    QImage image = clearImage(QSize(size, size), color);
    QPainter painter(&image);
    painter.setRenderHint(QPainter::Antialiasing);
    const qreal half = size / 2.0;
    const QPointF center(half, half);
    painter.setPen(Qt::NoPen);
    painter.setBrush(Qt::white);
    painter.drawEllipse(center, half * 0.96, half * 0.96);
    ring(painter, center, half * 0.82, color, size * 0.13);
    painter.setPen(Qt::NoPen);
    painter.setBrush(color);
    painter.drawEllipse(center, half * 0.36, half * 0.36);
    painter.end();
    return image;
}

QImage taggedSymbol(const QString& symbol, const QColor& color, const QColor& plate, const QFont& font, const qreal fontFudgeFactor, const int size)
{
    QImage image = clearImage(QSize(size, size), plate);
    QPainter painter(&image);
    painter.setRenderHint(QPainter::Antialiasing);
    const QRectF tag = QRectF(image.rect()).adjusted(size * 0.14, size * 0.14, -size * 0.14, -size * 0.14);
    painter.setPen(Qt::NoPen);
    painter.setBrush(plate);
    painter.drawRoundedRect(tag, size * 0.16, size * 0.16);
    ushort fontSize = 1;
    const int inset = qRound(size * 0.08);
    RoomAppearance::paintSymbol(painter, tag.toAlignedRect().adjusted(inset, inset, -inset, -inset), symbol, color, font, fontFudgeFactor, fontSize);
    painter.end();
    return image;
}

QString titleLevelLine(const int shownLevel, const QList<int>& areaLevels)
{
    const qsizetype sheet = areaLevels.indexOf(shownLevel);
    if (sheet < 0) {
        //: The second line of the title block that the blueprint style of the 3D map draws in a corner, as on a technical drawing: the level shown. The style sets it in capitals
        return QCoreApplication::translate("ModernGLWidget", "Level %1").arg(shownLevel);
    }
    //: The second line of the title block that the blueprint style of the 3D map draws in a corner, as on a technical drawing: the level shown, which of the area's levels it is counted from the lowest, and how many levels the area has. The style sets it in capitals
    return QCoreApplication::translate("ModernGLWidget", "Level %1 · sheet %2 of %3").arg(QString::number(shownLevel), QString::number(sheet + 1), QString::number(areaLevels.size()));
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

bool keepsLabelBox(const TMapLabel& label)
{
    return !neutral(label.bgColor) && label.bgColor.alpha() > 0;
}

QImage themedLabel(const TMapLabel& label, const MapStyle& style)
{
    // A tag's margin round the text, above and below it and twice that at its ends, as a part of the text's height
    constexpr qreal platePad = 0.3;
    const QSize canvas = label.pix.size();
    if (!style.themedMapLabels || label.text.isEmpty() || canvas.isEmpty()) {
        return QImage();
    }
    QFont font = label.font;
    if (font.family().isEmpty() && !style.labelFontFamily.isEmpty()) {
        font.setFamily(style.labelFontFamily);
        style.letter(font);
    }
    const bool ownTextColor = !neutral(label.fgColor) || !style.mapLabelColor.isValid();
    const QColor textColor = ownTextColor ? label.fgColor : withAlpha(style.mapLabelColor, label.fgColor.alpha());
    const bool box = keepsLabelBox(label);
    const bool outline = !neutral(label.outlineColor) && label.outlineColor != label.fgColor;
    // A label's own light color would be lost on the tag, so it keeps the halo instead
    const bool plate = !box && style.mapLabelPlate.isValid() && MapStyle::readsOnPlate(textColor, style.mapLabelPlate);
    // As for the tag, a label's own light color keeps the halo instead
    const bool knockout = !box && !plate && style.mapLabelKnockout.isValid() && MapStyle::readsOnPlate(textColor, style.mapLabelKnockout);
    const bool halo = !box && !plate && !knockout && !outline && style.mapLabelHalo.isValid();

    // Centered on the label's own picture, as the 2D map's label dialog centers it in the box drawn for it, so the
    // label keeps its place and size; only the text shrinks, to fit the box in the style's font and below its limit
    constexpr int flags = Qt::AlignCenter;
    static const QImage metricsDevice(1, 1, QImage::Format_ARGB32);
    const QRectF bounds = QFontMetricsF(font, &metricsDevice).boundingRect(QRectF(QPointF(), QSizeF(canvas)), flags, label.text);
    const qreal margin = halo ? 2.0 : 1.0;
    qreal shrink = 1.0;
    if (bounds.width() > 0.0 && bounds.height() > 0.0) {
        shrink = std::min({1.0, (canvas.width() - 2.0 * margin) / bounds.width(), (canvas.height() - 2.0 * margin) / bounds.height()});
        if (plate || knockout) {
            shrink = std::min({shrink, canvas.width() / (bounds.width() + 4.0 * platePad * bounds.height()), canvas.height() / ((1.0 + 2.0 * platePad) * bounds.height())});
        }
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
    if (plate) {
        const QRectF text = QFontMetricsF(font, &metricsDevice).boundingRect(textRect, flags, label.text);
        const qreal pad = text.height() * platePad;
        const QRectF tag = text.adjusted(-2.0 * pad, -pad, 2.0 * pad, pad).intersected(textRect.adjusted(0.5, 0.5, -0.5, -0.5));
        painter.setPen(Qt::NoPen);
        painter.setBrush(style.mapLabelPlate);
        painter.drawRoundedRect(tag, tag.height() / 2.0, tag.height() / 2.0);
    }
    if (knockout) {
        // Patches ever smaller laid one on another, so that over the text they add up to the knockout's opacity and
        // toward the edge they thin out, as paper scraped clean does
        constexpr int layers = 8;
        const QRectF text = QFontMetricsF(font, &metricsDevice).boundingRect(textRect, flags, label.text);
        const qreal pad = text.height() * platePad;
        QColor layer = style.mapLabelKnockout;
        layer.setAlphaF(1.0 - std::pow(1.0 - style.mapLabelKnockout.alphaF(), 1.0 / layers));
        painter.setPen(Qt::NoPen);
        painter.setBrush(layer);
        for (int index = layers; index >= 1; --index) {
            const qreal reach = pad * index / layers;
            const QRectF patch = text.adjusted(-2.0 * reach, -reach, 2.0 * reach, reach).intersected(textRect);
            painter.drawRoundedRect(patch, patch.height() / 2.0, patch.height() / 2.0);
        }
    }
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

namespace {
// Smooth noise round a circle, from 0 to 1, the same for every variant's seed on every machine
qreal loopNoise(const qreal turn, const int seed, const int bumps)
{
    const auto random = [seed](const int cell) {
        quint32 v = static_cast<quint32>(cell) * 747796405u + static_cast<quint32>(seed) * 2891336453u + 1u;
        v = ((v >> ((v >> 28u) + 4u)) ^ v) * 277803737u;
        return static_cast<qreal>(((v >> 22u) ^ v) & 0xffffu) / 65535.0;
    };
    const qreal at = turn * bumps;
    const int cell = static_cast<int>(std::floor(at));
    qreal f = at - cell;
    f = f * f * (3.0 - 2.0 * f);
    return random(cell % bumps) * (1.0 - f) + random((cell + 1) % bumps) * f;
}
} // namespace

QImage wash(const int variant, const int size)
{
    QImage image = clearImage(QSize(size, size), Qt::white);
    const qreal half = size / 2.0;
    for (int y = 0; y < size; ++y) {
        auto* line = reinterpret_cast<QRgb*>(image.scanLine(y));
        for (int x = 0; x < size; ++x) {
            const qreal dx = (x + 0.5 - half) / half;
            const qreal dy = (y + 0.5 - half) / half;
            const qreal distance = std::hypot(dx, dy);
            const qreal turn = (std::atan2(dy, dx) + M_PI) / (2.0 * M_PI);
            const qreal reach = 0.72 + 0.16 * loopNoise(turn, variant, 5) + 0.07 * loopNoise(turn, variant + 17, 13);
            if (distance >= reach) {
                continue;
            }
            const qreal depth = (reach - distance) / reach;
            // Faint toward the middle, and darker in a rim just inside the edge, where the paint dried
            const qreal body = 0.55 + 0.45 * std::min(1.0, depth / 0.25);
            const qreal pooled = std::exp(-std::pow((depth - 0.06) / 0.06, 2.0)) * 0.35;
            const qreal soft = std::min(1.0, depth / 0.05);
            const int alpha = std::clamp(qRound(255.0 * soft * std::min(1.0, 0.55 * body + pooled)), 0, 255);
            line[x] = qRgba(255, 255, 255, alpha);
        }
    }
    return image;
}

bool environmentMarked(const EnvironmentKit::Kit kit)
{
    return kit == EnvironmentKit::Kit::Forest || kit == EnvironmentKit::Kit::Water;
}

QImage environmentMark(const EnvironmentKit::Kit kit, const QColor& ink, const int size)
{
    if (!environmentMarked(kit)) {
        return QImage();
    }
    QImage image = clearImage(QSize(size, size), ink);
    QPainter painter(&image);
    painter.setRenderHint(QPainter::Antialiasing);
    const qreal unit = size / 16.0;
    QPen pen(ink, unit * 0.7, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
    painter.setPen(pen);
    if (kit == EnvironmentKit::Kit::Forest) {
        // Two trees, a crown on a trunk, the second a little behind
        for (const QPointF& foot : {QPointF(6.0, 12.0), QPointF(10.5, 10.5)}) {
            const QPointF base = foot * unit;
            painter.setBrush(withAlpha(ink, 70));
            painter.drawLine(base, base - QPointF(0.0, 2.5 * unit));
            painter.drawEllipse(base - QPointF(0.0, 4.5 * unit), 2.3 * unit, 2.3 * unit);
        }
    } else {
        // Two short waves, one under the other and set off from it
        painter.setBrush(Qt::NoBrush);
        for (const QPointF& start : {QPointF(3.0, 7.0), QPointF(5.0, 10.5)}) {
            QPainterPath wave(start * unit);
            const QPointF at = start * unit;
            wave.cubicTo(at + QPointF(1.5, -1.5) * unit, at + QPointF(2.5, -1.5) * unit, at + QPointF(4.0, 0.0) * unit);
            wave.cubicTo(at + QPointF(5.5, 1.5) * unit, at + QPointF(6.5, 1.5) * unit, at + QPointF(8.0, 0.0) * unit);
            painter.drawPath(wave);
        }
    }
    painter.end();
    return image;
}

int scaleBarRooms(const qreal pixelsPerRoom, const qreal shortest, const qreal longest)
{
    if (!(pixelsPerRoom > 0.0)) {
        return 0;
    }
    int chosen = 0;
    for (const int rooms : {2, 5, 10, 20, 50, 100, 200, 500, 1000}) {
        const qreal length = rooms * pixelsPerRoom;
        if (length > longest) {
            break;
        }
        chosen = rooms;
        if (length >= shortest) {
            break;
        }
    }
    return chosen * pixelsPerRoom >= shortest ? chosen : 0;
}

} // namespace MapDecals
