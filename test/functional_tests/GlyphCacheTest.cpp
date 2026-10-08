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

#include <QFontDatabase>
#include <QFontMetrics>
#include <QImage>
#include <QPainter>
#include <QPixmap>
#include <QScopeGuard>
#include <QtTest/QtTest>

#include "TGlyphCache.h"

#include "GroupedTest.h"

// TTextEdit paints undecorated graphemes through TGlyphCache instead of
// drawText(cell, Qt::AlignCenter), so whatever it draws has to be pixel for
// pixel what drawText() would have drawn - on scaled displays too, where the two
// differ unless the cache rounds its origin exactly as QTextLine::draw() does.
class GlyphCacheTest : public QObject
{
    Q_OBJECT

private:
    static constexpr int csmTextFlags = Qt::AlignCenter | Qt::TextDontClip | Qt::TextSingleLine;

    struct Cell
    {
        QString grapheme;
        int columns;
    };

    static QList<Cell> cells()
    {
        return {
                {qsl("W"), 1},
                {qsl("g"), 1},
                {qsl("_"), 1},
                {qsl("|"), 1},
                {qsl("\t"), 8},
                // Precomposed, then as a base letter and a combining accent
                {qsl("é"), 1},
                {qsl("é"), 1},
                {qsl("—"), 1},
                {qsl("→"), 1},
                {qsl("⚔"), 1},
                {qsl("█"), 1},
                {qsl("┼"), 1},
                {qsl("中"), 2},
                {qsl("␀"), 1},
                {qsl("🐉"), 2},
                {qsl("👍🏽"), 2},
        };
    }

    static QFont testFont(const int pointSize, const QFont::Weight weight = QFont::Normal, const QFont::StyleStrategy strategy = QFont::PreferDefault, const bool decorated = false)
    {
        QFont font = QFontDatabase::systemFont(QFontDatabase::FixedFont);
        font.setPointSize(pointSize);
        font.setWeight(weight);
        font.setStyleStrategy(strategy);
        font.setUnderline(decorated);
        font.setOverline(decorated);
        font.setStrikeOut(decorated);
        return font;
    }

    // The font TTextEdit drew an undecorated cell with before the cache, once
    // a paint had toggled bold both ways
    static QFont styled(QFont font, const TGlyphCache::Style style)
    {
        font.setUnderline(false);
        font.setOverline(false);
        font.setStrikeOut(false);
        if (style.testFlag(TGlyphCache::Bold)) {
            font.setBold(true);
        } else if (font.bold()) {
            font.setBold(false);
        }
        if (font.italic() != style.testFlag(TGlyphCache::Italic)) {
            font.setItalic(style.testFlag(TGlyphCache::Italic));
        }
        return font;
    }

    // Runs of two cells share a color, so a queue both grows and breaks on a change
    static QColor cellColor(const int index) { return (index / 2) % 2 ? QColor(90, 160, 230) : QColor(220, 200, 120); }

    // Draws every cell once into a fresh surface, starting at xOffset columns,
    // with drawText() or with the cache - which, when queued, is handed the
    // whole line before it draws any of it.
    static QImage render(const QFont& font, const qreal devicePixelRatio, const bool viaPixmap, const TGlyphCache::Style style, TGlyphCache* cache, const int xOffset = 0, const bool queued = false)
    {
        const QFontMetrics metrics(font);
        const int cellWidth = metrics.averageCharWidth();
        const int cellHeight = metrics.height();
        const QSize logicalSize(40 * cellWidth, 3 * cellHeight);

        QPixmap pixmap(logicalSize * devicePixelRatio);
        pixmap.setDevicePixelRatio(devicePixelRatio);
        pixmap.fill(Qt::black);
        QImage image(logicalSize, QImage::Format_ARGB32_Premultiplied);
        image.fill(Qt::black);

        QPainter painter;
        if (viaPixmap) {
            painter.begin(&pixmap);
        } else {
            painter.begin(&image);
        }
        painter.setFont(font);
        if (cache) {
            cache->setFont(painter.font(), *painter.device());
        } else {
            painter.setFont(styled(painter.font(), style));
        }
        int column = xOffset;
        int index = 0;
        for (const Cell& cell : cells()) {
            const QRect rect(column * cellWidth, cellHeight, cell.columns * cellWidth, cellHeight);
            const QColor color = cellColor(index++);
            if (queued) {
                cache->queueCentered(painter, rect, cell.grapheme, style, color);
            } else {
                painter.setPen(color);
                if (cache) {
                    cache->drawCentered(painter, rect, cell.grapheme, style);
                } else {
                    painter.drawText(rect, csmTextFlags, cell.grapheme);
                }
            }
            column += cell.columns;
        }
        if (queued) {
            cache->flush(painter);
        }
        painter.end();
        return viaPixmap ? pixmap.toImage() : image;
    }

