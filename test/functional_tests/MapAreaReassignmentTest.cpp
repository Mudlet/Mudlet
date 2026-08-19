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

/*
 * Moving a room between areas keeps every per-area index up to date without
 * rescanning the areas involved.
 *
 * Most of the tests here work the same way: drive the map through the
 * incremental path (TMap::setRoomArea, TMap::setRoomCoordinates), then force
 * the authoritative full rescan (TArea::calcSpan + determineAreaExits) and
 * require that it changes nothing - including the bytes of the saved map file,
 * which stores the area extents and the area exit records.
 *
 * assigningARoomDoesNotRescanTheArea() is the odd one out: it asserts that the
 * rescan does not happen at all, which is what the issue was about.
 *
 * Run with: ctest -R MapAreaReassignmentTest -V
 */

#include <QFileInfo>
#include <QtTest/QtTest>

#include <QElapsedTimer>
#include <QFile>
#include <QSaveFile>
#include <QTemporaryDir>

#include "Host.h"
#include "HostManager.h"
#include "MudletInstanceCoordinator.h"
#include "TArea.h"
#include "TMap.h"
#include "TRoom.h"
#include "TRoomDB.h"
#include "mudlet.h"

#include "GroupedTest.h"

namespace {
// Sizes for the reported cost curve: a per-room cost that climbs with them is
// what an assignment that rescans the area looks like.
const QList<int> scmScalingRoomCounts{1000, 2000, 4000, 8000};

// Bounds of the Z levels and coordinates the tests place rooms on, so that the
// per-area indexes can be sampled over a fixed range.
const int scmProbedZMinimum = -8;
const int scmProbedZMaximum = 8;
const int scmProbedCoordinateLimit = 100000;

// Deterministic and self-contained, so that a failure reproduces exactly.
class SmallRandom
{
public:
    explicit SmallRandom(quint32 seed)
    : mState(seed)
    {
    }
    int next(int bound)
    {
        mState = mState * 1103515245u + 12345u;
        return static_cast<int>((mState >> 16) % static_cast<quint32>(bound));
    }

private:
    quint32 mState;
};
} // namespace

class MapAreaReassignmentTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;
    Host* mpHost = nullptr;
    const QString mProfileName = qsl("MapAreaReassignment-Test");
    QTemporaryDir mSaveDir;

    // Everything a saved map records about an area's rooms, plus the derived
    // data the mapper draws from:
    struct AreaState
    {
        QSet<int> rooms;
        QList<int> zLevels;
        QMap<int, int> xminForZ;
        QMap<int, int> xmaxForZ;
        QMap<int, int> yminForZ;
        QMap<int, int> ymaxForZ;
        int minX = 0;
        int maxX = 0;
        int minY = 0;
        int maxY = 0;
        int minZ = 0;
        int maxZ = 0;
        QList<int> exitRoomIds;
        QMultiMap<int, QPair<QString, int>> exitData;
        QMap<int, QSet<int>> roomsForZ;
        QMap<int, QList<int>> roomsInGrid;

        bool operator==(const AreaState& other) const
        {
            return rooms == other.rooms && zLevels == other.zLevels && xminForZ == other.xminForZ && xmaxForZ == other.xmaxForZ && yminForZ == other.yminForZ && ymaxForZ == other.ymaxForZ
                   && minX == other.minX && maxX == other.maxX && minY == other.minY && maxY == other.maxY && minZ == other.minZ && maxZ == other.maxZ && exitRoomIds == other.exitRoomIds
                   && exitData == other.exitData && roomsForZ == other.roomsForZ && roomsInGrid == other.roomsInGrid;
        }
    };

    TMap* map() const { return mpHost->mpMap.data(); }
    TRoomDB* roomDB() const { return mpHost->mpMap->mpRoomDB.get(); }

    AreaState captureAreaState(TArea* pArea) const
    {
        AreaState state;
        state.rooms = pArea->rooms;
        state.zLevels = pArea->zLevels;
        state.xminForZ = pArea->xminForZ;
        state.xmaxForZ = pArea->xmaxForZ;
        state.yminForZ = pArea->yminForZ;
        state.ymaxForZ = pArea->ymaxForZ;
        state.minX = pArea->min_x;
        state.maxX = pArea->max_x;
        state.minY = pArea->min_y;
        state.maxY = pArea->max_y;
        state.minZ = pArea->min_z;
        state.maxZ = pArea->max_z;
        state.exitRoomIds = pArea->getAreaExitRoomIds();
        state.exitData = pArea->getAreaExitRoomData();
        // Sampled over a fixed range rather than over the area's own zLevels,
        // so that entries for a Z level the area does not admit to having are
        // still visible:
        for (int z = scmProbedZMinimum; z <= scmProbedZMaximum; ++z) {
            const QSet<int> roomsForZ = pArea->getRoomsForZ(z);
            if (!roomsForZ.isEmpty()) {
                state.roomsForZ.insert(z, roomsForZ);
            }
            QList<int> roomsInGrid = pArea->getGridIndex().roomsInViewport(z, -scmProbedCoordinateLimit, scmProbedCoordinateLimit, -scmProbedCoordinateLimit, scmProbedCoordinateLimit);
            if (!roomsInGrid.isEmpty()) {
                std::sort(roomsInGrid.begin(), roomsInGrid.end());
                state.roomsInGrid.insert(z, roomsInGrid);
            }
        }
        return state;
    }

    // Both of these are what map loading and the map auditor run to rebuild an
    // area from its room set, so they define the expected answers.
    static void recalculateFully(TArea* pArea)
    {
        pArea->determineAreaExits();
        pArea->calcSpan();
    }

    QString describeMismatch(int areaId) const { return qsl("area %1 differs from a full recalculation").arg(areaId); }

    bool everyAreaMatchesFullRecalculation(QString& firstMismatch) const
    {
        const QList<int> areaIds = roomDB()->getAreaIDList();
        for (const int areaId : areaIds) {
            TArea* pArea = roomDB()->getArea(areaId);
            if (!pArea) {
                continue;
            }

            const AreaState incremental = captureAreaState(pArea);
            recalculateFully(pArea);
            if (!(incremental == captureAreaState(pArea))) {
                firstMismatch = describeMismatch(areaId);
                return false;
            }
        }
        return true;
    }

    bool saveMapToFile(const QString& fileName) const
    {
        QSaveFile file(fileName);
        if (!file.open(QIODevice::WriteOnly)) {
            return false;
        }
        QDataStream out(&file);
        if (mudlet::scmRunTimeQtVersion >= QVersionNumber(5, 13, 0)) {
            out.setVersion(mudlet::scmQDataStreamFormat_5_12);
        }
        if (!map()->serialize(out, map()->mDefaultVersion)) {
            return false;
        }
        return file.commit();
    }

    static QByteArray fileContents(const QString& fileName)
    {
        QFile file(fileName);
        if (!file.open(QIODevice::ReadOnly)) {
            return {};
        }
        return file.readAll();
    }

    void clearMap()
    {
        map()->mapClear();
        QVERIFY(roomDB()->getRoomIDList().isEmpty());
    }

    // Two areas of nine rooms each on two z levels, wired up with normal and
    // special exits within and across the areas, plus a couple of exit stubs
    // that no area bookkeeping should ever touch.
    void buildTwoAreaMap(int& areaA, int& areaB)
    {
        clearMap();
        areaA = roomDB()->addArea(qsl("area A"));
        areaB = roomDB()->addArea(qsl("area B"));
        QVERIFY(areaA > 0);
        QVERIFY(areaB > 0);

        for (int i = 0; i < 18; ++i) {
            const int roomId = 100 + i;
            QVERIFY(map()->addRoom(roomId));
            QVERIFY(map()->setRoomArea(roomId, i < 9 ? areaA : areaB));
            QVERIFY(map()->setRoomCoordinates(roomId, (i % 3) * 5, (i % 4) * -3, i % 2));
        }

        for (int i = 0; i < 17; ++i) {
            QVERIFY(map()->setExit(100 + i, 101 + i, DIR_EAST));
            QVERIFY(map()->setExit(101 + i, 100 + i, DIR_WEST));
        }
        // Across the area boundary in both directions, plus a special exit
        // pair that only the area of the *target* room decides on:
        QVERIFY(map()->setExit(102, 112, DIR_UP));
        QVERIFY(map()->setExit(112, 102, DIR_DOWN));
        roomDB()->getRoom(103)->setSpecialExit(115, qsl("teleport"));
        roomDB()->getRoom(115)->setSpecialExit(103, qsl("return"));
        roomDB()->getRoom(104)->setExitStub(DIR_NORTH, true);
        roomDB()->getRoom(113)->setExitStub(DIR_SOUTHWEST, true);
    }

    // Four areas of rooms on three Z levels, sharing plenty of coordinates,
    // wired up with a mix of normal and special exits.
    void buildRandomMap(QList<int>& areaIds, QList<int>& roomIds)
    {
        clearMap();
        for (int i = 0; i < 4; ++i) {
            const int areaId = roomDB()->addArea(qsl("random area %1").arg(i));
            QVERIFY(areaId > 0);
            areaIds.append(areaId);
        }

        SmallRandom random(20260810u);
        for (int i = 0; i < 60; ++i) {
            const int roomId = 200 + i;
            QVERIFY(map()->addRoom(roomId));
            QVERIFY(map()->setRoomArea(roomId, areaIds.at(random.next(areaIds.count()))));
            QVERIFY(map()->setRoomCoordinates(roomId, random.next(7) - 3, random.next(7) - 3, random.next(3) - 1));
            roomIds.append(roomId);
        }
        for (int i = 0; i < 80; ++i) {
            const int from = roomIds.at(random.next(roomIds.count()));
            const int to = roomIds.at(random.next(roomIds.count()));
            if (from == to) {
                continue;
            }
            if (random.next(4) == 0) {
                roomDB()->getRoom(from)->setSpecialExit(to, qsl("special%1").arg(i));
            } else {
                QVERIFY(map()->setExit(from, to, DIR_NORTH + (i % 8)));
            }
        }

        QString mismatch;
        QVERIFY2(everyAreaMatchesFullRecalculation(mismatch), qPrintable(mismatch));
    }

    void applyRandomOperation(SmallRandom& random, const QList<int>& areaIds, const QList<int>& roomIds, int step)
    {
        const int roomId = roomIds.at(random.next(roomIds.count()));
        switch (random.next(3)) {
        case 0:
            QVERIFY(map()->setRoomArea(roomId, areaIds.at(random.next(areaIds.count()))));
            break;
        case 1:
            QVERIFY(map()->setRoomCoordinates(roomId, random.next(9) - 4, random.next(9) - 4, random.next(3) - 1));
            break;
        default:
            QVERIFY(map()->setExit(roomId, roomIds.at(random.next(roomIds.count())), DIR_NORTH + (step % 8)));
            break;
        }
    }

    // Returns the nanoseconds spent per room, or 0.0 if anything went wrong.
    double buildRoomsIntoOneArea(int roomCount)
    {
        map()->mapClear();
        const int areaId = roomDB()->addArea(qsl("bulk area"));
        if (areaId <= 0) {
            return 0.0;
        }

        QElapsedTimer timer;
        timer.start();
        for (int i = 0; i < roomCount; ++i) {
            const int roomId = 1 + i;
            if (!map()->addRoom(roomId) || !map()->setRoomCoordinates(roomId, i % 128, i / 128, 0) || !map()->setRoomArea(roomId, areaId)) {
                return 0.0;
            }
        }
        return static_cast<double>(timer.nsecsElapsed()) / roomCount;
    }

    void deleteProfileDirectory(const QString& profileName)
    {
        const QString path = mudlet::getMudletPath(enums::profileHomePath, profileName);
        QDir dir(path);
        if (dir.exists()) {
            dir.removeRecursively();
        }
    }

    // setupConfig() consults portable.txt before the XDG logic
    static bool portableMarkerPresent()
    {
        return QFileInfo::exists(qsl("%1/portable.txt").arg(QCoreApplication::applicationDirPath())) || QFileInfo::exists(qsl("%1/.config/mudlet/portable.txt").arg(QDir::homePath()));
    }

