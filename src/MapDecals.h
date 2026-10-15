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

class QFontMetricsF;
class TMapLabel;

namespace EnvironmentKit {
enum class Kit;
}

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
// Which of levels to call out beside the shown one: at most most of them, nearest it first, listed top to bottom
QList<int> calloutLevels(const QList<int>& levels, int shownLevel, int most);
// A level's name, as L1 or B1
QString levelName(int level);
// A level's callout: its name and how many rooms it has
QString levelCallout(int level, int rooms);
// Where to put count platforms, listed top to bottom, stacked spacing apart up from the anchor where spacing is
// negative, else down from it: each on the anchor's side, and between top and bottom where they fit there
QList<qreal> stackPlatforms(int count, qreal anchor, qreal spacing, qreal top, qreal bottom);
// The mark MapStyle::PlayerMarker::Pill draws on the player's room: a white disc, a ring and a dot, size texels across
QImage bullseye(const QColor& color, int size);
// A room's symbol in color on a rounded tag of plate, as MapStyle::symbolPlate draws it, size texels across
QImage taggedSymbol(const QString& symbol, const QColor& color, const QColor& plate, const QFont& font, qreal fontFudgeFactor, int size);
// A room's name in color on a rounded tag of plate, as MapStyle::roomNamePlate draws it, pad texels round the text
QImage taggedName(const QString& name, const QColor& color, const QColor& plate, const QFont& font, const QFontMetricsF& metrics, int pad);
// The title block's second line, for the shown level of an area with these levels, lowest first
QString titleLevelLine(int shownLevel, const QList<int>& areaLevels);

// The ring MapStyle::PlayerRing::Thin draws around the player's room
QGradientStops thinPlayerRingStops(const QColor& color);

// Lit from the map's north west: lighter there and darker toward the south east, once drawn with its first row north
QImage blockSheen(bool round, int size);
// Soft, darkest under its middle and fading out toward its edges
QImage blockShadow(bool round, int size);

// A watercolor wash, white so that it takes the color it is drawn in: clear at its edges, and pooling a little darker
// just inside them as watercolor dries, along an edge that wanders differently for each of scmWashVariants variants
constexpr int scmWashVariants = 4;
QImage wash(int variant, int size);
// Whether MapStyle::environmentMarks draws on a room of this kind of ground
bool environmentMarked(EnvironmentKit::Kit kit);
// What MapStyle::environmentMarks draws on a room of a kind of ground, in ink, size texels across: a null image for
// a kind it leaves bare
QImage environmentMark(EnvironmentKit::Kit kit, const QColor& ink, int size);
// How many rooms a scale bar spans, so that it is a round number of at least two and between shortest and longest
// pixels long, given how many pixels a room's spacing is; 0 where none fits
int scaleBarRooms(qreal pixelsPerRoom, qreal shortest, qreal longest);

// The label drawn again in the style's font and color, without the box behind it, where the label keeps the
// defaults for them, in place of the label's own picture and the same size: a null image when the style leaves labels
// as they are, or the label is a picture
QImage themedLabel(const TMapLabel& label, const MapStyle& style);
// Whether themedLabel() keeps the box behind the label, which it does only for a box given a hue on purpose
bool keepsLabelBox(const TMapLabel& label);

} // namespace MapDecals
