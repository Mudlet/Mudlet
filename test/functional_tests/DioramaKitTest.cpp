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
 * The diorama map style's kits. A spec reaches the guess only through the colors a
 * map can be given, and the layout of tall buildings not at all, as it has no Lua
 * entry point.
 */

#include <QtTest/QtTest>

#include "DioramaKit.h"

#include "GroupedTest.h"

using DioramaKit::Kit;

class DioramaKitTest : public QObject
{
    Q_OBJECT

    static constexpr float kRoomSize = 2.0f / 3.0f;
    static constexpr float kRoomHeight = kRoomSize / 2.0f;

private slots:
    void test_guessFollowsHue_data()
    {
        QTest::addColumn<QColor>("color");
        QTest::addColumn<Kit>("kit");
        QTest::newRow("blue") << QColor(60, 120, 200) << Kit::Water;
        QTest::newRow("cyan") << QColor(0, 180, 200) << Kit::Water;
        QTest::newRow("navy") << QColor(0, 0, 128) << Kit::Water;
        QTest::newRow("green") << QColor(90, 160, 70) << Kit::Forest;
        QTest::newRow("dark green") << QColor(0, 100, 0) << Kit::Forest;
        QTest::newRow("grey") << QColor(170, 165, 155) << Kit::Road;
        QTest::newRow("bluish grey") << QColor(120, 125, 135) << Kit::Road;
        QTest::newRow("tan") << QColor(196, 170, 128) << Kit::Road;
        QTest::newRow("brown") << QColor(120, 80, 40) << Kit::Road;
        QTest::newRow("red") << QColor(190, 70, 50) << Kit::Building;
        QTest::newRow("bright orange") << QColor(240, 150, 30) << Kit::Building;
        QTest::newRow("purple") << QColor(130, 110, 175) << Kit::Building;
        QTest::newRow("yellow") << QColor(230, 220, 40) << Kit::Building;
        QTest::newRow("near black") << QColor(15, 10, 30) << Kit::Road;
        QTest::newRow("black") << QColor(Qt::black) << Kit::Road;
        QTest::newRow("near white") << QColor(245, 245, 250) << Kit::Building;
        QTest::newRow("white") << QColor(Qt::white) << Kit::Building;
        QTest::newRow("no color") << QColor() << Kit::Plain;
    }

    void test_guessFollowsHue()
    {
        QFETCH(QColor, color);
        QFETCH(Kit, kit);
        QCOMPARE(DioramaKit::guess(color), kit);
    }

    void test_namesRoundTrip()
    {
        const QStringList names = DioramaKit::names();
        QCOMPARE(names, QStringList({qsl("plain"), qsl("water"), qsl("road"), qsl("forest"), qsl("building")}));
        for (const QString& name : names) {
            const std::optional<Kit> kit = DioramaKit::fromName(name);
            QVERIFY(kit);
            QCOMPARE(DioramaKit::name(*kit), name);
        }
        QVERIFY(!DioramaKit::fromName(qsl("castle")));
        QVERIFY(!DioramaKit::fromName(QString()));
    }

    void test_storedKitsRoundTripThroughUserData()
    {
        QMap<QString, QString> userData{{qsl("someone.else"), qsl("kept")}};
        DioramaKit::store(userData, {{310, Kit::Water}, {-1, Kit::Forest}});
        QCOMPARE(DioramaKit::stored(userData), (QHash<int, Kit>{{310, Kit::Water}, {-1, Kit::Forest}}));
        QCOMPARE(userData.value(qsl("someone.else")), qsl("kept"));

        // An entry another version wrote and this one cannot read is left out, not taken as plain
        userData.insert(DioramaKit::userDataKey(), qsl(R"({"5":"road","x":"water","6":"castle","7":3})"));
        QCOMPARE(DioramaKit::stored(userData), (QHash<int, Kit>{{5, Kit::Road}}));
        userData.insert(DioramaKit::userDataKey(), qsl("not json"));
        QVERIFY(DioramaKit::stored(userData).isEmpty());

        DioramaKit::store(userData, {});
        QVERIFY(!userData.contains(DioramaKit::userDataKey()));
        QCOMPARE(userData.size(), 1);
    }

