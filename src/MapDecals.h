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

#include "MapStyle.h"

#include <QGradient>
#include <QImage>

class TMapLabel;

// The pictures the modern 3D view lays on and around rooms for a map style, made without GL so that they can be
// checked on their own
namespace MapDecals {

// size texels across. color is the style's highlight color, else the room's; centerColor is the room's highlight
// center color, which only the disc shades to; round is for round rooms, which only the cell follows
QImage highlight(MapStyle::HighlightShape shape, const QColor& color, const QColor& centerColor, bool round, int size);
// Laid on the ground the room stands on, so that the room stands in it, rather than on its top
bool highlightOnGround(MapStyle::HighlightShape shape);
// Fills the room's own square or circle, whatever highlight radius the room was given
bool highlightFillsRoom(MapStyle::HighlightShape shape);

// The mark MapStyle::PlayerRing::Crosshair draws over the player's room, size texels across
QImage registrationMark(const QColor& color, int size);
// Where to put callouts, listed top to bottom, that each want to be at a height, so that they stay in that order at
// least spacing apart and, where they fit, between top and bottom
QList<qreal> spreadCallouts(const QList<qreal>& wanted, qreal spacing, qreal top, qreal bottom);

// The ring MapStyle::PlayerRing::Thin draws around the player's room
QGradientStops thinPlayerRingStops(const QColor& color);

// Lit from the map's north west: lighter there and darker toward the south east, once drawn with its first row north
QImage blockSheen(bool round, int size);
// Soft, darkest under its middle and fading out toward its edges
QImage blockShadow(bool round, int size);

// The label drawn again in the style's font and color, without the box behind it, where the label keeps the
// defaults for them, in place of the label's own picture and the same size: a null image when the style leaves labels
// as they are, or the label is a picture
QImage themedLabel(const TMapLabel& label, const MapStyle& style);

} // namespace MapDecals
