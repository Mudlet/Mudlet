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

#include <TAreaSpanIndex.h>

#include <QtTest/QtTest>

/*
 * Unit tests for TAreaSpanIndex.
 *
 * The class is pure data, so these tests state the expected extremes as
 * absolute values rather than comparing one code path against another.
 *
 * Coverage goals
 * - the extremes of a single Z level and of the index as a whole
 * - rooms sharing a coordinate: the bucket has to be emptied by the last of
 *   them, not the first
 * - Z levels appearing and disappearing
 * - the "extremes moved" answer that TArea uses to decide whether to
 *   republish, in both the true and the false direction
 * - removals of coordinates and Z levels that were never added
 */
class TAreaSpanIndexTest : public QObject
{
    Q_OBJECT

private:
    static void addRooms(TAreaSpanIndex& index, const QList<std::tuple<int, int, int>>& rooms)
    {
        for (const auto& [x, y, z] : rooms) {
            index.addRoom(x, y, z);
        }
    }

private slots:

    void freshIndexIsEmpty()
    {
        TAreaSpanIndex index;
        QVERIFY(index.empty());
        QVERIFY(!index.hasZ(0));
        QVERIFY(index.zLevels().isEmpty());
    }

    void singleRoomIsItsOwnExtreme()
    {
        TAreaSpanIndex index;
        index.addRoom(7, -4, 2);

        QVERIFY(!index.empty());
        QVERIFY(index.hasZ(2));
        QCOMPARE(index.minZ(), 2);
        QCOMPARE(index.maxZ(), 2);
        const TAreaSpanIndex::Extremes overall = index.overallExtremes();
        QCOMPARE(overall.minX, 7);
        QCOMPARE(overall.maxX, 7);
        QCOMPARE(overall.minY, -4);
        QCOMPARE(overall.maxY, -4);
    }

    void extremesAreTheLowestAndHighestCoordinates()
    {
        TAreaSpanIndex index;
        addRooms(index, {{-3, 5, 0}, {11, -2, 0}, {4, 9, 0}});

        const TAreaSpanIndex::Extremes overall = index.overallExtremes();
        QCOMPARE(overall.minX, -3);
        QCOMPARE(overall.maxX, 11);
        QCOMPARE(overall.minY, -2);
        QCOMPARE(overall.maxY, 9);
    }

    void eachZLevelHasItsOwnExtremes()
    {
        TAreaSpanIndex index;
        addRooms(index, {{0, 0, 0}, {100, 100, 0}, {-50, -50, 3}, {-10, -10, 3}});

        const TAreaSpanIndex::Extremes ground = index.extremesForZ(0);
        QCOMPARE(ground.minX, 0);
        QCOMPARE(ground.maxX, 100);
        QCOMPARE(ground.minY, 0);
        QCOMPARE(ground.maxY, 100);

        const TAreaSpanIndex::Extremes upstairs = index.extremesForZ(3);
        QCOMPARE(upstairs.minX, -50);
        QCOMPARE(upstairs.maxX, -10);
        QCOMPARE(upstairs.minY, -50);
        QCOMPARE(upstairs.maxY, -10);

        const TAreaSpanIndex::Extremes overall = index.overallExtremes();
        QCOMPARE(overall.minX, -50);
        QCOMPARE(overall.maxX, 100);
        QCOMPARE(overall.minY, -50);
        QCOMPARE(overall.maxY, 100);
    }

    void zLevelsAreAscendingAndDeduplicated()
    {
        TAreaSpanIndex index;
        addRooms(index, {{0, 0, 4}, {1, 1, -2}, {2, 2, 4}, {3, 3, 0}});

        const QList<int> expected{-2, 0, 4};
        QCOMPARE(index.zLevels(), expected);
        QCOMPARE(index.minZ(), -2);
        QCOMPARE(index.maxZ(), 4);
    }

    void aSharedCoordinateSurvivesTheFirstRemoval()
    {
        TAreaSpanIndex index;
        addRooms(index, {{2, 2, 0}, {2, 2, 0}, {8, 8, 0}});

        index.removeRoom(2, 2, 0);
        const TAreaSpanIndex::Extremes stillThere = index.extremesForZ(0);
        QCOMPARE(stillThere.minX, 2);
        QCOMPARE(stillThere.minY, 2);

        index.removeRoom(2, 2, 0);
        const TAreaSpanIndex::Extremes gone = index.extremesForZ(0);
        QCOMPARE(gone.minX, 8);
        QCOMPARE(gone.minY, 8);
    }

