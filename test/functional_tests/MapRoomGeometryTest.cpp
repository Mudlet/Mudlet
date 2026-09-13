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
 * Covers the three parts of TMap that answer a question about where a room is
 * and what colour it should be drawn in:
 *
 *   - connectExitStubByDirection(), which picks the nearest room in a direction
 *     that has a stub pointing back, and is the only one of the three with a Lua
 *     route (connectExitStub() with a direction and no target room);
 *   - detectRoomCollisions(), which has no caller at all outside these tests.
 *     Nothing else uses it: the mapper's collision borders come from
 *     TAreaGridIndex::roomsInViewportWithCollisions(), and Lua's
 *     getCollisionLocationsInArea() from TArea::getCollisionNodes();
 *   - getColor(), which only T2DMap's painter and the room properties dialog
 *     call, so nothing reaches its environment-code arithmetic from Lua.
 *
 * Run with: ctest -R MapRoomGeometryTest -V
 */

#include <QFileInfo>
#include <QTemporaryDir>
#include <QtTest/QtTest>

#include <algorithm>

#include "Host.h"
#include "HostManager.h"
#include "MudletInstanceCoordinator.h"
#include "TMap.h"
#include "TRoom.h"
#include "TRoomDB.h"
#include "mudlet.h"

#include "GroupedTest.h"

class MapRoomGeometryTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;
    Host* mpHost = nullptr;
    const QString mProfileName = qsl("MapRoomGeometry-Test");

    // setupConfig() consults portable.txt before the XDG logic
    static bool portableMarkerPresent()
    {
        return QFileInfo::exists(qsl("%1/portable.txt").arg(QCoreApplication::applicationDirPath())) || QFileInfo::exists(qsl("%1/.config/mudlet/portable.txt").arg(QDir::homePath()));
    }

    void deleteProfileDirectory() const
    {
        QDir dir(mudlet::getMudletPath(enums::profileHomePath, mProfileName));
        if (dir.exists()) {
            dir.removeRecursively();
        }
    }

    TMap* map() const { return mpHost->mpMap.data(); }

    int addRoomAt(const int id, const int areaId, const int x, const int y, const int z) const
    {
        return map()->addRoom(id) && map()->setRoomArea(id, areaId) && map()->setRoomCoordinates(id, x, y, z) ? id : 0;
    }

