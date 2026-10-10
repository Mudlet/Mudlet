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

/*
 * ExitAppearance is how the 2D map and the modern 3D view both draw doors,
 * one-way arrows and the up/down/in/out markers. None of it has a Lua entry
 * point: scripts set doors and stubs, but what is painted for them is only
 * visible as pixels.
 */

#include <QtTest/QtTest>

#include <QImage>
#include <QPainter>

#include "ExitAppearance.h"
#include "TMap.h"
#include "TRoom.h"

#include "GroupedTest.h"

namespace {
int pixelsOfColor(const QImage& image, const QColor& color)
{
    int count = 0;
    for (int y = 0; y < image.height(); ++y) {
        for (int x = 0; x < image.width(); ++x) {
            if (image.pixelColor(x, y) == color) {
                ++count;
            }
        }
    }
    return count;
}

QImage paintedMarkers(const TRoom& room, const QColor& roomColor)
{
    QImage image(128, 128, QImage::Format_ARGB32);
    image.fill(Qt::transparent);
    QPainter painter(&image);
    QPen pen;
    ExitAppearance::paintExitMarkers(painter, pen, room, 64.0f, 64.0f, 128.0, 128.0, roomColor, false);
    painter.end();
    return image;
}
} // namespace

class ExitAppearanceTest : public QObject
{
    Q_OBJECT

private slots:
    void test_doorColorsFollowTheirStatus()
    {
        QCOMPARE(ExitAppearance::doorColor(1), QColor(10, 155, 10));
        QCOMPARE(ExitAppearance::doorColor(2), QColor(155, 155, 10));
        QCOMPARE(ExitAppearance::doorColor(3), QColor(155, 10, 10));
    }

    void test_contrastIsBlackOnLightAndWhiteOnDark()
    {
        QCOMPARE(ExitAppearance::contrastColor(QColor(240, 240, 200)), QColor(Qt::black));
        QCOMPARE(ExitAppearance::contrastColor(QColor(20, 10, 60)), QColor(Qt::white));
    }

    // Two strokes mirrored across the exit, near the room's end of it
    void test_doorStrokesStraddleTheExitNearTheRoom()
    {
        const auto [strokeA, strokeB] = ExitAppearance::doorStrokes(QLineF(0.0, 0.0, 0.5, 0.0), 2.0);
        for (const QPointF& point : {strokeA.p1(), strokeA.p2(), strokeB.p1(), strokeB.p2()}) {
            QVERIFY2(point.x() > 0.0 && point.x() < 0.5, qPrintable(qsl("a stroke reaches x %1").arg(point.x())));
        }
        QVERIFY(qAbs(strokeA.p1().y()) > 0.01);
        QCOMPARE(strokeA.p1().y(), -strokeB.p1().y());
        QCOMPARE(strokeA.p2().y(), -strokeB.p2().y());
        QCOMPARE(strokeA.p1().x(), strokeB.p1().x());
        QVERIFY2((strokeA.p1().y() > 0) == (strokeA.p2().y() > 0), "a stroke crosses the exit line");
    }

    void test_dashesAreEmptyForASolidLine()
    {
        QVERIFY(ExitAppearance::visibleDashes(Qt::SolidLine).isEmpty());
        QVERIFY(ExitAppearance::visibleDashes(Qt::NoPen).isEmpty());
        const QList<qreal> dots = ExitAppearance::visibleDashes(Qt::DotLine);
        QCOMPARE(dots.size(), 2);
        QVERIFY(dots.at(0) > 0.0 && dots.at(1) > 0.0);
        QCOMPARE(ExitAppearance::visibleDashes(Qt::DashDotLine).size(), 4);
    }

    void test_markerShowsTheDoorOnIt_data()
    {
        QTest::addColumn<int>("direction");
        QTest::addColumn<QString>("key");
        QTest::newRow("up") << static_cast<int>(DIR_UP) << qsl("up");
        QTest::newRow("down") << static_cast<int>(DIR_DOWN) << qsl("down");
        QTest::newRow("in") << static_cast<int>(DIR_IN) << qsl("in");
        QTest::newRow("out") << static_cast<int>(DIR_OUT) << qsl("out");
    }

    void test_markerShowsTheDoorOnIt()
    {
        QFETCH(int, direction);
        QFETCH(QString, key);
        TRoom room(nullptr);
        const QColor roomColor(200, 200, 200);
        QVERIFY2(pixelsOfColor(paintedMarkers(room, roomColor), Qt::black) == 0, "a marker for no exit at all");

        room.exitStubs.append(direction);
        const QImage plain = paintedMarkers(room, roomColor);
        QVERIFY2(pixelsOfColor(plain, Qt::black) > 0, "no marker in the contrasting color");
        QCOMPARE(pixelsOfColor(plain, ExitAppearance::doorColor(3)), 0);

        room.doors.insert(key, 3);
        QVERIFY2(pixelsOfColor(paintedMarkers(room, roomColor), ExitAppearance::doorColor(3)) > 0, "the marker does not show its locked door");
    }
};

#include "ExitAppearanceTest.moc"
MUDLET_GROUPED_TEST_MAIN(ExitAppearanceTest)
