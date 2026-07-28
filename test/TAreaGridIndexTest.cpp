/***************************************************************************
 *   Copyright (C) 2026 by Mudlet Makers                                   *
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

#include <TAreaGridIndex.h>

#include <QtTest/QtTest>

/*
 * Unit tests for TAreaGridIndex.
 *
 * The class is pure data (no GUI / Host / TMap dependencies), so these tests
 * run without needing the full Mudlet application stack.
 *
 * Coverage goals
 * - addRoom: single cell, multiple rooms at same cell, different cells, duplicate, cross-Z
 * - removeRoom: normal, last-room-at-cell prunes bucket, non-existent id/z (no
 *   crash), wrong coords leaves correct cell intact
 * - moveRoom: same-coords no-op, different XY same Z, different Z, pruning of empty cell
 * - rebuild (per-Z): populates, overwrites prior Z level, does not affect other Z levels
 * - rebuild (full): populates all Z levels, overwrites previous state, empty map -> isEmpty
 * - roomsAt: existing cell, non-existent cell (no crash), stable empty reference
 * - roomsInViewport: all in, some out, empty Z, multiple rooms per cell, inclusive edges
 * - roomsInViewportWithCollisions: solo rooms, colliding rooms, out-of-viewport exclusion, empty Z
 * - isEmpty / clear / size
 * - Integration: add + full rebuild + remove + move
 */
class TAreaGridIndexTest : public QObject
{
    Q_OBJECT

private slots:

    // -------------------------------------------------------------------------
    // addRoom
    // -------------------------------------------------------------------------

    void addRoom_singleCell_idAppearsAtThatCell()
    {
        TAreaGridIndex idx;
        idx.addRoom(42, 0, 3, 5);
        QVERIFY(idx.roomsAt(0, 3, 5).contains(42));
    }

    void addRoom_multipleRoomsAtSameCell_allPresent()
    {
        TAreaGridIndex idx;
        idx.addRoom(1, 0, 2, 2);
        idx.addRoom(2, 0, 2, 2);
        const QSet<int>& cell = idx.roomsAt(0, 2, 2);
        QVERIFY(cell.contains(1));
        QVERIFY(cell.contains(2));
        QCOMPARE(cell.size(), 2);
    }

    void addRoom_roomsAtDifferentCells_segregated()
    {
        TAreaGridIndex idx;
        idx.addRoom(10, 0, 1, 2);
        idx.addRoom(20, 0, 3, 4);
        QVERIFY(idx.roomsAt(0, 1, 2).contains(10));
        QVERIFY(!idx.roomsAt(0, 1, 2).contains(20));
        QVERIFY(idx.roomsAt(0, 3, 4).contains(20));
        QVERIFY(!idx.roomsAt(0, 3, 4).contains(10));
    }

    void addRoom_duplicate_noDoubleEntry()
    {
        TAreaGridIndex idx;
        idx.addRoom(7, 0, 1, 1);
        idx.addRoom(7, 0, 1, 1); // duplicate
        QCOMPARE(idx.roomsAt(0, 1, 1).size(), 1);
        QVERIFY(idx.roomsAt(0, 1, 1).contains(7));
    }

    void addRoom_acrossZLevels_segregated()
    {
        TAreaGridIndex idx;
        idx.addRoom(100, 0, 5, 5);
        idx.addRoom(200, 1, 5, 5);
        QVERIFY(idx.roomsAt(0, 5, 5).contains(100));
        QVERIFY(!idx.roomsAt(0, 5, 5).contains(200));
        QVERIFY(idx.roomsAt(1, 5, 5).contains(200));
        QVERIFY(!idx.roomsAt(1, 5, 5).contains(100));
    }

    // -------------------------------------------------------------------------
    // removeRoom
    // -------------------------------------------------------------------------

    void removeRoom_idNoLongerAtCell()
    {
        TAreaGridIndex idx;
        idx.addRoom(1, 0, 3, 3);
        idx.addRoom(2, 0, 3, 3);
        idx.removeRoom(1, 0, 3, 3);
        QVERIFY(!idx.roomsAt(0, 3, 3).contains(1));
        QVERIFY(idx.roomsAt(0, 3, 3).contains(2));
    }

