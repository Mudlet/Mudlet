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

#include "MapStyle.h"

#include "ExitAppearance.h"
#include "RoomAppearance.h"
#include "utils.h"

#include <QCoreApplication>
#include <QFont>

#include <algorithm>

namespace {
QList<MapStyle> registeredStyles()
{
    QList<MapStyle> styles;

    MapStyle tiles;
    tiles.id = qsl("tiles");
    styles.append(tiles);

    MapStyle blueprint;
    blueprint.id = qsl("blueprint");
    blueprint.palette = MapStyle::Palette::Ink;
    blueprint.roomFlattening = 16.0f;
    blueprint.edges = MapStyle::Edges::InkOutlines;
    blueprint.lit = false;
    blueprint.background = QColor(18, 60, 110);
    blueprint.ink = QColor(225, 242, 255);
    blueprint.gridColor = QColor(150, 200, 255, 38);
    blueprint.labelFontFamily = qsl("Bitstream Vera Sans");
    blueprint.exitWidthFactor = 0.5f;
    blueprint.doors = MapStyle::Doors::Swings;
    blueprint.floorPlates = false;
    styles.append(blueprint);

    MapStyle terminal;
    terminal.id = qsl("terminal");
    terminal.palette = MapStyle::Palette::Terminal;
    terminal.lit = false;
    terminal.background = QColor(Qt::black);
    // The ANSI palette's white, which a terminal draws its plain text in, so the rooms' colors stand out from the lines
    terminal.ink = QColor(170, 170, 170);
    terminal.labelFontFamily = qsl("Bitstream Vera Sans Mono");
    terminal.exitWidthFactor = 0.5f;
    terminal.floorPlates = false;
    terminal.glyphs = MapStyle::Glyphs::Terminal;
    terminal.statusLine = true;
    styles.append(terminal);

    MapStyle transit;
    transit.id = qsl("transit");
    transit.palette = MapStyle::Palette::Stations;
    // Stations smaller than tiles' blocks, so that the lines between them have room to show
    transit.roomScale = 5.0f;
    transit.roomFlattening = 16.0f;
    transit.shape = MapStyle::Shape::Round;
    transit.edges = MapStyle::Edges::StationRings;
    transit.lit = false;
    transit.background = QColor(245, 243, 236);
    transit.ink = QColor(35, 35, 40);
    transit.labelFontFamily = qsl("Bitstream Vera Sans");
    transit.exitWidthFactor = 3.0f;
    transit.doors = MapStyle::Doors::Bars;
    transit.floorPlates = false;
    transit.playerMarker = MapStyle::PlayerMarker::Flag;
    transit.environmentLines = true;
    transit.interchangeExits = 5;
    transit.labelHalos = true;
    styles.append(transit);

    MapStyle hologram;
    hologram.id = qsl("hologram");
    hologram.palette = MapStyle::Palette::Light;
    // Blocks tall enough that their sides show as a box of light, rather than as one line under the top
    hologram.roomFlattening = 3.0f;
    hologram.edges = MapStyle::Edges::GlowingWireframes;
    hologram.lit = false;
    hologram.background = QColor(3, 9, 18);
    hologram.ink = QColor(70, 225, 255);
    hologram.gridColor = QColor(70, 200, 255, 26);
    hologram.labelFontFamily = qsl("Bitstream Vera Sans Mono");
    hologram.postEffects = MapStyle::PostEffect::Scanlines | MapStyle::PostEffect::Bloom;
    hologram.floorPlates = false;
    hologram.playerMarker = MapStyle::PlayerMarker::Pulse;
    hologram.labelTint = QColor(127, 227, 255, 160);
    hologram.scannerOverlay = true;
    styles.append(hologram);

    return styles;
}
} // namespace

QString MapStyle::displayName() const
{
    if (id == qsl("tiles")) {
        //: Name of the 3D map style that draws rooms as lit blocks in their environment colors, chosen in the Mapper preferences
        return QCoreApplication::translate("MapStyle", "Tiles");
    }
    if (id == qsl("blueprint")) {
        //: Name of the 3D map style that draws rooms as thin white outlines on a deep blue drafting sheet, chosen in the Mapper preferences
        return QCoreApplication::translate("MapStyle", "Blueprint");
    }
    if (id == qsl("terminal")) {
        //: Name of the 3D map style that draws rooms and exits as colored text characters on a black screen, as roguelike games do, chosen in the Mapper preferences
        return QCoreApplication::translate("MapStyle", "Terminal");
    }
    if (id == qsl("transit")) {
        //: Name of the 3D map style that draws rooms as stations and exits as colored lines, as a metro or subway map does, chosen in the Mapper preferences
        return QCoreApplication::translate("MapStyle", "Transit");
    }
    if (id == qsl("hologram")) {
        //: Name of the 3D map style that draws rooms and exits as glowing lines of light that every level shows through, as a science fiction hologram does, chosen in the Mapper preferences
        return QCoreApplication::translate("MapStyle", "Hologram");
    }
    return id;
}

namespace {
// Light that dim would barely show over the background
QColor liftedLight(const QColor& color)
{
    constexpr int dimmest = 150;
    if (color.lightness() < dimmest) {
        return QColor::fromHsl(color.hslHue(), color.hslSaturation(), dimmest, color.alpha());
    }
    return color;
}
} // namespace

