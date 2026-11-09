/***************************************************************************
 *   Copyright (C) 2026 by Mudlet Developers - mudlet@mudlet.org           *
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

#include "MapColourPreview.h"

#include "Host.h"
#include "uiDesign.h"

#include <QEvent>
#include <QFontMetrics>
#include <QPainter>
#include <QPainterPath>
#include <QPen>

#include <algorithm>
#include <optional>

namespace uiDesign {

namespace {

constexpr int scmPadding = 6;
constexpr int scmInfoPadding = 4;
// The rooms stand on a grid this many cells across and down, the cells as large as the widget lets them be
constexpr int scmColumns = 7;
constexpr int scmRows = 4;
constexpr int scmLargestCell = 64;
constexpr int scmSmallestCell = 8;

struct PreviewRoom
{
    QPoint cell;
    QColor Host::* pFill = nullptr;
};

// No room under the info box's corner, which covers the top left as it does on the map
const QList<PreviewRoom>& rooms()
{
    static const QList<PreviewRoom> placed{{{4, 0}, &Host::mGreen_2},
                                           {{5, 0}, &Host::mYellow_2},
                                           {{2, 1}, &Host::mBlue_2},
                                           {{3, 1}, &Host::mMagenta_2},
                                           {{4, 1}, &Host::mCyan_2},
                                           {{1, 2}, &Host::mRed_2},
                                           {{3, 2}, &Host::mWhite_2},
                                           {{3, 3}, &Host::mLightBlack_2}};
    return placed;
}

const QList<std::pair<QPoint, QPoint>>& links()
{
    static const QList<std::pair<QPoint, QPoint>> joined{
            {{4, 0}, {5, 0}}, {{4, 0}, {4, 1}}, {{2, 1}, {3, 1}}, {{3, 1}, {4, 1}}, {{3, 1}, {3, 2}}, {{2, 1}, {1, 2}}, {{3, 2}, {3, 3}}, {{4, 1}, {5, 2}}};
    return joined;
}

constexpr QPoint scmOverlappingCell{5, 2};
constexpr QPoint scmLowerLevelCell{6, 1};
constexpr QPoint scmUpperLevelCell{1, 3};

} // namespace

MapColourPreview::MapColourPreview(QWidget* pParent)
: QWidget(pParent)
{
    // As tall as whatever stands beside it and never taller, and given whatever width is left over
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Ignored);
    setFocusPolicy(Qt::NoFocus);
    retranslate();
}

void MapColourPreview::setHost(Host* pHost)
{
    mpHost = pHost;
    update();
}

QSize MapColourPreview::sizeHint() const
{
    return {0, 0};
}

QSize MapColourPreview::minimumSizeHint() const
{
    return {0, 0};
}

void MapColourPreview::retranslate()
{
    //: Screen reader name of the picture beside the sixteen colors on the Mapper settings page, showing a small map drawn in the colors chosen there
    setAccessibleName(tr("Preview of the map colors"));
}

void MapColourPreview::changeEvent(QEvent* pEvent)
{
    QWidget::changeEvent(pEvent);
    if (pEvent->type() == QEvent::LanguageChange) {
        retranslate();
        update();
    }
}

void MapColourPreview::paintEvent(QPaintEvent* pEvent)
{
    Q_UNUSED(pEvent)
    Host* pHost = mpHost;
    if (!pHost) {
        return;
    }
    const Host& host = *pHost;

    const int cell = std::clamp(std::min((width() - 2 * scmPadding) / scmColumns, (height() - 2 * scmPadding) / scmRows), scmSmallestCell, scmLargestCell);
    const QPoint origin((width() - scmColumns * cell) / 2, (height() - scmRows * cell) / 2);
    const auto centreOf = [&origin, cell](const QPoint& at) {
        return QPointF(origin.x() + (at.x() + 0.5) * cell, origin.y() + (at.y() + 0.5) * cell);
    };
    // Rooms, borders and exits measured as T2DMap measures them, a cell standing for its mRoomWidth
    const qreal side = cell * host.mRoomSize;
    const auto roomAt = [&centreOf, side](const QPoint& at) {
        const QPointF centre = centreOf(at);
        return QRectF(centre.x() - side / 2.0, centre.y() - side / 2.0, side, side);
    };
    const int borderWidth = std::max(1, static_cast<int>(1.0 / host.mRoomBorderSize * side));

    const ThemeTokens tokens = themeTokens();
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    const qreal half = scmInputBorderWidth / 2.0;
    QPainterPath outline;
    outline.addRoundedRect(QRectF(rect()).adjusted(half, half, -half, -half), scmRadiusInput, scmRadiusInput);
    painter.fillPath(outline, host.mBgColor_2);
    painter.setClipPath(outline);

    // Through the rooms' centres and on across the whole picture, in the pen T2DMap draws its grid with: as wide as
    // the grid width setting makes it against a room's width, and cosmetic with high quality graphics
    painter.setRenderHint(QPainter::Antialiasing, host.mMapperUseAntiAlias);
    QPen gridPen(host.mMapGridColor);
    gridPen.setWidthF(host.mMapGridLineSize * cell / 50.0);
    gridPen.setCosmetic(host.mMapperUseAntiAlias);
    painter.setPen(gridPen);
    const auto firstLine = [cell](const int through) {
        const int first = through % cell;
        return first < 0 ? first + cell : first;
    };
    for (int x = firstLine(origin.x() + cell / 2); x < width(); x += cell) {
        painter.drawLine(x, 0, x, height());
    }
    for (int y = firstLine(origin.y() + cell / 2); y < height(); y += cell) {
        painter.drawLine(0, y, width(), y);
    }

    painter.setPen(QPen(host.mFgColor_2, 1.0 / host.mLineSize * side));
    for (const auto& [from, to] : links()) {
        painter.drawLine(centreOf(from), centreOf(to));
    }

    // A level down filled and a level up outlined, each shifted off its cell the way the map shifts them
    const qreal shift = 0.3 * side;
    painter.setPen(Qt::NoPen);
    painter.setBrush(host.mLowerLevelColor);
    painter.drawRect(roomAt(scmLowerLevelCell).translated(-shift, shift));
    painter.setPen(QPen(host.mUpperLevelColor, 1));
    painter.setBrush(Qt::NoBrush);
    painter.drawRect(roomAt(scmUpperLevelCell).translated(shift, -shift));

    // The border grows outward from the room, faded on a room under 12px wide as the map fades it
    const auto drawRoom = [&painter, borderWidth, cell](const QRectF& square, const QColor& fill, const std::optional<QColor>& border) {
        painter.fillRect(square, fill);
        if (!border) {
            return;
        }
        QColor ink = *border;
        if (cell < 12) {
            ink.setAlpha(255 * cell / 12);
        }
        QPainterPath ring;
        ring.setFillRule(Qt::OddEvenFill);
        ring.addRect(square.adjusted(-borderWidth, -borderWidth, borderWidth, borderWidth));
        ring.addRect(square);
        painter.fillPath(ring, ink);
    };
    const std::optional<QColor> roomBorder = host.mMapperShowRoomBorders ? std::optional<QColor>(host.mRoomBorderColor) : std::nullopt;
    for (const PreviewRoom& room : rooms()) {
        drawRoom(roomAt(room.cell), host.*room.pFill, roomBorder);
    }
    // Two rooms on one spot, the second over the first, bordered whether borders are on or not
    const QRectF overlapping = roomAt(scmOverlappingCell);
    painter.fillRect(overlapping, host.mLightRed_2);
    drawRoom(overlapping, host.mLightBlue_2, host.mRoomCollisionBorderColor);

    // Anchored at the top left, so both of its lines are on show however short the picture is
    QFont font = pHost->getDisplayFont();
    font.setPixelSize(std::clamp(cell / 2, 9, 13));
    const QFontMetrics metrics(font);
    //: Sample text in the info box of the map colors preview on the Mapper settings page: the name of the room the player is in
    const QString roomName = tr("Merchant's Crossing");
    //: Sample text in the info box of the map colors preview on the Mapper settings page: the area the room is in, and its number
    const QString area = tr("Area: Old Town (12)");
    const QRect info(scmPadding, scmPadding, 2 * scmInfoPadding + std::max(metrics.horizontalAdvance(roomName), metrics.horizontalAdvance(area)), 2 * scmInfoPadding + 2 * metrics.lineSpacing());
    painter.fillRect(info, host.mMapInfoBg);
    // theme-fixed: the words are drawn on the map's own colours, so they take the map's own choice of ink, as T2DMap makes it
    const QColor infoInk = host.mBgColor_2.lightness() > 127 ? QColor(Qt::black) : QColor(Qt::white);
    painter.setFont(font);
    painter.setPen(infoInk);
    painter.drawText(info.left() + scmInfoPadding, info.top() + scmInfoPadding + metrics.ascent(), roomName);
    painter.drawText(info.left() + scmInfoPadding, info.top() + scmInfoPadding + metrics.lineSpacing() + metrics.ascent(), area);

    painter.setClipping(false);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(QPen(tokens.border, scmInputBorderWidth));
    painter.setBrush(Qt::NoBrush);
    painter.drawPath(outline);
}

} // namespace uiDesign
