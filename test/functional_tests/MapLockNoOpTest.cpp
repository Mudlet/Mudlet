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

/*
 * lockRoom(), lockExit() and lockSpecialExit() used to mark the map unsaved,
 * ask for a redraw and invalidate the pathfinding graph on every call, whether
 * or not the requested state differed from the current one. Mapper scripts that
 * re-apply door and lock state on each room they visit make that call on every
 * step of a speedwalk, so a whole walk's worth of map saves and full initGraph()
 * rebuilds came out of calls that changed nothing.
 *
 * Each case here asserts both directions: a call that repeats the current state
 * leaves the two flags alone, and a call that really changes it still sets both.
 * The second half is what stops the fix from being "never do the work".
 */

#include <QSignalSpy>
#include <QTemporaryDir>
#include <QtTest/QtTest>

#include "PortableModeTestHelper.h"
#include "ProfileTestHelper.h"
#include "Host.h"
#include "MudletInstanceCoordinator.h"
#include "TLuaInterpreter.h"
#include "TMap.h"
#include "TRoom.h"
#include "TRoomDB.h"
#include "TelnetServerStub.h"
#include "ctelnet.h"
#include "mudlet.h"

#include "GroupedTest.h"

namespace {
const int scmFirstRoomId = 1;
const int scmSecondRoomId = 2;
const QString scmSpecialExitCommand = qsl("enter portal");
} // namespace

class MapLockNoOpTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;
    TelnetServerStub* mpServer = nullptr;
    Host* mpHost = nullptr;
    const QString mHostname = qsl("Map-Lock-NoOp-Test-Host");
    QString mPort;
    const QString mLocalhost = qsl("localhost");

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
    }

    void cleanupTestCase() { mSavedXdg.isNull() ? qunsetenv("XDG_CONFIG_HOME") : qputenv("XDG_CONFIG_HOME", mSavedXdg); }

    void init()
    {
        mpServer = new TelnetServerStub(qApp);
        mpServer->start(mLocalhost, 0);
        mPort = QString::number(mpServer->serverPort());
        mudlet::start();
        mudlet::self()->setupConfig();
        QCOMPARE(mudlet::getMudletPath(enums::mainPath), qsl("%1/mudlet").arg(mConfigDir.path()));
        mudlet::self()->takeOwnershipOfInstanceCoordinator(std::make_unique<MudletInstanceCoordinator>("MudletInstanceCoordinator"));
        mudlet::self()->init();
        mudlet::self()->setStorePasswordsSecurely(false);
        deleteProfileDirectory();

        mpHost = TestProfile::create(mHostname, mLocalhost, mPort);
        if (!mpHost) {
            QFAIL("No active host available for the test.");
        }

        QSignalSpy connectionSpy(&(mpHost->mTelnet), &cTelnet::signal_connected);
        if (!connectionSpy.wait(2000)) {
            QFAIL("Could not connect with the host.");
        }

        QVERIFY(mpHost->mpMap);
        QVERIFY(mpHost->mpMap->mpRoomDB);
        buildMap();
    }

    void cleanup()
    {
        delete mpServer;
        mpServer = nullptr;
        mpHost = nullptr;
        deleteProfileDirectory();
        delete mudlet::self();
    }

    void test_lockRoomOnlyDirtiesTheMapWhenTheLockChanges()
    {
        TRoom* pRoom = room(scmFirstRoomId);
        QVERIFY(pRoom);
        QVERIFY2(!pRoom->isLocked, "a freshly added room is expected to start unlocked");

        markMapClean();
        QVERIFY(runLua(qsl("assert(lockRoom(%1, false))").arg(scmFirstRoomId)));
        QVERIFY(!pRoom->isLocked);
        verifyMapUntouched(qsl("lockRoom(false) on an already unlocked room"));

        markMapClean();
        QVERIFY(runLua(qsl("assert(lockRoom(%1, true))").arg(scmFirstRoomId)));
        QVERIFY(pRoom->isLocked);
        verifyMapDirtied(qsl("lockRoom(true) on an unlocked room"));

        markMapClean();
        QVERIFY(runLua(qsl("assert(lockRoom(%1, true))").arg(scmFirstRoomId)));
        QVERIFY(pRoom->isLocked);
        verifyMapUntouched(qsl("lockRoom(true) on an already locked room"));

        markMapClean();
        QVERIFY(runLua(qsl("assert(lockRoom(%1, false))").arg(scmFirstRoomId)));
        QVERIFY(!pRoom->isLocked);
        verifyMapDirtied(qsl("lockRoom(false) on a locked room"));
    }

    void test_lockExitOnlyDirtiesTheMapWhenTheLockChanges()
    {
        TRoom* pRoom = room(scmFirstRoomId);
        QVERIFY(pRoom);
        QVERIFY2(!pRoom->hasExitLock(DIR_NORTH), "a freshly added exit is expected to start unlocked");

        markMapClean();
        QVERIFY(runLua(qsl("lockExit(%1, \"north\", false)").arg(scmFirstRoomId)));
        QVERIFY(!pRoom->hasExitLock(DIR_NORTH));
        verifyMapUntouched(qsl("lockExit(false) on an already unlocked exit"));

        markMapClean();
        QVERIFY(runLua(qsl("lockExit(%1, \"north\", true)").arg(scmFirstRoomId)));
        QVERIFY(pRoom->hasExitLock(DIR_NORTH));
        verifyMapDirtied(qsl("lockExit(true) on an unlocked exit"));

        markMapClean();
        QVERIFY(runLua(qsl("lockExit(%1, \"north\", true)").arg(scmFirstRoomId)));
        QVERIFY(pRoom->hasExitLock(DIR_NORTH));
        verifyMapUntouched(qsl("lockExit(true) on an already locked exit"));

        markMapClean();
        QVERIFY(runLua(qsl("lockExit(%1, \"north\", false)").arg(scmFirstRoomId)));
        QVERIFY(!pRoom->hasExitLock(DIR_NORTH));
        verifyMapDirtied(qsl("lockExit(false) on a locked exit"));
    }

    void test_lockSpecialExitOnlyDirtiesTheMapWhenTheLockChanges()
    {
        TRoom* pRoom = room(scmFirstRoomId);
        QVERIFY(pRoom);
        QVERIFY2(!pRoom->hasSpecialExitLock(scmSpecialExitCommand), "a freshly added special exit is expected to start unlocked");

        markMapClean();
        QVERIFY(runLua(lockSpecialExitScript(false)));
        QVERIFY(!pRoom->hasSpecialExitLock(scmSpecialExitCommand));
        verifyMapUntouched(qsl("lockSpecialExit(false) on an already unlocked special exit"));

        markMapClean();
        QVERIFY(runLua(lockSpecialExitScript(true)));
        QVERIFY(pRoom->hasSpecialExitLock(scmSpecialExitCommand));
        verifyMapDirtied(qsl("lockSpecialExit(true) on an unlocked special exit"));

        markMapClean();
        QVERIFY(runLua(lockSpecialExitScript(true)));
        QVERIFY(pRoom->hasSpecialExitLock(scmSpecialExitCommand));
        verifyMapUntouched(qsl("lockSpecialExit(true) on an already locked special exit"));

        markMapClean();
        QVERIFY(runLua(lockSpecialExitScript(false)));
        QVERIFY(!pRoom->hasSpecialExitLock(scmSpecialExitCommand));
        verifyMapDirtied(qsl("lockSpecialExit(false) on a locked special exit"));
    }

    // A special exit that does not exist still has to be reported as a bad
    // argument, which is the one thing the no-op check runs ahead of.
    void test_lockSpecialExitStillRejectsAnExitThatDoesNotExist()
    {
        markMapClean();
        QVERIFY(runLua(qsl("assert(lockSpecialExit(%1, %2, \"no such command\", true) == nil)").arg(scmFirstRoomId).arg(scmSecondRoomId)));
        verifyMapUntouched(qsl("lockSpecialExit() naming an exit that does not exist"));
    }