    void removeRoom_lastRoomAtCell_cellPruned()
    {
        TAreaGridIndex idx;
        idx.addRoom(42, 0, 7, 8);
        idx.removeRoom(42, 0, 7, 8);
        QVERIFY(idx.roomsAt(0, 7, 8).isEmpty());
        QVERIFY(idx.isEmpty());
    }

    void removeRoom_nonExistentId_nocrash()
    {
        TAreaGridIndex idx;
        idx.addRoom(1, 0, 0, 0);
        // Should not crash
        idx.removeRoom(999, 0, 0, 0);
        QVERIFY(idx.roomsAt(0, 0, 0).contains(1));
    }

    void removeRoom_nonExistentZ_nocrash()
    {
        TAreaGridIndex idx;
        // Should not crash when Z level has no rooms
        idx.removeRoom(1, 99, 0, 0);
        QVERIFY(idx.isEmpty());
    }

    void removeRoom_wrongCoords_idRemainsAtCorrectCell()
    {
        TAreaGridIndex idx;
        idx.addRoom(5, 0, 2, 3);
        idx.removeRoom(5, 0, 9, 9); // wrong x/y
        QVERIFY(idx.roomsAt(0, 2, 3).contains(5));
    }

    // -------------------------------------------------------------------------
    // moveRoom
    // -------------------------------------------------------------------------

    void moveRoom_sameCoords_isNoOp()
    {
        TAreaGridIndex idx;
        idx.addRoom(10, 0, 4, 4);
        idx.moveRoom(10, 0, 4, 4, 0, 4, 4); // no-op
        QVERIFY(idx.roomsAt(0, 4, 4).contains(10));
    }

    void moveRoom_differentXY_sameZ_appearsAtNewCell()
    {
        TAreaGridIndex idx;
        idx.addRoom(10, 0, 1, 1);
        idx.moveRoom(10, 0, 1, 1, 0, 5, 6);
        QVERIFY(!idx.roomsAt(0, 1, 1).contains(10));
        QVERIFY(idx.roomsAt(0, 5, 6).contains(10));
    }

    void moveRoom_differentZ_appearsAtNewCellOnly()
    {
        TAreaGridIndex idx;
        idx.addRoom(10, 0, 2, 2);
        idx.moveRoom(10, 0, 2, 2, 3, 2, 2);
        QVERIFY(!idx.roomsAt(0, 2, 2).contains(10));
        QVERIFY(idx.roomsAt(3, 2, 2).contains(10));
    }

    void moveRoom_partialMove_oldCellPrunedWhenEmpty()
    {
        TAreaGridIndex idx;
        idx.addRoom(10, 0, 0, 0); // only room at (z=0,x=0,y=0)
        idx.moveRoom(10, 0, 0, 0, 1, 0, 0);
        QVERIFY(idx.roomsAt(0, 0, 0).isEmpty());
    }

    // -------------------------------------------------------------------------
    // rebuild (per-Z)
    // -------------------------------------------------------------------------

    void rebuildPerZ_populatesFromMapping()
    {
        TAreaGridIndex idx;
        const QHash<int, QPair<int, int>> mapping = {{1, {2, 3}}, {4, {5, 6}}};
        idx.rebuild(0, mapping);
        QVERIFY(idx.roomsAt(0, 2, 3).contains(1));
        QVERIFY(idx.roomsAt(0, 5, 6).contains(4));
    }

    void rebuildPerZ_overwritesPriorZLevel()
    {
        TAreaGridIndex idx;
        idx.addRoom(99, 0, 9, 9); // stale data on Z=0

        const QHash<int, QPair<int, int>> mapping = {{1, {0, 0}}};
        idx.rebuild(0, mapping);

        QVERIFY(!idx.roomsAt(0, 9, 9).contains(99)); // stale gone
        QVERIFY(idx.roomsAt(0, 0, 0).contains(1));
    }

