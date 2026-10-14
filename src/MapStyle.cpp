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
#include "Host.h"
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
    tiles.palette = MapStyle::Palette::Soft;
    tiles.roomScale = 3.2f;
    tiles.edges = MapStyle::Edges::ViewedLevelBase;
    tiles.roomFlattening = 2.5f;
    tiles.defaultBackground = QColor(16, 22, 32);
    tiles.highlightShape = MapStyle::HighlightShape::Glow;
    tiles.highlightScale = 1.8f;
    tiles.playerRing = MapStyle::PlayerRing::Thin;
    tiles.playerColor = QColor(255, 208, 90);
    tiles.themedMapLabels = true;
    tiles.mapLabelColor = QColor(150, 190, 225);
    tiles.mapLabelHalo = QColor(16, 22, 32, 200);
    tiles.mapLabelMaxHeight = 0.9f;
    tiles.roomNameColor = QColor(185, 205, 225, 230);
    tiles.roomNameScale = 0.6f;
    tiles.compassFace = QColor(18, 26, 38, 210);
    tiles.compassRim = QColor(110, 140, 170);
    tiles.compassNeedle = QColor(235, 96, 80);
    tiles.infoBackground = QColor(18, 26, 38, 190);
    tiles.infoTextColor = QColor(215, 225, 235);
    tiles.shadedBlocks = true;
    tiles.dashedStairs = true;
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
    blueprint.inkTint = 0.3f;
    blueprint.inkLightness = 0.8f;
    blueprint.labelFontFamily = qsl("Bitstream Vera Sans Mono");
    blueprint.labelLetterSpacing = 112.0;
    blueprint.labelCapitals = true;
    blueprint.roomNameGap = 1.0;
    blueprint.exitWidthFactor = 0.5f;
    blueprint.doors = MapStyle::Doors::Swings;
    blueprint.floorPlates = false;
    blueprint.highlightShape = MapStyle::HighlightShape::DashedEllipse;
    blueprint.highlightScale = 1.3f;
    blueprint.highlightColor = QColor(225, 242, 255, 220);
    blueprint.playerRing = MapStyle::PlayerRing::Crosshair;
    blueprint.playerColor = QColor(255, 112, 90);
    blueprint.playerBeacon = false;
    blueprint.themedMapLabels = true;
    blueprint.mapLabelColor = QColor(225, 242, 255);
    blueprint.mapLabelHalo = QColor(18, 60, 110, 210);
    blueprint.mapLabelMaxHeight = 0.6f;
    blueprint.roomNameColor = QColor(210, 232, 252, 235);
    blueprint.roomNameScale = 0.6f;
    blueprint.compassFace = QColor(18, 60, 110, 220);
    blueprint.compassRim = QColor(225, 242, 255);
    blueprint.compassNeedle = QColor(255, 112, 90);
    blueprint.infoBackground = QColor(14, 48, 90, 210);
    blueprint.infoTextColor = QColor(225, 242, 255);
    blueprint.titleBlock = true;
    blueprint.levelCallouts = true;
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
    terminal.groundGlyphs = true;
    // A text screen's selection: the cell filled, its character in reverse video
    terminal.highlightShape = MapStyle::HighlightShape::Cell;
    terminal.highlightColor = QColor(255, 255, 85);
    terminal.statusLine = true;
    terminal.themedMapLabels = true;
    terminal.mapLabelColor = QColor(170, 170, 170);
    terminal.mapLabelHalo = QColor(0, 0, 0, 220);
    terminal.mapLabelMaxHeight = 0.6f;
    terminal.roomNameColor = QColor(170, 170, 170, 220);
    terminal.roomNameScale = 0.6f;
    terminal.compassFace = QColor(0, 0, 0, 220);
    terminal.compassRim = QColor(170, 170, 170);
    terminal.compassNeedle = QColor(255, 85, 85);
    terminal.infoBackground = QColor(24, 24, 24, 220);
    terminal.infoTextColor = QColor(170, 170, 170);
    styles.append(terminal);

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
    return id;
}

