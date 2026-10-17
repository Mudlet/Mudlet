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

#include "EnvironmentKit.h"

#include <QBrush>
#include <QColor>
#include <QPointF>
#include <QString>

#include <optional>

class Host;
class QFont;
class QPainter;
class QRect;
class TMap;
class TRoom;

// How a room looks, shared by the 2D map and the modern 3D view so that the
// two draw a room from the same colours, symbol and rings.
namespace RoomAppearance {

// What a room shows, worked out once for whichever 3D map style draws it
struct Look
{
    QColor color;
    // Empty when the room has none
    QString symbol;
    // The room's own, invalid unless one was set; symbolColor() picks the drawn one against the color a style draws the room in
    QColor symbolColor;
    bool highlighted = false;
    QColor highlightColor;
    QColor highlightCenterColor;
    // In rooms' spacing
    float highlightRadius = 0.0f;
    bool playerRoom = false;
    // How many levels the room is above the viewed one, negative below
    int levelOffset = 0;

    bool onViewedLevel() const { return levelOffset == 0; }
};

Look look(const TMap& map, const Host& host, const TRoom& room, int playerRoomId, int viewedLevel);

QColor environmentColor(const TMap& map, const Host& host, int environmentId);
// Whether the map gives the environment a color, rather than it being drawn in the default ANSI red
bool environmentDefined(const TMap& map, int environmentId);

// The character a map drawn as a text screen shows for a room, roguelike fashion: @ for the player, the room's
// own symbol, < > or X for stairs up, down or both, else its ground's character where one is given, or #
// (. for an area in grid mode, which is open ground)
QString terminalGlyph(const TRoom& room, const Look& look, bool gridMode, std::optional<EnvironmentKit::Kit> ground = std::nullopt, bool asciiGround = false);
// The roguelike character for a kind of ground: " grass, ≈ water, # road, ♣ forest, ■ building; plain open ground
// in an area in grid mode is .
// in an area in grid mode is .; in ASCII, for a font without those, ~ water, * forest and = building
QString groundGlyph(EnvironmentKit::Kit ground, bool gridMode, bool ascii = false);
// Whether the font, or one the system falls back on, has a glyph for every character of the text
bool fontShows(const QFont& font, const QString& text);

// What a terminal map's exit is drawn in where it runs straight across or up the screen: a dim trail of dots, which
// unlike - and | neither runs together into a solid line nor breaks into dashes
constexpr QChar terminalExitDot{0x00B7};
// The character, terminalExitDot or / or \, that reads as a line running at degrees, counterclockwise from rightward
QChar terminalLineGlyph(qreal degrees);

// The room's exits in every direction and its special exits, wherever they lead; stubs are not exits
int exitCount(const TRoom& room);
// The room an exit draws its line to on the 3D map, else none: an exit drawn as a custom line has that instead,
// and no line leads to a hidden room
const TRoom* drawnExit(const TMap& map, const TRoom& room, int direction);
// Whether a line drawn on the room's own level leads to a room of the same environment, which draws the line between
// them in its color where the style colors lines by environment
bool onEnvironmentLine(const TMap& map, const TRoom& room);

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
