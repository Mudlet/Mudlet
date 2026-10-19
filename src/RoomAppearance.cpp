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

#include "RoomAppearance.h"

#include "ExitAppearance.h"
#include "Host.h"
#include "TMap.h"
#include "TRoom.h"
#include "TRoomDB.h"
#include "utils.h"

#include <QFontMetrics>
#include <QGlyphRun>
#include <QPainter>
#include <QStringList>
#include <QTextLayout>

#include <cmath>

namespace RoomAppearance {

Look look(const TMap& map, const Host& host, const TRoom& room, const int playerRoomId, const int viewedLevel)
{
    Look result;
    result.color = environmentColor(map, host, room.environment);
    result.symbol = room.mSymbol;
    result.symbolColor = room.mSymbolColor;
    result.highlighted = room.highlight;
    if (result.highlighted) {
        result.highlightColor = room.highlightColor;
        result.highlightCenterColor = room.highlightColor2;
        result.highlightRadius = room.highlightRadius;
    }
    result.playerRoom = room.getId() == playerRoomId;
    result.levelOffset = room.z() - viewedLevel;
    return result;
}

QString groundGlyph(const EnvironmentKit::Kit ground, const bool gridMode, const bool ascii)
{
    switch (ground) {
    case EnvironmentKit::Kit::Plain:
        return gridMode ? qsl(".") : qsl("\"");
    case EnvironmentKit::Kit::Water:
        return ascii ? qsl("~") : QString(QChar(0x2248));
    case EnvironmentKit::Kit::Road:
        return qsl("#");
    case EnvironmentKit::Kit::Forest:
        return ascii ? qsl("*") : QString(QChar(0x2663));
    case EnvironmentKit::Kit::Building:
        return ascii ? qsl("=") : QString(QChar(0x25A0));
    }
    return qsl("#");
}

bool fontShows(const QFont& font, const QString& text)
{
    QTextLayout layout(text, font);
    layout.beginLayout();
    layout.createLine();
    layout.endLayout();
    const QList<QGlyphRun> runs = layout.glyphRuns();
    if (runs.isEmpty()) {
        return false;
    }
    for (const QGlyphRun& run : runs) {
        // Glyph 0 is every font's "missing glyph" box
        if (run.glyphIndexes().contains(0)) {
            return false;
        }
    }
    return true;
}

QString terminalGlyph(const TRoom& room, const Look& look, const bool gridMode, const std::optional<EnvironmentKit::Kit> ground, const bool asciiGround)
{
    if (look.playerRoom) {
        return qsl("@");
    }
    if (!look.symbol.isEmpty()) {
        return look.symbol;
    }
    const bool up = room.getExit(DIR_UP) > 0 || room.exitStubs.contains(DIR_UP);
    const bool down = room.getExit(DIR_DOWN) > 0 || room.exitStubs.contains(DIR_DOWN);
    // X as Dwarf Fortress marks a staircase that goes both ways
    if (up && down) {
        return qsl("X");
    }
    if (up) {
        return qsl("<");
    }
    if (down) {
        return qsl(">");
    }
    if (ground) {
        return groundGlyph(*ground, gridMode, asciiGround);
    }
    return gridMode ? qsl(".") : qsl("#");
}

QChar terminalLineGlyph(const qreal degrees)
{
    // A line reads the same either way along it, so only its angle within a half turn counts
    qreal angle = std::fmod(degrees, 180.0);
    if (angle < 0.0) {
        angle += 180.0;
    }
    if (angle < 22.5 || angle >= 157.5 || (angle >= 67.5 && angle < 112.5)) {
        return terminalExitDot;
    }
    return angle < 67.5 ? QLatin1Char('/') : QLatin1Char('\\');
}

int exitCount(const TRoom& room)
{
    int count = 0;
    for (int direction = DIR_NORTH; direction <= DIR_OUT; ++direction) {
        if (room.getExit(direction) > 0) {
            ++count;
        }
    }
    for (const int destination : room.getSpecialExits()) {
        if (destination > 0) {
            ++count;
        }
    }
    return count;
}

const TRoom* drawnExit(const TMap& map, const TRoom& room, const int direction)
{
    const int destination = room.getExit(direction);
    if (destination <= 0 || room.customLines.contains(TRoom::dirCodeToShortString(direction))) {
        return nullptr;
    }
    const TRoom* pDestination = map.mpRoomDB->getRoom(destination);
    return pDestination && !pDestination->isHidden() ? pDestination : nullptr;
}

bool onEnvironmentLine(const TMap& map, const TRoom& room)
{
    if (room.isHidden()) {
        return false;
    }
    const auto sameLine = [&room](const TRoom* pOther) {
        return pOther && pOther->environment == room.environment && pOther->z() == room.z() && pOther->getArea() == room.getArea();
    };
    for (int direction = DIR_NORTH; direction <= DIR_SOUTHWEST; ++direction) {
        if (sameLine(drawnExit(map, room, direction))) {
            return true;
        }
    }
    // A one-way exit into the room draws a line to it as well
    const QMultiHash<int, int>& entrances = map.mpRoomDB->getEntranceHash();
    for (auto it = entrances.constFind(room.getId()); it != entrances.cend() && it.key() == room.getId(); ++it) {
        const TRoom* pFrom = map.mpRoomDB->getRoom(it.value());
        if (!pFrom || pFrom->isHidden() || !sameLine(pFrom)) {
            continue;
        }
        for (int direction = DIR_NORTH; direction <= DIR_SOUTHWEST; ++direction) {
            if (const TRoom* pTo = drawnExit(map, *pFrom, direction); pTo && pTo->getId() == room.getId()) {
                return true;
            }
        }
    }
    return false;
}

namespace {
// The color an environment is drawn in: an ANSI color's number, or a custom color's environment ID
std::optional<int> colorEnvironment(const TMap& map, int env)
{
    if (map.mEnvColors.contains(env)) {
        env = map.mEnvColors[env];
    } else if (!map.mCustomEnvColors.contains(env)) {
        return std::nullopt;
    }
    if ((env >= 1 && env < 256) || map.mCustomEnvColors.contains(env)) {
        return env;
    }
    return std::nullopt;
}
} // namespace

bool environmentDefined(const TMap& map, const int environmentId)
{
    return colorEnvironment(map, environmentId).has_value();
}

QColor environmentColor(const TMap& map, const Host& host, const int environmentId)
{
    const int env = colorEnvironment(map, environmentId).value_or(1);
    switch (env) {
    case 1:
        return host.mRed_2;
    case 2:
        return host.mGreen_2;
    case 3:
        return host.mYellow_2;
    case 4:
        return host.mBlue_2;
    case 5:
        return host.mMagenta_2;
    case 6:
        return host.mCyan_2;
    case 7:
        return host.mWhite_2;
    case 8:
        return host.mBlack_2;
    case 9:
        return host.mLightRed_2;
    case 10:
        return host.mLightGreen_2;
    case 11:
        return host.mLightYellow_2;
    case 12:
        return host.mLightBlue_2;
    case 13:
        return host.mLightMagenta_2;
    case 14:
        return host.mLightCyan_2;
    case 15:
        return host.mLightWhite_2;
    case 16:
        return host.mLightBlack_2;
    default:
        if (map.mCustomEnvColors.contains(env)) {
            return map.mCustomEnvColors[env];
        }
        if (env > 16 && env < 232) {
            quint8 const base = env - 16;
            quint8 r = base / 36;
            quint8 g = (base - (r * 36)) / 6;
            quint8 b = (base - (r * 36)) - (g * 6);
            r = r == 0 ? 0 : (r - 1) * 40 + 95;
            g = g == 0 ? 0 : (g - 1) * 40 + 95;
            b = b == 0 ? 0 : (b - 1) * 40 + 95;
            return QColor(r, g, b, 255);
        }
        if (env > 231 && env < 256) {
            quint8 const k = ((env - 232) * 10) + 8;
            return QColor(k, k, k, 255);
        }
        // mEnvColors can remap onto an id with no colour, as map files aren't validated. Fall back as
        // for an unmapped env: the background colour would make the room invisible.
        return host.mRed_2;
    }
}

QColor symbolColor(const QColor& roomSymbolColor, const QColor& roomColor)
{
    if (roomSymbolColor.isValid()) {
        return roomSymbolColor;
    }
    return roomColor.lightness() > 127 ? QColor(Qt::black) : QColor(Qt::white);
}

QGradientStops highlightStops(const QColor& highlightColor, const QColor& highlightCenterColor)
{
    return {{0.0, highlightCenterColor}, {0.85, highlightColor}};
}

QGradientStops playerRoomStops(const int style, const quint8 innerDiameterPercentage, const QColor& innerColor, const QColor& outerColor)
{
    const double factor = innerDiameterPercentage / 100.0;
    const bool solid = (innerDiameterPercentage == 0);

    switch (style) {
    case 1: // Simple(?) shaded red ring:
        if (solid) {
            return {{0.000, QColor(255, 0, 0, 255)}, {0.990, QColor(255, 0, 0, 255)}, {1.000, QColor(255, 0, 0, 0)}};
        }
        return {{0.000, QColor(255, 0, 0, 0)}, {factor * 0.980, QColor(255, 0, 0, 0)}, {factor * 1.020, QColor(255, 0, 0, 255)}, {0.980, QColor(255, 0, 0, 255)}, {1.000, QColor(255, 0, 0, 0)}};

    case 2: // Shaded bicolor (blue-yellow - so it ALWAYS contrasts with underlying room color) Ring:
        if (solid) {
            return {{0.000, QColor(255, 255, 0, 255)}, {0.990, QColor(0, 0, 255, 255)}, {1.000, QColor(0, 0, 255, 0)}};
        }
        return {{0.000, QColor(255, 255, 0, 0)}, {factor * 0.980, QColor(255, 255, 0, 0)}, {factor * 1.020, QColor(255, 255, 0, 255)}, {0.980, QColor(0, 0, 255, 255)}, {1.000, QColor(0, 0, 255, 0)}};

    case 3: { // User set ring:
        if (solid) {
            QColor transparentOuter(outerColor);
            transparentOuter.setAlpha(0);
            return {{0.000, innerColor}, {0.990, outerColor}, {1.000, transparentOuter}};
        }
        QColor transparentInner(innerColor);
        transparentInner.setAlpha(0);
        QColor transparentOuter(outerColor);
        transparentOuter.setAlpha(0);
        return {{0.000, transparentInner}, {factor * 0.980, transparentInner}, {factor * 1.020, innerColor}, {0.980, outerColor}, {1.000, transparentOuter}};
    }

    default: // Sort of emulates the original code:
        return {{0, Qt::white}, {0.7, QColor(255, 0, 0, 200)}, {0.799, QColor(150, 100, 100, 100)}, {0.80, QColor(150, 100, 100, 150)}, {0.95, QColor(255, 0, 0, 150)}};
    }
}

QColor playerBeaconColor(const int style, const QColor& outerColor)
{
    switch (style) {
    case 2:
        return QColor(255, 255, 0);
    case 3:
        return QColor(outerColor.red(), outerColor.green(), outerColor.blue());
    default:
        return QColor(255, 0, 0);
    }
}

QColor strongHighlightFillColor()
{
    return QColor(255, 0, 0, 150);
}

qreal playerRoomRadius(const quint8 outerDiameterPercentage, const qreal roomVisualSize)
{
    return (outerDiameterPercentage / 200.0) * roomVisualSize * M_SQRT2;
}

QPointF nameOffset(const QString& positionData)
{
    QPointF offset{0, 0};
    if (positionData.isEmpty()) {
        return offset;
    }
    const QStringList posXY = positionData.split(QLatin1Char(' '));
    bool ok1, ok2;
    double posX, posY;

    switch (posXY.count()) {
    case 1:
        // one value: treat as Y offset
        posY = posXY[0].toDouble(&ok1);
        if (ok1) {
            offset.setY(posY);
        }
        break;
    case 2:
        posX = posXY[0].toDouble(&ok1);
        posY = posXY[1].toDouble(&ok2);
        if (ok1 && ok2) {
            offset.setX(posX);
            offset.setY(posY);
        }
        break;
    }
    return offset;
}

QColor nameColor(const QString& colorData, const QColor& background)
{
    if (!colorData.isEmpty()) {
        const QColor userColor(colorData);
        if (userColor.isValid()) {
            return userColor;
        }
    }
    return ExitAppearance::contrastColor(background);
}

void paintSymbol(QPainter& painter, const QRect& rect, const QString& text, const QColor& color, const QFont& font, const qreal fontFudgeFactor, ushort& fontSize)
{
    static const unsigned int minimumUsableFontSize = 4;

    QString symbolString = text;
    painter.setPen(color);
    painter.setFont(font);
    painter.setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing | QPainter::SmoothPixmapTransform, true);