    void rebuildPerZ_doesNotAffectOtherZLevels()
    {
        TAreaGridIndex idx;
        idx.addRoom(55, 1, 3, 3); // on Z=1

        const QHash<int, QPair<int, int>> mapping = {{1, {0, 0}}};
        idx.rebuild(0, mapping); // rebuild only Z=0

        QVERIFY(idx.roomsAt(1, 3, 3).contains(55)); // Z=1 still intact
        QVERIFY(idx.roomsAt(0, 0, 0).contains(1));
    }

    // -------------------------------------------------------------------------
    // rebuild (full -- all Z levels at once)
    // -------------------------------------------------------------------------

    void rebuildFull_populatesAllZLevels()
    {
        TAreaGridIndex idx;
        const QHash<int, QHash<int, QPair<int, int>>> zToRoomXY = {{0, {{1, {2, 3}}, {2, {4, 5}}}}, {1, {{3, {0, 0}}}}};
        idx.rebuild(zToRoomXY);
        QVERIFY(idx.roomsAt(0, 2, 3).contains(1));
        QVERIFY(idx.roomsAt(0, 4, 5).contains(2));
        QVERIFY(idx.roomsAt(1, 0, 0).contains(3));
    }

    void rebuildFull_overwritesPreviousState()
    {
        TAreaGridIndex idx;
        idx.addRoom(99, 5, 9, 9); // stale data

        const QHash<int, QHash<int, QPair<int, int>>> zToRoomXY = {{0, {{1, {0, 0}}}}};
        idx.rebuild(zToRoomXY);

        QVERIFY(!idx.roomsAt(5, 9, 9).contains(99)); // stale gone
        QVERIFY(idx.roomsAt(0, 0, 0).contains(1));
    }

    void rebuildFull_emptyMapping_indexIsEmpty()
    {
        TAreaGridIndex idx;
        idx.addRoom(1, 0, 0, 0);
        idx.rebuild(QHash<int, QHash<int, QPair<int, int>>>{});
        QVERIFY(idx.isEmpty());
    }

    // -------------------------------------------------------------------------
    // roomsAt
    // -------------------------------------------------------------------------

    void roomsAt_existingCell_returnsCorrectSet()
    {
        TAreaGridIndex idx;
        idx.addRoom(7, 2, 10, 20);
        idx.addRoom(8, 2, 10, 20);
        const QSet<int> expected = {7, 8};
        QCOMPARE(idx.roomsAt(2, 10, 20), expected);
    }

    void roomsAt_nonExistentCell_returnsEmptySet()
    {
        TAreaGridIndex idx;
        idx.addRoom(1, 0, 0, 0);
        // (0, 99, 99) was never populated
        QVERIFY(idx.roomsAt(0, 99, 99).isEmpty());
    }

    void roomsAt_returnsStableReferenceForEmptyCell()
    {
        TAreaGridIndex idx;
        // Two calls to a non-existent cell must return references to the same
        // static empty set (no dangling references).
        const QSet<int>& ref1 = idx.roomsAt(77, 1, 2);
        const QSet<int>& ref2 = idx.roomsAt(77, 1, 2);
        QVERIFY(&ref1 == &ref2);
        QVERIFY(ref1.isEmpty());
    }

    // -------------------------------------------------------------------------
    // roomsInViewport
    // -------------------------------------------------------------------------

    void roomsInViewport_allRoomsInViewport_allReturned()
    {
        TAreaGridIndex idx;
        idx.addRoom(1, 0, 0, 0);
        idx.addRoom(2, 0, 1, 1);
        idx.addRoom(3, 0, 2, 2);

        const QList<int> result = idx.roomsInViewport(0, 0, 2, 0, 2);
        const QSet<int> visited(result.constBegin(), result.constEnd());

        QCOMPARE(visited, QSet<int>({1, 2, 3}));
    }

    void roomsInViewport_someRoomsOutsideViewport_onlyInsideReturned()
    {
        TAreaGridIndex idx;
        idx.addRoom(1, 0, 0, 0); // inside [0..2] x [0..2]
        idx.addRoom(2, 0, 5, 5); // outside
        idx.addRoom(3, 0, 2, 2); // inside

        const QList<int> result = idx.roomsInViewport(0, 0, 2, 0, 2);
        const QSet<int> visited(result.constBegin(), result.constEnd());

        QVERIFY(visited.contains(1));
        QVERIFY(visited.contains(3));
        QVERIFY(!visited.contains(2));
    }

