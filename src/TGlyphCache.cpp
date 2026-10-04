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

#include <QGuiApplication>
#include <QPaintDevice>
#include <QPainter>
#include <QRect>
#include <QTextLayout>

namespace {
// At this size Qt stops blitting glyphs one by one from its glyph cache and
// fills all of a call's glyphs as one path (QPaintEngineEx::shouldDrawCachedGlyphs),
// which blends the edges where two glyphs meet once instead of twice.
bool drawnAsOnePath(const QPainter& painter, const QRawFont& font)
{
    constexpr qreal maxCachedGlyphSize = 64;
    const qreal pixelSize = font.pixelSize();
    return pixelSize * pixelSize * qAbs(painter.deviceTransform().determinant()) >= maxCachedGlyphSize * maxCachedGlyphSize;
}
} // namespace

TGlyphCache::TGlyphCache()
: mFontDatabaseConnection(QObject::connect(qGuiApp, &QGuiApplication::fontDatabaseChanged, [this]() {
    mEntries.clear();
    mQueuedFont = QRawFont();
}))
{
}

TGlyphCache::~TGlyphCache()
{
    QObject::disconnect(mFontDatabaseConnection);
}

void TGlyphCache::setFont(const QFont& font, const QPaintDevice& device)
{
    const int dpiX = device.logicalDpiX();
    const int dpiY = device.logicalDpiY();
    if (font == mFont && dpiX == mDpiX && dpiY == mDpiY) {
        return;
    }
    mEntries.clear();
    mFont = font;
    mDpiX = dpiX;
    mDpiY = dpiY;
}

QPointF TGlyphCache::origin(const QRect& cell, const Entry& entry)
{
    // The same centring qt_format_text() applies for Qt::AlignCenter, truncated
    // to the 1/64 pixel grid as QTextLine::draw() does before the painter's
    // scale is applied - otherwise glyphs land a device pixel away from where
    // drawText() puts them on scaled displays.
    const auto toFixedGrid = [](const qreal value) {
        return static_cast<int>(value * 64) / 64.0;
    };
    return QPointF(toFixedGrid(cell.x() + (cell.width() - entry.advance) / 2), toFixedGrid(cell.y() + (cell.height() - entry.height) / 2));
}

qreal TGlyphCache::drawCentered(QPainter& painter, const QRect& cell, QStringView grapheme, const Style style)
{
    const qreal bottom = queueCentered(painter, cell, grapheme, style, painter.pen().color());
    flush(painter);
    return bottom;
}

qreal TGlyphCache::queueCentered(QPainter& painter, const QRect& cell, QStringView grapheme, const Style style, const QColor& color)
{
    Q_ASSERT_X(mDpiX > 0, "TGlyphCache::queueCentered(...)", "setFont() has not been called, so there is no font to shape with");
    if (grapheme.isEmpty()) {
        return cell.y();
    }
    const bool cacheable = grapheme.size() <= csmMaxCachedLength;
    const Entry uncached = cacheable ? Entry() : shape(grapheme, style);
    const Entry& entry = cacheable ? lookup(grapheme, style) : uncached;
    const QPointF at = origin(cell, entry);
    for (const Run& run : entry.runs) {
        // drawGlyphRun() hands every glyph of a call to one font engine
        if (!mQueuedGlyphs.isEmpty() && (run.font != mQueuedFont || color != mQueuedColor)) {
            flush(painter);
        }
        if (mQueuedGlyphs.isEmpty()) {
            mQueuedFont = run.font;
            mQueuedColor = color;
        }
        mQueuedGlyphs.append(run.glyphs);
        // drawGlyphRun() adds its position argument to each of these, so with
        // a zero position they have to carry the origin themselves; the sum is
        // the one drawGlyphRun(origin, ...) would have worked out.
        for (const QPointF& position : run.positions) {
            mQueuedPositions.append(at + position);
        }
        if (drawnAsOnePath(painter, run.font)) {
            flush(painter);
        }
    }
    return at.y() + entry.inkBottom;
}

qreal TGlyphCache::inkBottom(const QRect& cell, QStringView grapheme, const Style style)
{
    if (grapheme.isEmpty()) {
        return cell.y();
    }
    const bool cacheable = grapheme.size() <= csmMaxCachedLength;
    const Entry uncached = cacheable ? Entry() : shape(grapheme, style);
    const Entry& entry = cacheable ? lookup(grapheme, style) : uncached;
    return origin(cell, entry).y() + entry.inkBottom;
}

void TGlyphCache::flush(QPainter& painter)
{
    if (mQueuedGlyphs.isEmpty()) {
        return;
    }
    if (painter.pen().color() != mQueuedColor) {
        painter.setPen(mQueuedColor);
    }
    // Raw data rather than setGlyphIndexes(), which would share the queue's
    // buffers and so make the clear() below allocate new ones.
    QGlyphRun run;
    run.setRawFont(mQueuedFont);
    run.setRawData(mQueuedGlyphs.constData(), mQueuedPositions.constData(), static_cast<int>(mQueuedGlyphs.size()));
    painter.drawGlyphRun(QPointF(), run);
    mQueuedGlyphs.clear();
    mQueuedPositions.clear();
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

QFont TGlyphCache::styled(QFont font, const Style style)
{
    // The weights drawText() ended up with once a line held both bold and
    // plain text: bold is always Bold, and plain is the display font's own
    // weight unless that counts as bold, as setBold(false) makes it Normal.
    if (style.testFlag(Bold)) {
        font.setBold(true);
    } else if (font.bold()) {
        font.setBold(false);
    }
    if (font.italic() != style.testFlag(Italic)) {
        font.setItalic(style.testFlag(Italic));
    }
    return font;
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

    QFont font = styled(mFont, style);
    // Decorations belong to the drawText() path, even ones set on the display
    // font itself, which the glyph runs would otherwise carry.
    font.setUnderline(false);
    font.setOverline(false);
    font.setStrikeOut(false);

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
    for (const QGlyphRun& glyphRun : layout.glyphRuns()) {
        entry.runs.append(Run{glyphRun.rawFont(), glyphRun.glyphIndexes(), glyphRun.positions()});
    }
    entry.advance = line.horizontalAdvance();
    // Not line.height(), which is rounded up and would lift glyphs above where
    // drawText() puts them.
    entry.height = line.ascent() + line.descent();
    for (const Run& run : std::as_const(entry.runs)) {
        for (qsizetype i = 0; i < run.glyphs.size() && i < run.positions.size(); ++i) {
            entry.inkBottom = std::max(entry.inkBottom, run.positions.at(i).y() + run.font.boundingRect(run.glyphs.at(i)).bottom());
        }
    }
    return entry;
}