QColor MapStyle::roomColor(const QColor& environmentColor) const
{
    switch (palette) {
    case Palette::Environment:
        return environmentColor;
    case Palette::Ink: {
        // Enough of the environment's color to tell areas of the plan apart, not so much that the ink stops standing out
        constexpr float tint = 0.18f;
        const QColor paper = background.isValid() ? background : QColor(Qt::black);
        return QColor::fromRgbF(paper.redF() + (environmentColor.redF() - paper.redF()) * tint,
                                paper.greenF() + (environmentColor.greenF() - paper.greenF()) * tint,
                                paper.blueF() + (environmentColor.blueF() - paper.blueF()) * tint,
                                environmentColor.alphaF());
    }
    case Palette::Terminal:
        // No darker than dim text on a terminal, so that a room in black or navy does not vanish into the screen
        if (environmentColor.lightness() < 96) {
            return QColor::fromHsl(environmentColor.hslHue(), environmentColor.hslSaturation(), 96, environmentColor.alpha());
        }
        return environmentColor;
    case Palette::Stations:
        return QColor(255, 255, 255, environmentColor.alpha());
    case Palette::Light: {
        // Saturated and in a narrow band of lightness, so that areas read apart by hue alone and none glows much
        // brighter than the rest
        constexpr int grayest = 40;
        constexpr int dimmest = 150;
        constexpr int brightest = 185;
        if (environmentColor.hslSaturation() < grayest || environmentColor.hslHue() < 0) {
            QColor light = ink.isValid() ? ink : QColor(Qt::cyan);
            light.setAlpha(environmentColor.alpha());
            return liftedLight(light);
        }
        return QColor::fromHsl(environmentColor.hslHue(), 255, std::clamp(environmentColor.lightness(), dimmest, brightest), environmentColor.alpha());
    }
    }
    return environmentColor;
}

QColor MapStyle::symbolColor(const QColor& roomSymbolColor, const QColor& drawnRoomColor) const
{
    // The automatic choice is made against the environment's color, which the ink palette barely fills a room with
    if (palette == Palette::Ink && ink.isValid() && !roomSymbolColor.isValid()) {
        return ink;
    }
    // A character is drawn in the room's color rather than on it, and lifted as a room's is, so that it shows on the dark screen;
    // light has no fill to draw a symbol on either
    if (palette == Palette::Terminal || palette == Palette::Light) {
        return roomSymbolColor.isValid() ? roomColor(roomSymbolColor) : drawnRoomColor;
    }
    return RoomAppearance::symbolColor(roomSymbolColor, drawnRoomColor);
}

QColor MapStyle::exitColor(const QColor& profileExitColor) const
{
    return ink.isValid() ? ink : profileExitColor;
}

QColor MapStyle::roomIdColor(const QColor& drawnRoomColor) const
{
    if (palette == Palette::Terminal) {
        return drawnRoomColor;
    }
    // One color for every room, as each color an ID is drawn in needs its own digits made
    if (palette == Palette::Light) {
        return ink.isValid() ? ink : QColor(Qt::cyan);
    }
    return palette == Palette::Ink && ink.isValid() ? ink : ExitAppearance::contrastColor(drawnRoomColor);
}

QColor MapStyle::wireColor(const QColor& roomBorderColor, const QColor& drawnRoomColor) const
{
    // Added onto the background, a dark or clear border would leave nothing of the room to see
    if (!roomBorderColor.isValid() || roomBorderColor.alpha() == 0) {
        return drawnRoomColor;
    }
    QColor color = liftedLight(roomBorderColor);
    color.setAlpha(drawnRoomColor.alpha());
    return color;
}

QColor MapStyle::playerRoomColor() const
{
    if (palette == Palette::Ink) {
        // Red would sink into the blue of a drafting sheet
        return QColor(255, 190, 60);
    }
    if (palette == Palette::Light) {
        // Apart from the ink's cyan and the greens and blues most areas are drawn in
        return QColor(255, 79, 216);
    }
    return QColor(Qt::red);
}

QColor MapStyle::ringColor(const QColor& environmentColor, const QColor& profileBackground) const
{
    return ExitAppearance::standOutColor(environmentColor, backgroundColor(profileBackground));
}

QColor MapStyle::backgroundColor(const QColor& profileBackground) const
{
    return background.isValid() ? background : profileBackground;
}

bool MapStyle::roundRooms(const bool profileRoundRooms) const
{
    switch (shape) {
    case Shape::Profile:
        return profileRoundRooms;
    case Shape::Round:
        return true;
    }
    return profileRoundRooms;
}

QFont MapStyle::labelFont(const QFont& mapSymbolFont) const
{
    QFont font = mapSymbolFont;
    if (!labelFontFamily.isEmpty()) {
        font.setFamily(labelFontFamily);
    }
    return font;
}

QString MapStyle::defaultId()
{
    return qsl("tiles");
}

const QList<MapStyle>& MapStyle::all()
{
    static const QList<MapStyle> styles = registeredStyles();
    return styles;
}

QStringList MapStyle::ids()
{
    QStringList result;
    for (const MapStyle& style : all()) {
        result.append(style.id);
    }
    return result;
}

const MapStyle* MapStyle::find(const QString& id)
{
    for (const MapStyle& style : all()) {
        if (style.id == id) {
            return &style;
        }
    }
    return nullptr;
}

const MapStyle& MapStyle::findOrDefault(const QString& id)
{
    if (const MapStyle* style = find(id)) {
        return *style;
    }
    return *find(defaultId());
}
