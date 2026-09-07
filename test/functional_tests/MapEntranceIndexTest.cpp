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
 * Drives TRoomDB through the map mutations that touch the entrance index and
 * emits a canonical digest of the resulting state after every step. The digest
 * is written to $MUDLET_ENTRANCE_DIGEST so the same run can be compared
 * between two builds - a behavioural difference between them shows up as a
 * diff, without either side having to know what the right answer is.
 *
 * Only public TRoomDB API is used, so this compiles against both.
 *
 * Run with: ctest -R MapEntranceIndexTest -V
 */

#include <QtTest/QtTest>

#include <QFile>
#include <QTemporaryDir>
#include <QTextStream>

#include "PortableModeTestHelper.h"
#include "SettingsTestHelper.h"
#include "Host.h"
#include "HostManager.h"
#include "MudletInstanceCoordinator.h"
#include "TArea.h"
#include "TMap.h"
#include "TRoom.h"
#include "TRoomDB.h"
#include "mudlet.h"

#include "GroupedTest.h"

class MapEntranceIndexTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;
    Host* mpHost = nullptr;
    const QString mProfileName = qsl("MapEntranceIndex-Test");
    QStringList mDigest;

    TMap* map() const { return mpHost->mpMap.data(); }
    TRoomDB* roomDB() const { return mpHost->mpMap->mpRoomDB.get(); }

    // Every entranceMap entry, canonically ordered so two runs are comparable.
    QString entranceDigest() const
    {
        const QMultiHash<int, int>& entrances = roomDB()->getEntranceHash();
        QStringList entries;
        for (auto it = entrances.cbegin(); it != entrances.cend(); ++it) {
            entries << qsl("%1<-%2").arg(it.key()).arg(it.value());
        }
        entries.sort();
        return entries.join(qsl(" "));
    }

    // What entranceMap would hold if it were rebuilt from the rooms that
    // actually exist right now. Divergence from entranceDigest() is the stale
    // or missing entry this PR's reverse index could introduce.
    QString groundTruthDigest() const
    {
        QStringList entries;
        const QList<TRoom*> allRooms = roomDB()->getRoomPtrList();
        for (TRoom* pR : allRooms) {
            const QHash<int, int> exits = pR->getExits();
            const QList<int> targets = exits.keys();
            for (const int target : targets) {
                entries << qsl("%1<-%2").arg(target).arg(pR->getId());
            }
        }
        entries.sort();
        entries.removeDuplicates();
        return entries.join(qsl(" "));
    }

    // Every exit every surviving room still holds, so a phantom exit left
    // pointing at a deleted room is visible directly rather than only through
    // the index.
    QString exitDigest() const
    {
        QStringList entries;
        const QList<TRoom*> allRooms = roomDB()->getRoomPtrList();
        for (TRoom* pR : allRooms) {
            const QHash<int, int> exits = pR->getExits();
            QList<int> targets = exits.keys();
            std::sort(targets.begin(), targets.end());
            QStringList targetTexts;
            for (const int target : targets) {
                targetTexts << QString::number(target);
            }
            entries << qsl("%1:[%2]").arg(pR->getId()).arg(targetTexts.join(qsl(",")));
        }
        entries.sort();
        return entries.join(qsl(" "));
    }

    QString areaDigest() const
    {
        QStringList entries;
        const QList<int> areaIds = roomDB()->getAreaIDList();
        for (const int areaId : areaIds) {
            TArea* pA = roomDB()->getArea(areaId);
            if (!pA) {
                continue;
            }
            QList<int> areaRooms = pA->getAreaRooms().values();
            std::sort(areaRooms.begin(), areaRooms.end());
            QStringList roomTexts;
            for (const int roomId : areaRooms) {
                roomTexts << QString::number(roomId);
            }
            entries << qsl("area%1:[%2]").arg(areaId).arg(roomTexts.join(qsl(",")));
        }
        entries.sort();
        return entries.join(qsl(" "));
    }

    // A phantom exit is one pointing at a room that no longer exists. The
    // pre-existing code tolerates spurious *index* entries, but a surviving
    // room holding an exit to a deleted room is user-visible.
    QString phantomExitDigest() const
    {
        QStringList entries;
        const QList<TRoom*> allRooms = roomDB()->getRoomPtrList();
        for (TRoom* pR : allRooms) {
            const QHash<int, int> exits = pR->getExits();
            const QList<int> targets = exits.keys();
            for (const int target : targets) {
                if (target > 0 && !roomDB()->hasRoom(target)) {
                    entries << qsl("%1->%2").arg(pR->getId()).arg(target);
                }
            }
        }
        entries.sort();
        return entries.join(qsl(" "));
    }

    void record(const QString& step)
    {
        mDigest << qsl("== %1").arg(step);
        mDigest << qsl("  entrance : %1").arg(entranceDigest());
        mDigest << qsl("  truth    : %1").arg(groundTruthDigest());
        mDigest << qsl("  exits    : %1").arg(exitDigest());
        mDigest << qsl("  areas    : %1").arg(areaDigest());
        mDigest << qsl("  phantom  : %1").arg(phantomExitDigest());
    }

    void makeRoom(int id, int areaId, int x, int y, int z)
    {
        QVERIFY(map()->addRoom(id));
        QVERIFY(map()->setRoomCoordinates(id, x, y, z));
        QVERIFY(map()->setRoomArea(id, areaId));
    }

    void link(int fromId, int toId, int direction)
    {
        TRoom* pR = roomDB()->getRoom(fromId);
        QVERIFY(pR);
        QVERIFY(pR->setExit(toId, direction));
    }

    void special(int fromId, int toId, const QString& cmd)
    {
        TRoom* pR = roomDB()->getRoom(fromId);
        QVERIFY(pR);
        pR->setSpecialExit(toId, cmd);
    }

