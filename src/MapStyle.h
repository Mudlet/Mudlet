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
#include <QFlags>
#include <QList>
#include <QString>
#include <QStringList>

class QFont;

// A look for the modern 3D mapper: one renderer draws every style, and a preset
// says how. A style is picked per profile by its id, which is what
// setConfig("mapStyle") takes, so an id must never change once released.
struct MapStyle
{
    enum class Palette {
        // Each room in its environment's color, as on the 2D map
        Environment
    };

    enum class Shape {
        // Round where the profile has round rooms, else square
        Profile
    };

    enum class Edges {
        // A rim around each room on the viewed level, so that level stands apart from the others
        ViewedLevelRim
    };

    enum class Glyphs {
        // The room's own symbol, drawn on its top
        RoomSymbols
    };

    enum class PostEffect { NoPostEffects = 0x0 };
    Q_DECLARE_FLAGS(PostEffects, PostEffect)

    QString id;
    Palette palette = Palette::Environment;
    // How many times a room's width fits in the space between rooms
    float roomScale = 4.0f;
    // How many times a room's height fits in its width
    float roomFlattening = 8.0f;
    Shape shape = Shape::Profile;
    Edges edges = Edges::ViewedLevelRim;
    // Lit blocks are shaded by the scene's light, unlit ones are their flat color
    bool lit = true;
    // Invalid for the profile's mapper background color
    QColor background;
    // Empty for the map's symbol font
    QString labelFontFamily;
    PostEffects postEffects;
    Glyphs glyphs = Glyphs::RoomSymbols;

    QString displayName() const;
    QColor roomColor(const QColor& environmentColor) const;
    QColor backgroundColor(const QColor& profileBackground) const;
    bool roundRooms(bool profileRoundRooms) const;
    QFont labelFont(const QFont& mapSymbolFont) const;

    static QString defaultId();
    // In the order the preferences list them
    static const QList<MapStyle>& all();
    static QStringList ids();
    // nullptr when no style has that id
    static const MapStyle* find(const QString& id);
    static const MapStyle& findOrDefault(const QString& id);
};

Q_DECLARE_OPERATORS_FOR_FLAGS(MapStyle::PostEffects)
