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

#pragma once

#include <QBrush>
#include <QColor>
#include <QPointF>

class Host;
class QFont;
class QPainter;
class QRect;
class QString;
class TMap;

// How a room looks, shared by the 2D map and the modern 3D view so that the
// two draw a room from the same colours, symbol and rings.
namespace RoomAppearance {

QColor environmentColor(const TMap& map, const Host& host, int environmentId);

// roomSymbolColor is the room's own, which is invalid unless one was set
QColor symbolColor(const QColor& roomSymbolColor, const QColor& roomColor);

// For a radial gradient over the highlight's whole radius
QGradientStops highlightStops(const QColor& highlightColor, const QColor& highlightCenterColor);

// For a radial gradient over playerRoomRadius()
QGradientStops playerRoomStops(int style, quint8 innerDiameterPercentage, const QColor& innerColor, const QColor& outerColor);

// The color of the column of light over the player's room in the 3D view, in the ring's style
QColor playerBeaconColor(int style, const QColor& outerColor);

// Fills the player's room under the ring when the strong highlight is on
QColor strongHighlightFillColor();

// At 100% the ring passes through the corners of a room roomVisualSize across
qreal playerRoomRadius(quint8 outerDiameterPercentage, qreal roomVisualSize);

// A room name's offset, in rooms' spacing with y increasing down the map, from
// the room's ROOM_UI_NAMEPOS: "x y", or just "y"
QPointF nameOffset(const QString& positionData);

// From the room's ROOM_UI_NAMECOLOR, else whichever of black or white stands out on background
QColor nameColor(const QString& colorData, const QColor& background);

// Draws text centered in rect at the largest size that fits, or nothing if
// even the smallest is too big. fontSize is where the search for that size
// starts and is left near the size found, so a caller drawing many symbols of
// one size can carry it from one to the next.
void paintSymbol(QPainter& painter, const QRect& rect, const QString& text, const QColor& color, const QFont& font, qreal fontFudgeFactor, ushort& fontSize);

} // namespace RoomAppearance
