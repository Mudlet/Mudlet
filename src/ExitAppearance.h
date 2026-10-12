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

#include <QColor>
#include <QLineF>
#include <QList>

#include <utility>

class QPainter;
class QPen;
class TRoom;

// How exits, doors and the up/down/in/out markers look, shared by the 2D map
// and the modern 3D view so that the two draw them alike.
namespace ExitAppearance {

// status is a TRoom::doors value: 1 open, 2 closed, otherwise locked
QColor doorColor(int status);

// The fill of the arrowhead on a one-way exit
QColor oneWayArrowColor();

// Black or white, whichever stands out on background
QColor contrastColor(const QColor& background);

// The two strokes of a door on exitLine, which starts at the room's center;
// roomSpan is a room's spacing across plus its spacing down
std::pair<QLineF, QLineF> doorStrokes(const QLineF& exitLine, qreal roomSpan);

// Alternate lengths drawn and skipped by a line of this style with a round or
// square cap, in multiples of its width; empty for a solid line
QList<qreal> visibleDashes(Qt::PenStyle style);

// The up/down/in/out exit and stub markers inside a room roomWidth by
// roomHeight centered on rx, ry. pen is left as the outer pen last used.
void paintExitMarkers(QPainter& painter, QPen& pen, const TRoom& room, float rx, float ry, qreal roomWidth, qreal roomHeight, const QColor& roomColor, bool cosmeticPen);

} // namespace ExitAppearance
