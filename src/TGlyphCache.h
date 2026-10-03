#ifndef MUDLET_TGLYPHCACHE_H
#define MUDLET_TGLYPHCACHE_H

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

#include <QFlags>
#include <QColor>
#include <QFont>
#include <QGlyphRun>
#include <QHash>
#include <QList>
#include <QMetaObject>
#include <QPointF>
#include <QRawFont>
#include <QString>
#include <QStringView>

class QPainter;
class QPaintDevice;
class QRect;

// QPainter::drawText() lays out and shapes its text from scratch on every call,
// which for a console painted one grapheme per cell is most of the cost of a
// frame. A console usually shows a small set of distinct graphemes, so this
// shapes each one once per font style and replays the glyphs afterwards.
class TGlyphCache
{
public:
    // Qt's underline, overline and strike-out are not offered: drawGlyphRun()
    // draws them differently from drawText(), so decorated text is left to that.
    enum StyleFlag : quint8 { Plain = 0x00, Bold = 0x01, Italic = 0x02 };
    Q_DECLARE_FLAGS(Style, StyleFlag)

    TGlyphCache();
    ~TGlyphCache();
    Q_DISABLE_COPY_MOVE(TGlyphCache)

    // Empties the cache when the font, or the resolution it is drawn at,
    // differs from the one the cached glyphs were shaped for.
    void setFont(const QFont&, const QPaintDevice&);
    // Places the grapheme exactly where drawText(cell, Qt::AlignCenter |
    // Qt::TextDontClip | Qt::TextSingleLine, grapheme) would have put it,
    // shaped with the font given to the last setFont().
    void drawCentered(QPainter&, const QRect& cell, QStringView grapheme, Style);
    // As drawCentered() in `color`, but the glyphs only reach the painter at
    // the next flush(), so that a line costs one draw call per color rather
    // than one per cell. Anything else drawn with the painter has to wait for
    // that flush, or it lands beneath glyphs queued before it.
    void queueCentered(QPainter&, const QRect& cell, QStringView grapheme, Style, const QColor& color);
    // Draws the queued glyphs, leaving the painter's pen in their color.
    void flush(QPainter&);
    qsizetype size() const { return mEntries.size(); }
    // The display font as a cell in this style is drawn with, for text that
    // does not go through the cache, so that both paths agree on its weight.
    static QFont styled(QFont, Style);

    // Large enough for any real game's repertoire, small enough that a flood
    // of distinct CJK or emoji cannot grow it without bound.
    static constexpr qsizetype csmMaxEntries = 4096;
    // In UTF-16 code units. A grapheme can carry any number of combining marks,
    // so without this a flood of distinct long ones would keep megabytes each;
    // the longest real ones, such as a family emoji with skin tones, fit easily.
    static constexpr qsizetype csmMaxCachedLength = 32;

private:
    struct Key
    {
        QString text;
        Style style;
        bool operator==(const Key&) const = default;
    };
    friend size_t qHash(const Key& key, size_t seed) noexcept { return qHashMulti(seed, key.text, key.style.toInt()); }

    struct Run
    {
        QRawFont font;
        QList<quint32> glyphs;
        QList<QPointF> positions;
    };

    struct Entry
    {
        // More than one when part of the grapheme comes from a fallback font.
        QList<Run> runs;
        qreal advance = 0.0;
        qreal height = 0.0;
    };

    const Entry& lookup(QStringView grapheme, Style);
    Entry shape(QStringView grapheme, Style) const;

    QFont mFont;
    int mDpiX = 0;
    int mDpiY = 0;
    QHash<Key, Entry> mEntries;
    // Each cached glyph holds on to the font file it was shaped from, so a
    // font being installed or removed has to send every grapheme back to be
    // resolved again - as drawText() would have done on its next call. mFont
    // can stay, as Qt drops every font's resolved files when the database changes.
    QMetaObject::Connection mFontDatabaseConnection;

    QRawFont mQueuedFont;
    QColor mQueuedColor;
    QList<quint32> mQueuedGlyphs;
    QList<QPointF> mQueuedPositions;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(TGlyphCache::Style)

#endif // MUDLET_TGLYPHCACHE_H