private slots:
    void initTestCase()
    {
        if (portableMarkerPresent()) {
            QSKIP("portable.txt present - it takes precedence over XDG_CONFIG_HOME, so the config dir cannot be redirected");
        }

        // A config root of this process's own. Sharing the developer's
        // ~/.config/mudlet means sharing a profile list, so a second copy of
        // this test running at the same time is told the name it types is
        // already in use and never gets an enabled Connect button. Since #9712
        // the opt-in that makes setupConfig() adopt a directory is
        // $XDG_CONFIG_HOME/mudlet/profiles, not the mudlet directory alone.
        QVERIFY(mConfigDir.isValid());
        QVERIFY(QDir().mkpath(qsl("%1/mudlet/profiles").arg(mConfigDir.path())));
        mSavedXdg = qgetenv("XDG_CONFIG_HOME");
        qputenv("XDG_CONFIG_HOME", mConfigDir.path().toUtf8());

        mudlet::start();
        mudlet::self()->setupConfig();
        QCOMPARE(mudlet::getMudletPath(enums::mainPath), qsl("%1/mudlet").arg(mConfigDir.path()));
        mudlet::self()->takeOwnershipOfInstanceCoordinator(std::make_unique<MudletInstanceCoordinator>("MudletInstanceCoordinator"));
        mudlet::self()->init();
        mudlet::self()->setStorePasswordsSecurely(false);
        deleteProfileDirectory(mProfileName);

        QVERIFY(mSaveDir.isValid());

        auto& hostManager = mudlet::self()->getHostManager();
        QVERIFY2(hostManager.addHost(mProfileName, qsl("23"), QString(), QString()), "failed to create the Host");
        mpHost = hostManager.getHost(mProfileName);
        QVERIFY(mpHost);
        QVERIFY(map());
    }

    void cleanupTestCase()
    {
        // Null when initTestCase skipped or failed ahead of mudlet::start(), and
        // getMudletPath() dereferences the instance rather than checking it
        if (mudlet::self()) {
            deleteProfileDirectory(mProfileName);
        }
        mSavedXdg.isNull() ? qunsetenv("XDG_CONFIG_HOME") : qputenv("XDG_CONFIG_HOME", mSavedXdg);
    }

    // The room, its coordinates and its exits all end up where they should.
    void roomChangesArea()
    {
        int areaA = 0;
        int areaB = 0;
        buildTwoAreaMap(areaA, areaB);

        TArea* pAreaA = roomDB()->getArea(areaA);
        TArea* pAreaB = roomDB()->getArea(areaB);
        QVERIFY(pAreaA);
        QVERIFY(pAreaB);
        TRoom* pRoom = roomDB()->getRoom(104);
        QVERIFY(pRoom);
        const int z = pRoom->z();

        QVERIFY(pAreaA->getAreaRooms().contains(104));
        QVERIFY(pAreaA->getRoomsForZ(z).contains(104));

        QVERIFY(map()->setRoomArea(104, areaB));

        QCOMPARE(pRoom->getArea(), areaB);
        QVERIFY(!pAreaA->getAreaRooms().contains(104));
        QVERIFY(!pAreaA->getRoomsForZ(z).contains(104));
        QVERIFY(pAreaB->getAreaRooms().contains(104));
        QVERIFY(pAreaB->getRoomsForZ(z).contains(104));
        QVERIFY(pAreaB->getGridIndex().roomsAt(z, pRoom->x(), pRoom->y()).contains(104));

        // The room's own exits and stubs are none of the area's business:
        QCOMPARE(pRoom->getEast(), 105);
        QCOMPARE(pRoom->getWest(), 103);
        QVERIFY(pRoom->exitStubs.contains(DIR_NORTH));

        // Its neighbours in the area it left now exit that area through it,
        // and it exits its new area through them:
        QVERIFY(pAreaA->getAreaExitRoomIds().contains(103));
        QVERIFY(pAreaA->getAreaExitRoomIds().contains(105));
        QVERIFY(pAreaB->getAreaExitRoomIds().contains(104));

        QString mismatch;
        QVERIFY2(everyAreaMatchesFullRecalculation(mismatch), qPrintable(mismatch));
    }

    // Special exits are decided by the area of the room they lead to, so a
    // move has to be reflected in the records of the rooms leading to it.
    void specialExitsFollowTheMovedRoom()
    {
        int areaA = 0;
        int areaB = 0;
        buildTwoAreaMap(areaA, areaB);

        TArea* pAreaA = roomDB()->getArea(areaA);
        QVERIFY(pAreaA);
        // Room 103 (area A) has a special exit to room 115 (area B), so it is
        // an area exit of A until 115 joins A too:
        QVERIFY(pAreaA->getAreaExitRoomIds().contains(103));

        QVERIFY(map()->setRoomArea(115, areaA));
        QString mismatch;
        QVERIFY2(everyAreaMatchesFullRecalculation(mismatch), qPrintable(mismatch));

        QVERIFY(map()->setRoomArea(115, areaB));
        QVERIFY(pAreaA->getAreaExitRoomIds().contains(103));
        QVERIFY2(everyAreaMatchesFullRecalculation(mismatch), qPrintable(mismatch));
    }

    // Emptying an area and moving every room back again.
    void areaEmptiedAndRefilled()
    {
        int areaA = 0;
        int areaB = 0;
        buildTwoAreaMap(areaA, areaB);

        TArea* pAreaA = roomDB()->getArea(areaA);
        QVERIFY(pAreaA);
        const QList<int> areaARooms = pAreaA->getAreaRooms().values();
        QVERIFY(areaARooms.count() > 1);
        for (int i = 0; i < areaARooms.count() - 1; ++i) {
            QVERIFY(map()->setRoomArea(areaARooms.at(i), areaB));
        }

        const int maxXOfTheLastRoom = pAreaA->max_x;
        const int maxYOfTheLastRoom = pAreaA->max_y;
        QVERIFY(map()->setRoomArea(areaARooms.constLast(), areaB));
        QVERIFY(pAreaA->getAreaRooms().isEmpty());
        QVERIFY(pAreaA->zLevels.isEmpty());
        QVERIFY(pAreaA->xminForZ.isEmpty());
        QVERIFY(pAreaA->ymaxForZ.isEmpty());
        // The overall extents are the one thing an emptied area holds on to,
        // and they are written into the map file, so they are pinned here
        // rather than only compared against the rescan (which keeps them too):
        QCOMPARE(pAreaA->max_x, maxXOfTheLastRoom);
        QCOMPARE(pAreaA->max_y, maxYOfTheLastRoom);
        QString mismatch;
        QVERIFY2(everyAreaMatchesFullRecalculation(mismatch), qPrintable(mismatch));

        for (const int roomId : areaARooms) {
            QVERIFY(map()->setRoomArea(roomId, areaA));
        }
        QCOMPARE(pAreaA->getAreaRooms().count(), areaARooms.count());
        QVERIFY2(everyAreaMatchesFullRecalculation(mismatch), qPrintable(mismatch));
    }

    // The default area is the one every room starts out claiming to be in
    // without ever being added to it, so it is the one most likely to collect
    // rooms it does not hold.
    void movesToAndFromTheDefaultArea()
    {
        int areaA = 0;
        int areaB = 0;
        buildTwoAreaMap(areaA, areaB);

        TArea* pDefaultArea = roomDB()->getArea(-1);
        QVERIFY(pDefaultArea);
        QVERIFY2(pDefaultArea->getAreaRooms().isEmpty(), "rooms that were never in the default area must not be recorded in it");
        for (int z = scmProbedZMinimum; z <= scmProbedZMaximum; ++z) {
            QVERIFY2(pDefaultArea->getRoomsForZ(z).isEmpty(), "the default area's Z index must not collect rooms it does not hold");
            QVERIFY(pDefaultArea->getGridIndex().roomsInViewport(z, -scmProbedCoordinateLimit, scmProbedCoordinateLimit, -scmProbedCoordinateLimit, scmProbedCoordinateLimit).isEmpty());
        }

        QVERIFY(map()->setRoomArea(104, -1));
        QCOMPARE(roomDB()->getRoom(104)->getArea(), -1);
        QVERIFY(pDefaultArea->getAreaRooms().contains(104));
        QString mismatch;
        QVERIFY2(everyAreaMatchesFullRecalculation(mismatch), qPrintable(mismatch));

        QVERIFY(map()->setRoomArea(104, areaA));
        QVERIFY(pDefaultArea->getAreaRooms().isEmpty());
        QVERIFY2(everyAreaMatchesFullRecalculation(mismatch), qPrintable(mismatch));
    }

    // Moving a room to the area it is already in is a removal and an addition
    // of the same room, which has to leave the coordinate refcounts as it
    // found them.
    void movingARoomToItsOwnArea()
    {
        int areaA = 0;
        int areaB = 0;
        buildTwoAreaMap(areaA, areaB);

        TArea* pAreaA = roomDB()->getArea(areaA);
        QVERIFY(pAreaA);
        const AreaState before = captureAreaState(pAreaA);

        QVERIFY(map()->setRoomArea(104, areaA));
        QVERIFY2(before == captureAreaState(pAreaA), "moving a room to its own area changed the area");
        QString mismatch;
        QVERIFY2(everyAreaMatchesFullRecalculation(mismatch), qPrintable(mismatch));
    }

    // Coordinate changes and room deletions shrink the area extents as much as
    // a rescan would.
    void movedAndDeletedRoomsShrinkTheExtents()
    {
        int areaA = 0;
        int areaB = 0;
        buildTwoAreaMap(areaA, areaB);

        QVERIFY(map()->setRoomCoordinates(107, 4000, -4000, 7));
        QString mismatch;
        QVERIFY2(everyAreaMatchesFullRecalculation(mismatch), qPrintable(mismatch));

        QVERIFY(map()->setRoomCoordinates(107, 1, 1, 0));
        QVERIFY2(everyAreaMatchesFullRecalculation(mismatch), qPrintable(mismatch));

        QSet<int> toRemove{106, 107};
        roomDB()->removeRoom(toRemove);
        QVERIFY(!roomDB()->getArea(areaA)->getAreaRooms().contains(106));
        QVERIFY2(everyAreaMatchesFullRecalculation(mismatch), qPrintable(mismatch));

        // Room 112 is in area B and is what room 102 in area A exits up to, so
        // deleting it retires an area exit record as well as a room:
        QVERIFY(roomDB()->getArea(areaA)->getAreaExitRoomIds().contains(102));
        QVERIFY(roomDB()->removeRoom(112));
        QVERIFY(!roomDB()->getArea(areaB)->getAreaRooms().contains(112));
        QVERIFY2(everyAreaMatchesFullRecalculation(mismatch), qPrintable(mismatch));
    }

    // A long random sequence of the operations that maintain the indexes,
    // checked against a full recalculation after every step so that a failure
    // says which step broke it.
    void randomOperationsMatchFullRecalculation()
    {
        QList<int> areaIds;
        QList<int> roomIds;
        buildRandomMap(areaIds, roomIds);
        if (QTest::currentTestFailed()) {
            return;
        }

        SmallRandom random(1234567u);
        QString mismatch;
        for (int step = 0; step < 300; ++step) {
            applyRandomOperation(random, areaIds, roomIds, step);
            if (QTest::currentTestFailed()) {
                return;
            }
            QVERIFY2(everyAreaMatchesFullRecalculation(mismatch), qPrintable(qsl("step %1: %2").arg(step).arg(mismatch)));
        }
    }

    // The same sequence without the intermediate recalculations, which are a
    // repair as much as a check: only this run can catch bookkeeping that
    // drifts over many operations rather than in one.
    void randomOperationsWithoutIntermediateRecalculation()
    {
        QList<int> areaIds;
        QList<int> roomIds;
        buildRandomMap(areaIds, roomIds);
        if (QTest::currentTestFailed()) {
            return;
        }

        SmallRandom random(7654321u);
        for (int step = 0; step < 300; ++step) {
            applyRandomOperation(random, areaIds, roomIds, step);
            if (QTest::currentTestFailed()) {
                return;
            }
        }

        QString mismatch;
        QVERIFY2(everyAreaMatchesFullRecalculation(mismatch), qPrintable(mismatch));
    }

    // The map file records the area extents and the area exits, so a map built
    // by moving rooms around has to serialize to the same bytes as one whose
    // areas were rebuilt from scratch.
    void savedMapDoesNotDependOnHowTheAreasWereBuilt()
    {
        int areaA = 0;
        int areaB = 0;
        buildTwoAreaMap(areaA, areaB);
        for (int i = 0; i < 9; i += 2) {
            QVERIFY(map()->setRoomArea(100 + i, areaB));
            QVERIFY(map()->setRoomArea(109 + i, areaA));
        }

        const QString incrementalFile = qsl("%1/incremental.dat").arg(mSaveDir.path());
        QVERIFY(saveMapToFile(incrementalFile));

        const QList<int> areaIds = roomDB()->getAreaIDList();
        for (const int areaId : areaIds) {
            if (TArea* pArea = roomDB()->getArea(areaId)) {
                recalculateFully(pArea);
            }
        }

        const QString recalculatedFile = qsl("%1/recalculated.dat").arg(mSaveDir.path());
        QVERIFY(saveMapToFile(recalculatedFile));

        const QByteArray incrementalBytes = fileContents(incrementalFile);
        QVERIFY(!incrementalBytes.isEmpty());
        QCOMPARE(incrementalBytes, fileContents(recalculatedFile));

        map()->mapClear();
        QVERIFY(map()->restore(incrementalFile));
        map()->audit();
        QCOMPARE(roomDB()->getRoomIDList().count(), 18);
        QString mismatch;
        QVERIFY2(everyAreaMatchesFullRecalculation(mismatch), qPrintable(mismatch));

        // A loaded map has to go on being maintained incrementally:
        const int loadedAreaB = roomDB()->getAreaNamesMap().key(qsl("area B"));
        QVERIFY(loadedAreaB > 0);
        for (int i = 0; i < 18; i += 3) {
            QVERIFY(map()->setRoomArea(100 + i, loadedAreaB));
        }
        QVERIFY2(everyAreaMatchesFullRecalculation(mismatch), qPrintable(mismatch));
    }

    // Assigning a room to an area must not rescan the area, whatever the
    // machine's clock says. A room that the area's room set claims but that
    // lives somewhere else is invisible to the incremental bookkeeping and
    // impossible for a rescan to miss, so the extents say which one ran.
    void assigningARoomDoesNotRescanTheArea()
    {
        int areaA = 0;
        int areaB = 0;
        buildTwoAreaMap(areaA, areaB);

        TArea* pAreaA = roomDB()->getArea(areaA);
        QVERIFY(pAreaA);
        TRoom* pStranger = roomDB()->getRoom(117);
        QVERIFY(pStranger);
        QCOMPARE(pStranger->getArea(), areaB);
        QVERIFY(map()->setRoomCoordinates(117, 6000, -6000, 0));

        const int maxXBefore = pAreaA->max_x;
        const int maxYBefore = pAreaA->max_y;
        pAreaA->rooms.insert(117);

        QVERIFY(map()->setRoomArea(105, areaB));
        QVERIFY(map()->setRoomArea(105, areaA));
        QCOMPARE(pAreaA->max_x, maxXBefore);
        QCOMPARE(pAreaA->max_y, maxYBefore);

        // ... and the same probe run through the rescan, to show that it does
        // report the stranger and so could have failed above:
        pAreaA->calcSpan();
        QCOMPARE(pAreaA->max_x, 6000);
        QCOMPARE(pAreaA->max_y, 6000);
    }

    // Reported rather than asserted on: a timing threshold is not dependable
    // on a machine that is running the rest of the test suite at the same
    // time. assigningARoomDoesNotRescanTheArea() is the check that has teeth.
    void bulkAreaAssignmentCostPerRoom()
    {
        // Untimed, so that the first timed run is not the one that pays for
        // warming the allocator up:
        buildRoomsIntoOneArea(scmScalingRoomCounts.constFirst());

        for (const int roomCount : scmScalingRoomCounts) {
            double perRoom = 0.0;
            for (int repeat = 0; repeat < 2; ++repeat) {
                // The best of several runs is the one least disturbed by
                // whatever else the machine is doing:
                const double thisRun = buildRoomsIntoOneArea(roomCount);
                QVERIFY2(thisRun > 0.0, "building the rooms failed");
                perRoom = (repeat == 0) ? thisRun : qMin(perRoom, thisRun);
            }
            qInfo().nospace() << "built " << roomCount << " rooms in " << (perRoom * roomCount * 1.0e-9) << "s (" << (perRoom * 1.0e-3) << "us per room)";
        }
    }
};

#include "MapAreaReassignmentTest.moc"
MUDLET_GROUPED_TEST_MAIN(MapAreaReassignmentTest)