    const QFontMetrics mapSymbolFontMetrics = painter.fontMetrics();
    QVector<bool> isUsable;
    for (const quint32 codePoint : symbolString.toUcs4()) {
        isUsable.append(mapSymbolFontMetrics.inFontUcs4(codePoint));
    }

    QFont fontForThisSymbol = font;
    const bool needToFallback = isUsable.contains(false);
    // Oh dear at least one grapheme is not represented in either the selected
    // or any font as set elsewhere
    if (needToFallback) {
        symbolString = QString(QChar::ReplacementCharacter);
        // Clear the setting that may be forcing only the specified font to be
        // used, as it may not have the Replacement Character glyph...
        fontForThisSymbol.setStyleStrategy(static_cast<QFont::StyleStrategy>(font.styleStrategy() & ~(QFont::NoFontMerging)));
    }

    const qreal fudgeFactor = rect.width() * fontFudgeFactor;
    QRectF testRectangle(0, 0, fudgeFactor, fudgeFactor);
    testRectangle.moveCenter(rect.center());
    QRectF boundaryRect;
    // Try larger font sizes until it won't fit
    do {
        fontForThisSymbol.setPointSize(++fontSize);
        painter.setFont(fontForThisSymbol);
        boundaryRect = painter.boundingRect(rect, Qt::AlignCenter, symbolString);
        // Use a limit on fontSize otherwise some broken fonts can
        // lock the system into a very slow loop as it gets very large
    } while (testRectangle.contains(boundaryRect) && fontSize < 255);
    // Then try smaller ones until it will
    do {
        fontForThisSymbol.setPointSize(--fontSize);
        painter.setFont(fontForThisSymbol);
        boundaryRect = painter.boundingRect(rect, Qt::AlignCenter, symbolString);
    } while (!testRectangle.contains(boundaryRect) && fontSize > minimumUsableFontSize);

    if (testRectangle.contains(boundaryRect)) {
        fontForThisSymbol.setPointSize(++fontSize);
        painter.drawText(rect, Qt::AlignCenter | Qt::TextSingleLine, symbolString);
    }
    // Else, it still doesn't fit, must be a long string, too bad, so nothing
    // is drawn and nothing will be shown for it
}

} // namespace RoomAppearance