private:
    TRoom* room(const int id) const { return mpHost->mpMap->mpRoomDB->getRoom(id); }

    bool runLua(const QString& script) const { return mpHost->getLuaInterpreter()->compileAndExecuteScript(script); }

    QString lockSpecialExitScript(const bool lock) const
    {
        return qsl("assert(lockSpecialExit(%1, %2, \"%3\", %4))").arg(QString::number(scmFirstRoomId), QString::number(scmSecondRoomId), scmSpecialExitCommand, lock ? qsl("true") : qsl("false"));
    }

    void buildMap()
    {
        QVERIFY(runLua(qsl("local areaId = addAreaName(\"Lock-NoOp-Area\")\n"
                           "assert(addRoom(%1, areaId))\n"
                           "assert(addRoom(%2, areaId))\n"
                           "assert(setExit(%1, %2, \"north\"))\n"
                           "assert(addSpecialExit(%1, %2, \"%3\"))")
                               .arg(QString::number(scmFirstRoomId), QString::number(scmSecondRoomId), scmSpecialExitCommand)));
    }

    // The map is saved from mUnsavedMap and the pathfinding graph is rebuilt
    // from mMapGraphNeedsUpdate, so these two are the whole cost a no-op used to
    // impose. updateArea() is left out: it debounces itself onto the event loop
    // and there is nothing to observe without spinning it.
    void markMapClean()
    {
        mpHost->mpMap->resetUnsaved();
        mpHost->mpMap->mMapGraphNeedsUpdate = false;
    }

    void verifyMapUntouched(const QString& call)
    {
        QVERIFY2(!mpHost->mpMap->isUnsaved(), qPrintable(qsl("%1 changed nothing yet marked the map unsaved").arg(call)));
        QVERIFY2(!mpHost->mpMap->mMapGraphNeedsUpdate, qPrintable(qsl("%1 changed nothing yet invalidated the pathfinding graph").arg(call)));
    }

    void verifyMapDirtied(const QString& call)
    {
        QVERIFY2(mpHost->mpMap->isUnsaved(), qPrintable(qsl("%1 changed the lock state but left the map marked as saved").arg(call)));
        QVERIFY2(mpHost->mpMap->mMapGraphNeedsUpdate, qPrintable(qsl("%1 changed the lock state but left the pathfinding graph valid").arg(call)));
    }

    void deleteProfileDirectory()
    {
        QDir dir(mudlet::getMudletPath(enums::profileHomePath, mHostname));
        if (dir.exists()) {
            dir.removeRecursively();
        }
    }
};

#include "MapLockNoOpTest.moc"
MUDLET_GROUPED_TEST_MAIN(MapLockNoOpTest)