    void roomsInViewport_emptyZ_returnsEmpty()
    {
        TAreaGridIndex idx;
        idx.addRoom(1, 0, 0, 0); // on Z=0

        const QList<int> result = idx.roomsInViewport(99, 0, 10, 0, 10);

        QCOMPARE(result.size(), 0);
    }

    void roomsInViewport_multipleRoomsPerCell_allReturned()
    {
        TAreaGridIndex idx;
        idx.addRoom(1, 0, 3, 3);
        idx.addRoom(2, 0, 3, 3); // same cell as room 1

        const QList<int> result = idx.roomsInViewport(0, 0, 5, 0, 5);
        const QSet<int> visited(result.constBegin(), result.constEnd());

        QVERIFY(visited.contains(1));
        QVERIFY(visited.contains(2));
        QCOMPARE(visited.size(), 2);
    }

    void roomsInViewport_exactBounds_inclusiveEdges()
    {
        TAreaGridIndex idx;
        idx.addRoom(1, 0, 0, 0);   // at minX, minY
        idx.addRoom(2, 0, 10, 0);  // at maxX, minY
        idx.addRoom(3, 0, 0, 10);  // at minX, maxY
        idx.addRoom(4, 0, 10, 10); // at maxX, maxY
        idx.addRoom(5, 0, 11, 5);  // just outside (x > maxX)

        const QList<int> result = idx.roomsInViewport(0, 0, 10, 0, 10);
        const QSet<int> visited(result.constBegin(), result.constEnd());

        QVERIFY(visited.contains(1));
        QVERIFY(visited.contains(2));
        QVERIFY(visited.contains(3));
        QVERIFY(visited.contains(4));
        QVERIFY(!visited.contains(5));
        QCOMPARE(visited.size(), 4);
    }

    // -------------------------------------------------------------------------
    // roomsInViewportWithCollisions
    // -------------------------------------------------------------------------

    void roomsInViewportWithCollisions_soloRooms_collisionFalse()
    {
        TAreaGridIndex idx;
        idx.addRoom(1, 0, 0, 0);
        idx.addRoom(2, 0, 1, 0);
        idx.addRoom(3, 0, 2, 0);

        const auto result = idx.roomsInViewportWithCollisions(0, 0, 2, 0, 0);
        QCOMPARE(result.size(), 3);
        for (const auto& [id, collision] : result) {
            QVERIFY(!collision); // each room is alone in its cell
        }
    }

    void roomsInViewportWithCollisions_sharedCell_collisionTrue()
    {
        TAreaGridIndex idx;
        idx.addRoom(1, 0, 5, 5);
        idx.addRoom(2, 0, 5, 5); // same cell as room 1 → collision

        const auto result = idx.roomsInViewportWithCollisions(0, 5, 5, 5, 5);
        QCOMPARE(result.size(), 2);
        QSet<int> ids;
        for (const auto& [id, collision] : result) {
            ids.insert(id);
            QVERIFY(collision);
        }
        QVERIFY(ids.contains(1));
        QVERIFY(ids.contains(2));
    }

    void roomsInViewportWithCollisions_mixedCells_flagPerCell()
    {
        TAreaGridIndex idx;
        idx.addRoom(1, 0, 0, 0); // solo
        idx.addRoom(2, 0, 1, 0); // shared
        idx.addRoom(3, 0, 1, 0); // shares cell with 2

        const auto result = idx.roomsInViewportWithCollisions(0, 0, 1, 0, 0);
        QCOMPARE(result.size(), 3);
        QMap<int, bool> idToCollision;
        for (const auto& [id, collision] : result) {
            idToCollision[id] = collision;
        }
        QVERIFY(!idToCollision[1]); // solo cell
        QVERIFY(idToCollision[2]);  // shared cell
        QVERIFY(idToCollision[3]);  // shared cell
    }

