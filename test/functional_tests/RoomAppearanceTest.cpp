/***************************************************************************
 *   Copyright (C) 2026 by Mudlet Developers - mudlet@mudlet.org           *
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

/*
 * RoomAppearance is what the 2D map and the modern 3D view both draw a room
 * from. None of it has a Lua entry point: scripts set a room's symbol and
 * highlight, but what is painted for them is only visible as pixels.
 */

#include <QtTest/QtTest>

#include <QFont>
#include <QImage>
#include <QPainter>

#include "RoomAppearance.h"

#include "GroupedTest.h"

namespace {
// The bounding box of the pixels drawn on a transparent image
QRect paintedArea(const QImage& image)
{
    QRect area;
    for (int y = 0; y < image.height(); ++y) {
        for (int x = 0; x < image.width(); ++x) {
            if (qAlpha(image.pixel(x, y)) > 128) {
                area |= QRect(x, y, 1, 1);
            }
        }
    }
    return area;
}

QImage paintedSymbol(const QString& symbol, const int size, const QColor& color = Qt::black)
{
    QImage image(size, size, QImage::Format_ARGB32);
    image.fill(Qt::transparent);
    QPainter painter(&image);
    ushort fontSize = 1;
    RoomAppearance::paintSymbol(painter, image.rect(), symbol, color, QFont(qsl("Bitstream Vera Sans Mono"), 12), 1.0, fontSize);
    painter.end();
    return image;
}
} // namespace

class RoomAppearanceTest : public QObject
{
    Q_OBJECT

private slots:
    void test_symbolColorContrastsWithTheRoomUnlessSet()
    {
        QCOMPARE(RoomAppearance::symbolColor(QColor(), QColor(230, 210, 90)), QColor(Qt::black));
        QCOMPARE(RoomAppearance::symbolColor(QColor(), QColor(30, 40, 120)), QColor(Qt::white));
        QCOMPARE(RoomAppearance::symbolColor(QColor(255, 230, 0), QColor(230, 210, 90)), QColor(255, 230, 0));
    }

    void test_highlightFadesFromTheCenterColor()
    {
        const QGradientStops stops = RoomAppearance::highlightStops(QColor(Qt::red), QColor(Qt::yellow));
        QCOMPARE(stops.size(), 2);
        QCOMPARE(stops.first(), QGradientStop(0.0, QColor(Qt::yellow)));
        QCOMPARE(stops.last(), QGradientStop(0.85, QColor(Qt::red)));
    }

    void test_playerRoomStylesFollowTheirSettings()
    {
        const QColor inner(10, 20, 30);
        const QColor outer(200, 100, 50);

        const QGradientStops original = RoomAppearance::playerRoomStops(0, 70, inner, outer);
        QCOMPARE(original.first().second, QColor(Qt::white));
        QCOMPARE(original.last(), QGradientStop(0.95, QColor(255, 0, 0, 150)));

        const QGradientStops redRing = RoomAppearance::playerRoomStops(1, 50, inner, outer);
        QCOMPARE(redRing.first().second.alpha(), 0);
        QVERIFY(qFuzzyCompare(redRing.at(2).first, 0.51));
        QCOMPARE(redRing.at(2).second, QColor(255, 0, 0, 255));

        const QGradientStops solidCustom = RoomAppearance::playerRoomStops(3, 0, inner, outer);
        QCOMPARE(solidCustom.first().second, inner);
        QCOMPARE(solidCustom.at(1).second, outer);
        QCOMPARE(solidCustom.last().second.alpha(), 0);

        const QGradientStops ringCustom = RoomAppearance::playerRoomStops(3, 40, inner, outer);
        QCOMPARE(ringCustom.size(), 5);
        QCOMPARE(ringCustom.at(2).second, inner);
        QCOMPARE(ringCustom.at(3).second, outer);
    }

    void test_playerRoomRingReachesTheCornersAtFullSize()
    {
        QCOMPARE(RoomAppearance::playerRoomRadius(100, 2.0), M_SQRT2);
        QCOMPARE(RoomAppearance::playerRoomRadius(50, 2.0), M_SQRT2 / 2.0);
    }

    void test_symbolIsDrawnLargeAndCentered_data()
    {
        QTest::addColumn<QString>("symbol");
        QTest::addColumn<int>("size");
        QTest::newRow("letter on a 2D room") << qsl("$") << 40;
        QTest::newRow("letter on a 3D texture") << qsl("$") << 128;
        QTest::newRow("two letters") << qsl("AB") << 128;
        QTest::newRow("dingbat") << qsl("✉") << 128;
    }

    void test_symbolIsDrawnLargeAndCentered()
    {
        QFETCH(QString, symbol);
        QFETCH(int, size);
        const QImage image = paintedSymbol(symbol, size);
        const QRect area = paintedArea(image);
        QVERIFY2(!area.isEmpty(), "nothing was drawn");
        // Line spacing is part of what must fit, so a glyph never fills the whole rect
        QVERIFY2(qMax(area.width(), area.height()) >= size * 0.4, qPrintable(qsl("drawn %1x%2 on %3x%3").arg(area.width()).arg(area.height()).arg(size)));
        QVERIFY2(qAbs(area.center().x() - size / 2) <= size / 8, qPrintable(qsl("drawn off center, at x %1").arg(area.center().x())));
    }

    void test_symbolIsDrawnInItsColor()
    {
        const QImage image = paintedSymbol(qsl("W"), 64, QColor(0, 200, 0));
        const QRect area = paintedArea(image);
        QVERIFY(!area.isEmpty());
        bool foundOpaque = false;
        for (int y = area.top(); y <= area.bottom() && !foundOpaque; ++y) {
            for (int x = area.left(); x <= area.right(); ++x) {
                const QRgb pixel = image.pixel(x, y);
                if (qAlpha(pixel) == 255) {
                    QCOMPARE(QColor(pixel), QColor(0, 200, 0));
                    foundOpaque = true;
                    break;
                }
            }
        }
        QVERIFY(foundOpaque);
    }

    void test_symbolTooLongToFitDrawsNothing()
    {
        const QImage image = paintedSymbol(qsl("A very long room symbol"), 20);
        QVERIFY(paintedArea(image).isEmpty());
    }
};

#include "RoomAppearanceTest.moc"
MUDLET_GROUPED_TEST_MAIN(RoomAppearanceTest)
