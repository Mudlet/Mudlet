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

#include "EnvironmentKit.h"
#include "ExitAppearance.h"
#include "Host.h"
#include "RoomAppearance.h"
#include "utils.h"

#include <QCoreApplication>
#include <QFont>
#include <QGuiApplication>
#include <QIcon>
#include <QPainter>
#include <QPainterPath>
#include <QPixmap>
#include <QScreen>
#include <QtMath>

#include <algorithm>
#include <array>
#include <cmath>
#include <utility>

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
    tiles.roomNameColor = QColor(214, 226, 240);
    tiles.roomNameScale = 0.62f;
    tiles.uprightNames = true;
    tiles.nameHalos = true;
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
    blueprint.inkTint = 0.2f;
    blueprint.inkLightness = 0.7f;
    blueprint.inkOutlineWidth = 0.07f;
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
    blueprint.roomNameColor = QColor(232, 244, 255);
    blueprint.roomNameScale = 0.62f;
    blueprint.uprightNames = true;
    blueprint.nameHalos = true;
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
    // The ANSI palette's green, a dim tone of the names' bright green, so the lines between rooms sit back behind them
    terminal.exitInk = QColor(0, 170, 0);
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
    // The ANSI palette's bright green, as a green phosphor screen draws its text
    terminal.roomNameColor = QColor(85, 255, 85);
    terminal.roomNameScale = 0.7f;
    terminal.uprightNames = true;
    terminal.nameHalos = true;
    terminal.compassFace = QColor(0, 0, 0, 220);
    terminal.compassRim = QColor(170, 170, 170);
    terminal.compassNeedle = QColor(255, 85, 85);
    terminal.infoBackground = QColor(24, 24, 24, 220);
    terminal.infoTextColor = QColor(170, 170, 170);
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
    transit.playerMarker = MapStyle::PlayerMarker::Pill;
    transit.environmentLines = true;
    transit.mixedLineColor = QColor(178, 178, 178);
    transit.inkRimsOnLines = true;
    transit.stationRing = 0.13f;
    transit.interchangeExits = 5;
    transit.labelHalos = true;
    transit.roomNameColor = QColor(35, 35, 40);
    transit.roomNameScale = 0.7f;
    transit.uprightNames = true;
    transit.levelPlatforms = true;
    transit.lineLegend = true;
    transit.highlightShape = MapStyle::HighlightShape::Halo;
    transit.highlightColor = QColor(250, 196, 40);
    transit.highlightScale = 1.1f;
    transit.themedMapLabels = true;
    transit.mapLabelColor = QColor(35, 35, 40);
    transit.mapLabelMaxHeight = 0.8f;
    transit.compassFace = QColor(255, 255, 255, 220);
    transit.compassRim = QColor(35, 35, 40);
    transit.compassNeedle = QColor(214, 48, 39);
    transit.infoBackground = QColor(255, 255, 255, 220);
    transit.infoTextColor = QColor(35, 35, 40);
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
    hologram.labelTint = QColor(150, 235, 255, 230);
    hologram.scannerOverlay = true;
    // The other levels as faint outlines, cyan above and blue below, rather than every room echoed in its own hue
    // around the viewed level's
    hologram.levelsAboveColor = QColor(35, 120, 130);
    hologram.levelsBelowColor = QColor(25, 60, 125);
    hologram.highlightShape = MapStyle::HighlightShape::Rings;
    hologram.highlightColor = QColor(255, 214, 90);
    hologram.labelLetterSpacing = 108.0;
    hologram.labelCapitals = true;
    hologram.roomNameGap = 1.0;
    hologram.themedMapLabels = true;
    hologram.mapLabelColor = QColor(Qt::white);
    hologram.mapLabelHalo = QColor(3, 9, 18, 200);
    hologram.mapLabelMaxHeight = 0.6f;
    hologram.roomNameColor = QColor(Qt::white);
    hologram.roomNameScale = 0.75f;
    hologram.uprightNames = true;
    hologram.nameHalos = true;
    hologram.softNameHalos = true;
    hologram.compassFace = QColor(3, 9, 18, 200);
    hologram.compassRim = QColor(70, 225, 255);
    hologram.compassNeedle = QColor(255, 79, 216);
    hologram.infoBackground = QColor(6, 22, 38, 200);
    hologram.infoTextColor = QColor(127, 227, 255);
    styles.append(hologram);

    MapStyle inkwash;
    inkwash.id = qsl("inkwash");
    inkwash.palette = MapStyle::Palette::Wash;
    // Flat, as brushed onto the paper rather than standing on it
    inkwash.roomFlattening = 32.0f;
    inkwash.edges = MapStyle::Edges::InkLozenges;
    inkwash.lit = false;
    inkwash.background = QColor(241, 234, 218);
    inkwash.ink = QColor(38, 34, 32);
    inkwash.labelFontFamily = qsl("Bitstream Vera Serif");
    // Fine, as a pen line on a sketch, yet wide enough for a stroke to show it thinning
    inkwash.exitWidthFactor = 1.75f;
    inkwash.postEffects = MapStyle::PostEffect::PaperGrain | MapStyle::PostEffect::Vignette;
    inkwash.floorPlates = false;
    inkwash.playerMarker = MapStyle::PlayerMarker::Seal;
    inkwash.playerNote = true;
    inkwash.washReach = 2.6f;
    inkwash.environmentMarks = true;
    inkwash.highlightShape = MapStyle::HighlightShape::DoubleEllipse;
    inkwash.highlightColor = QColor(176, 124, 46);
    inkwash.labelItalic = true;
    inkwash.themedMapLabels = true;
    inkwash.mapLabelColor = QColor(74, 62, 50);
    inkwash.mapLabelKnockout = QColor(244, 237, 220, 235);
    // For a label in a light color of its own, which the paper would lose: a rim of ink round its letters
    inkwash.mapLabelHalo = QColor(38, 34, 32, 200);
    inkwash.mapLabelMaxHeight = 0.7f;
    inkwash.roomNameColor = QColor(92, 80, 66);
    inkwash.roomNameScale = 0.85f;
    inkwash.nameHalos = true;
    inkwash.softNameHalos = true;
    inkwash.compass = MapStyle::Compass::Arrow;
    inkwash.compassRim = QColor(38, 34, 32);
    inkwash.scaleBar = true;
    inkwash.infoBackground = QColor(241, 234, 218, 220);
    inkwash.infoTextColor = QColor(38, 34, 32);
    styles.append(inkwash);

    MapStyle diorama;
    diorama.id = qsl("diorama");
    diorama.palette = MapStyle::Palette::Painted;
    // Chunky blocks, wider and much taller than tiles', that still leave room between them for the paths
    diorama.roomScale = 3.0f;
    diorama.roomFlattening = 2.0f;
    diorama.edges = MapStyle::Edges::ModelBlocks;
    // Between the sky's blue and its warm haze, for what fades into it
    diorama.background = QColor(222, 230, 228);
    // The paths' trodden earth, darker than the board they cross
    diorama.ink = QColor(168, 146, 110);
    diorama.exitWidthFactor = 2.0f;
    diorama.postEffects = MapStyle::PostEffect::Sky | MapStyle::PostEffect::TiltShift;
    // The board is the floor
    diorama.floorPlates = false;
    diorama.environmentKits = true;
    diorama.pavedPathStrength = 0.3f;
    diorama.playerMarker = MapStyle::PlayerMarker::Token;
    // A line on the board round the room, in the script's color, and in lamplight where it gave none: in the gap
    // between the room's piece and its neighbours', where a building does not hide it
    diorama.highlightShape = MapStyle::HighlightShape::Outline;
    diorama.highlightColor = QColor(255, 214, 79);
    diorama.highlightScale = 1.3f;
    // Map labels on white card tags, as a model's are
    diorama.labelFontFamily = qsl("Bitstream Vera Sans");
    diorama.themedMapLabels = true;
    diorama.mapLabelColor = QColor(38, 56, 92);
    diorama.mapLabelPlate = QColor(255, 255, 255, 235);
    diorama.mapLabelMaxHeight = 0.7f;
    diorama.symbolPlate = QColor(255, 255, 255, 235);
    diorama.roomNameColor = QColor(46, 60, 88);
    diorama.roomNameScale = 0.75f;
    diorama.roomNameBold = true;
    diorama.roomNameHalo = QColor(255, 255, 255, 235);
    diorama.compassFace = QColor(255, 255, 255, 220);
    diorama.compassRim = QColor(138, 118, 86);
    diorama.compassNeedle = QColor(214, 58, 48);
    diorama.infoBackground = QColor(255, 255, 255, 220);
    diorama.infoTextColor = QColor(38, 56, 92);
    styles.append(diorama);

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
    if (id == qsl("inkwash")) {
        //: Name of the 3D map style that draws rooms as blots and exits as brush strokes of ink on paper, as an East Asian ink wash painting does, chosen in the Mapper preferences
        return QCoreApplication::translate("MapStyle", "Ink wash");
    }
    if (id == qsl("diorama")) {
        //: Name of the 3D map style that draws rooms as painted blocks standing on a tabletop with a game token on the player's room, as a miniature model of the map does, chosen in the Mapper preferences
        return QCoreApplication::translate("MapStyle", "Diorama");
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
    case Palette::Stations:
        return QColor(255, 255, 255, environmentColor.alpha());
    case Palette::Soft: {
        const QColor hsl = environmentColor.toHsl();
        return QColor::fromHslF(std::max(hsl.hslHueF(), 0.0f), hsl.hslSaturationF() * 0.7f, 0.25f + hsl.lightnessF() * 0.6f, hsl.alphaF());
    }
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
    case Palette::Wash: {
        // However strong or dark the color, the room stays pale enough for the ink drawn on it to read
        const QColor paper = background.isValid() ? background : QColor(Qt::white);
        const QColor wash = washColor(environmentColor);
        constexpr float tint = 0.45f;
        const float strength = tint * wash.alphaF();
        return QColor::fromRgbF(paper.redF() + (wash.redF() - paper.redF()) * strength,
                                paper.greenF() + (wash.greenF() - paper.greenF()) * strength,
                                paper.blueF() + (wash.blueF() - paper.blueF()) * strength,
                                environmentColor.alphaF());
    }
    case Palette::Painted: {
        // A model paint is never as strong, as dark or as bright as a screen's color, and a warm light falls on it
        const float hue = environmentColor.hslHueF();
        const float saturation = hue < 0.0f ? 0.0f : environmentColor.hslSaturationF() * 0.7f;
        const float lightness = 0.28f + 0.5f * environmentColor.lightnessF();
        const QColor paint = QColor::fromHslF(qMax(hue, 0.0f), saturation, lightness);
        const QColor warm(222, 184, 140);
        constexpr float toWarm = 0.12f;
        return QColor::fromRgbF(paint.redF() + (warm.redF() - paint.redF()) * toWarm,
                                paint.greenF() + (warm.greenF() - paint.greenF()) * toWarm,
                                paint.blueF() + (warm.blueF() - paint.blueF()) * toWarm,
                                environmentColor.alphaF());
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
    return RoomAppearance::symbolColor(roomSymbolColor, symbolPlate.isValid() ? symbolPlate : drawnRoomColor);
}

bool MapStyle::readsOnPlate(const QColor& color, const QColor& plate)
{
    // Faint as it is, a color shows mostly what is under it rather than how it differs from the tag
    constexpr int faintest = 64;
    return color.alpha() >= faintest && qAbs(qGray(color.rgb()) - qGray(plate.rgb())) >= 96;
}

QColor MapStyle::roomNameHaloFor(const QColor& nameColor) const
{
    if (!roomNameHalo.isValid() || readsOnPlate(nameColor, roomNameHalo)) {
        return roomNameHalo;
    }
    // Judged as readsOnPlate judges, by gray, so that a yellow is rimmed in black
    QColor rim = qGray(nameColor.rgb()) > 127 ? QColor(Qt::black) : QColor(Qt::white);
    rim.setAlpha(roomNameHalo.alpha());
    return readsOnPlate(nameColor, rim) ? rim : QColor();
}

QColor MapStyle::over(const QColor& top, const QColor& under)
{
    const float alpha = top.alphaF();
    return QColor::fromRgbF(under.redF() + (top.redF() - under.redF()) * alpha, under.greenF() + (top.greenF() - under.greenF()) * alpha, under.blueF() + (top.blueF() - under.blueF()) * alpha);
}

qreal MapStyle::contrastRatio(const QColor& first, const QColor& second)
{
    const auto luminance = [](const QColor& color) {
        const auto linear = [](const qreal channel) {
            return channel <= 0.04045 ? channel / 12.92 : std::pow((channel + 0.055) / 1.055, 2.4);
        };
        return 0.2126 * linear(color.redF()) + 0.7152 * linear(color.greenF()) + 0.0722 * linear(color.blueF());
    };
    const qreal a = luminance(first);
    const qreal b = luminance(second);
    return (qMax(a, b) + 0.05) / (qMin(a, b) + 0.05);
}

namespace {
QColor tinted(const QColor& color, const QColor& tint)
{
    if (!tint.isValid()) {
        return color;
    }
    return QColor::fromRgbF(color.redF() * tint.redF(), color.greenF() * tint.greenF(), color.blueF() * tint.blueF(), color.alphaF() * tint.alphaF());
}

// What text is seen against: its rim or plate, tinted as the text is, over the background, or the background alone
QColor backingFor(const QColor& rim, const QColor& tint, const QColor& background)
{
    return rim.isValid() ? MapStyle::over(tinted(rim, tint), background) : background;
}
} // namespace

QColor MapStyle::roomNameBacking(const QColor& profileBackground) const
{
    const QColor background = backgroundColor(profileBackground);
    return backingFor(roomNameHaloFor(roomNameColorFor(QString(), background)), labelTint, background);
}

QColor MapStyle::shownRoomNameColor(const QColor& profileBackground) const
{
    const QColor background = backgroundColor(profileBackground);
    return over(tinted(roomNameColorFor(QString(), background), labelTint), roomNameBacking(profileBackground));
}

QColor MapStyle::mapLabelBacking(const QColor& profileBackground) const
{
    const QColor background = backgroundColor(profileBackground);
    if (mapLabelPlate.isValid() && readsOnPlate(mapLabelColor, mapLabelPlate)) {
        return backingFor(mapLabelPlate, labelTint, background);
    }
    if (mapLabelKnockout.isValid() && readsOnPlate(mapLabelColor, mapLabelKnockout)) {
        return backingFor(mapLabelKnockout, labelTint, background);
    }
    return backingFor(mapLabelHalo, labelTint, background);
}

QColor MapStyle::shownMapLabelColor(const QColor& profileBackground) const
{
    return over(tinted(mapLabelColor, labelTint), mapLabelBacking(profileBackground));
}

QColor MapStyle::exitColor(const QColor& profileExitColor) const
{
    if (exitInk.isValid()) {
        return exitInk;
    }
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

QColor MapStyle::otherLevelColorFor(const QColor& drawnColor, const bool above) const
{
    const QColor& levelColor = above ? levelsAboveColor : levelsBelowColor;
    if (!levelColor.isValid()) {
        return drawnColor;
    }
    QColor color = levelColor;
    color.setAlphaF(levelColor.alphaF() * drawnColor.alphaF());
    return color;
}

float MapStyle::otherLevelSaturation(const float levelSaturation, const bool above) const
{
    return (above ? levelsAboveColor : levelsBelowColor).isValid() ? 1.0f : levelSaturation;
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
    if (palette == Palette::Wash) {
        // A seal's paste, a muted vermilion, the one color on an ink painting
        return QColor(190, 66, 46);
    }
    if (palette == Palette::Painted) {
        // A game token's glossy red, brighter than any room's paint so that it stands out among them
        return QColor(214, 48, 40);
    }
    return QColor(Qt::red);
}

QColor MapStyle::ringColor(const QColor& environmentColor, const QColor& profileBackground) const
{
    return ExitAppearance::standOutColor(environmentColor, backgroundColor(profileBackground));
}

QColor MapStyle::washColor(const QColor& environmentColor) const
{
    const float hue = environmentColor.hslHueF();
    const float saturation = hue < 0.0f ? 0.0f : environmentColor.hslSaturationF();
    const QColor wash = QColor::fromHslF(qMax(hue, 0.0f), qMin(saturation, 0.55f), 0.58f);
    return QColor::fromRgbF(wash.redF(), wash.greenF(), wash.blueF(), 0.15f + 0.6f * saturation);
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
        const auto contrast = [this, &background](const QColor& color) {
            const QColor backing = backingFor(roomNameHaloFor(color), labelTint, background);
            return contrastRatio(over(tinted(color, labelTint), backing), backing);
        };
        // Kept to a background it was chosen for: on one the profile picked, a pale name could vanish
        if (contrast(roomNameColor) >= scmLeastTextContrast) {
            return roomNameColor;
        }
        // Measured, not judged by lightness as contrastColor() does, which puts white on a saturated yellow or green
        const QColor black(Qt::black);
        const QColor white(Qt::white);
        return contrast(black) >= contrast(white) ? black : white;
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

QColor MapStyle::highlightColorFor(const QColor& roomHighlightColor, const QColor& roomHighlightCenterColor) const
{
    constexpr int faintest = 64;
    if (roomHighlightColor.isValid() && roomHighlightColor.alpha() < faintest && roomHighlightCenterColor.isValid() && roomHighlightCenterColor.alpha() > roomHighlightColor.alpha()) {
        return highlightColorFor(roomHighlightCenterColor);
    }
    return highlightColorFor(roomHighlightColor);
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
    if (labelItalic) {
        font.setItalic(true);
    }
}

QFont MapStyle::roomIdFont(const QFont& mapSymbolFont) const
{
    QFont font = labelFont(mapSymbolFont);
    font.setLetterSpacing(mapSymbolFont.letterSpacingType(), mapSymbolFont.letterSpacing());
    font.setItalic(mapSymbolFont.italic());
    return font;
}

qreal MapStyle::roomNameSpan(const qreal nameWidth, const qreal spaceWidth) const
{
    return nameWidth + spaceWidth * roomNameGap;
}

namespace {
// Six rooms, three across and two down, and the exits between them: a forest, a road, water and a town of rose roofs,
// so that a style's palette shows how it tells areas apart
constexpr int scmPreviewColumns = 3;
constexpr int scmPreviewRows = 2;
constexpr std::array<QColor, scmPreviewColumns * scmPreviewRows> scmPreviewEnvironments{
        QColor(40, 160, 60), QColor(40, 160, 60), QColor(50, 90, 200), QColor(150, 150, 150), QColor(205, 85, 105), QColor(50, 90, 200)};
constexpr std::array<std::pair<int, int>, 6> scmPreviewExits{{{0, 1}, {1, 2}, {3, 4}, {4, 5}, {0, 3}, {2, 5}}};
constexpr int scmPreviewPlayerRoom = 1;
// The 2D map's defaults, which a profile keeps unless it was given its own
constexpr QColor scmPreviewProfileBackground(QColorConstants::Black);
constexpr QColor scmPreviewProfileExits(QColorConstants::LightGray);
constexpr QColor scmPreviewProfilePlayer(QColorConstants::Red);

// A room as a diorama builds it for its kit: water sunk into the board, paving, grass under a tree (none where the
// player stands, so as not to hide the pawn) or a plastered house under a roof
void drawPreviewKit(QPainter& painter, const EnvironmentKit::Kit kit, const QColor& fill, const QRectF& room, const bool player)
{
    const qreal side = room.width();
    painter.setPen(Qt::NoPen);
    switch (kit) {
    case EnvironmentKit::Kit::Water: {
        painter.setBrush(fill.darker(115));
        painter.drawRect(room.adjusted(-side * 0.1, -side * 0.1, side * 0.1, side * 0.1));
        painter.setPen(QPen(QColor(255, 255, 255, 170), std::max(0.6, side * 0.08), Qt::SolidLine, Qt::RoundCap));
        painter.drawLine(room.topLeft() + QPointF(side * 0.2, side * 0.35), room.topLeft() + QPointF(side * 0.7, side * 0.25));
        return;
    }
    case EnvironmentKit::Kit::Road:
        painter.setBrush(fill);
        painter.drawRect(room);
        return;
    case EnvironmentKit::Kit::Forest: {
        painter.setBrush(fill);
        painter.drawRect(room);
        if (player) {
            return;
        }
        const QPointF foot(room.center().x(), room.center().y() + side * 0.2);
        painter.setBrush(QColor(0, 0, 0, 50));
        painter.drawEllipse(foot + QPointF(side * 0.15, 0.0), side * 0.35, side * 0.12);
        painter.setBrush(MapStyle::scmTrunkColor);
        painter.drawRect(QRectF(foot.x() - side * 0.07, foot.y() - side * 0.2, side * 0.14, side * 0.2));
        const QColor crown = QColor::fromRgbF(fill.redF() * MapStyle::scmCrownDarkening, fill.greenF() * MapStyle::scmCrownDarkening, fill.blueF() * MapStyle::scmCrownDarkening);
        painter.setBrush(crown);
        painter.drawPolygon(QPolygonF{foot + QPointF(-side * 0.38, -side * 0.15), foot + QPointF(side * 0.38, -side * 0.15), foot + QPointF(0.0, -side * 1.15)});
        return;
    }
    case EnvironmentKit::Kit::Building: {
        const QColor walls = QColor::fromRgbF(fill.redF() + (MapStyle::scmPlaster.redF() - fill.redF()) * MapStyle::scmToPlaster,
                                              fill.greenF() + (MapStyle::scmPlaster.greenF() - fill.greenF()) * MapStyle::scmToPlaster,
                                              fill.blueF() + (MapStyle::scmPlaster.blueF() - fill.blueF()) * MapStyle::scmToPlaster);
        const QRectF wall(room.left(), room.top() + side * 0.25, side, side * 0.75);
        painter.setBrush(QColor(0, 0, 0, 50));
        painter.drawEllipse(QPointF(wall.center().x() + side * 0.2, wall.bottom()), side * 0.6, side * 0.15);
        painter.setBrush(walls);
        painter.drawRect(wall);
        painter.setBrush(walls.darker(115));
        painter.drawRect(QRectF(wall.center().x() - side * 0.12, wall.bottom() - side * 0.35, side * 0.24, side * 0.35));
        painter.setBrush(fill);
        const qreal eaves = side * 0.12;
        painter.drawPolygon(QPolygonF{QPointF(wall.left() - eaves, wall.top()), QPointF(wall.right() + eaves, wall.top()), QPointF(wall.center().x(), room.top() - side * 0.35)});
        return;
    }
    case EnvironmentKit::Kit::Plain:
        painter.setBrush(fill);
        painter.drawRect(room);
        return;
    }
}

QPainterPath previewRoomPath(const QRectF& room, const bool round)
{
    QPainterPath path;
    if (round) {
        path.addEllipse(room);
    } else {
        const qreal corner = room.width() * 0.12;
        path.addRoundedRect(room, corner, corner);
    }
    return path;
}
} // namespace

QPixmap MapStyle::previewPixmap(const QSize& size, const qreal devicePixelRatio) const
{
    // Nothing to draw on, and the grid's steps would come to nothing
    if (size.isEmpty() || devicePixelRatio <= 0.0) {
        return {};
    }
    QPixmap pixmap(QSize(qCeil(size.width() * devicePixelRatio), qCeil(size.height() * devicePixelRatio)));
    pixmap.setDevicePixelRatio(devicePixelRatio);
    pixmap.fill(Qt::transparent);

    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::TextAntialiasing);
    const QRectF bounds(0.0, 0.0, size.width(), size.height());
    const qreal corner = std::min(bounds.width(), bounds.height()) / 7.0;
    QPainterPath sheetPath;
    sheetPath.addRoundedRect(bounds.adjusted(0.5, 0.5, -0.5, -0.5), corner, corner);
    painter.setClipPath(sheetPath);

    const QColor sheet = backgroundColor(scmPreviewProfileBackground);
    if (postEffects.testFlag(PostEffect::Sky)) {
        QLinearGradient sky(bounds.topLeft(), bounds.bottomLeft());
        sky.setColorAt(0.0, scmSkyTop);
        sky.setColorAt(1.0, scmSkyFoot);
        painter.fillRect(bounds, sky);
    } else {
        painter.fillRect(bounds, sheet);
    }

    const qreal pitch = std::min(bounds.width() / scmPreviewColumns, bounds.height() / (scmPreviewRows + 0.4));
    const QPointF origin((bounds.width() - pitch * scmPreviewColumns) / 2.0, (bounds.height() - pitch * scmPreviewRows) / 2.0);
    // Bigger than the style draws them against their spacing, so that the smallest still reads, but in proportion
    const qreal side = pitch * std::clamp(2.0 / roomScale, 0.42, 0.62);
    auto centre = [&](const int room) {
        return origin + QPointF(pitch * (room % scmPreviewColumns + 0.5), pitch * (room / scmPreviewColumns + 0.5));
    };
    auto roomRect = [&](const int room) {
        const QPointF middle = centre(room);
        return QRectF(middle.x() - side / 2.0, middle.y() - side / 2.0, side, side);
    };

    if (environmentKits) {
        // The board the rooms stand on, seen from above and in front: narrower at the back, over its thick front edge
        const qreal back = origin.y() - pitch * 0.2;
        const qreal front = origin.y() + pitch * (scmPreviewRows + 0.1);
        const qreal inset = pitch * 0.3;
        painter.setPen(Qt::NoPen);
        painter.setBrush(scmBoardTop);
        painter.drawPolygon(QPolygonF{QPointF(inset, back), QPointF(bounds.width() - inset, back), QPointF(bounds.width(), front), QPointF(0.0, front)});
        painter.setBrush(scmBoardFront);
        painter.drawRect(QRectF(0.0, front, bounds.width(), pitch * 0.35));
    }

    if (gridColor.isValid()) {
        QColor line = gridColor;
        line.setAlpha(std::min(255, line.alpha() * 3));
        painter.setPen(QPen(line, 0.0));
        // From the line before the rooms, so the margins either side of them are ruled alike
        for (qreal x = std::fmod(origin.x(), pitch); x < bounds.width(); x += pitch) {
            painter.drawLine(QPointF(x, 0.0), QPointF(x, bounds.height()));
        }
        for (qreal y = std::fmod(origin.y(), pitch); y < bounds.height(); y += pitch) {
            painter.drawLine(QPointF(0.0, y), QPointF(bounds.width(), y));
        }
    }

    if (washReach > 0.0f) {
        // Each wash a few overlapping blots with soft edges, as paint spreads on wet paper, and stronger than the map
        // lays it down, so that it still shows at the size of an icon
        constexpr std::array<QPointF, 3> blots{QPointF(0.0, 0.0), QPointF(0.22, -0.16), QPointF(-0.18, 0.2)};
        painter.setPen(Qt::NoPen);
        for (int room = 0; room < static_cast<int>(scmPreviewEnvironments.size()); ++room) {
            QColor wash = washColor(scmPreviewEnvironments.at(room));
            wash.setAlphaF(std::min(1.0f, wash.alphaF() * 1.3f));
            for (int blot = 0; blot < static_cast<int>(blots.size()); ++blot) {
                const QPointF at = centre(room) + blots.at((blot + room) % blots.size()) * pitch;
                const qreal reach = pitch * (blot == 0 ? 0.55 : 0.4);
                QRadialGradient spread(at, reach);
                spread.setColorAt(0.0, wash);
                spread.setColorAt(0.55, wash);
                QColor edge = wash;
                edge.setAlpha(0);
                spread.setColorAt(1.0, edge);
                painter.setBrush(spread);
                painter.drawEllipse(at, reach, reach);
            }
        }
    }

    const bool terminalGlyphs = glyphs == Glyphs::Terminal;
    // Not std::clamp, whose bounds cross on a preview small enough that a quarter of its pitch is under a pixel
    const qreal exitWidth = std::min(std::max(pitch * 0.07 * exitWidthFactor, 0.75), pitch * 0.22);
    for (const auto& [from, to] : scmPreviewExits) {
        const QColor fromEnvironment = scmPreviewEnvironments.at(from);
        QColor line = exitColor(scmPreviewProfileExits);
        if (environmentLines) {
            if (fromEnvironment == scmPreviewEnvironments.at(to)) {
                line = ringColor(fromEnvironment, scmPreviewProfileBackground);
            } else if (mixedLineColor.isValid()) {
                line = mixedLineColor;
            }
        }
        if (edges == Edges::GlowingWireframes) {
            QColor glow = line;
            glow.setAlpha(70);
            painter.setPen(QPen(glow, exitWidth * 3.0, Qt::SolidLine, Qt::RoundCap));
            painter.drawLine(centre(from), centre(to));
        }
        QPen pen(line, exitWidth, terminalGlyphs ? Qt::DotLine : Qt::SolidLine, Qt::RoundCap);
        painter.setPen(pen);
        painter.drawLine(centre(from), centre(to));
    }

    const bool round = roundRooms(false);
    QFont glyphFont(labelFontFamily);
    glyphFont.setPixelSize(std::max(6, qRound(pitch * 0.8)));
    glyphFont.setBold(true);
    for (int room = 0; room < static_cast<int>(scmPreviewEnvironments.size()); ++room) {
        const QColor environment = scmPreviewEnvironments.at(room);
        const QColor fill = roomColor(environment);
        const QRectF rect = roomRect(room);
        const bool player = room == scmPreviewPlayerRoom;
        if (terminalGlyphs) {
            painter.setFont(glyphFont);
            painter.setPen(player ? scmPreviewProfilePlayer : fill);
            // A cell of the screen's black behind each character, so the dotted exits stop at it
            painter.fillRect(rect.adjusted(-side * 0.3, -side * 0.3, side * 0.3, side * 0.3), sheet);
            painter.drawText(rect.adjusted(-side, -side, side, side), Qt::AlignCenter, player ? qsl("@") : qsl("#"));
            continue;
        }
        const QPainterPath path = previewRoomPath(rect, round);
        switch (edges) {
        case Edges::ViewedLevelBase: {
            painter.setPen(Qt::NoPen);
            painter.setBrush(roomBorderColor(Host::scmDefaultRoomBorderColor, fill, sheet));
            painter.drawPath(previewRoomPath(rect.adjusted(-side * 0.12, -side * 0.12, side * 0.12, side * 0.2), round));
            if (shadedBlocks) {
                QLinearGradient shade(rect.topLeft(), rect.bottomRight());
                shade.setColorAt(0.0, fill.lighter(120));
                shade.setColorAt(1.0, fill.darker(120));
                painter.setBrush(shade);
            } else {
                painter.setBrush(fill);
            }
            painter.drawPath(path);
            break;
        }
        case Edges::InkOutlines:
            painter.setBrush(fill);
            painter.setPen(QPen(ink.isValid() ? ink : scmPreviewProfileExits, std::max(1.0, side * inkOutlineWidth)));
            painter.drawPath(path);
            break;
        case Edges::StationRings:
            painter.setBrush(fill);
            painter.setPen(QPen(ringColor(environment, scmPreviewProfileBackground), std::max(1.0, side * stationRing * 1.5)));
            painter.drawPath(path);
            break;
        case Edges::GlowingWireframes: {
            QColor haze = fill;
            haze.setAlpha(45);
            painter.setBrush(haze);
            QColor glow = fill;
            glow.setAlpha(80);
            painter.setPen(QPen(glow, 3.0));
            painter.drawPath(path);
            painter.setBrush(Qt::NoBrush);
            painter.setPen(QPen(fill, 1.0));
            painter.drawPath(path);
            break;
        }
        case Edges::InkLozenges: {
            // Drawn by hand: the wash shows through, inside two pen strokes that do not quite meet
            QColor tint = fill;
            tint.setAlpha(110);
            painter.setPen(Qt::NoPen);
            painter.setBrush(tint);
            painter.drawRect(rect);
            painter.setBrush(Qt::NoBrush);
            const QColor penColor = ink.isValid() ? ink : scmPreviewProfileExits;
            const qreal wobble = side * 0.07;
            for (const qreal turn : {1.0, -1.0}) {
                const QPolygonF stroke{rect.topLeft() + QPointF(-wobble, wobble * turn),
                                       rect.topRight() + QPointF(wobble * turn, -wobble),
                                       rect.bottomRight() + QPointF(wobble, -wobble * turn),
                                       rect.bottomLeft() + QPointF(-wobble * turn, wobble),
                                       rect.topLeft() + QPointF(wobble * turn, -wobble)};
                painter.setPen(QPen(penColor, turn > 0.0 ? 1.0 : 0.6, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
                painter.drawPolyline(stroke);
            }
            break;
        }
        case Edges::ModelBlocks: {
            if (environmentKits) {
                drawPreviewKit(painter, EnvironmentKit::guess(environment), fill, rect, player);
                break;
            }
            painter.setPen(Qt::NoPen);
            painter.setBrush(QColor(0, 0, 0, 50));
            painter.drawEllipse(rect.translated(side * 0.2, side * 0.35));
            painter.setBrush(fill.darker(140));
            painter.drawPath(previewRoomPath(rect.translated(0.0, side * 0.25), round));
            QLinearGradient light(rect.topLeft(), rect.bottomRight());
            light.setColorAt(0.0, fill.lighter(125));
            light.setColorAt(1.0, fill);
            painter.setBrush(light);
            painter.drawPath(path);
            break;
        }
        case Edges::ViewedLevelRim:
            painter.setBrush(fill);
            painter.setPen(QPen(Host::scmDefaultRoomBorderColor, 1.0));
            painter.drawPath(path);
            break;
        }
    }

    const QRectF playerRect = roomRect(scmPreviewPlayerRoom);
    const QPointF playerCentre = playerRect.center();
    const QColor playerTone = playerColor.isValid() ? playerColor : scmPreviewProfilePlayer;
    painter.setBrush(Qt::NoBrush);
    switch (playerMarker) {
    case PlayerMarker::Ring: {
        if (terminalGlyphs) {
            break;
        }
        const qreal radius = side * 0.85;
        if (playerRing == PlayerRing::Profile) {
            painter.setPen(QPen(playerTone, std::max(1.2, side * 0.2)));
            painter.drawEllipse(playerCentre, radius, radius);
            break;
        }
        painter.setPen(QPen(playerTone, 1.2));
        painter.drawEllipse(playerCentre, radius, radius);
        if (playerRing == PlayerRing::Crosshair) {
            const qreal reach = radius * 1.5;
            painter.drawLine(playerCentre - QPointF(reach, 0.0), playerCentre + QPointF(reach, 0.0));
            painter.drawLine(playerCentre - QPointF(0.0, reach), playerCentre + QPointF(0.0, reach));
        }
        break;
    }
    case PlayerMarker::Pill: {
        const qreal radius = side * 0.8;
        painter.setPen(QPen(ink.isValid() ? ink : scmPreviewProfileExits, 1.2));
        painter.setBrush(Qt::white);
        painter.drawEllipse(playerCentre, radius, radius);
        painter.setPen(Qt::NoPen);
        painter.setBrush(playerRoomColor());
        painter.drawEllipse(playerCentre, radius * 0.5, radius * 0.5);
        break;
    }
    case PlayerMarker::Pulse: {
        painter.setPen(QPen(playerRoomColor(), 1.0));
        QColor fill = playerRoomColor();
        fill.setAlpha(110);
        painter.setBrush(fill);
        painter.drawPath(previewRoomPath(playerRect, round));
        const qreal half = side * 0.3;
        const QPointF top = playerCentre - QPointF(0.0, side * 1.05);
        painter.setPen(Qt::NoPen);
        painter.setBrush(playerRoomColor());
        painter.drawPolygon(QPolygonF{top + QPointF(0.0, -half), top + QPointF(half, 0.0), top + QPointF(0.0, half), top + QPointF(-half, 0.0)});
        break;
    }
    case PlayerMarker::Seal: {
        painter.setPen(Qt::NoPen);
        painter.setBrush(playerRoomColor());
        painter.drawRect(playerRect);
        painter.setPen(QPen(sheet, std::max(0.8, side * 0.08)));
        painter.setBrush(Qt::NoBrush);
        painter.drawRect(playerRect.adjusted(side * 0.2, side * 0.2, -side * 0.2, -side * 0.2));
        break;
    }
    case PlayerMarker::Token: {
        // A pawn standing on the room: its foot, its body and a ball for its head
        const QColor paint = playerRoomColor();
        const qreal foot = playerRect.center().y() + side * 0.15;
        painter.setPen(QPen(paint.darker(150), 0.6));
        painter.setBrush(paint);
        painter.drawEllipse(QPointF(playerCentre.x(), foot), side * 0.42, side * 0.16);
        painter.drawPolygon(QPolygonF{QPointF(playerCentre.x() - side * 0.3, foot),
                                      QPointF(playerCentre.x() + side * 0.3, foot),
                                      QPointF(playerCentre.x() + side * 0.1, foot - side * 0.75),
                                      QPointF(playerCentre.x() - side * 0.1, foot - side * 0.75)});
        painter.drawEllipse(QPointF(playerCentre.x(), foot - side * 0.9), side * 0.24, side * 0.24);
        break;
    }
    }

    if (postEffects.testFlag(PostEffect::Scanlines)) {
        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor(0, 0, 0, 70));
        for (qreal y = 0.0; y < bounds.height(); y += 2.0) {
            painter.drawRect(QRectF(0.0, y, bounds.width(), 0.6));
        }
    }
    if (postEffects.testFlag(PostEffect::Vignette)) {
        QRadialGradient vignette(bounds.center(), std::max(bounds.width(), bounds.height()) * 0.75);
        vignette.setColorAt(0.6, QColor(0, 0, 0, 0));
        vignette.setColorAt(1.0, QColor(60, 40, 20, 70));
        painter.fillRect(bounds, vignette);
    }

    // A hairline round the sheet, so one as light as the menu behind it still shows its edge
    painter.setClipping(false);
    painter.setBrush(Qt::NoBrush);
    painter.setPen(QPen(QColor(128, 128, 128, 150), 0.0));
    painter.drawPath(sheetPath);
    return pixmap;
}

QIcon MapStyle::previewIcon(const QSize& size) const
{
    QList<qreal> ratios{1.0, 2.0};
    for (const QScreen* screen : QGuiApplication::screens()) {
        if (!ratios.contains(screen->devicePixelRatio())) {
            ratios.append(screen->devicePixelRatio());
        }
    }
    QIcon icon;
    for (const qreal ratio : ratios) {
        icon.addPixmap(previewPixmap(size, ratio));
    }
    return icon;
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
