/***************************************************************************
 *   Copyright (C) 2026 by Jens Wegener - code@jenswegener.net             *
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
 * Nesting an area keeps its two map file fields in the meaning they have
 * everywhere: zoneAreaRef is the area it is nested in (0 at the top level), and
 * isZone marks an area that has others nested in it. The Lua side is covered by
 * MapAreaHierarchy_spec.lua; these check what lands in the fields.
 */

#include <QTemporaryDir>
#include <QtTest/QtTest>

#include "Host.h"
#include "HostManager.h"
#include "MudletApp.h"
#include "MudletInstanceCoordinator.h"
#include "PortableModeTestHelper.h"
#include "SettingsTestHelper.h"
#include "TArea.h"
#include "TMap.h"
#include "TRoomDB.h"
#include "mudlet.h"

#include "GroupedTest.h"

class MapAreaNestingTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;
    Host* mpHost = nullptr;
    const QString mProfileName = qsl("MapAreaNesting-Test");

    TMap* map() const { return mpHost->mpMap.data(); }
    TRoomDB* roomDB() const { return mpHost->mpMap->mpRoomDB.get(); }
    TArea* area(const int id) const { return roomDB()->getArea(id); }

    int newArea(const QString& name) const
    {
        const int id = roomDB()->addArea(name);
        Q_ASSERT(id > 0);
        return id;
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
        QCOMPARE(MudletApp::getMudletPath(enums::mainPath), qsl("%1/mudlet").arg(mConfigDir.path()));
        mudlet::self()->takeOwnershipOfInstanceCoordinator(std::make_unique<MudletInstanceCoordinator>("MudletInstanceCoordinator"));
        mudlet::self()->init();
        mudlet::self()->setStorePasswordsSecurely(false);
        TestSettings::deleteProfileDirectory(mProfileName);

        auto* hostManager = HostManager::self();
        QVERIFY2(hostManager->addHost(mProfileName, qsl("23"), QString(), QString()), "failed to create the Host");
        mpHost = hostManager->getHost(mProfileName);
        QVERIFY(mpHost);
        QVERIFY(map());
    }

    void cleanupTestCase()
    {
        if (mudlet::self()) {
            TestSettings::deleteProfileDirectory(mProfileName);
        }
        mSavedXdg.isNull() ? qunsetenv("XDG_CONFIG_HOME") : qputenv("XDG_CONFIG_HOME", mSavedXdg);
    }

    void cleanup()
    {
        for (const int id : roomDB()->getAreaNamesMap().keys()) {
            if (id > 0) {
                roomDB()->removeArea(id);
            }
        }
    }

    void aNewAreaIsNeitherNestedNorAZone()
    {
        const int id = newArea(qsl("Plain"));

        QCOMPARE(area(id)->zoneAreaRef, 0);
        QVERIFY(!area(id)->isZone);
    }

    void nestingMarksTheChildByItsRefAndTheParentAsAZone()
    {
        const int continent = newArea(qsl("Continent"));
        const int zone = newArea(qsl("Zone"));

        QCOMPARE(roomDB()->setAreaParent(zone, continent), TRoomDB::AreaParentResult::Changed);

        QCOMPARE(area(zone)->zoneAreaRef, continent);
        QVERIFY2(!area(zone)->isZone, "the child has nothing nested in it");
        QVERIFY(area(continent)->isZone);
        QCOMPARE(area(continent)->zoneAreaRef, 0);
    }

    void aZoneStaysOneUntilItsLastChildLeaves()
    {
        const int continent = newArea(qsl("Continent"));
        const int first = newArea(qsl("First"));
        const int second = newArea(qsl("Second"));
        roomDB()->setAreaParent(first, continent);
        roomDB()->setAreaParent(second, continent);

        roomDB()->setAreaParent(first, std::nullopt);
        QVERIFY(area(continent)->isZone);
        QCOMPARE(area(first)->zoneAreaRef, 0);

        roomDB()->setAreaParent(second, std::nullopt);
        QVERIFY(!area(continent)->isZone);
    }

    void movingAChildMakesItsNewParentTheZone()
    {
        const int oldParent = newArea(qsl("Old parent"));
        const int newParent = newArea(qsl("New parent"));
        const int child = newArea(qsl("Child"));
        roomDB()->setAreaParent(child, oldParent);

        roomDB()->setAreaParent(child, newParent);

        QVERIFY(!area(oldParent)->isZone);
        QVERIFY(area(newParent)->isZone);
    }

    void aZoneCanItselfBeNested()
    {
        const int continent = newArea(qsl("Continent"));
        const int zone = newArea(qsl("Zone"));
        const int subzone = newArea(qsl("Subzone"));
        roomDB()->setAreaParent(zone, continent);
        roomDB()->setAreaParent(subzone, zone);

        QVERIFY(area(zone)->isZone);
        QCOMPARE(area(zone)->zoneAreaRef, continent);
    }

    void aParentThatOnlyHadANameGetsAnAreaToBeAZone()
    {
        const int named = 4242;
        QVERIFY(roomDB()->setAreaName(named, qsl("Named only")));
        QVERIFY(!area(named));
        const int child = newArea(qsl("Child"));

        QCOMPARE(roomDB()->setAreaParent(child, named), TRoomDB::AreaParentResult::Changed);

        QVERIFY(area(named));
        QVERIFY(area(named)->isZone);
    }

    void theDefaultAreaCannotBeAParent()
    {
        const int child = newArea(qsl("Child"));

        QCOMPARE(roomDB()->setAreaParent(child, -1), TRoomDB::AreaParentResult::DefaultAreaAsParent);
        QCOMPARE(area(child)->zoneAreaRef, 0);
    }

    void deletingAZoneHandsItsChildrenToItsOwnParent()
    {
        const int continent = newArea(qsl("Continent"));
        const int middle = newArea(qsl("Middle"));
        const int child = newArea(qsl("Child"));
        roomDB()->setAreaParent(middle, continent);
        roomDB()->setAreaParent(child, middle);

        QVERIFY(roomDB()->removeArea(middle));

        QCOMPARE(area(child)->zoneAreaRef, continent);
        QVERIFY(area(continent)->isZone);
    }

    void deletingATopLevelZoneLeavesItsChildrenAtTheTopLevel()
    {
        const int continent = newArea(qsl("Continent"));
        const int child = newArea(qsl("Child"));
        roomDB()->setAreaParent(child, continent);

        QVERIFY(roomDB()->removeArea(continent));

        QCOMPARE(area(child)->zoneAreaRef, 0);
        QVERIFY(!area(child)->isZone);
    }

    void deletingAZonesOnlyChildLeavesItNoZone()
    {
        const int continent = newArea(qsl("Continent"));
        const int child = newArea(qsl("Child"));
        roomDB()->setAreaParent(child, continent);

        QVERIFY(roomDB()->removeArea(child));

        QVERIFY2(!area(continent)->isZone, "the deleted child still counted");
    }

    // setAreaParent() refuses a loop, but a map file can hold one
    void aLoopFromAMapFileEndsTheSubtree()
    {
        const int first = newArea(qsl("First"));
        const int second = newArea(qsl("Second"));
        area(first)->zoneAreaRef = second;
        area(second)->zoneAreaRef = first;

        QCOMPARE(roomDB()->getAreaChildren(first, true), QList<int>{second});
        QCOMPARE(roomDB()->getAreaChildren(second, true), QList<int>{first});
    }

    void aJsonMapStoresTheParentAndTheZonesFollowFromIt()
    {
        const int continent = newArea(qsl("Continent"));
        const int zone = newArea(qsl("Zone"));
        roomDB()->setAreaParent(zone, continent);
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const QString path = dir.filePath(qsl("nesting.json"));

        QVERIFY(map()->writeJsonMapFile(path).first);
        roomDB()->setAreaParent(zone, std::nullopt);
        QVERIFY(map()->readJsonMapFile(path).first);

        QCOMPARE(area(zone)->zoneAreaRef, continent);
        QVERIFY(area(continent)->isZone);
        QVERIFY(!area(zone)->isZone);
    }
};

#include "MapAreaNestingTest.moc"
MUDLET_GROUPED_TEST_MAIN(MapAreaNestingTest)