    static int differingPixels(const QImage& a, const QImage& b)
    {
        if (a.size() != b.size()) {
            return -1;
        }
        int count = 0;
        for (int y = 0; y < a.height(); ++y) {
            for (int x = 0; x < a.width(); ++x) {
                count += a.pixel(x, y) != b.pixel(x, y);
            }
        }
        return count;
    }

    static bool hasInk(const QImage& image)
    {
        for (int y = 0; y < image.height(); ++y) {
            for (int x = 0; x < image.width(); ++x) {
                if (image.pixel(x, y) != qRgb(0, 0, 0)) {
                    return true;
                }
            }
        }
        return false;
    }

private slots:
    void matchesDrawText_data()
    {
        QTest::addColumn<int>("pointSize");
        QTest::addColumn<qreal>("devicePixelRatio");
        QTest::addColumn<bool>("viaPixmap");
        QTest::addColumn<int>("style");
        QTest::addColumn<int>("xOffset");
        QTest::addColumn<int>("weight");
        QTest::addColumn<int>("strategy");
        QTest::addColumn<bool>("decoratedFont");

        const QList<std::pair<const char*, TGlyphCache::Style>> styles = {
                {"plain", TGlyphCache::Plain},
                {"bold", TGlyphCache::Bold},
                {"italic", TGlyphCache::Italic},
                {"bold italic", TGlyphCache::Bold | TGlyphCache::Italic},
        };
        // 11pt is where the system monospace font on Linux shows whether the
        // origin is truncated to the 1/64 pixel grid, at 1.25x
        for (const int pointSize : {9, 10, 11, 13}) {
            for (const qreal ratio : {1.0, 1.25, 1.5, 2.0, 3.0}) {
                for (const auto& [styleName, style] : styles) {
                    QTest::addRow("%dpt, pixmap at %.2fx, %s", pointSize, ratio, styleName)
                            << pointSize << ratio << true << int(style.toInt()) << 0 << int(QFont::Normal) << int(QFont::PreferDefault) << false;
                }
            }
            QTest::addRow("%dpt, image", pointSize) << pointSize << 1.0 << false << 0 << 0 << int(QFont::Normal) << int(QFont::PreferDefault) << false;
            // A horizontally scrolled console starts its cells left of zero
            QTest::addRow("%dpt, pixmap at 1.50x, scrolled", pointSize) << pointSize << 1.5 << true << 0 << -3 << int(QFont::Normal) << int(QFont::PreferDefault) << false;
        }
        for (const auto& [styleName, style] : styles) {
            // A display font picked by style name, such as "Fira Code Light" or
            // "Fira Code SemiBold": Light stays Light, SemiBold's plain text is Normal
            QTest::addRow("demibold font, %s", styleName) << 10 << 1.0 << true << int(style.toInt()) << 0 << int(QFont::DemiBold) << int(QFont::PreferDefault) << false;
            QTest::addRow("light font, %s", styleName) << 10 << 1.0 << true << int(style.toInt()) << 0 << int(QFont::Light) << int(QFont::PreferDefault) << false;
            // Decorations saved on the display font itself were never drawn on
            // undecorated cells, as those forced them off
            QTest::addRow("decorated font, %s", styleName) << 10 << 1.0 << true << int(style.toInt()) << 0 << int(QFont::Normal) << int(QFont::PreferDefault) << true;
            // What every console other than the main one draws with
            for (const qreal ratio : {1.0, 1.5}) {
                QTest::addRow("unantialiased at %.2fx, %s", ratio, styleName)
                        << 10 << ratio << true << int(style.toInt()) << 0 << int(QFont::Normal) << int(QFont::NoAntialias | QFont::PreferQuality) << false;
            }
        }
    }

