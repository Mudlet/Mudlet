/***************************************************************************
 *   Copyright (C) 2026 by Vadim Peretokin - vadim.peretokin@mudlet.org    *
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

#include "TGlyphCache.h"

#include <QPaintDevice>
#include <QPainter>
#include <QRect>
#include <QTextLayout>

void TGlyphCache::setFont(const QFont& font, const QPaintDevice* device)
{
    const int dpiX = device->logicalDpiX();
    const int dpiY = device->logicalDpiY();
    if (font == mFont && dpiX == mDpiX && dpiY == mDpiY) {
        return;
    }
    mEntries.clear();
    mFont = font;
    mDpiX = dpiX;
    mDpiY = dpiY;
}

void TGlyphCache::drawCentered(QPainter& painter, const QRect& cell, QStringView grapheme, const Style style)
{
    if (grapheme.isEmpty()) {
        return;
    }
    const Entry& entry = lookup(grapheme, style);
    // The same centring qt_format_text() applies for Qt::AlignCenter, truncated
    // to the 1/64 pixel grid as QTextLine::draw() does before the painter's
    // scale is applied - otherwise glyphs land a device pixel away from where
    // drawText() puts them on scaled displays.
    const auto toFixedGrid = [](const qreal value) {
        return static_cast<int>(value * 64) / 64.0;
    };
    const QPointF origin(toFixedGrid(cell.x() + (cell.width() - entry.advance) / 2), toFixedGrid(cell.y() + (cell.height() - entry.height) / 2));
    for (const QGlyphRun& run : entry.runs) {
        painter.drawGlyphRun(origin, run);
    }
}

const TGlyphCache::Entry& TGlyphCache::lookup(QStringView grapheme, const Style style)
{
    // Borrowing the caller's characters keeps a cache hit free of allocations.
    const Key probe{QString::fromRawData(grapheme.data(), grapheme.size()), style};
    if (const auto it = mEntries.constFind(probe); it != mEntries.cend()) {
        return *it;
    }
    if (mEntries.size() >= csmMaxEntries) {
        mEntries.clear();
    }
    return *mEntries.insert(Key{grapheme.toString(), style}, shape(grapheme, style));
}

TGlyphCache::Entry TGlyphCache::shape(QStringView grapheme, const Style style) const
{
    QString text = grapheme.toString();
    // As drawText() does for single line text.
    for (QChar& ch : text) {
        if (ch == QChar::Tabulation || ch == QChar::CarriageReturn || ch == QChar::LineFeed) {
            ch = QChar::Space;
        }
    }

    QFont font = mFont;
    font.setBold(style.testFlag(Bold));
    font.setItalic(style.testFlag(Italic));

    QTextLayout layout(text, font);
    QTextOption option;
    option.setFlags(QTextOption::IncludeTrailingSpaces);
    layout.setTextOption(option);
    layout.beginLayout();
    QTextLine line = layout.createLine();
    layout.endLayout();

    Entry entry;
    if (!line.isValid()) {
        return entry;
    }
    entry.runs = layout.glyphRuns();
    entry.advance = line.horizontalAdvance();
    // Not line.height(), which is rounded up and would lift glyphs by a device
    // pixel on scaled displays compared to where drawText() puts them.
    entry.height = line.ascent() + line.descent();
    return entry;
}