    void roomsInViewportWithCollisions_outOfViewport_excluded()
    {
        TAreaGridIndex idx;
        idx.addRoom(1, 0, 5, 5);  // inside viewport
        idx.addRoom(2, 0, 20, 5); // outside viewport

        const auto result = idx.roomsInViewportWithCollisions(0, 0, 10, 0, 10);
        QCOMPARE(result.size(), 1);
        QCOMPARE(result.first().first, 1);
    }

    void roomsInViewportWithCollisions_emptyZ_returnsEmpty()
    {
        TAreaGridIndex idx;
        idx.addRoom(1, 0, 5, 5);

        const auto result = idx.roomsInViewportWithCollisions(99, 0, 10, 0, 10);
        QVERIFY(result.isEmpty());
    }

    // -------------------------------------------------------------------------
    // isEmpty / clear / size
    // -------------------------------------------------------------------------

    void isEmpty_newIndex_isTrue()
    {
        TAreaGridIndex idx;
        QVERIFY(idx.isEmpty());
    }

    void isEmpty_afterAddRoom_isFalse()
    {
        TAreaGridIndex idx;
        idx.addRoom(1, 0, 0, 0);
        QVERIFY(!idx.isEmpty());
    }

    void clear_removesAllData()
    {
        TAreaGridIndex idx;
        idx.addRoom(1, 0, 0, 0);
        idx.addRoom(2, 1, 5, 5);
        idx.clear();
        QVERIFY(idx.isEmpty());
        QVERIFY(idx.roomsAt(0, 0, 0).isEmpty());
        QVERIFY(idx.roomsAt(1, 5, 5).isEmpty());
    }

    void size_reflectsRoomCount()
    {
        TAreaGridIndex idx;
        QCOMPARE(idx.size(), 0);
        idx.addRoom(1, 0, 0, 0);
        QCOMPARE(idx.size(), 1);
        idx.addRoom(2, 0, 0, 0); // same cell, different id
        QCOMPARE(idx.size(), 2);
        idx.addRoom(3, 1, 5, 5); // different Z
        QCOMPARE(idx.size(), 3);
        idx.removeRoom(2, 0, 0, 0);
        QCOMPARE(idx.size(), 2);
    }

    // -------------------------------------------------------------------------
    // Integration
    // -------------------------------------------------------------------------

    void integration_addRebuildRemoveMove()
    {
        TAreaGridIndex idx;

        // Start by adding a few rooms manually
        idx.addRoom(1, 0, 0, 0);
        idx.addRoom(2, 0, 1, 0);

        // Full rebuild replaces everything
        const QHash<int, QHash<int, QPair<int, int>>> zToRoomXY = {{0, {{1, {0, 0}}, {2, {1, 0}}, {3, {2, 0}}}}, {1, {{4, {0, 0}}}}};
        idx.rebuild(zToRoomXY);

        // Verify rebuild state
        QVERIFY(idx.roomsAt(0, 0, 0).contains(1));
        QVERIFY(idx.roomsAt(0, 1, 0).contains(2));
        QVERIFY(idx.roomsAt(0, 2, 0).contains(3));
        QVERIFY(idx.roomsAt(1, 0, 0).contains(4));

        // Remove room 3
        idx.removeRoom(3, 0, 2, 0);
        QVERIFY(!idx.roomsAt(0, 2, 0).contains(3));

        // Move room 4 from z=1 to z=0 at (3, 0)
        idx.moveRoom(4, 1, 0, 0, 0, 3, 0);
        QVERIFY(!idx.roomsAt(1, 0, 0).contains(4));
        QVERIFY(idx.roomsAt(0, 3, 0).contains(4));

        // Z=1 should be empty now (all rooms moved out)
        QVERIFY(idx.roomsAt(1, 0, 0).isEmpty());

        // Final size: rooms 1, 2, 4 remain
        QCOMPARE(idx.size(), 3);
    }
};

QTEST_GUILESS_MAIN(TAreaGridIndexTest)

#include "TAreaGridIndexTest.moc"