QColor MapStyle::roomColor(const QColor& environmentColor) const
{
    switch (palette) {
    case Palette::Environment:
        return environmentColor;
    case Palette::Ink: {
        const float tint = inkTint;
        const QColor paper = background.isValid() ? background : QColor(Qt::black);
        QColor wash = environmentColor.toHsl();
        if (wash.lightnessF() < inkLightness) {
            wash = QColor::fromHslF(std::max(wash.hslHueF(), 0.0f), wash.hslSaturationF(), inkLightness, wash.alphaF());
        }
        wash = wash.toRgb();
        return QColor::fromRgbF(paper.redF() + (wash.redF() - paper.redF()) * tint,
                                paper.greenF() + (wash.greenF() - paper.greenF()) * tint,
                                paper.blueF() + (wash.blueF() - paper.blueF()) * tint,
                                environmentColor.alphaF());
    }
    case Palette::Terminal:
        // No darker than dim text on a terminal, so that a room in black or navy does not vanish into the screen
        if (environmentColor.lightness() < 96) {
            return QColor::fromHsl(environmentColor.hslHue(), environmentColor.hslSaturation(), 96, environmentColor.alpha());
        }
        return environmentColor;
    case Palette::Soft: {
        const QColor hsl = environmentColor.toHsl();
        return QColor::fromHslF(std::max(hsl.hslHueF(), 0.0f), hsl.hslSaturationF() * 0.7f, 0.25f + hsl.lightnessF() * 0.6f, hsl.alphaF());
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
    // A character is drawn in the room's color rather than on it, and lifted as a room's is, so that it shows on the dark screen
    if (palette == Palette::Terminal) {
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
    return palette == Palette::Ink && ink.isValid() ? ink : ExitAppearance::contrastColor(drawnRoomColor);
}

QColor MapStyle::playerRoomColor() const
{
    if (palette == Palette::Ink) {
        // Red would sink into the blue of a drafting sheet
        return QColor(255, 190, 60);
    }
    return QColor(Qt::red);
}

QColor MapStyle::backgroundColor(const QColor& profileBackground) const
{
    if (background.isValid()) {
        return background;
    }
    if (defaultBackground.isValid() && profileBackground == QColor(Qt::black)) {
        return defaultBackground;
    }
    return profileBackground;
}

bool MapStyle::themesPlayerMarker(const int profilePlayerRoomStyle, const bool strongHighlight) const
{
    if (profilePlayerRoomStyle != 0 || strongHighlight) {
        return false;
    }
    return playerRing != PlayerRing::Profile || playerColor.isValid();
}

QColor MapStyle::infoBackgroundColor(const QColor& profileInfoBackground) const
{
    if (infoBackground.isValid() && profileInfoBackground == Host::scmDefaultMapInfoBg) {
        return infoBackground;
    }
    return profileInfoBackground;
}

QColor MapStyle::roomBorderColor(const QColor& profileBorder, const QColor& roomColor, const QColor& background) const
{
    if (edges != Edges::ViewedLevelBase || profileBorder != Host::scmDefaultRoomBorderColor) {
        return profileBorder;
    }
    return QColor::fromRgbF((roomColor.redF() + background.redF()) / 2.0f, (roomColor.greenF() + background.greenF()) / 2.0f, (roomColor.blueF() + background.blueF()) / 2.0f, roomColor.alphaF());
}

QColor MapStyle::roomNameColorFor(const QString& roomColorData, const QColor& background) const
{
    if (roomNameColor.isValid() && !QColor(roomColorData).isValid()) {
        return roomNameColor;
    }
    return RoomAppearance::nameColor(roomColorData, background);
}

QColor MapStyle::highlightColorFor(const QColor& roomHighlightColor) const
{
    if (!roomHighlightColor.isValid() && highlightColor.isValid()) {
        return highlightColor;
    }
    return roomHighlightColor;
}

bool MapStyle::roundRooms(const bool profileRoundRooms) const
{
    switch (shape) {
    case Shape::Profile:
        return profileRoundRooms;
    }
    return profileRoundRooms;
}

QFont MapStyle::labelFont(const QFont& mapSymbolFont) const
{
    QFont font = mapSymbolFont;
    if (!labelFontFamily.isEmpty()) {
        font.setFamily(labelFontFamily);
        letter(font);
    }
    return font;
}

void MapStyle::letter(QFont& font) const
{
    if (labelLetterSpacing != 100.0) {
        font.setLetterSpacing(QFont::PercentageSpacing, labelLetterSpacing);
    }
    if (labelCapitals) {
        font.setCapitalization(QFont::AllUppercase);
    }
}

QFont MapStyle::roomIdFont(const QFont& mapSymbolFont) const
{
    QFont font = labelFont(mapSymbolFont);
    font.setLetterSpacing(mapSymbolFont.letterSpacingType(), mapSymbolFont.letterSpacing());
    return font;
}

qreal MapStyle::roomNameSpan(const qreal nameWidth, const qreal spaceWidth) const
{
    return nameWidth + spaceWidth * roomNameGap;
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