    void matchesDrawText()
    {
        QFETCH(int, pointSize);
        QFETCH(qreal, devicePixelRatio);
        QFETCH(bool, viaPixmap);
        QFETCH(int, style);
        QFETCH(int, xOffset);
        QFETCH(int, weight);
        QFETCH(int, strategy);
        QFETCH(bool, decoratedFont);

        const QFont font = testFont(pointSize, QFont::Weight(weight), QFont::StyleStrategy(strategy), decoratedFont);
        const auto cacheStyle = TGlyphCache::Style::fromInt(style);
        const QImage expected = render(font, devicePixelRatio, viaPixmap, cacheStyle, nullptr, xOffset);
        QVERIFY2(hasInk(expected), "drawText() drew nothing, so there is nothing to compare against");

        TGlyphCache cache;
        QCOMPARE(differingPixels(render(font, devicePixelRatio, viaPixmap, cacheStyle, &cache, xOffset), expected), 0);
        // A second pass is served entirely from the cache
        const qsizetype shaped = cache.size();
        QCOMPARE(differingPixels(render(font, devicePixelRatio, viaPixmap, cacheStyle, &cache, xOffset), expected), 0);
        QCOMPARE(cache.size(), shaped);
    }

    void batchesMatchDrawText_data()
    {
        QTest::addColumn<int>("pointSize");
        QTest::addColumn<qreal>("devicePixelRatio");
        QTest::addColumn<int>("style");

        for (const int pointSize : {10, 11}) {
            for (const qreal ratio : {1.0, 1.25, 2.0}) {
                QTest::addRow("%dpt at %.2fx, plain", pointSize, ratio) << pointSize << ratio << int(TGlyphCache::Plain);
                QTest::addRow("%dpt at %.2fx, bold italic", pointSize, ratio) << pointSize << ratio << int((TGlyphCache::Bold | TGlyphCache::Italic).toInt());
            }
        }
        // Past 64 device pixels Qt fills a call's glyphs as a single path rather
        // than blitting them from its glyph cache
        QTest::addRow("40pt at 2.00x, plain") << 40 << 2.0 << int(TGlyphCache::Plain);
        QTest::addRow("40pt at 2.00x, bold italic") << 40 << 2.0 << int((TGlyphCache::Bold | TGlyphCache::Italic).toInt());
    }

    // A line drawn as one queue - across color changes and the fallback fonts
    // the emoji and CJK cells need - comes out as it does cell by cell.
    void batchesMatchDrawText()
    {
        QFETCH(int, pointSize);
        QFETCH(qreal, devicePixelRatio);
        QFETCH(int, style);

        const QFont font = testFont(pointSize);
        const auto cacheStyle = TGlyphCache::Style::fromInt(style);
        const QImage expected = render(font, devicePixelRatio, true, cacheStyle, nullptr);
        TGlyphCache cache;
        QCOMPARE(differingPixels(render(font, devicePixelRatio, true, cacheStyle, &cache, 0, true), expected), 0);
    }

    // Glyphs are shaped in logical coordinates, so moving a window to a screen
    // with a different scale must not need them shaped again.
    void keepsGlyphsAcrossDevicePixelRatios()
    {
        const QFont font = testFont(10);
        TGlyphCache cache;
        render(font, 1.0, true, TGlyphCache::Plain, &cache);
        const qsizetype shaped = cache.size();
        QVERIFY(shaped > 0);

        QCOMPARE(differingPixels(render(font, 2.0, true, TGlyphCache::Plain, &cache), render(font, 2.0, true, TGlyphCache::Plain, nullptr)), 0);
        QCOMPARE(cache.size(), shaped);
    }

    void emptiesWhenTheFontChanges()
    {
        TGlyphCache cache;
        render(testFont(10), 1.0, true, TGlyphCache::Plain, &cache);
        QVERIFY(cache.size() > 0);

        QImage device(10, 10, QImage::Format_ARGB32_Premultiplied);
        QPainter painter(&device);
        painter.setFont(testFont(10));
        cache.setFont(painter.font(), *painter.device());
        QVERIFY2(cache.size() > 0, "setting the same font again should keep the cached glyphs");

        painter.setFont(testFont(14));
        cache.setFont(painter.font(), *painter.device());
        QCOMPARE(cache.size(), 0);
    }

