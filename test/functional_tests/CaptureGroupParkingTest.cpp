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
 * clearCaptureGroups() parks the emptied capture vectors for the next fire and
 * clamps how many entries it keeps with resize(). resize() downwards destroys
 * the elements but never lowers capacity(), so the two vector blocks stayed at
 * the biggest fire's high water mark for the rest of the session: the line fed
 * below parked capacity for 6000 entries, 187KB of std::string slots plus 23KB
 * of positions, and that grows with the longest line a trigger ever matched.
 *
 * A match-all trigger reaches those counts without a hostile line: its /g loop
 * accumulates every match on the line into one capture list, so its capture
 * count scales with matches per line rather than with the pattern's group count.
 *
 * The capacity is private state with no Lua reader, which is what keeps this out
 * of a spec.
 *
 * Run with: ctest -R CaptureGroupParkingTest -V
 */

#include <QTemporaryDir>
#include <QtTest/QtTest>

#include "PortableModeTestHelper.h"
#include "ProfileTestHelper.h"
#include "Host.h"
#include "MudletApp.h"
#include "MudletInstanceCoordinator.h"
#include "TLuaInterpreter.h"
#include "TelnetServerStub.h"
#include "ctelnet.h"
#include "mudlet.h"

extern "C" {
#if defined(INCLUDE_VERSIONED_LUA_HEADERS)
#include <lua5.1/lauxlib.h>
#include <lua5.1/lua.h>
#else
#include <lauxlib.h>
#include <lua.h>
#endif
}

#include "GroupedTest.h"

class CaptureGroupParkingTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;
    TelnetServerStub* mpServer = nullptr;
    Host* mpHost = nullptr;
    const QString mHostname = "CaptureGroupParking-Test";
    QString mPort; // assigned the stub's actual ephemeral port in initTestCase()
    const QString mLocalhost = "localhost";

    void runLua(const QString& code)
    {
        lua_State* L = mpHost->mLuaInterpreter.getLuaGlobalState();
        if (luaL_dostring(L, code.toUtf8().constData()) != 0) {
            const QString error = QString::fromUtf8(lua_tostring(L, -1));
            lua_pop(L, 1);
            QFAIL(qPrintable(qsl("Lua error running test script: %1").arg(error)));
        }
    }

    int luaInt(const QString& global)
    {
        lua_State* L = mpHost->mLuaInterpreter.getLuaGlobalState();
        lua_getglobal(L, global.toUtf8().constData());
        const int value = static_cast<int>(lua_tointeger(L, -1));
        lua_pop(L, 1);
        return value;
    }

    void checkParkedCapacity(const char* when)
    {
        const TLuaInterpreter& interpreter = mpHost->mLuaInterpreter;
        const std::size_t bound = TLuaInterpreter::scmMaxParkedCaptureSlack;
        QVERIFY2(interpreter.mSpareCaptureGroupList.size() <= TLuaInterpreter::scmMaxParkedCaptures,
                 qPrintable(qsl("%1: parked capture count %2 is past the cap of %3").arg(when).arg(interpreter.mSpareCaptureGroupList.size()).arg(TLuaInterpreter::scmMaxParkedCaptures)));
        QVERIFY2(interpreter.mSpareCaptureGroupList.capacity() <= bound,
                 qPrintable(qsl("%1: parked capture vector still holds capacity for %2 entries (%3 KB), bound is %4")
                                    .arg(when)
                                    .arg(interpreter.mSpareCaptureGroupList.capacity())
                                    .arg(interpreter.mSpareCaptureGroupList.capacity() * sizeof(std::string) / 1024)
                                    .arg(bound)));
        QVERIFY2(interpreter.mSpareCaptureGroupPosList.capacity() <= bound,
                 qPrintable(qsl("%1: parked position vector still holds capacity for %2 entries (%3 KB), bound is %4")
                                    .arg(when)
                                    .arg(interpreter.mSpareCaptureGroupPosList.capacity())
                                    .arg(interpreter.mSpareCaptureGroupPosList.capacity() * sizeof(int) / 1024)
                                    .arg(bound)));
    }