private slots:
    void initTestCase()
    {
        if (portableMarkerPresent()) {
            QSKIP("portable.txt present - it takes precedence over XDG_CONFIG_HOME, so the config dir cannot be redirected");
        }

        // A config root of this process's own, so this never reads or writes
        // the developer's ~/.config/mudlet. Since #9712 the opt-in that makes
        // setupConfig() adopt a directory is $XDG_CONFIG_HOME/mudlet/profiles,
        // not the mudlet directory alone.
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
        QVERIFY2(hostManager.addHost(mProfileName, qsl("23"), QString(), QString()), "failed to create the test Host");
        mpHost = hostManager.getHost(mProfileName);
        QVERIFY(mpHost);
    }

    void cleanupTestCase()
    {
        mpHost = nullptr;
        // Null when initTestCase skipped or failed ahead of mudlet::start(), and
        // getMudletPath() dereferences the instance rather than checking it
        if (mudlet::self()) {
            deleteProfileDirectory();
            delete mudlet::self();
        }
        mSavedXdg.isNull() ? qunsetenv("XDG_CONFIG_HOME") : qputenv("XDG_CONFIG_HOME", mSavedXdg);
    }

    // Every test starts from an empty map, so the room and area ids it makes are
    // its own. mapClear() also puts the sixteen default custom colours back, so
    // each test sees the environment maps a real profile would have.
    void init() { map()->mapClear(); }

    // The pick is the nearest candidate, and a candidate has to be in the same
    // area, on the right side, on the same plane in the axes the direction does
    // not move along, and carry a stub pointing back.
    void test_exitStubConnectsToTheNearestRoomWithAMatchingStub()
    {
        TRoomDB* pDB = map()->mpRoomDB.get();
        const int areaId = pDB->addArea(qsl("Stub Area"));
        QVERIFY(areaId > 0);
        const int otherAreaId = pDB->addArea(qsl("Other Area"));
        QVERIFY(otherAreaId > 0);

        QCOMPARE(addRoomAt(1, areaId, 0, 0, 0), 1);
        // no stub back, so it is passed over even though it is the closest:
        QCOMPARE(addRoomAt(2, areaId, 1, 0, 0), 2);
        // right direction and has a stub back, but a floor up:
        QCOMPARE(addRoomAt(3, areaId, 2, 0, 1), 3);
        // due west, so on the wrong side entirely:
        QCOMPARE(addRoomAt(4, areaId, -2, 0, 0), 4);
        // the nearest room that qualifies:
        QCOMPARE(addRoomAt(5, areaId, 4, 0, 0), 5);
        // nearer than room 5, but in another area:
        QCOMPARE(addRoomAt(6, otherAreaId, 3, 0, 0), 6);
        // qualifies in every way room 5 does, only further off - so it is the
        // distance comparison rather than any of the filters that rejects it:
        QCOMPARE(addRoomAt(7, areaId, 9, 0, 0), 7);

        pDB->getRoom(1)->setExitStub(DIR_EAST, true);
        for (const int id : {3, 4, 5, 6, 7}) {
            pDB->getRoom(id)->setExitStub(DIR_WEST, true);
        }

        QCOMPARE(map()->connectExitStubByDirection(1, DIR_EAST), QString());
        QCOMPARE(pDB->getRoom(1)->getEast(), 5);
        // ...and the room it reached gets the exit back:
        QCOMPARE(pDB->getRoom(5)->getWest(), 1);
        QCOMPARE(pDB->getRoom(6)->getWest(), -1);
        QCOMPARE(pDB->getRoom(7)->getWest(), -1);
    }

    // North is the direction worth testing beside east. scmUnitVectors puts
    // north at -y because that is the drawing direction, while room coordinates
    // have north at +y - so the candidate test for a direction that moves along
    // y accepts the offset whose sign DIFFERS from the unit vector's, the
    // opposite of what the x and z tests do. A room laid out due north has to
    // be found in spite of that.
    void test_exitStubConnectsNorthwardsAcrossTheFlippedYAxis()
    {
        TRoomDB* pDB = map()->mpRoomDB.get();
        const int areaId = pDB->addArea(qsl("Northward Area"));
        QVERIFY(areaId > 0);

        QCOMPARE(addRoomAt(1, areaId, 0, 0, 0), 1);
        // north of room 1:
        QCOMPARE(addRoomAt(2, areaId, 0, 3, 0), 2);
        // south of it, and must not be picked:
        QCOMPARE(addRoomAt(3, areaId, 0, -2, 0), 3);

        pDB->getRoom(1)->setExitStub(DIR_NORTH, true);
        pDB->getRoom(2)->setExitStub(DIR_SOUTH, true);
        pDB->getRoom(3)->setExitStub(DIR_SOUTH, true);

        QCOMPARE(map()->connectExitStubByDirection(1, DIR_NORTH), QString());
        QCOMPARE(pDB->getRoom(1)->getNorth(), 2);
        QCOMPARE(pDB->getRoom(2)->getSouth(), 1);
        QCOMPARE(pDB->getRoom(3)->getSouth(), -1);
    }

    void test_exitStubConnectionReportsAnUnknownRoom()
    {
        const QString error = map()->connectExitStubByDirection(9999, DIR_EAST);
        QVERIFY2(error.contains(qsl("does not exist")), qPrintable(error));
    }

    void test_exitStubConnectionReportsAMissingStub()
    {
        const int areaId = map()->mpRoomDB->addArea(qsl("Stubless Area"));
        QVERIFY(areaId > 0);
        QCOMPARE(addRoomAt(1, areaId, 0, 0, 0), 1);

        const QString error = map()->connectExitStubByDirection(1, DIR_EAST);
        QVERIFY2(error.contains(qsl("does not have an exit stub")), qPrintable(error));
        QVERIFY2(error.contains(qsl("east")), qPrintable(error));
    }

    void test_exitStubConnectionReportsWhenNothingIsInTheWay()
    {
        TRoomDB* pDB = map()->mpRoomDB.get();
        const int areaId = pDB->addArea(qsl("Lonely Area"));
        QVERIFY(areaId > 0);
        QCOMPARE(addRoomAt(1, areaId, 0, 0, 0), 1);
        // Behind, not ahead:
        QCOMPARE(addRoomAt(2, areaId, -3, 0, 0), 2);

        pDB->getRoom(1)->setExitStub(DIR_EAST, true);
        pDB->getRoom(2)->setExitStub(DIR_WEST, true);

        const QString error = map()->connectExitStubByDirection(1, DIR_EAST);
        QVERIFY2(error.contains(qsl("does not have another room in the indicated direction")), qPrintable(error));
        QCOMPARE(pDB->getRoom(1)->getEast(), -1);
    }

    // A room always collides with itself, so the answer for a room standing on
    // its own is a list of one rather than an empty one.
    void test_roomCollisionsFindRoomsSharingCoordinates()
    {
        TRoomDB* pDB = map()->mpRoomDB.get();
        const int areaId = pDB->addArea(qsl("Crowded Area"));
        QVERIFY(areaId > 0);
        const int otherAreaId = pDB->addArea(qsl("Elsewhere"));
        QVERIFY(otherAreaId > 0);

        QCOMPARE(addRoomAt(1, areaId, 2, 3, 4), 1);
        QCOMPARE(addRoomAt(2, areaId, 2, 3, 4), 2);
        // same x and y but a floor away:
        QCOMPARE(addRoomAt(3, areaId, 2, 3, 5), 3);
        QCOMPARE(addRoomAt(4, areaId, 9, 9, 9), 4);
        // exactly on top of room 1, but collisions are an area's own business:
        QCOMPARE(addRoomAt(5, otherAreaId, 2, 3, 4), 5);

        QList<int> collisions = map()->detectRoomCollisions(1);
        std::sort(collisions.begin(), collisions.end());
        QCOMPARE(collisions, QList<int>({1, 2}));

        QCOMPARE(map()->detectRoomCollisions(4), QList<int>({4}));
    }

    void test_roomCollisionsForAnUnknownRoomAreEmpty() { QVERIFY(map()->detectRoomCollisions(9999).isEmpty()); }

    void test_colorOfAnUnknownRoomIsInvalid() { QVERIFY(!map()->getColor(9999).isValid()); }

    // An environment code the map has no entry for in either of its two colour
    // maps falls back to the first of the sixteen built-in colours. That covers
    // the other fifteen built-in codes too, so a room set straight to
    // environment 16 comes out red rather than light black: reaching the
    // built-in codes past the first takes an mEnvColors entry, which the next
    // test uses. A custom colour for the code is the other way past the
    // fallback, which the one after that uses.
    void test_unmappedEnvironmentsFallBackToTheFirstColour()
    {
        const int areaId = map()->mpRoomDB->addArea(qsl("Colour Area"));
        QVERIFY(areaId > 0);
        QCOMPARE(addRoomAt(1, areaId, 0, 0, 0), 1);
        QCOMPARE(addRoomAt(2, areaId, 1, 0, 0), 2);
        QCOMPARE(addRoomAt(3, areaId, 2, 0, 0), 3);

        map()->mpRoomDB->getRoom(1)->environment = 1;
        map()->mpRoomDB->getRoom(2)->environment = 16;
        map()->mpRoomDB->getRoom(3)->environment = 999;

        QCOMPARE(map()->getColor(1), mpHost->mRed_2);
        QCOMPARE(map()->getColor(2), mpHost->mRed_2);
        QCOMPARE(map()->getColor(3), mpHost->mRed_2);
    }

    // mEnvColors maps a game's own environment ids onto the colour codes, so
    // every case below reaches getColor() through that indirection.
    void test_mappedEnvironmentsResolveThroughTheColourCodes()
    {
        const int areaId = map()->mpRoomDB->addArea(qsl("Mapped Colour Area"));
        QVERIFY(areaId > 0);
        for (int id = 1; id <= 6; ++id) {
            QCOMPARE(addRoomAt(id, areaId, id, 0, 0), id);
            map()->mpRoomDB->getRoom(id)->environment = 50 + id;
        }

        map()->mEnvColors[51] = 4;    // one of the sixteen
        map()->mEnvColors[52] = 16;   // the last of the sixteen
        map()->mEnvColors[53] = 200;  // inside the 6x6x6 colour cube
        map()->mEnvColors[54] = 240;  // inside the greyscale ramp
        map()->mEnvColors[55] = 1000; // outside every range there is
        // What a profile using setCustomEnvColor() alongside a game's own
        // environment ids ends up with, the 257-272 set mapClear() restores
        // included: the indirection lands on a code that has a custom colour.
        map()->mEnvColors[56] = 257;

        QCOMPARE(map()->getColor(1), mpHost->mBlue_2);
        QCOMPARE(map()->getColor(2), mpHost->mLightBlack_2);
        // 200 - 16 = 184; 184 / 36 = 5, (184 - 180) / 6 = 0, 184 - 180 = 4;
        // then 5 and 4 scale to (5 - 1) * 40 + 95 = 255 and (4 - 1) * 40 + 95 =
        // 215, while a 0 component stays 0:
        QCOMPARE(map()->getColor(3), QColor(255, 0, 215, 255));
        // (240 - 232) * 10 + 8 = 88:
        QCOMPARE(map()->getColor(4), QColor(88, 88, 88, 255));
        QVERIFY2(!map()->getColor(5).isValid(), "an unrenderable colour code produced a colour anyway");
        QCOMPARE(map()->getColor(6), map()->mCustomEnvColors.value(257));
    }

    void test_customEnvironmentColoursWinOverTheFallback()
    {
        const int areaId = map()->mpRoomDB->addArea(qsl("Custom Colour Area"));
        QVERIFY(areaId > 0);
        QCOMPARE(addRoomAt(1, areaId, 0, 0, 0), 1);
        QCOMPARE(addRoomAt(2, areaId, 1, 0, 0), 2);

        // Not in mEnvColors, so without a custom colour these would both be
        // treated as environment 1:
        map()->mpRoomDB->getRoom(1)->environment = 300;
        map()->mpRoomDB->getRoom(2)->environment = 20;
        map()->mCustomEnvColors[300] = QColor(12, 34, 56);
        map()->mCustomEnvColors[20] = QColor(200, 100, 50);

        QCOMPARE(map()->getColor(1), QColor(12, 34, 56));
        QCOMPARE(map()->getColor(2), QColor(200, 100, 50));
    }
};

#include "MapRoomGeometryTest.moc"
MUDLET_GROUPED_TEST_MAIN(MapRoomGeometryTest)