    void removingTheLastRoomOnALevelDropsTheLevel()
    {
        TAreaSpanIndex index;
        addRooms(index, {{1, 1, 0}, {2, 2, 5}});

        index.removeRoom(2, 2, 5);
        QVERIFY(!index.hasZ(5));
        const QList<int> expected{0};
        QCOMPARE(index.zLevels(), expected);
        QCOMPARE(index.maxZ(), 0);
    }

    void emptyingTheIndexReportsEmpty()
    {
        TAreaSpanIndex index;
        index.addRoom(1, 1, 0);
        index.removeRoom(1, 1, 0);

        QVERIFY(index.empty());
        QVERIFY(index.zLevels().isEmpty());
    }

    void addReportsWhetherTheExtremesMoved()
    {
        TAreaSpanIndex index;
        QVERIFY2(index.addRoom(5, 5, 0), "the first room on a Z level always moves its extremes");
        QVERIFY2(index.addRoom(9, 9, 0), "a room beyond the current maximum moves it");
        QVERIFY2(!index.addRoom(7, 7, 0), "a room inside the current range leaves the extremes alone");
        QVERIFY2(!index.addRoom(5, 9, 0), "a room on both existing extremes leaves them alone");
        QVERIFY2(index.addRoom(1, 7, 0), "a room below the current minimum moves it");
        QVERIFY2(index.addRoom(7, 7, 1), "a new Z level always counts as a move");
    }

    void removeReportsWhetherTheExtremesMoved()
    {
        TAreaSpanIndex index;
        addRooms(index, {{1, 1, 0}, {5, 5, 0}, {9, 9, 0}});

        QVERIFY2(!index.removeRoom(5, 5, 0), "removing a room inside the range leaves the extremes alone");
        QVERIFY2(index.removeRoom(9, 9, 0), "removing the room at the maximum moves it");
        QVERIFY2(index.removeRoom(1, 1, 0), "removing the last room drops the Z level");
    }

    void removalsOfThingsNeverAddedAreIgnored()
    {
        TAreaSpanIndex index;
        index.addRoom(4, 4, 0);

        QVERIFY(!index.removeRoom(4, 4, 99));
        QVERIFY(!index.removeRoom(1000, 1000, 0));

        // The one real room is still the only thing the index knows about:
        const TAreaSpanIndex::Extremes overall = index.overallExtremes();
        QCOMPARE(overall.minX, 4);
        QCOMPARE(overall.maxX, 4);
        QCOMPARE(index.zLevels(), QList<int>{0});
    }

    // The overall extremes are kept separately from the per-level ones, so a
    // level that comes and goes must not leave anything behind in them.
    void overallExtremesFollowLevelsThatComeAndGo()
    {
        TAreaSpanIndex index;
        index.addRoom(0, 0, 0);
        index.addRoom(400, -400, 7);
        index.removeRoom(400, -400, 7);

        const TAreaSpanIndex::Extremes overall = index.overallExtremes();
        QCOMPARE(overall.minX, 0);
        QCOMPARE(overall.maxX, 0);
        QCOMPARE(overall.minY, 0);
        QCOMPARE(overall.maxY, 0);
    }

    void clearForgetsEverything()
    {
        TAreaSpanIndex index;
        addRooms(index, {{1, 2, 3}, {4, 5, 6}});

        index.clear();
        QVERIFY(index.empty());
        QVERIFY(index.zLevels().isEmpty());
        QVERIFY(!index.hasZ(3));
    }

    void extremesOfAnUnknownLevelAreZeroes()
    {
        TAreaSpanIndex index;
        index.addRoom(11, 12, 0);

        const TAreaSpanIndex::Extremes nothing = index.extremesForZ(42);
        QCOMPARE(nothing.minX, 0);
        QCOMPARE(nothing.maxX, 0);
        QCOMPARE(nothing.minY, 0);
        QCOMPARE(nothing.maxY, 0);
    }
};

#include "TAreaSpanIndexTest.moc"
QTEST_MAIN(TAreaSpanIndexTest)
