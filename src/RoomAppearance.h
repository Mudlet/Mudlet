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

// At 100% the ring passes through the corners of a room roomVisualSize across
qreal playerRoomRadius(quint8 outerDiameterPercentage, qreal roomVisualSize);

// Draws text centered in rect at the largest size that fits, or nothing if
// even the smallest is too big. fontSize is where the search for that size
// starts and is left near the size found, so a caller drawing many symbols of
// one size can carry it from one to the next.
void paintSymbol(QPainter& painter, const QRect& rect, const QString& text, const QColor& color, const QFont& font, qreal fontFudgeFactor, ushort& fontSize);

} // namespace RoomAppearance