    void emptiesWhenTheResolutionChanges()
    {
        TGlyphCache cache;
        render(testFont(10), 1.0, false, TGlyphCache::Plain, &cache);
        QVERIFY(cache.size() > 0);

        QImage highDpi(10, 10, QImage::Format_ARGB32_Premultiplied);
        highDpi.setDotsPerMeterX(highDpi.dotsPerMeterX() * 2);
        highDpi.setDotsPerMeterY(highDpi.dotsPerMeterY() * 2);
        QPainter painter(&highDpi);
        painter.setFont(testFont(10));
        cache.setFont(painter.font(), *painter.device());
        QCOMPARE(cache.size(), 0);
    }

    // Package fonts are installed and removed while consoles are showing text
    void reshapesWhenFontsAreInstalledOrRemoved()
    {
#ifndef INCLUDE_FONTS
        QSKIP("Built with WITH_FONTS=NO, so there is no bundled font to install");
#else
        const QString family = qsl("Ubuntu Mono");
        if (QFontDatabase::hasFamily(family)) {
            QSKIP("Ubuntu Mono is already installed here, so registering it would change nothing");
        }
        QFont font(family);
        font.setPointSize(13);
        TGlyphCache cache;
        const QImage withFallback = render(font, 1.0, false, TGlyphCache::Plain, &cache);
        QVERIFY(cache.size() > 0);

        const int id = QFontDatabase::addApplicationFont(qsl(":/fonts/ubuntu-font-family-0.83/UbuntuMono-R.ttf"));
        QVERIFY2(id != -1, "could not register the bundled Ubuntu Mono");
        // Other tests share this binary, so a failure below must not leave it installed
        auto unregister = qScopeGuard([id] {
            QFontDatabase::removeApplicationFont(id);
        });
        QCOMPARE(cache.size(), 0);
        const QImage withFont = render(font, 1.0, false, TGlyphCache::Plain, nullptr);
        QVERIFY2(withFont != withFallback, "registering the font did not change how drawText() draws, so this proves nothing");
        QCOMPARE(differingPixels(render(font, 1.0, false, TGlyphCache::Plain, &cache), withFont), 0);

        unregister.dismiss();
        QFontDatabase::removeApplicationFont(id);
        QCOMPARE(cache.size(), 0);
        QCOMPARE(differingPixels(render(font, 1.0, false, TGlyphCache::Plain, &cache), render(font, 1.0, false, TGlyphCache::Plain, nullptr)), 0);
#endif
    }

    void drawsOverlongGraphemesWithoutKeepingThem()
    {
        QString grapheme = qsl("a");
        while (grapheme.size() <= TGlyphCache::csmMaxCachedLength) {
            grapheme += QChar(0x0301);
        }
        const QFont font = testFont(13);
        const QFontMetrics metrics(font);
        const QRect cell(0, metrics.height(), metrics.averageCharWidth(), metrics.height());
        QImage expected(4 * metrics.averageCharWidth(), 3 * metrics.height(), QImage::Format_ARGB32_Premultiplied);
        expected.fill(Qt::black);
        QImage actual = expected;
        {
            QPainter painter(&expected);
            painter.setFont(font);
            painter.setPen(Qt::white);
            painter.drawText(cell, csmTextFlags, grapheme);
        }
        TGlyphCache cache;
        {
            QPainter painter(&actual);
            painter.setFont(font);
            painter.setPen(Qt::white);
            cache.setFont(painter.font(), *painter.device());
            cache.drawCentered(painter, cell, grapheme, TGlyphCache::Plain);
        }
        QVERIFY(hasInk(expected));
        QCOMPARE(differingPixels(actual, expected), 0);
        QCOMPARE(cache.size(), 0);
    }

    void staysBounded()
    {
        QImage device(64, 64, QImage::Format_ARGB32_Premultiplied);
        QPainter painter(&device);
        painter.setFont(testFont(10));
        TGlyphCache cache;
        cache.setFont(painter.font(), *painter.device());
        for (char32_t codepoint = 0x4E00; codepoint < 0x4E00 + TGlyphCache::csmMaxEntries + 100; ++codepoint) {
            cache.drawCentered(painter, QRect(0, 0, 32, 32), QString::fromUcs4(&codepoint, 1), TGlyphCache::Plain);
            QVERIFY(cache.size() <= TGlyphCache::csmMaxEntries);
        }
    }
};

#include "GlyphCacheTest.moc"
MUDLET_GROUPED_TEST_MAIN(GlyphCacheTest)