private slots:
    void initTestCase()
    {
        if (portableMarkerPresent()) {
            QSKIP("portable.txt present - it takes precedence over XDG_CONFIG_HOME, so the config dir cannot be redirected");
        }

        // A config root of this process's own, so a concurrent copy of this test
        // does not share a profile list with it
        QVERIFY(mConfigDir.isValid());
        QVERIFY(QDir().mkpath(qsl("%1/mudlet/profiles").arg(mConfigDir.path())));
        mSavedXdg = qgetenv("XDG_CONFIG_HOME");
        qputenv("XDG_CONFIG_HOME", mConfigDir.path().toUtf8());

        mpServer = new TelnetServerStub(qApp);
        mpServer->start(mLocalhost, 0); // ephemeral OS-assigned port avoids collisions across concurrent test runs
        mPort = QString::number(mpServer->serverPort());
        mudlet::start();
        mudlet::self()->setupConfig();
        QCOMPARE(MudletApp::getMudletPath(enums::mainPath), qsl("%1/mudlet").arg(mConfigDir.path()));
        mudlet::self()->takeOwnershipOfInstanceCoordinator(std::make_unique<MudletInstanceCoordinator>("MudletInstanceCoordinator"));
        mudlet::self()->init();
        mudlet::self()->setStorePasswordsSecurely(false);
        deleteProfileDirectory(mHostname);

        startProfile(mHostname, mLocalhost, mPort);
        mpHost = mudlet::self()->getActiveHost();
        QVERIFY2(mpHost, "No active host after profile creation");
    }

    void cleanupTestCase()
    {
        mpHost = nullptr;
        delete mpServer;
        mpServer = nullptr;
        if (mudlet::self()) {
            deleteProfileDirectory(mHostname);
            delete mudlet::self();
        }
        mSavedXdg.isNull() ? qunsetenv("XDG_CONFIG_HOME") : qputenv("XDG_CONFIG_HOME", mSavedXdg);
    }

    void test_parkedCaptureStorageIsBounded()
    {
        // Argument 8 is the match-all flag, so this trigger's captures scale
        // with the number of words on the line
        runLua(qsl("capturesSeen = 0\n"
                   "tempComplexRegexTrigger('GCAP', [[(\\w+)]], [[capturesSeen = #matches]], 0, 0, 0, 0, 1, 0, 0, 0, 0, 0)\n"
                   "local words = {}\n"
                   "for i = 1, 3000 do words[i] = 'word' .. i end\n"
                   "feedTriggers(table.concat(words, ' ') .. '\\n')\n"));

        // Overshooting the bound rather than merely the cap is what makes the
        // checks below bite, whatever the bound is tuned to
        const int capturesSeen = luaInt(qsl("capturesSeen"));
        QVERIFY2(static_cast<std::size_t>(capturesSeen) > TLuaInterpreter::scmMaxParkedCaptureSlack,
                 qPrintable(qsl("the match-all fire has to overshoot the bound of %1 for this test to mean anything, it produced %2 captures")
                                    .arg(TLuaInterpreter::scmMaxParkedCaptureSlack)
                                    .arg(capturesSeen)));

        checkParkedCapacity("after the match-all fire");
        // QVERIFY2 inside the helper only returns from the helper
        QVERIFY(!QTest::currentTestFailed());

        // The parked vectors are handed back out on the next fire and parked
        // again, so a later ordinary fire must not carry the old block along
        runLua(qsl("tempRegexTrigger('^plain (\\\\w+)$', [[plainCaptures = #matches]])\n"
                   "feedTriggers('plain line\\n')\n"));
        QCOMPARE(luaInt(qsl("plainCaptures")), 2);

        checkParkedCapacity("after a following ordinary fire");
        QVERIFY(!QTest::currentTestFailed());

        // A trigger that overshoots the cap by less than the slack keeps its
        // block between fires instead of reallocating it each way every fire
        runLua(qsl("local words = {}\n"
                   "for i = 1, 350 do words[i] = 'word' .. i end\n"
                   "local line = table.concat(words, ' ') .. '\\n'\n"
                   "feedTriggers(line)\n"
                   "feedTriggers(line)\n"));

        const auto moderateCaptures = static_cast<std::size_t>(luaInt(qsl("capturesSeen")));
        QVERIFY2(moderateCaptures > TLuaInterpreter::scmMaxParkedCaptures && moderateCaptures <= TLuaInterpreter::scmMaxParkedCaptureSlack,
                 qPrintable(qsl("this fire has to land between the cap of %1 and the bound of %2, it produced %3 captures")
                                    .arg(TLuaInterpreter::scmMaxParkedCaptures)
                                    .arg(TLuaInterpreter::scmMaxParkedCaptureSlack)
                                    .arg(moderateCaptures)));
        QVERIFY2(mpHost->mLuaInterpreter.mSpareCaptureGroupList.capacity() >= moderateCaptures,
                 qPrintable(
                         qsl("a %1 capture fire under the bound should keep its block, the parked one holds %2").arg(moderateCaptures).arg(mpHost->mLuaInterpreter.mSpareCaptureGroupList.capacity())));
        checkParkedCapacity("after a moderate overshoot");
    }

    void startProfile(const QString& hostname, const QString& address, const QString& port)
    {
        auto host = TestProfile::create(hostname, address, port);
        if (!host) {
            QFAIL("No active host available for the test.");
        }

        QSignalSpy spy(&(host->mTelnet), &cTelnet::signal_connected);
        if (!spy.wait(2000)) {
            QFAIL("Could not connect with the host.");
        }
    }

    void deleteProfileDirectory(const QString& profileName)
    {
        const QString path = MudletApp::getMudletPath(enums::profileHomePath, profileName);
        QDir dir(path);

        if (!dir.exists()) {
            return;
        }
        dir.removeRecursively();
    }
};

#include "CaptureGroupParkingTest.moc"
MUDLET_GROUPED_TEST_MAIN(CaptureGroupParkingTest)
