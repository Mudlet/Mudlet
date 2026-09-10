/***************************************************************************
 *   Copyright (C) 2026 by the Mudlet authors                              *
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
 *   Free Software Foundation, Inc.,                                         *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

/*
 * slot_spread() used to scale each selected room's offset from the highlighted
 * centre room by a factor. That is a no-op when every selected room sits on
 * top of that centre room - its offset is zero, and zero times any factor is
 * still zero - so "Spread..." did nothing to a stack of coincident rooms, which
 * is exactly when a user reaches for it. The fix detects the all-coincident
 * case and instead fans the rooms out onto a sequence of small integer offsets
 * around the centre.
 *
 * The selection is private to the mapper widget and there is no Lua way to
 * make one, so this cannot be a busted spec.
 *
 * Run with: ctest -R MapSpreadCoincidentTest -V
 */

#include <QFileInfo>
#include <QInputDialog>
#include <QTemporaryDir>
#include <QTimer>
#include <QtTest/QtTest>

#include <algorithm>

#include "Host.h"
#include "HostManager.h"
#include "MudletInstanceCoordinator.h"
#include "T2DMap.h"
#include "TMap.h"
#include "TRoom.h"
#include "TRoomDB.h"
#include "mudlet.h"

#include "GroupedTest.h"

class MapSpreadCoincidentTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;
    Host* mpHost = nullptr;
    T2DMap* mp2dMap = nullptr;
    const QString mProfileName = qsl("MapSpreadCoincident-Test");
    const QString mAreaName = qsl("stack area");
    int mAreaId = 0;

    // Eight rooms all sharing the centre coordinate - the degenerate "stacked
    // on top of each other" case the fix targets.
    static constexpr int scmCentreRoom = 1;
    static constexpr int scmSpreadFactor = 3;
    static constexpr int scmStackX = 7;
    static constexpr int scmStackY = -4;
    const QList<int> mRoomIds{scmCentreRoom, 2, 3, 4, 5, 6, 7, 8};

    static bool portableMarkerPresent()
    {
        return QFileInfo::exists(qsl("%1/portable.txt").arg(QCoreApplication::applicationDirPath()))
               || QFileInfo::exists(qsl("%1/.config/mudlet/portable.txt").arg(QDir::homePath()));
    }

    TMap* map() const { return mpHost->mpMap.data(); }
    TRoomDB* roomDB() const { return mpHost->mpMap->mpRoomDB.get(); }

    void deleteProfileDirectory() const
    {
        QDir dir(mudlet::getMudletPath(enums::profileHomePath, mProfileName));
        if (dir.exists()) {
            dir.removeRecursively();
        }
    }

    void buildStackedMap()
    {
        map()->mapClear();
        QVERIFY(roomDB()->getRoomIDList().isEmpty());

        mAreaId = roomDB()->addArea(mAreaName);
        QVERIFY(mAreaId > 0);

        for (const int roomId : mRoomIds) {
            QVERIFY(map()->addRoom(roomId));
            QVERIFY(map()->setRoomArea(roomId, mAreaId));
            QVERIFY(map()->setRoomCoordinates(roomId, scmStackX, scmStackY, 0));
        }

        mp2dMap->switchArea(mAreaName);
        QCOMPARE(mp2dMap->mAreaID, mAreaId);
    }

    void selectAllRooms()
    {
        mp2dMap->mMultiSelectionSet = QSet<int>(mRoomIds.begin(), mRoomIds.end());
        QVERIFY(mp2dMap->getCenterSelection());
        QVERIFY(mp2dMap->mMultiSelectionSet.contains(mp2dMap->getCenterSelectedRoomId()));
    }

    // slot_spread() pops a modal QInputDialog::getInt(). Answer it from a
    // queued callback so the call returns with our chosen factor.
    void answerSpreadDialog(int value)
    {
        QTimer::singleShot(0, qApp, [value]() {
            auto* pDialog = qobject_cast<QInputDialog*>(QApplication::activeModalWidget());
            if (!pDialog) {
                // Fall back to scanning top-level widgets in case the modal
                // has not been activated yet on the offscreen platform.
                for (auto* pWidget : QApplication::topLevelWidgets()) {
                    if ((pDialog = qobject_cast<QInputDialog*>(pWidget))) {
                        break;
                    }
                }
            }
            QVERIFY2(pDialog, "the Spread dialog did not appear");
            pDialog->setIntValue(value);
            pDialog->accept();
        });
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
        QCOMPARE(mudlet::getMudletPath(enums::mainPath), qsl("%1/mudlet").arg(mConfigDir.path()));
        mudlet::self()->takeOwnershipOfInstanceCoordinator(std::make_unique<MudletInstanceCoordinator>("MudletInstanceCoordinator"));
        mudlet::self()->init();
        mudlet::self()->setStorePasswordsSecurely(false);
        deleteProfileDirectory();

        auto& hostManager = mudlet::self()->getHostManager();
        QVERIFY2(hostManager.addHost(mProfileName, qsl("23"), QString(), QString()), "failed to create the Host");
        mpHost = hostManager.getHost(mProfileName);
        QVERIFY(mpHost);
        QVERIFY(map());

        mp2dMap = new T2DMap();
        mp2dMap->mpMap = map();
        mp2dMap->mpHost = mpHost;
    }

    void cleanupTestCase()
    {
        delete mp2dMap;
        mp2dMap = nullptr;
        if (mudlet::self()) {
            deleteProfileDirectory();
        }
        mSavedXdg.isNull() ? qunsetenv("XDG_CONFIG_HOME") : qputenv("XDG_CONFIG_HOME", mSavedXdg);
    }

    void spreadFansOutCoincidentRooms()
    {
        buildStackedMap();
        selectAllRooms();

        const int centreRoomId = mp2dMap->getCenterSelectedRoomId();
        QVERIFY(centreRoomId > 0);

        answerSpreadDialog(scmSpreadFactor);
        mp2dMap->slot_spread();

        // The highlighted centre room is the anchor and must not move.
        const TRoom* pCentre = roomDB()->getRoom(centreRoomId);
        QVERIFY(pCentre);
        QCOMPARE(pCentre->x(), scmStackX);
        QCOMPARE(pCentre->y(), scmStackY);

        // Collect every selected room's (x, y) after the spread.
        QHash<QPair<int, int>, int> occupiedCells;
        for (const int roomId : mRoomIds) {
            const TRoom* pRoom = roomDB()->getRoom(roomId);
            QVERIFY(pRoom);
            const auto key = qMakePair(pRoom->x(), pRoom->y());
            QVERIFY2(!occupiedCells.contains(key),
                     qPrintable(qsl("rooms %1 and %2 collided on (%3,%4) after spread")
                                        .arg(occupiedCells.value(key))
                                        .arg(roomId)
                                        .arg(key.first)
                                        .arg(key.second)));
            occupiedCells.insert(key, roomId);
        }

        // No room may remain stuck on the original stack coordinate except the
        // anchor itself - otherwise "spread" did not actually separate them.
        int coincidentCount = 0;
        for (const int roomId : mRoomIds) {
            const TRoom* pRoom = roomDB()->getRoom(roomId);
            if (pRoom->x() == scmStackX && pRoom->y() == scmStackY) {
                ++coincidentCount;
            }
        }
        QCOMPARE(coincidentCount, 1);

        // Every moved room must lie on an integral multiple of the spread factor
        // away from the centre along the axes the integer sequence uses, i.e.
        // its offset is divisible by the factor.
        for (const int roomId : mRoomIds) {
            if (roomId == centreRoomId) {
                continue;
            }
            const TRoom* pRoom = roomDB()->getRoom(roomId);
            QCOMPARE((pRoom->x() - scmStackX) % scmSpreadFactor, 0);
            QCOMPARE((pRoom->y() - scmStackY) % scmSpreadFactor, 0);
        }
    }

    // The dialog's minimum is 1, and for a coincident stack that is the natural
    // "shuffle them onto adjacent cells" choice - the smallest move that
    // actually separates the rooms. It must not be swallowed as a no-op the way
    // the scaling path treats a factor of 1.
    void spreadFactorOneSeparatesCoincidentRooms()
    {
        buildStackedMap();
        selectAllRooms();

        answerSpreadDialog(1);
        mp2dMap->slot_spread();

        // Every room must have left the stack coordinate except the anchor -
        // a factor of 1 places the movable rooms on the first ring, one cell out.
        int coincidentCount = 0;
        QHash<QPair<int, int>, int> occupiedCells;
        for (const int roomId : mRoomIds) {
            const TRoom* pRoom = roomDB()->getRoom(roomId);
            QVERIFY(pRoom);
            if (pRoom->x() == scmStackX && pRoom->y() == scmStackY) {
                ++coincidentCount;
            }
            const auto key = qMakePair(pRoom->x(), pRoom->y());
            QVERIFY2(!occupiedCells.contains(key),
                     qPrintable(qsl("rooms %1 and %2 collided on (%3,%4) at factor 1")
                                        .arg(occupiedCells.value(key))
                                        .arg(roomId)
                                        .arg(key.first)
                                        .arg(key.second)));
            occupiedCells.insert(key, roomId);
        }
        QCOMPARE(coincidentCount, 1);

        // At factor 1 the moved rooms sit exactly one cell away from the anchor
        // (the ring-1 offsets are all unit vectors or their sums, magnitude 1).
        for (const int roomId : mRoomIds) {
            const TRoom* pRoom = roomDB()->getRoom(roomId);
            if (pRoom->x() == scmStackX && pRoom->y() == scmStackY) {
                continue;
            }
            const int chebyshev = qMax(qAbs(pRoom->x() - scmStackX), qAbs(pRoom->y() - scmStackY));
            QCOMPARE(chebyshev, 1);
        }
    }

    // Sanity check: the non-coincident path is untouched - spread still scales
    // existing offsets by the factor.
    void spreadStillScalesNonCoincidentRooms()
    {
        buildStackedMap();
        // Space two rooms out from the stack centre so they are NOT coincident.
        QVERIFY(map()->setRoomCoordinates(2, scmStackX + 2, scmStackY, 0));
        QVERIFY(map()->setRoomCoordinates(3, scmStackX, scmStackY + 4, 0));

        selectAllRooms();
        const int centreRoomId = mp2dMap->getCenterSelectedRoomId();
        QVERIFY(centreRoomId > 0);
        const TRoom* pCentreBefore = roomDB()->getRoom(centreRoomId);
        const int cx = pCentreBefore->x();
        const int cy = pCentreBefore->y();
        const int room2OffsetXBefore = roomDB()->getRoom(2)->x() - cx;
        const int room3OffsetYBefore = roomDB()->getRoom(3)->y() - cy;

        answerSpreadDialog(2);
        mp2dMap->slot_spread();

        const TRoom* pRoom2 = roomDB()->getRoom(2);
        const TRoom* pRoom3 = roomDB()->getRoom(3);
        QCOMPARE(pRoom2->x() - cx, room2OffsetXBefore * 2);
        QCOMPARE(pRoom3->y() - cy, room3OffsetYBefore * 2);
    }

    // Spreading a stack must not drop a room onto a cell a different room
    // already occupies: the sequence advances past occupied candidates.
    void spreadSkipsOccupiedCells()
    {
        buildStackedMap();

        // A pre-existing room - not part of the selection - parked exactly
        // where the first offset (due north, distance = spread) would land.
        constexpr int scmForeignRoom = 999;
        QVERIFY(map()->addRoom(scmForeignRoom));
        QVERIFY(map()->setRoomArea(scmForeignRoom, mAreaId));
        const int occupiedX = scmStackX;
        const int occupiedY = scmStackY + scmSpreadFactor;
        QVERIFY(map()->setRoomCoordinates(scmForeignRoom, occupiedX, occupiedY, 0));

        selectAllRooms();
        QVERIFY(!mp2dMap->mMultiSelectionSet.contains(scmForeignRoom));

        answerSpreadDialog(scmSpreadFactor);
        mp2dMap->slot_spread();

        // The foreign room is untouched.
        const TRoom* pForeign = roomDB()->getRoom(scmForeignRoom);
        QVERIFY(pForeign);
        QCOMPARE(pForeign->x(), occupiedX);
        QCOMPARE(pForeign->y(), occupiedY);

        // No selected room may have landed on the occupied cell.
        for (const int roomId : mRoomIds) {
            const TRoom* pRoom = roomDB()->getRoom(roomId);
            QVERIFY(pRoom);
            QVERIFY2(!(pRoom->x() == occupiedX && pRoom->y() == occupiedY),
                     qPrintable(qsl("room %1 was placed onto the occupied cell (%2,%3)")
                                        .arg(roomId)
                                        .arg(occupiedX)
                                        .arg(occupiedY)));
        }

        // Selected rooms must still be pairwise non-colliding after the skip.
        QHash<QPair<int, int>, int> occupiedCells;
        for (const int roomId : mRoomIds) {
            const TRoom* pRoom = roomDB()->getRoom(roomId);
            const auto key = qMakePair(pRoom->x(), pRoom->y());
            QVERIFY2(!occupiedCells.contains(key),
                     qPrintable(qsl("selected rooms %1 and %2 collided on (%3,%4)")
                                        .arg(occupiedCells.value(key))
                                        .arg(roomId)
                                        .arg(key.first)
                                        .arg(key.second)));
            occupiedCells.insert(key, roomId);
        }

        // And none of them may collide with the foreign room either.
        QVERIFY2(!occupiedCells.contains(qMakePair(occupiedX, occupiedY)),
                 "a selected room shares the foreign room's cell");
    }
};

#include "MapSpreadCoincidentTest.moc"

MUDLET_GROUPED_TEST_MAIN(MapSpreadCoincidentTest)
