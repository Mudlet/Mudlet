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
        Environment,
        // A drafting sheet: rooms filled with the style's background, only faintly tinted by their environment's color, and drawn in its ink
        Ink,
        // A text screen: each room's character in its environment's color, lifted where that would vanish into the
        // dark background, as a terminal shows its black as dark gray
        Terminal,
        // A metro map: every room filled white as a station is, its environment's color going to the ring around it
        Stations,
        // Projected light: every room glowing in its environment's hue, at full strength and kept bright enough to
        // glow on the dark background; a gray environment glows in the style's ink
        Light,
        // Ink on paper: every room in a dark ink keeping a little of its environment's hue, so areas still tell apart
        Wash,
        // A painted miniature: each room in its environment's color, a little desaturated, kept from the darkest and
        // lightest, and warmed, as model paints are
        Painted
    };

    enum class Shape {
        // Round where the profile has round rooms, else square
        Profile,
        // Always round, whatever the profile says
        Round
    };

    enum class Edges {
        // A rim around each room on the viewed level, so that level stands apart from the others
        ViewedLevelRim,
        // A thin line around each room's top in the style's ink: solid on the viewed level, dashed and dimmed
        // with no fill on the others
        InkOutlines,
        // A metro map's station: a ring just inside each room's edge in ringColor(), and a heavier one in the style's ink
        // around an interchange, a room with interchangeExits or more exits
        StationRings,
        // A hologram: no fill, every edge of each room's block and of each exit's bar a glowing line, all added onto what
        // is behind rather than covering it, so that rooms and levels show through each other; a highlighted room is
        // circled by two glowing rings in its highlight color
        GlowingWireframes,
        // An ink painting: no rim, each room's top a blot of ink with soft, uneven edges and each exit a brush stroke
        // thinning toward its end, uneven in a way each takes from where it stands so that it holds still
        InkBlots,
        // A tabletop model: no rim, each room a block shaded under soft light from above with its top's edges rounded
        // off, darkening where it meets the ground and casting a soft shadow there, and each exit a low path on the ground
        ModelBlocks
    };

    enum class Doors {
        // The 2D map's door strokes in the door's status color
        Strokes,
        // A plan's door: a leaf hinged at the side of the opening and the arc it swings through,
        // swung open for an open door and shut across the opening for a closed or locked one
        Swings,
        // A short bar across the line just outside the room, in the door's status color
        Bars
    };

    enum class PlayerMarker {
        // The profile's ring around the player's room, and a column of light over it
        Ring,
        // A flag standing on the player's room reading "YOU", as on a map at a station's entrance
        Flag,
        // The player's room drawn in playerRoomColor(), with a diamond of light in that color hovering over it and
        // sending out a ring of light each time the player arrives
        Pulse,
        // The player's room stamped in playerRoomColor() as a seal on a painting, with a frame cut into it
        Seal,
        // A game's pawn in playerRoomColor(), a foot, a body and a ball for a head, standing on the player's room,
        // which keeps its own color
        Token
    };

    enum class Glyphs {
        // The room's own symbol, drawn on its top
        RoomSymbols,
        // No blocks: every room is a character on a grid of character cells half rooms' spacing across, as
        // RoomAppearance::terminalGlyph() picks, and its exits are rows of line characters in the cells between
        Terminal
    };

    enum class PostEffect {
        NoPostEffects = 0x0,
        // Every few rows of pixels of what is drawn on the map dimmed, as on a projected image
        Scanlines = 0x1,
        // The background drawn as paper, with a faint grain that stays put on the screen
        PaperGrain = 0x2,
        // The background a pale sky, light blue at the top of the view fading to a warm haze at its foot
        Sky = 0x4,
        // A tilt-shift photograph's shallow focus: sharp in a band across the view through the player, blurring
        // toward the view's top and foot, so that the map looks like a model seen up close
        TiltShift = 0x8,
        // The brightest of what is drawn blurred and added back, so that lines of light glow softly
        Bloom = 0x10
    };
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
    // The Ink palette's lines and text; invalid for the profile's exit color
    QColor ink;
    // Invalid to draw the profile's grid, when it shows one; else the style's own, always shown, with its lines
    // between the rooms and every fifth one stronger
    QColor gridColor;
    // Exit lines' width, as a part of the width the profile's exit size gives
    float exitWidthFactor = 1.0f;
    Doors doors = Doors::Strokes;
    // A faint plate under each level's rooms, which tints the background behind rooms filled with it
    bool floorPlates = true;
    // A line along the view's foot naming the area and the player's room, as a terminal's status line
    bool statusLine = false;
    PlayerMarker playerMarker = PlayerMarker::Ring;
    // Exit lines between two rooms of one environment drawn in ringColor() of it, as a metro map draws its lines,
    // and the rest in exitColor()
    bool environmentLines = false;
    // The fewest exits that make a room an interchange, as Edges::StationRings draws one; 0 for none
    int interchangeExits = 0;
    // Each room built as DioramaKit picks for its environment, a room with rooms straight above it a building of
    // that many storeys, all standing on a wooden board under the viewed level
    bool environmentKits = false;
    // Room names and map labels drawn with a rim of the background around them, so that lines under them do not
    // run through the text
    bool labelHalos = false;
    // Room names and map labels multiplied by this color, to sit back in the scene; invalid for their own colors
    QColor labelTint;
    // Brackets in the view's corners and, in its top left, the area's name, levels and rooms, as on a scanner's display
    bool scannerOverlay = false;

    QString displayName() const;
    QColor roomColor(const QColor& environmentColor) const;
    QColor backgroundColor(const QColor& profileBackground) const;
    bool roundRooms(bool profileRoundRooms) const;
    QFont labelFont(const QFont& mapSymbolFont) const;
    // roomSymbolColor is the room's own, invalid unless one was set; drawnRoomColor is roomColor()'s
    QColor symbolColor(const QColor& roomSymbolColor, const QColor& drawnRoomColor) const;
    QColor exitColor(const QColor& profileExitColor) const;
    QColor roomIdColor(const QColor& drawnRoomColor) const;
    // roomBorderColor is the room's own, invalid unless one was set; drawnRoomColor is roomColor()'s
    QColor wireColor(const QColor& roomBorderColor, const QColor& drawnRoomColor) const;
    // The player's room, where its marker could not be made
    QColor playerRoomColor() const;
    // A ring or a line in an environment's color, made darker or lighter where it would not stand out on backgroundColor()
    QColor ringColor(const QColor& environmentColor, const QColor& profileBackground) const;

    static QString defaultId();
    // In the order the preferences list them
    static const QList<MapStyle>& all();
    static QStringList ids();
    // nullptr when no style has that id
    static const MapStyle* find(const QString& id);
    static const MapStyle& findOrDefault(const QString& id);
};

Q_DECLARE_OPERATORS_FOR_FLAGS(MapStyle::PostEffects)
