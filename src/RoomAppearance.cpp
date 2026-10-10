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

#include "Host.h"
#include "TMap.h"

#include <QFontMetrics>
#include <QPainter>

#include <cmath>

namespace RoomAppearance {

QColor environmentColor(const TMap& map, const Host& host, int env)
{
    if (map.mEnvColors.contains(env)) {
        env = map.mEnvColors[env];
    } else if (!map.mCustomEnvColors.contains(env)) {
        env = 1;
    }
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

qreal playerRoomRadius(const quint8 outerDiameterPercentage, const qreal roomVisualSize)
{
    return (outerDiameterPercentage / 200.0) * roomVisualSize * M_SQRT2;
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
