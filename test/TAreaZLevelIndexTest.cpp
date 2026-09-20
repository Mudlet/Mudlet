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

#include <TAreaZLevelIndex.h>

#include <QtTest/QtTest>

/*
 * Unit tests for TAreaZLevelIndex.
 *
 * The class is pure data (no GUI / Host / TMap dependencies), so these tests
 * run without needing the full Mudlet application stack.
 *
 * Coverage goals
 * - addRoom: single-Z, multi-Z, duplicate (no-op)
 * - removeRoom: normal, last-room-on-Z prunes bucket, non-existent ID/Z (no
 * crash)
 * - moveRoom: same-Z no-op, cross-Z move
 * - rebuild: from scratch overwrites any prior state
 * - roomsForZ: existing Z, non-existent Z returns stable empty ref
 * - isEmpty / clear
 * - Room IDs are NOT necessarily sequential or in ascending order
 */
class TAreaZLevelIndexTest : public QObject
{
    Q_OBJECT

private slots:

    // -------------------------------------------------------------------------
    // addRoom
    // -------------------------------------------------------------------------

    void addRoom_singleZ_idAppearsOnThatLevel()
    {
        TAreaZLevelIndex idx;
        idx.addRoom(42, 0);
        QVERIFY(idx.roomsForZ(0).contains(42));
    }

    void addRoom_multipleRoomsOnSameZ()
    {
        TAreaZLevelIndex idx;
        idx.addRoom(1, 5);
        idx.addRoom(2, 5);
        idx.addRoom(3, 5);
        const QSet<int> expected = {1, 2, 3};
        QCOMPARE(idx.roomsForZ(5), expected);
    }

    void addRoom_roomsOnDifferentZLevels()
    {
        TAreaZLevelIndex idx;
        idx.addRoom(10, -1);
        idx.addRoom(20, 0);
        idx.addRoom(30, 1);

        QCOMPARE(idx.roomsForZ(-1), QSet<int>{10});
        QCOMPARE(idx.roomsForZ(0), QSet<int>{20});
        QCOMPARE(idx.roomsForZ(1), QSet<int>{30});
    }

    void addRoom_nonSequentialIds_allStoredCorrectly()
    {
        // Room IDs are not necessarily in ascending order (per problem statement)
        TAreaZLevelIndex idx;
        idx.addRoom(9999, 0);
        idx.addRoom(1, 0);
        idx.addRoom(500, 0);
        const QSet<int> expected = {1, 500, 9999};
        QCOMPARE(idx.roomsForZ(0), expected);
    }

    void addRoom_duplicate_noDoubleEntry()
    {
        TAreaZLevelIndex idx;
        idx.addRoom(7, 3);
        idx.addRoom(7, 3); // duplicate
        QCOMPARE(idx.roomsForZ(3).size(), 1);
        QVERIFY(idx.roomsForZ(3).contains(7));
    }

    // -------------------------------------------------------------------------
    // removeRoom
    // -------------------------------------------------------------------------

    void removeRoom_idNoLongerInLevel()
    {
        TAreaZLevelIndex idx;
        idx.addRoom(1, 0);
        idx.addRoom(2, 0);
        idx.removeRoom(1, 0);
        QVERIFY(!idx.roomsForZ(0).contains(1));
        QVERIFY(idx.roomsForZ(0).contains(2));
    }

    void removeRoom_lastRoomOnZ_zLevelBucketPruned()
    {
        TAreaZLevelIndex idx;
        idx.addRoom(42, 7);
        idx.removeRoom(42, 7);
        // The Z=7 bucket should be gone; roomsForZ should return an empty set
        QVERIFY(idx.roomsForZ(7).isEmpty());
        // And the index itself should be empty
        QVERIFY(idx.isEmpty());
    }

    void removeRoom_nonExistentId_nocrash()
    {
        TAreaZLevelIndex idx;
        idx.addRoom(1, 0);
        // Removing an ID that was never added should not crash
        idx.removeRoom(999, 0);
        QCOMPARE(idx.roomsForZ(0), QSet<int>{1});
    }

    void removeRoom_nonExistentZ_nocrash()
    {
        TAreaZLevelIndex idx;
        // Removing from a Z level that has no rooms should not crash
        idx.removeRoom(1, 99);
        QVERIFY(idx.isEmpty());
    }

    void removeRoom_wrongZ_idRemainsAtCorrectZ()
    {
        // Removing a room from the wrong Z level should not remove it from the
        // correct Z level where it actually lives.
        TAreaZLevelIndex idx;
        idx.addRoom(5, 2);
        idx.removeRoom(5, 3); // wrong Z
        QVERIFY(idx.roomsForZ(2).contains(5));
    }

    // -------------------------------------------------------------------------
    // moveRoom
    // -------------------------------------------------------------------------

    void moveRoom_sameZ_isNoOp()
    {
        TAreaZLevelIndex idx;
        idx.addRoom(10, 0);
        idx.moveRoom(10, 0, 0); // no-op
        QVERIFY(idx.roomsForZ(0).contains(10));
    }

    void moveRoom_crossZ_appearsOnNewLevelOnly()
    {
        TAreaZLevelIndex idx;
        idx.addRoom(10, 0);
        idx.moveRoom(10, 0, 5);
        QVERIFY(!idx.roomsForZ(0).contains(10));
        QVERIFY(idx.roomsForZ(5).contains(10));
    }

    void moveRoom_crossZ_oldBucketPrunedWhenEmpty()
    {
        TAreaZLevelIndex idx;
        idx.addRoom(10, 0); // only room on Z=0
        idx.moveRoom(10, 0, 1);
        QVERIFY(idx.roomsForZ(0).isEmpty());
    }

