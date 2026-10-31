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
 * clearCaptureGroups() keeps the emptied capture storage for the next fire.
 * clear() never lowers capacity(), so without a bound the storage would stay at
 * the biggest fire's high water mark for the rest of the session, and that grows
 * with the longest line a trigger ever matched.
 *
 * A match-all trigger grows it without a hostile line: its /g loop
 * accumulates every match on the line into one fire, so its capture
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
#include "TTrigger.h"
#include "TriggerUnit.h"
#include "TelnetServerStub.h"
#include "ctelnet.h"
#include "mudlet.h"

using namespace std::chrono_literals;

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
        const std::size_t bound = TCaptureGroups::scmMaxKeptCaptures;
        QVERIFY2(interpreter.mCaptureGroups.capturesCapacity() <= bound,
                 qPrintable(qsl("%1: kept capture storage still holds capacity for %2 captures, bound is %3").arg(when).arg(interpreter.mCaptureGroups.capturesCapacity()).arg(bound)));
        QVERIFY2(interpreter.mCaptureGroups.bytesCapacity() <= TCaptureGroups::scmMaxKeptBytes,
                 qPrintable(qsl("%1: kept capture storage still holds %2 bytes, bound is %3").arg(when).arg(interpreter.mCaptureGroups.bytesCapacity()).arg(TCaptureGroups::scmMaxKeptBytes)));
        QVERIFY2(interpreter.mCaptureGroupPosList.capacity() <= bound,
                 qPrintable(qsl("%1: kept position vector still holds capacity for %2 entries, bound is %3").arg(when).arg(interpreter.mCaptureGroupPosList.capacity()).arg(bound)));
    }

    // A multiline match's captures are kept twice: by the interpreter, and by the
    // trigger's spare state for the next state it opens. Growing to a match within
    // the bounds can double the storage, so what is kept stays under twice them.
    void checkKeptMultilineCapacity(const char* when, const TMultiCaptureGroups& groups, const std::size_t maxCaptures, const std::size_t maxBytes)
    {
        for (const auto& [kind, captures] : {std::pair{"numbered", &groups.captures()}, std::pair{"named", &groups.namedCaptures()}}) {
            QVERIFY2(captures->capturesCapacity() < 2 * maxCaptures,
                     qPrintable(qsl("%1: kept multiline %2 storage still holds capacity for %3 captures, twice the bound is %4")
                                        .arg(when)
                                        .arg(kind)
                                        .arg(captures->capturesCapacity())
                                        .arg(2 * maxCaptures)));
            QVERIFY2(captures->bytesCapacity() < 2 * maxBytes,
                     qPrintable(qsl("%1: kept multiline %2 storage still holds %3 bytes, twice the bound is %4").arg(when).arg(kind).arg(captures->bytesCapacity()).arg(2 * maxBytes)));
        }
    }

    void checkInterpreterMultilineCapacity(const char* when)
    {
        checkKeptMultilineCapacity(when, mpHost->mLuaInterpreter.mMultiCaptureGroups, TCaptureGroups::scmMaxKeptCaptures, TCaptureGroups::scmMaxKeptBytes);
    }

    void checkSpareMultilineCapacity(const char* when, const TTrigger& trigger)
    {
        checkKeptMultilineCapacity(when, trigger.mSpareCondition.mapped()->captures(), TMatchState::scmMaxSpareCaptures, TMatchState::scmMaxSpareBytes);
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
                   "captureBytes = 0\n"
                   "tempComplexRegexTrigger('GCAP', [[(\\w+)]], [[capturesSeen = #matches; captureBytes = #table.concat(matches)]], 0, 0, 0, 0, 1, 0, 0, 0, 0, 0)\n"
                   "local words = {}\n"
                   "for i = 1, 3000 do words[i] = 'word' .. i end\n"
                   "feedTriggers(table.concat(words, ' ') .. '\\n')\n"));

        // Overshooting both bounds is what makes the checks below bite, whatever
        // they are tuned to
        const int capturesSeen = luaInt(qsl("capturesSeen"));
        QVERIFY2(static_cast<std::size_t>(capturesSeen) > TCaptureGroups::scmMaxKeptCaptures,
                 qPrintable(
                         qsl("the match-all fire has to overshoot the bound of %1 for this test to mean anything, it produced %2 captures").arg(TCaptureGroups::scmMaxKeptCaptures).arg(capturesSeen)));
        const int captureBytes = luaInt(qsl("captureBytes"));
        QVERIFY2(static_cast<std::size_t>(captureBytes) > TCaptureGroups::scmMaxKeptBytes,
                 qPrintable(qsl("the match-all fire has to overshoot the bound of %1 bytes for this test to mean anything, it produced %2").arg(TCaptureGroups::scmMaxKeptBytes).arg(captureBytes)));

        checkParkedCapacity("after the match-all fire");
        // QVERIFY2 inside the helper only returns from the helper
        QVERIFY(!QTest::currentTestFailed());

        // The kept storage is reused by the next fire and kept again, so a later
        // ordinary fire must not carry the old block along
        runLua(qsl("tempRegexTrigger('^plain (\\\\w+)$', [[plainCaptures = #matches]])\n"
                   "feedTriggers('plain line\\n')\n"));
        QCOMPARE(luaInt(qsl("plainCaptures")), 2);

        checkParkedCapacity("after a following ordinary fire");
        QVERIFY(!QTest::currentTestFailed());

        // A trigger under the bound keeps its block between fires instead of
        // reallocating it every fire
        runLua(qsl("local words = {}\n"
                   "for i = 1, 350 do words[i] = 'word' .. i end\n"
                   "local line = table.concat(words, ' ') .. '\\n'\n"
                   "feedTriggers(line)\n"
                   "feedTriggers(line)\n"));

        const auto moderateCaptures = static_cast<std::size_t>(luaInt(qsl("capturesSeen")));
        QVERIFY2(moderateCaptures > TCaptureGroups::scmMaxKeptCaptures / 4 && moderateCaptures <= TCaptureGroups::scmMaxKeptCaptures,
                 qPrintable(qsl("this fire has to land under the bound of %1, but not far, it produced %2 captures").arg(TCaptureGroups::scmMaxKeptCaptures).arg(moderateCaptures)));
        QVERIFY2(
                mpHost->mLuaInterpreter.mCaptureGroups.capturesCapacity() >= moderateCaptures,
                qPrintable(qsl("a %1 capture fire under the bound should keep its block, the kept one holds %2").arg(moderateCaptures).arg(mpHost->mLuaInterpreter.mCaptureGroups.capturesCapacity())));
        checkParkedCapacity("after a fire under the bound");
        QVERIFY(!QTest::currentTestFailed());

        // A trigger whose command is all it runs does not park its captures, so
        // the alias the command matches sets its own over them
        runLua(qsl("disableTrigger('GCAP')\n"
                   "aliasCaptures = 0\n"
                   "permAlias('GCMDALIAS', '', '^gcapcmd$', [[aliasCaptures = #matches]])\n"
                   "tempComplexRegexTrigger('GCMD', [[(\\w+)]], '', 0, 0, 0, 0, 1, 0, 0, 0, 0, 0)\n"));
        TTrigger* commandTrigger = mpHost->getTriggerUnit()->findTrigger(qsl("GCMD"));
        QVERIFY(commandTrigger);
        commandTrigger->setCommand(qsl("gcapcmd"));
        runLua(qsl("local words = {}\n"
                   "for i = 1, 3000 do words[i] = 'word' .. i end\n"
                   "feedTriggers(table.concat(words, ' ') .. '\\n')\n"));
        QCOMPARE(luaInt(qsl("aliasCaptures")), 1);
        checkParkedCapacity("after a command trigger's alias");
    }

    void test_parkedMultilineCaptureStorageIsBounded()
    {
        // The trigger is match-all (argument 8), so condition 2 captures every
        // word of the line after the "---" condition 1 matched
        runLua(qsl("multiCapturesSeen = 0\n"
                   "multiCaptureBytes = 0\n"
                   "local code = [[multiCapturesSeen = #multimatches[2]; multiCaptureBytes = #table.concat(multimatches[2])]]\n"
                   "tempComplexRegexTrigger('GMCAP', [[^---$]], code, 1, 0, 0, 0, 1, 0, 0, 0, 0, 1)\n"
                   "tempComplexRegexTrigger('GMCAP', [[(\\w+)]], code, 1, 0, 0, 0, 1, 0, 0, 0, 0, 1)\n"
                   "local words = {}\n"
                   "for i = 1, 5000 do words[i] = 'word' .. i end\n"
                   "feedTriggers('---\\n' .. table.concat(words, ' ') .. '\\n')\n"));

        const int capturesSeen = luaInt(qsl("multiCapturesSeen"));
        QVERIFY2(static_cast<std::size_t>(capturesSeen) > 2 * TCaptureGroups::scmMaxKeptCaptures,
                 qPrintable(qsl("the multiline fire has to overshoot twice the bound of %1 for this test to mean anything, it produced %2 captures")
                                    .arg(TCaptureGroups::scmMaxKeptCaptures)
                                    .arg(capturesSeen)));
        const int captureBytes = luaInt(qsl("multiCaptureBytes"));
        QVERIFY2(static_cast<std::size_t>(captureBytes) > 2 * TCaptureGroups::scmMaxKeptBytes,
                 qPrintable(
                         qsl("the multiline fire has to overshoot twice the bound of %1 bytes for this test to mean anything, it produced %2").arg(TCaptureGroups::scmMaxKeptBytes).arg(captureBytes)));

        TTrigger* trigger = mpHost->getTriggerUnit()->findTrigger(qsl("GMCAP"));
        QVERIFY(trigger);
        QVERIFY2(trigger->mSpareCondition, "the completed state should be kept for the next state the trigger opens");
        checkInterpreterMultilineCapacity("interpreter, after the match-all fire");
        checkSpareMultilineCapacity("spare state, after the match-all fire", *trigger);
        QVERIFY(!QTest::currentTestFailed());

        // Under the interpreter's bound but over the spare's: only the interpreter keeps its block
        runLua(qsl("local words = {}\n"
                   "for i = 1, 350 do words[i] = 'word' .. i end\n"
                   "local lines = '---\\n' .. table.concat(words, ' ') .. '\\n'\n"
                   "feedTriggers(lines)\n"
                   "feedTriggers(lines)\n"));

        const auto moderateCaptures = static_cast<std::size_t>(luaInt(qsl("multiCapturesSeen")));
        QVERIFY2(moderateCaptures > TMatchState::scmMaxSpareCaptures && moderateCaptures > TCaptureGroups::scmMaxKeptCaptures / 4 && moderateCaptures <= TCaptureGroups::scmMaxKeptCaptures,
                 qPrintable(qsl("this fire has to land under the bound of %1, but not far, and over the spare bound of %2, it produced %3 captures")
                                    .arg(TCaptureGroups::scmMaxKeptCaptures)
                                    .arg(TMatchState::scmMaxSpareCaptures)
                                    .arg(moderateCaptures)));
        const std::size_t interpreterKept = mpHost->mLuaInterpreter.mMultiCaptureGroups.captures().capturesCapacity();
        QVERIFY2(interpreterKept >= moderateCaptures, qPrintable(qsl("the interpreter should keep its block for a %1 capture match, it holds %2").arg(moderateCaptures).arg(interpreterKept)));
        QVERIFY(trigger->mSpareCondition);
        checkInterpreterMultilineCapacity("interpreter, after a match under the bound");
        checkSpareMultilineCapacity("spare state, after a match over its bound", *trigger);
        QVERIFY(!QTest::currentTestFailed());

        // An ordinary match fits the spare, so the next state like it allocates nothing
        runLua(qsl("local words = {}\n"
                   "for i = 1, 20 do words[i] = 'word' .. i end\n"
                   "local lines = '---\\n' .. table.concat(words, ' ') .. '\\n'\n"
                   "feedTriggers(lines)\n"
                   "feedTriggers(lines)\n"));

        const auto smallCaptures = static_cast<std::size_t>(luaInt(qsl("multiCapturesSeen")));
        QVERIFY2(smallCaptures > TMatchState::scmMaxSpareCaptures / 2 && smallCaptures < TMatchState::scmMaxSpareCaptures,
                 qPrintable(qsl("this fire has to land under the spare bound of %1, but not far, it produced %2 captures").arg(TMatchState::scmMaxSpareCaptures).arg(smallCaptures)));
        QVERIFY(trigger->mSpareCondition);
        const std::size_t spareKept = trigger->mSpareCondition.mapped()->captures().captures().capturesCapacity();
        QVERIFY2(spareKept >= smallCaptures, qPrintable(qsl("the spare state should keep its block for a %1 capture match, it holds %2").arg(smallCaptures).arg(spareKept)));
        const auto smallBytes = static_cast<std::size_t>(luaInt(qsl("multiCaptureBytes")));
        const std::size_t spareBytesKept = trigger->mSpareCondition.mapped()->captures().captures().bytesCapacity();
        QVERIFY2(spareBytesKept >= smallBytes, qPrintable(qsl("the spare state should keep its bytes for a %1 byte match, it holds %2").arg(smallBytes).arg(spareBytesKept)));
        checkSpareMultilineCapacity("spare state, after a match under its bound", *trigger);
        QVERIFY(!QTest::currentTestFailed());

        // Three conditions over ordinary long game lines also fit the spare
        runLua(qsl("longBytesSeen = 0\n"
                   "local code = [[local n = 0; for _, row in ipairs(multimatches) do for _, c in ipairs(row) do n = n + #c end end; longBytesSeen = n]]\n"
                   "for i = 1, 3 do tempComplexRegexTrigger('GMLONG', '^c' .. i .. ' (.+)$', code, 1, 0, 0, 0, 0, 0, 0, 0, 0, 3) end\n"
                   "local lines = ''\n"
                   "for i = 1, 3 do lines = lines .. 'c' .. i .. ' ' .. string.rep('x', 177) .. '\\n' end\n"
                   "feedTriggers(lines)\n"
                   "feedTriggers(lines)\n"));
        const auto longBytes = static_cast<std::size_t>(luaInt(qsl("longBytesSeen")));
        QVERIFY2(longBytes > TMatchState::scmMaxSpareBytes / 4 && longBytes < TMatchState::scmMaxSpareBytes / 2,
                 qPrintable(qsl("this match has to fill a good part of the spare bound of %1 bytes, it captured %2").arg(TMatchState::scmMaxSpareBytes).arg(longBytes)));
        TTrigger* longTrigger = mpHost->getTriggerUnit()->findTrigger(qsl("GMLONG"));
        QVERIFY(longTrigger && longTrigger->mSpareCondition);
        const std::size_t longBytesKept = longTrigger->mSpareCondition.mapped()->captures().captures().bytesCapacity();
        QVERIFY2(longBytesKept >= longBytes, qPrintable(qsl("the spare state should keep its bytes for a %1 byte match, it holds %2").arg(longBytes).arg(longBytesKept)));
        QVERIFY(!QTest::currentTestFailed());

        // A fresh state reserves for every condition as big as the first; a guess
        // far past the spare bound must not stay with a match that fits it
        runLua(qsl("skewBytesSeen = 0\n"
                   "skewFirstSeen = 0\n"
                   "local code = [[local n = 0; for _, row in ipairs(multimatches) do for _, c in ipairs(row) do n = n + #c end end; skewBytesSeen = n; "
                   "skewFirstSeen = #multimatches[1][1] + #multimatches[1][2] ]]\n"
                   "tempComplexRegexTrigger('GMSKEW', '^s1 (.+)$', code, 1, 0, 0, 0, 0, 0, 0, 0, 0, 3)\n"
                   "tempComplexRegexTrigger('GMSKEW', '^s2$', code, 1, 0, 0, 0, 0, 0, 0, 0, 0, 3)\n"
                   "tempComplexRegexTrigger('GMSKEW', '^s3$', code, 1, 0, 0, 0, 0, 0, 0, 0, 0, 3)\n"
                   "local lines = 's1 ' .. string.rep('y', 1397) .. '\\ns2\\ns3\\n'\n"
                   "feedTriggers(lines)\n"
                   "feedTriggers(lines)\n"));
        const auto skewBytes = static_cast<std::size_t>(luaInt(qsl("skewBytesSeen")));
        const auto skewFirst = static_cast<std::size_t>(luaInt(qsl("skewFirstSeen")));
        QVERIFY2(3 * skewFirst > 2 * TMatchState::scmMaxSpareBytes && skewBytes < TMatchState::scmMaxSpareBytes,
                 qPrintable(qsl("the first condition's %1 bytes times 3 have to overshoot twice the spare bound of %2 while the %3 byte match fits it")
                                    .arg(skewFirst)
                                    .arg(TMatchState::scmMaxSpareBytes)
                                    .arg(skewBytes)));
        TTrigger* skewTrigger = mpHost->getTriggerUnit()->findTrigger(qsl("GMSKEW"));
        QVERIFY(skewTrigger && skewTrigger->mSpareCondition);
        const std::size_t skewBytesKept = skewTrigger->mSpareCondition.mapped()->captures().captures().bytesCapacity();
        QVERIFY2(skewBytesKept >= skewBytes, qPrintable(qsl("the spare state should keep its bytes for a %1 byte match, it holds %2").arg(skewBytes).arg(skewBytesKept)));
        checkSpareMultilineCapacity("spare state, after a match whose reserve overshot its bound", *skewTrigger);
        QVERIFY(!QTest::currentTestFailed());

        // Later conditions longer than the first outgrow that guess, and growing
        // doubles the storage, past the bound for a match that fits it
        runLua(qsl("growBytesSeen = 0\n"
                   "growFirstSeen = 0\n"
                   "local code = [[local n = 0; for _, row in ipairs(multimatches) do for _, c in ipairs(row) do n = n + #c end end; growBytesSeen = n; "
                   "growFirstSeen = #multimatches[1][1] + #multimatches[1][2] ]]\n"
                   "for i = 1, 3 do tempComplexRegexTrigger('GMGROW', '^t' .. i .. ' (.+)$', code, 1, 0, 0, 0, 0, 0, 0, 0, 0, 3) end\n"
                   "local lines = 't1 ' .. string.rep('z', 340) .. '\\nt2 ' .. string.rep('z', 400) .. '\\nt3 ' .. string.rep('z', 400) .. '\\n'\n"
                   "feedTriggers(lines)\n"
                   "feedTriggers(lines)\n"));
        const auto growBytes = static_cast<std::size_t>(luaInt(qsl("growBytesSeen")));
        const auto growFirst = static_cast<std::size_t>(luaInt(qsl("growFirstSeen")));
        QVERIFY2(3 * growFirst < growBytes && 6 * growFirst > TMatchState::scmMaxSpareBytes && growBytes < TMatchState::scmMaxSpareBytes,
                 qPrintable(qsl("the %1 byte match has to outgrow the guess of 3 x %2 bytes, which doubled has to overshoot the spare bound of %3 that the match fits")
                                    .arg(growBytes)
                                    .arg(growFirst)
                                    .arg(TMatchState::scmMaxSpareBytes)));
        TTrigger* growTrigger = mpHost->getTriggerUnit()->findTrigger(qsl("GMGROW"));
        QVERIFY(growTrigger && growTrigger->mSpareCondition);
        const std::size_t growBytesKept = growTrigger->mSpareCondition.mapped()->captures().captures().bytesCapacity();
        QVERIFY2(growBytesKept >= growBytes, qPrintable(qsl("the spare state should keep its bytes for a %1 byte match, it holds %2").arg(growBytes).arg(growBytesKept)));
        checkSpareMultilineCapacity("spare state, after a match that outgrew its reserve", *growTrigger);
        QVERIFY(!QTest::currentTestFailed());

        // The same for the number of captures
        runLua(qsl("growCapturesSeen = 0\n"
                   "growFirstCaptures = 0\n"
                   "local code = [[local n = 0; for _, row in ipairs(multimatches) do n = n + #row end; growCapturesSeen = n; growFirstCaptures = #multimatches[1] ]]\n"
                   "for i = 1, 3 do tempComplexRegexTrigger('GMGROWN', '^u' .. i .. ' ' .. string.rep([[(\\w)]], i == 1 and 11 or 12) .. '$', code, 1, 0, 0, 0, 0, 0, 0, 0, 0, 3) end\n"
                   "local lines = 'u1 abcdefghijk\\nu2 abcdefghijkl\\nu3 abcdefghijkl\\n'\n"
                   "feedTriggers(lines)\n"
                   "feedTriggers(lines)\n"));
        const auto growCaptures = static_cast<std::size_t>(luaInt(qsl("growCapturesSeen")));
        const auto growFirstCaptures = static_cast<std::size_t>(luaInt(qsl("growFirstCaptures")));
        QVERIFY2(3 * growFirstCaptures < growCaptures && 6 * growFirstCaptures > TMatchState::scmMaxSpareCaptures && growCaptures < TMatchState::scmMaxSpareCaptures,
                 qPrintable(qsl("the %1 capture match has to outgrow the guess of 3 x %2, which doubled has to overshoot the spare bound of %3 that the match fits")
                                    .arg(growCaptures)
                                    .arg(growFirstCaptures)
                                    .arg(TMatchState::scmMaxSpareCaptures)));
        TTrigger* growCapturesTrigger = mpHost->getTriggerUnit()->findTrigger(qsl("GMGROWN"));
        QVERIFY(growCapturesTrigger && growCapturesTrigger->mSpareCondition);
        const std::size_t growCapturesKept = growCapturesTrigger->mSpareCondition.mapped()->captures().captures().capturesCapacity();
        QVERIFY2(growCapturesKept >= growCaptures, qPrintable(qsl("the spare state should keep its block for a %1 capture match, it holds %2").arg(growCaptures).arg(growCapturesKept)));
        checkSpareMultilineCapacity("spare state, after a match that outgrew its reserved captures", *growCapturesTrigger);
        QVERIFY(!QTest::currentTestFailed());

        // Named groups are kept in storage of their own, which only a long capture
        // grows, as a match-all fire takes its named groups from the first match
        runLua(qsl("namedBytesSeen = 0\n"
                   "local code = [[namedBytesSeen = #multimatches[2].rest]]\n"
                   "tempComplexRegexTrigger('GMNAMED', [[^===$]], code, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1)\n"
                   "tempComplexRegexTrigger('GMNAMED', [[^(?<rest>n+)$]], code, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1)\n"
                   "feedTriggers('===\\n' .. string.rep('n', 70000) .. '\\n')\n"));
        const int namedBytesSeen = luaInt(qsl("namedBytesSeen"));
        QVERIFY2(static_cast<std::size_t>(namedBytesSeen) > 2 * TCaptureGroups::scmMaxKeptBytes,
                 qPrintable(qsl("the named capture has to overshoot twice the bound of %1 bytes for this test to mean anything, it was %2").arg(TCaptureGroups::scmMaxKeptBytes).arg(namedBytesSeen)));
        TTrigger* namedTrigger = mpHost->getTriggerUnit()->findTrigger(qsl("GMNAMED"));
        QVERIFY(namedTrigger && namedTrigger->mSpareCondition);
        checkInterpreterMultilineCapacity("interpreter, after a long named capture");
        checkSpareMultilineCapacity("spare state, after a long named capture", *namedTrigger);
    }

    void startProfile(const QString& hostname, const QString& address, const QString& port)
    {
        auto host = TestProfile::create(hostname, address, port);
        if (!host) {
            QFAIL("No active host available for the test.");
        }

        QSignalSpy spy(&(host->mTelnet), &cTelnet::signal_connected);
        if (!spy.wait(2s)) {
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