    void test_flatKitsLieOnTheBoardAndWaterBelowIt()
    {
        const QHash<int, DioramaKit::Piece> pieces =
                DioramaKit::layout({{1, 0, 0, 0, Kit::Water}, {2, 1, 0, 0, Kit::Road}, {3, 2, 0, 0, Kit::Forest}, {4, 3, 0, 0, Kit::Building}, {5, 4, 0, 0, Kit::Plain}}, 0, kRoomSize, kRoomHeight);
        const float board = -DioramaKit::groundBelowLevel(kRoomHeight);
        const DioramaKit::Piece water = pieces.value(1);
        QVERIFY(water.top < board);
        QVERIFY(water.bottom >= board - DioramaKit::waterPitDepth(kRoomHeight) - 1e-5f);
        QCOMPARE(water.halfWidth, 0.5f);
        const DioramaKit::Piece road = pieces.value(2);
        QCOMPARE(road.bottom, board);
        QVERIFY(road.top - road.bottom < kRoomHeight / 4.0f);
        QCOMPARE(road.peak, road.top);
        const DioramaKit::Piece forest = pieces.value(3);
        QVERIFY(forest.top - forest.bottom < kRoomHeight / 4.0f);
        QVERIFY2(forest.peak > forest.top + kRoomHeight / 2.0f, "a forest's trees are not taller than its grass");
        const DioramaKit::Piece building = pieces.value(4);
        QCOMPARE(building.bottom, board);
        QVERIFY2(building.top > board + kRoomHeight, "a building is no taller than a plain block");
        QVERIFY2(building.peak > building.top, "a building has no roof");
        const DioramaKit::Piece plain = pieces.value(5);
        QCOMPARE(plain.bottom, board);
        QCOMPARE(plain.top, board + kRoomHeight);
        QCOMPARE(plain.halfWidth, kRoomSize / 2.0f);
        for (const DioramaKit::Piece& piece : pieces) {
            QCOMPARE(piece.storeys, 1);
        }
    }

    // Rooms straight above one on the viewed level make one building, whatever their own kits, with a floor for each
    // and the roof on the last; a gap ends it, and a room off to the side stays as it is
    void test_roomsStackedOnTheViewedLevelMakeOneBuilding()
    {
        const QHash<int, DioramaKit::Piece> pieces = DioramaKit::layout({{1, 0, 0, 0, Kit::Road},
                                                                         {2, 0, 0, 1, Kit::Water},
                                                                         {3, 0, 0, 2, Kit::Forest},
                                                                         {4, 0, 0, 3, Kit::Building},
                                                                         {5, 0, 0, 5, Kit::Building},
                                                                         {6, 1, 0, 1, Kit::Building},
                                                                         {7, 2, 0, -1, Kit::Road},
                                                                         {8, 2, 0, 0, Kit::Road}},
                                                                        0,
                                                                        kRoomSize,
                                                                        kRoomHeight);
        float floor = -DioramaKit::groundBelowLevel(kRoomHeight);
        for (int storey = 0; storey < 4; ++storey) {
            const DioramaKit::Piece piece = pieces.value(storey + 1);
            QCOMPARE(piece.kit, Kit::Building);
            QCOMPARE(piece.storey, storey);
            QCOMPARE(piece.storeys, 4);
            QVERIFY2(qFuzzyCompare(piece.bottom, floor), qPrintable(qsl("storey %1 starts at %2, not on the one below at %3").arg(storey).arg(piece.bottom).arg(floor)));
            QVERIFY(piece.top > piece.bottom);
            if (storey < 3) {
                QCOMPARE(piece.peak, piece.top);
            } else {
                QVERIFY2(piece.peak > piece.top, "the top storey has no roof");
            }
            floor = piece.top;
        }
        QCOMPARE(pieces.value(5).storeys, 1);
        QCOMPARE(pieces.value(6).storeys, 1);
        QCOMPARE(pieces.value(6).storey, 0);
        // Built up from the viewed level only: one below it is no part of the road above it
        QCOMPARE(pieces.value(7).storeys, 1);
        QCOMPARE(pieces.value(8).storeys, 1);
        QCOMPARE(pieces.value(8).kit, Kit::Road);

        // Viewed from the level above, the rooms there start their own building
        const QHash<int, DioramaKit::Piece> higher = DioramaKit::layout({{1, 0, 0, 0, Kit::Road}, {2, 0, 0, 1, Kit::Road}, {3, 0, 0, 2, Kit::Road}}, 1, kRoomSize, kRoomHeight);
        QCOMPARE(higher.value(1).storeys, 1);
        QCOMPARE(higher.value(2).storeys, 2);
        QCOMPARE(higher.value(3).storey, 1);
    }
};

#include "DioramaKitTest.moc"
MUDLET_GROUPED_TEST_MAIN(DioramaKitTest)