private slots:
    void initTestCase()
    {
        if (portableMarkerPresent()) {
            QSKIP("portable.txt present - it takes precedence over XDG_CONFIG_HOME, so the config dir cannot be redirected");
        }

        QVERIFY(mConfigDir.isValid());
        QVERIFY(QDir().mkpath(qsl("%1/mudlet/profiles").arg(mConfigDir.path())));
        mSavedXdg = qgetenv("XDG_CONFIG_HOME");
        qputenv("XDG_CONFIG_HOME", mConfigDir.path().toUtf8());

        mudlet::start();
        mudlet::self()->setupConfig();
        mudlet::self()->takeOwnershipOfInstanceCoordinator(std::make_unique<MudletInstanceCoordinator>("MudletInstanceCoordinator"));
        mudlet::self()->init();
        mudlet::self()->setStorePasswordsSecurely(false);
        deleteProfileDirectory(mProfileName);

        auto& hostManager = mudlet::self()->getHostManager();
        QVERIFY2(hostManager.addHost(mProfileName, qsl("23"), QString(), QString()), "failed to create the Host");
        mpHost = hostManager.getHost(mProfileName);
        QVERIFY(mpHost);
        QVERIFY(map());
    }

    void cleanupTestCase()
    {
        const QByteArray digestPath = qgetenv("MUDLET_ENTRANCE_DIGEST");
        if (!digestPath.isEmpty()) {
            QFile file(QString::fromUtf8(digestPath));
            if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
                QTextStream stream(&file);
                stream << mDigest.join(QChar::LineFeed) << QChar::LineFeed;
            }
        }
        if (mSavedXdg.isEmpty()) {
            qunsetenv("XDG_CONFIG_HOME");
        } else {
            qputenv("XDG_CONFIG_HOME", mSavedXdg);
        }
    }

    // One scripted run through every mutation that touches the entrance index.
    void entranceIndexTracksTheMap()
    {
        QVERIFY(roomDB()->addArea(1, qsl("first")));
        QVERIFY(roomDB()->addArea(2, qsl("second")));

        // Two areas: 1..12 in area 1, 21..26 in area 2.
        for (int id = 1; id <= 12; ++id) {
            makeRoom(id, 1, id * 10, 0, 0);
        }
        for (int id = 21; id <= 26; ++id) {
            makeRoom(id, 2, id * 10, 100, 0);
        }
        record(qsl("rooms built"));

        // A chain of two-way normal exits, plus cross-area links and a couple
        // of rooms every other room points at, so a deletion has plenty of
        // incoming edges to clear.
        for (int id = 1; id <= 11; ++id) {
            link(id, id + 1, DIR_NORTH);
            link(id + 1, id, DIR_SOUTH);
        }
        for (int id = 21; id <= 25; ++id) {
            link(id, id + 1, DIR_EAST);
            link(id + 1, id, DIR_WEST);
        }
        link(12, 21, DIR_UP);
        link(21, 12, DIR_DOWN);
        for (int id = 2; id <= 10; ++id) {
            link(id, 6, DIR_NORTHEAST);
        }
        record(qsl("normal exits set"));

        // Special exits, including several into one room and one crossing areas.
        for (int id = 3; id <= 9; ++id) {
            special(id, 7, qsl("portal%1").arg(id));
        }
        special(4, 23, qsl("crossarea"));
        special(23, 4, qsl("crossback"));
        record(qsl("special exits set"));

        // Repoint an existing exit: the old entrance entry has to go away.
        link(5, 11, DIR_NORTHEAST);
        special(6, 8, qsl("portal6"));
        record(qsl("exits repointed"));

        // Remove exits outright.
        link(7, -1, DIR_NORTHEAST);
        special(8, -1, qsl("portal8"));
        record(qsl("exits removed"));

        // Exit stubs alongside real exits.
        TRoom* pStubRoom = roomDB()->getRoom(9);
        QVERIFY(pStubRoom);
        pStubRoom->setExitStub(DIR_WEST, true);
        record(qsl("exit stub set"));

        // Single-room deletion, the shape deleteRoom() takes: room 6 has nine
        // rooms pointing at it.
        QVERIFY(roomDB()->removeRoom(6));
        record(qsl("single room 6 deleted"));

        // Several single deletions in a row - the path the old static
        // snapshot in __removeRoom() reset itself on every call for.
        QVERIFY(roomDB()->removeRoom(2));
        QVERIFY(roomDB()->removeRoom(3));
        QVERIFY(roomDB()->removeRoom(4));
        record(qsl("rooms 2,3,4 deleted one at a time"));

        // clearSpecialExits() on a room that still has some.
        TRoom* pClearRoom = roomDB()->getRoom(5);
        QVERIFY(pClearRoom);
        pClearRoom->clearSpecialExits();
        record(qsl("special exits cleared on 5"));

        // Moving a room between areas.
        QVERIFY(map()->setRoomArea(11, 2));
        record(qsl("room 11 moved to area 2"));

        // Bulk deletion, the shape deleteArea() takes.
        QVERIFY(roomDB()->removeArea(2));
        record(qsl("area 2 deleted"));

        // Rebuild on top of the wreckage, reusing ids that were deleted - a
        // stale reverse-index entry for a reused id would surface here.
        QVERIFY(roomDB()->addArea(3, qsl("third")));
        for (int id = 2; id <= 4; ++id) {
            makeRoom(id, 3, id * 10, 200, 0);
        }
        link(2, 3, DIR_NORTH);
        link(3, 4, DIR_NORTH);
        link(4, 5, DIR_NORTH);
        link(5, 4, DIR_SOUTH);
        special(2, 5, qsl("reused"));
        record(qsl("rooms rebuilt with reused ids"));

        // And delete them again, so the reused ids go round a second time.
        QVERIFY(roomDB()->removeRoom(3));
        record(qsl("reused room 3 deleted"));

        // The invariant that matters to a user, checked directly rather than
        // through the digest: nothing still points at a room that is gone.
        QCOMPARE(phantomExitDigest(), QString());
    }
};

#include "MapEntranceIndexTest.moc"
MUDLET_GROUPED_TEST_MAIN(MapEntranceIndexTest)
