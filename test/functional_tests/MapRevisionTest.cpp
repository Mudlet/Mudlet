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
 * TMap::revision() counts every change to the map's contents or colors, which
 * what the 3D view keeps between frames is keyed on, and not the repaints that
 * every move of the player asks for. Built with or without the 3D mapper, so
 * every build checks the calls that count.
 *
 * Why not a spec: no Lua call reports the revision.
 */

#include <QFileInfo>
#include <QTemporaryDir>
#include <QtTest/QtTest>

#include "Host.h"
#include "HostManager.h"
#include "MudletApp.h"
#include "MudletInstanceCoordinator.h"
#include "TArea.h"
#include "TMap.h"
#include "TRoom.h"
#include "TRoomDB.h"
#include "ctelnet.h"
#include "mudlet.h"

#include "GroupedTest.h"

class MapRevisionTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;
    Host* mpHost = nullptr;
    const QString mProfileName = qsl("MapRevision-Test");

    static bool portableMarkerPresent()
    {
        return QFileInfo::exists(qsl("%1/portable.txt").arg(QCoreApplication::applicationDirPath())) || QFileInfo::exists(qsl("%1/.config/mudlet/portable.txt").arg(QDir::homePath()));
    }

    void deleteProfileDirectory() const
    {
        QDir dir(MudletApp::getMudletPath(enums::profileHomePath, mProfileName));
        if (dir.exists()) {
            dir.removeRecursively();
        }
    }

    TMap* map() const { return mpHost->mpMap.data(); }

    bool runLua(const QString& code) const { return mpHost->mLuaInterpreter.compileAndExecuteScript(code); }

    // Rooms 1 and 2 side by side, joined both ways, and room 3 below room 1, in an area of their own; another area
    // to move rooms to. Returns the other area's ID
    int buildMap() const
    {
        map()->mapClear();
        const int areaId = map()->mpRoomDB->addArea(qsl("Revision Area"));
        const int otherAreaId = map()->mpRoomDB->addArea(qsl("Revision Other Area"));
        const QList<std::tuple<int, int, int, int>> rooms{{1, 0, 0, 0}, {2, 2, 0, 0}, {3, 0, 2, -1}};
        for (const auto& [id, x, y, z] : rooms) {
            if (!(map()->addRoom(id) && map()->setRoomArea(id, areaId) && map()->setRoomCoordinates(id, x, y, z))) {
                return 0;
            }
        }
        if (!(map()->setExit(1, 2, DIR_EAST) && map()->setExit(2, 1, DIR_WEST) && map()->setExit(1, 3, DIR_DOWN))) {
            return 0;
        }
        TRoom* pEast = map()->mpRoomDB->getRoom(2);
        pEast->customLines[qsl("w")] = QList<QPointF>{QPointF(1.0, 1.0)};
        pEast->customLinesColor[qsl("w")] = QColor(Qt::green);
        pEast->customLinesStyle[qsl("w")] = Qt::SolidLine;
        pEast->customLinesArrow[qsl("w")] = false;
        map()->mRoomIdHash[map()->mProfileName] = 1;
        return otherAreaId;
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
        deleteProfileDirectory();

        auto* hostManager = HostManager::self();
        QVERIFY2(hostManager->addHost(mProfileName, qsl("23"), QString(), QString()), "failed to create the test Host");
        mpHost = hostManager->getHost(mProfileName);
        QVERIFY(mpHost);
    }

    void cleanupTestCase()
    {
        mpHost = nullptr;
        if (mudlet::self()) {
            deleteProfileDirectory();
            delete mudlet::self();
        }
        mSavedXdg.isNull() ? qunsetenv("XDG_CONFIG_HOME") : qputenv("XDG_CONFIG_HOME", mSavedXdg);
    }

    void test_luaCallsThatChangeTheMapCountItsRevisionUp_data()
    {
        QTest::addColumn<QString>("code");
        QTest::newRow("setExit") << qsl("setExit(2, 3, 'n')");
        QTest::newRow("setDoor") << qsl("setDoor(1, 'e', 2)");
        QTest::newRow("lockExit") << qsl("lockExit(1, 'east', true)");
        QTest::newRow("lockRoom") << qsl("lockRoom(2, true)");
        QTest::newRow("addCustomLine") << qsl("addCustomLine(1, {{1, 1, 0}}, 'e', 'solid line', {255, 0, 0}, false)");
        QTest::newRow("removeCustomLine") << qsl("removeCustomLine(2, 'w')");
        QTest::newRow("setExitStub") << qsl("setExitStub(2, 'n', true)");
        QTest::newRow("addSpecialExit") << qsl("addSpecialExit(1, 3, 'jump')");
        QTest::newRow("setRoomEnv") << qsl("setRoomEnv(2, 5)");
        QTest::newRow("setCustomEnvColor") << qsl("setCustomEnvColor(300, 10, 200, 10, 255)");
        QTest::newRow("setRoomCoordinates") << qsl("setRoomCoordinates(2, 4, 2, 0)");
        QTest::newRow("deleteRoom") << qsl("deleteRoom(2)");
        QTest::newRow("setRoomArea") << qsl("setRoomArea(2, %1)");
        QTest::newRow("setMapEnvironmentKit") << qsl("setMapEnvironmentKit(300, 'water')");
        QTest::newRow("setMapUserData") << qsl("setMapUserData('perf.test', '1')");
        QTest::newRow("clearMapUserDataItem") << qsl("setMapUserData('perf.test', '1') clearMapUserDataItem('perf.test')");
        QTest::newRow("auditAreas") << qsl("auditAreas()");
        QTest::newRow("loadMap") << qsl("local file = getMudletHomeDir() .. '/revision-test.dat' assert(saveMap(file)) assert(loadMap(file))");
    }

    void test_luaCallsThatChangeTheMapCountItsRevisionUp()
    {
        QFETCH(QString, code);
        const int otherAreaId = buildMap();
        QVERIFY(otherAreaId > 0);
        const quint64 before = map()->revision();
        QVERIFY(runLua(code.arg(otherAreaId)));
        QVERIFY2(map()->revision() > before, qPrintable(code));
    }

    void test_movesOfThePlayerLeaveTheRevisionAsItWas_data()
    {
        QTest::addColumn<QString>("code");
        QTest::newRow("centerview") << qsl("centerview(2)");
        QTest::newRow("gotoRoom") << qsl("gotoRoom(2)");
        QTest::newRow("updateMap") << qsl("updateMap()");
    }

    void test_movesOfThePlayerLeaveTheRevisionAsItWas()
    {
        QFETCH(QString, code);
        QVERIFY(buildMap() > 0);
        const quint64 before = map()->revision();
        QVERIFY(runLua(code));
        QCOMPARE(map()->revision(), before);
    }

    void test_roomUserDataTheViewsDrawCountsTheRevisionsUp_data()
    {
        QTest::addColumn<QString>("setup");
        QTest::addColumn<QString>("code");
        QTest::newRow("setRoomUserData") << QString() << qsl("setRoomUserData(2, 'room.ui_nameColor', '#ff0000')");
        QTest::newRow("clearRoomUserDataItem") << qsl("setRoomUserData(2, 'room.ui_showName', 'true')") << qsl("clearRoomUserDataItem(2, 'room.ui_showName')");
        QTest::newRow("clearRoomUserData") << qsl("setRoomUserData(2, 'visited', '1') setRoomUserData(2, 'room.ui_nameOffset', '0 1')") << qsl("clearRoomUserData(2)");
    }

    void test_roomUserDataTheViewsDrawCountsTheRevisionsUp()
    {
        QFETCH(QString, setup);
        QFETCH(QString, code);
        QVERIFY(buildMap() > 0);
        QVERIFY(setup.isEmpty() || runLua(setup));
        map()->resetUnsaved();
        const quint64 revision = map()->revision();
        const quint64 appearanceRevision = map()->appearanceRevision();
        QVERIFY(runLua(qsl("assert(%1)").arg(code)));
        QVERIFY2(map()->isUnsaved(), "the change would not be saved");
        QVERIFY2(map()->revision() > revision, qPrintable(code));
        QVERIFY2(map()->appearanceRevision() > appearanceRevision, qPrintable(code));
    }

    void test_roomUserDataNotDrawnLeavesTheRevisionsAsTheyWere_data()
    {
        QTest::addColumn<QString>("setup");
        QTest::addColumn<QString>("code");
        QTest::newRow("setRoomUserData") << QString() << qsl("setRoomUserData(2, 'visited', '1')");
        QTest::newRow("clearRoomUserDataItem") << qsl("setRoomUserData(2, 'visited', '1')") << qsl("clearRoomUserDataItem(2, 'visited')");
        QTest::newRow("clearRoomUserData") << qsl("setRoomUserData(2, 'visited', '1') setRoomUserData(2, 'notes', 'x')") << qsl("clearRoomUserData(2)");
    }

    void test_roomUserDataNotDrawnLeavesTheRevisionsAsTheyWere()
    {
        QFETCH(QString, setup);
        QFETCH(QString, code);
        QVERIFY(buildMap() > 0);
        QVERIFY(setup.isEmpty() || runLua(setup));
        map()->resetUnsaved();
        // Lets the announcement queued by setting the map up go out before counting
        QTest::qWait(10);
        const QSignalSpy repaints(map(), &TMap::signal_areaChanged);
        const quint64 revision = map()->revision();
        const quint64 mapRevision = map()->mapRevision();
        const quint64 appearanceRevision = map()->appearanceRevision();
        QVERIFY(runLua(qsl("assert(%1)").arg(code)));
        QVERIFY2(map()->isUnsaved(), "the change would not be saved");
        QCOMPARE(map()->revision(), revision);
        QCOMPARE(map()->mapRevision(), mapRevision);
        QCOMPARE(map()->appearanceRevision(), appearanceRevision);
        // A map info contributor may show the key, so the views still have to paint again
        QTRY_COMPARE(repaints.count(), 1);
    }

    void test_theAtcpRoomNumberLeavesTheRevisionAsItWas()
    {
        QVERIFY(buildMap() > 0);
        const quint64 before = map()->revision();
        mpHost->mTelnet.setATCPVariables(QByteArrayLiteral("Room.Num 2"));
        QCOMPARE(map()->mRoomIdHash.value(map()->mProfileName), 2);
        QCOMPARE(map()->revision(), before);
    }

    void test_clearingAuditingAndRestoringTheMapCountItsRevisionUp()
    {
        QVERIFY(buildMap() > 0);
        quint64 before = map()->revision();
        map()->audit();
        QVERIFY2(map()->revision() > before, "an audit left the revision as it was");

        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        const QString file = directory.filePath(qsl("map.dat"));
        QVERIFY(runLua(qsl("assert(saveMap([[%1]]))").arg(file)));
        before = map()->revision();
        QVERIFY(map()->restore(file));
        QVERIFY2(map()->revision() > before, "restoring a map left the revision as it was");

        before = map()->revision();
        map()->mapClear();
        QVERIFY2(map()->revision() > before, "clearing the map left the revision as it was");
    }
};

#include "MapRevisionTest.moc"
MUDLET_GROUPED_TEST_MAIN(MapRevisionTest)