    void moveRoom_crossZ_otherRoomsOnOldLevelUnaffected()
    {
        TAreaZLevelIndex idx;
        idx.addRoom(1, 0);
        idx.addRoom(2, 0);
        idx.moveRoom(1, 0, 1);
        QVERIFY(!idx.roomsForZ(0).contains(1));
        QVERIFY(idx.roomsForZ(0).contains(2)); // room 2 stays
        QVERIFY(idx.roomsForZ(1).contains(1));
    }

    // -------------------------------------------------------------------------
    // rebuild
    // -------------------------------------------------------------------------

    void rebuild_populatesIndexFromMapping()
    {
        TAreaZLevelIndex idx;
        const QHash<int, int> mapping = {{1, 0}, {2, 0}, {3, 1}, {4, -1}};
        idx.rebuild(mapping);

        const QSet<int> z0Expected = {1, 2};
        QCOMPARE(idx.roomsForZ(0), z0Expected);
        QCOMPARE(idx.roomsForZ(1), QSet<int>{3});
        QCOMPARE(idx.roomsForZ(-1), QSet<int>{4});
    }

    void rebuild_overwritesPreviousState()
    {
        TAreaZLevelIndex idx;
        idx.addRoom(99, 99); // stale data

        const QHash<int, int> mapping = {{1, 0}};
        idx.rebuild(mapping);

        QVERIFY(!idx.roomsForZ(99).contains(99)); // stale data gone
        QVERIFY(idx.roomsForZ(0).contains(1));
    }

    void rebuild_emptyMapping_indexIsEmpty()
    {
        TAreaZLevelIndex idx;
        idx.addRoom(1, 0);
        idx.rebuild({});
        QVERIFY(idx.isEmpty());
    }

    void rebuild_nonSequentialIds()
    {
        // Room IDs are not necessarily in ascending order
        TAreaZLevelIndex idx;
        const QHash<int, int> mapping = {{9999, 0}, {1, 0}, {500, 0}};
        idx.rebuild(mapping);
        const QSet<int> expected = {1, 500, 9999};
        QCOMPARE(idx.roomsForZ(0), expected);
    }

    // -------------------------------------------------------------------------
    // roomsForZ
    // -------------------------------------------------------------------------

    void roomsForZ_nonExistentZ_returnsEmptySet()
    {
        TAreaZLevelIndex idx;
        idx.addRoom(1, 0);
        // Z=99 has no rooms
        const QSet<int>& result = idx.roomsForZ(99);
        QVERIFY(result.isEmpty());
    }

    void roomsForZ_returnsStableReferenceForEmptyZ()
    {
        TAreaZLevelIndex idx;
        // Two calls to a non-existent Z should return references to the same
        // static empty set (not dangling references).
        const QSet<int>& ref1 = idx.roomsForZ(77);
        const QSet<int>& ref2 = idx.roomsForZ(77);
        QVERIFY(&ref1 == &ref2);
        QVERIFY(ref1.isEmpty());
    }

    // -------------------------------------------------------------------------
    // isEmpty / clear
    // -------------------------------------------------------------------------

    void isEmpty_newIndex_isTrue()
    {
        TAreaZLevelIndex idx;
        QVERIFY(idx.isEmpty());
    }

    void isEmpty_afterAddRoom_isFalse()
    {
        TAreaZLevelIndex idx;
        idx.addRoom(1, 0);
        QVERIFY(!idx.isEmpty());
    }

    void isEmpty_afterRemoveLastRoom_isTrue()
    {
        TAreaZLevelIndex idx;
        idx.addRoom(1, 0);
        idx.removeRoom(1, 0);
        QVERIFY(idx.isEmpty());
    }

    void clear_removesAllRooms()
    {
        TAreaZLevelIndex idx;
        idx.addRoom(1, 0);
        idx.addRoom(2, 1);
        idx.clear();
        QVERIFY(idx.isEmpty());
        QVERIFY(idx.roomsForZ(0).isEmpty());
        QVERIFY(idx.roomsForZ(1).isEmpty());
    }

    // -------------------------------------------------------------------------
    // Edge / integration
    // -------------------------------------------------------------------------

    void integration_addRebuildRemove()
    {
        // Simulate a complete lifecycle: add rooms, rebuild, remove one, verify.
        TAreaZLevelIndex idx;

        const QHash<int, int> mapping = {{1, 0}, {2, 0}, {3, 1}};
        idx.rebuild(mapping);

        idx.removeRoom(1, 0);
        QVERIFY(!idx.roomsForZ(0).contains(1));
        QVERIFY(idx.roomsForZ(0).contains(2));

        idx.addRoom(4, 0);
        QVERIFY(idx.roomsForZ(0).contains(4));

        idx.moveRoom(3, 1, 0);
        QVERIFY(idx.roomsForZ(0).contains(3));
        QVERIFY(idx.roomsForZ(1).isEmpty());
    }

    void singleZArea_allRoomsOnSameLevel()
    {
        // The primary use case: a large area where every room is on Z=0.
        // Verifies that the index correctly reflects all rooms on Z=0 and
        // that other Z levels return empty sets.
        TAreaZLevelIndex idx;
        constexpr int N = 1000;
        for (int i = 1; i <= N; ++i) {
            idx.addRoom(i, 0);
        }
        QCOMPARE(idx.roomsForZ(0).size(), N);
        QVERIFY(idx.roomsForZ(-1).isEmpty());
        QVERIFY(idx.roomsForZ(1).isEmpty());
    }
};

QTEST_GUILESS_MAIN(TAreaZLevelIndexTest)

#include "TAreaZLevelIndexTest.moc"
