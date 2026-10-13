#ifndef MUDLET_EXITSCENE_H
#define MUDLET_EXITSCENE_H

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

#include <QChar>
#include <QColor>
#include <QList>
#include <QVector3D>

class Host;
class TMap;

// The exit lines, doors, stubs, custom lines and area-exit arrows of an area as
// the modern 3D view draws them, worked out apart from any OpenGL. Positions are
// in map coordinates: one unit between neighboring rooms, y increasing northward.
namespace ExitScene {

struct Options
{
    int areaId = 0;
    int centerZ = 0;
    int levelsAbove = 0;
    int levelsBelow = 0;
    // The width of an exit line, in rooms' spacing
    float lineWidth = 0.05f;
    // How tall a line stands; kept below a room's height so that rooms cover them
    float lineHeight = 0.05f;
    // In and out exits drawn as a short stub from their room, rather than all the way
    bool inOutStubs = false;
    bool largeAreaExitArrows = false;
    // Doors drawn as a plan's door swing, as MapStyle::Doors::Swings says, rather than the 2D map's strokes
    bool doorSwings = false;
    // Doors drawn as a bar across the line, as MapStyle::Doors::Bars says
    bool doorBars = false;
    // Lines between rooms of one environment, and stubs, drawn in its color made to stand out on background, as
    // MapStyle::environmentLines says, rather than in exitColor
    bool environmentLines = false;
    // Exit lines between rooms on one level, their stubs and doors drawn as Glyphs, as MapStyle::Glyphs::Terminal says
    bool glyphs = false;
    // Only where area exits can be clicked, with nothing to draw, for picking without the cost of the whole scene
    bool areaExitsOnly = false;
    QColor exitColor;
    // The map's background, against which area exits are outlined
    QColor background;
};

// A straight bar, width across and height tall, centered on the line from -> to;
// level is the z whose distance from the viewed level fades it
struct Segment
{
    QVector3D from;
    QVector3D to;
    float width = 0.0f;
    float height = 0.0f;
    QColor color;
    int level = 0;
    // A whole exit line out of its room, that a brush may taper; not a door, a dash or a piece of a custom line
    bool stroke = false;
};

// A flat filled triangle
struct Triangle
{
    QVector3D a;
    QVector3D b;
    QVector3D c;
    QColor color;
    int level = 0;
};

// A character in a cell of the grid, half a room's spacing across, that a map drawn as text is laid out on
struct Glyph
{
    QVector3D at;
    // The line it is part of runs this way, so that the one drawing it can pick the character that reads so on screen
    QVector3D along;
    // Null for a line's character
    QChar character;
    QColor color;
    int level = 0;
};

// An area exit's arrow, from its room's center to its tip, for clicking through to destinationId
struct AreaExit
{
    QVector3D from;
    QVector3D to;
    int destinationId = 0;
    int level = 0;
};

// Drawn with the depth test, so overlaps are settled by height, not list order: a door stands
// taller than its line and an arrowhead's fill sits a little off its outline
struct Scene
{
    QList<Segment> segments;
    QList<Triangle> triangles;
    // At most one in a cell, a door's over a line's
    QList<Glyph> glyphs;
    // Not drawn: where an area exit can be clicked
    QList<AreaExit> areaExits;
    // The first room with a dotted or dashed line drawn solid, as its pattern would repeat too often along it; 0 if none
    int solidDashRoomId = 0;
};

Scene build(const TMap& map, const Host& host, const Options& options);

} // namespace ExitScene

#endif // MUDLET_EXITSCENE_H
