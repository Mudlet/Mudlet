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
    // What marks a highlighted room
    enum class HighlightShape {
        // The 2D map's disc, shading from the room's highlight center color out to its highlight color
        Disc,
        // A soft glow over the room, brightest at its middle and fading out past it
        Glow,
        // A dashed ring around the room, as a pencil marks a spot on a plan
        DashedEllipse,
        // Two thin rings around the room, one inside the other
        DoubleEllipse,
        // A broad soft ring on the ground round the room, clear in its middle
        Halo,
        // The room's own square or circle filled, as a text screen's cursor fills its cell
        Cell,
        // Two bright rings with a faint glow between them
        Rings
    };

    // What rings the player's room, where the profile keeps the original player room marker
    enum class PlayerRing {
        // That marker
        Profile,
        // A thin ring in playerColor
        Thin,
        // A registration mark in playerColor: a thin ring crossed by two lines that run on past it
        Crosshair
    };

    enum class Palette {
        // Each room in its environment's color, as on the 2D map
        Environment,
        // A drafting sheet: rooms filled with the style's background, only faintly tinted by their environment's color, and drawn in its ink
        Ink,
        // A text screen: each room's character in its environment's color, lifted where that would vanish into the
        // dark background, as a terminal shows its black as dark gray
        Terminal,
        // Each room in its environment's color, softened toward a lit pastel: less saturated, and kept from the
        // darkest and lightest
        Soft
    };

    enum class Shape {
        // Round where the profile has round rooms, else square
        Profile
    };

    enum class Edges {
        // A rim around each room on the viewed level, so that level stands apart from the others
        ViewedLevelRim,
        // A thin line around each room's top in the style's ink: solid on the viewed level, dashed and dimmed
        // with no fill on the others
        InkOutlines,
        // A base around each room on the viewed level, halfway from the room's color to the background, so the
        // room reads as a block with shaded sides
        ViewedLevelBase
    };

    enum class Doors {
        // The 2D map's door strokes in the door's status color
        Strokes,
        // A plan's door: a leaf hinged at the side of the opening and the arc it swings through,
        // swung open for an open door and shut across the opening for a closed or locked one
        Swings
    };

    enum class Glyphs {
        // The room's own symbol, drawn on its top
        RoomSymbols,
        // No blocks: every room is a character on a grid of character cells half rooms' spacing across, as
        // RoomAppearance::terminalGlyph() picks, and its exits are rows of line characters in the cells between
        Terminal
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
    // Used instead of the profile's mapper background where the profile keeps the default black; invalid to keep it
    QColor defaultBackground;
    HighlightShape highlightShape = HighlightShape::Disc;
    // Every shape but the disc in this color; invalid for the room's own highlight color
    QColor highlightColor;
    // How much bigger than the highlight radius the room was given a shape is drawn, but for the cell
    float highlightScale = 1.0f;
    PlayerRing playerRing = PlayerRing::Profile;
    // The player's ring and the beacon over the player's room, where the profile keeps the original player room
    // marker; invalid for that marker's red
    QColor playerColor;
    // Map labels of text drawn again in labelFontFamily and mapLabelColor without the box behind them, but for what
    // the label was given on purpose: its own font, a text color with a hue, a box with a hue
    bool themedMapLabels = false;
    QColor mapLabelColor;
    // Around the text of a themed label with no box, so it reads over rooms; invalid for none
    QColor mapLabelHalo;
    // In rooms' spacing; 0 for no limit
    float mapLabelMaxHeight = 0.0f;
    // Room names, as quieter annotations: their color where the room was given none of its own, invalid for the
    // color that stands out on the background, and their size against the usual where the map was given no font
    QColor roomNameColor;
    float roomNameScale = 1.0f;
    // Invalid for the background, the color that stands out on it, and red
    QColor compassFace;
    QColor compassRim;
    QColor compassNeedle;
    // The room info box, where the profile keeps the default background for it; invalid for that, and for black or
    // white text, whichever stands out on the background
    QColor infoBackground;
    QColor infoTextColor;
    // Each room's top shaded from light at its north west to dark at its south east, with a soft shadow on the ground
    // to its south east
    bool shadedBlocks = false;
    // Exits straight up or down drawn dashed, as a stair is on a plan
    bool dashedStairs = false;
    PostEffects postEffects;
    Glyphs glyphs = Glyphs::RoomSymbols;
    // With the Terminal glyphs, a room with no symbol or stairs shown by the kind of ground its environment is, as
    // RoomAppearance::groundGlyph() draws it, rather than as #; and a symbol underlined in its room's color
    bool groundGlyphs = false;
    // The Ink palette's lines and text; invalid for the profile's exit color
    QColor ink;
    // How much of its environment's color the Ink palette fills a room with, from none to all of it
    float inkTint = 0.18f;
    // A column rising from the player's room
    bool playerBeacon = true;
    // The style's lettering: its spacing as a percentage of the font's usual, and whether it is set in capitals
    qreal labelLetterSpacing = 100.0;
    bool labelCapitals = false;
    // A drafting sheet's title block in a corner of the view: the area's name, and which of its levels is shown
    bool titleBlock = false;
    // Each other level shown named, with its count of rooms, beside it at the end of a leader line
    bool levelCallouts = false;
    // Invalid to draw the profile's grid, when it shows one; else the style's own, always shown, with its lines
    // between the rooms and every fifth one stronger
    QColor gridColor;
    // Exit lines' width, as a part of the width the profile's exit size gives
    float exitWidthFactor = 1.0f;
    Doors doors = Doors::Strokes;
    // A faint plate under each level's rooms, which tints the background behind rooms filled with it
    bool floorPlates = true;
    // A line along the view's foot naming the area, the player's room and the ways out of it, and the room under the
    // mouse pointer, as a terminal's status line in the profile's ANSI colors
    bool statusLine = false;

    QString displayName() const;
    // Whether this style's player ring and color take the place of the profile's player room marker
    bool themesPlayerMarker(int profilePlayerRoomStyle, bool strongHighlight) const;
    // The info box's background, given the profile's
    QColor infoBackgroundColor(const QColor& profileInfoBackground) const;
    // A room's border, given the profile's, where the room was given none of its own
    QColor roomBorderColor(const QColor& profileBorder, const QColor& roomColor, const QColor& background) const;
    // A room's name color, given the color it was given as user data
    QColor roomNameColorFor(const QString& roomColorData, const QColor& background) const;
    QColor roomColor(const QColor& environmentColor) const;
    QColor backgroundColor(const QColor& profileBackground) const;
    bool roundRooms(bool profileRoundRooms) const;
    QFont labelFont(const QFont& mapSymbolFont) const;
    // Sets the style's lettering on a font that has its family
    void letter(QFont& font) const;
    // roomSymbolColor is the room's own, invalid unless one was set; drawnRoomColor is roomColor()'s
    QColor symbolColor(const QColor& roomSymbolColor, const QColor& drawnRoomColor) const;
    QColor exitColor(const QColor& profileExitColor) const;
    QColor roomIdColor(const QColor& drawnRoomColor) const;
    // The player's room, where its marker could not be made
    QColor playerRoomColor() const;

    static QString defaultId();
    // In the order the preferences list them
    static const QList<MapStyle>& all();
    static QStringList ids();
    // nullptr when no style has that id
    static const MapStyle* find(const QString& id);
    static const MapStyle& findOrDefault(const QString& id);
};

Q_DECLARE_OPERATORS_FOR_FLAGS(MapStyle::PostEffects)
