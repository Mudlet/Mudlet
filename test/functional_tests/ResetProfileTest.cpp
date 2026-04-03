/***************************************************************************
 *   Copyright (C) 2026 by Vadim Peretokin - vadim.peretokin@mudlet.org     *
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
 * Comprehensive tests for Host::resetProfile_phase1() / resetProfile_phase2().
 * Covers temp item removal, permanent item survival, stopwatches, events,
 * Lua state reinit, sysLoadEvent, flags, UI cleanup, two-phase correctness,
 * ANSI colors, map survival, telnet connection, Geyser reload, double reset,
 * and script event handler re-registration.
 *
 * The mudlet instance and profile are created once in initTestCase().
 * Each test calls resetProfile() at the end (via cleanup()) to restore
 * a clean state for the next test.
 *
 * Run with: ctest -R ResetProfileTest -V
 */

#include <QtTest/QtTest>

#include "Host.h"
#include "TAlias.h"
#include "TKey.h"
#include "TLuaInterpreter.h"
#include "TMainConsole.h"
#include "TMap.h"
#include "TRoomDB.h"
#include "TScript.h"
#include "TTimer.h"
#include "TTrigger.h"
#include "TelnetServerStub.h"
#include "ctelnet.h"
#include "dlgConnectionProfiles.h"
#include "mudlet.h"

extern "C" {
#if defined(INCLUDE_VERSIONED_LUA_HEADERS)
#include <lua5.1/lauxlib.h>
#include <lua5.1/lua.h>
#include <lua5.1/lualib.h>
#else
#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>
#endif
}

extern void qInitResources_mudlet();
extern void qInitResources_qm();
extern void qInitResources_additional_splash_screens();
extern void qInitResources_mudlet_fonts_common();
extern void qInitResources_mudlet_fonts_posix();
void initializeQRCResourcesForResetProfileTest();

class ResetProfileTest : public QObject {
  Q_OBJECT

private:
  TelnetServerStub *mpServer = nullptr;
  Host *mpHost = nullptr;
  const QString mHostname = "ResetProfile-Test";
  const QString mPort = "4003";
  const QString mLocalhost = "localhost";

  void performReset() {
    mpHost->resetProfile_phase1();
    QCoreApplication::processEvents();
  }

  int countTempTriggers() {
    int count = 0;
    for (auto *trigger : mpHost->getTriggerUnit()->getTriggerRootNodeList()) {
      if (trigger->isTemporary()) {
        ++count;
      }
    }
    return count;
  }

  int countTempAliases() {
    int count = 0;
    for (auto *alias : mpHost->getAliasUnit()->getAliasRootNodeList()) {
      if (alias->isTemporary()) {
        ++count;
      }
    }
    return count;
  }

  int countTempTimers() {
    int count = 0;
    for (auto *timer : mpHost->getTimerUnit()->getTimerRootNodeList()) {
      if (timer->isTemporary()) {
        ++count;
      }
    }
    return count;
  }

  int countTempKeys() {
    int count = 0;
    for (auto *key : mpHost->getKeyUnit()->getKeyRootNodeList()) {
      if (key->isTemporary()) {
        ++count;
      }
    }
    return count;
  }

private slots:
  void initTestCase() {
    initializeQRCResourcesForResetProfileTest();

    mpServer = new TelnetServerStub(qApp);
    mpServer->start(mLocalhost, mPort.toUShort());
    mudlet::start();
    mudlet::self()->setupConfig();
    mudlet::self()->takeOwnershipOfInstanceCoordinator(
        std::make_unique<MudletInstanceCoordinator>(
            "MudletInstanceCoordinator"));
    mudlet::self()->init();
    mudlet::self()->setStorePasswordsSecurely(false);
    deleteProfileDirectory(mHostname);

    startProfile(mHostname, mLocalhost, mPort);
    mpHost = mudlet::self()->getActiveHost();
    QVERIFY2(mpHost, "No active host after profile creation");
  }

  void cleanupTestCase() {
    mpHost = nullptr;
    delete mpServer;
    mpServer = nullptr;
    deleteProfileDirectory(mHostname);
    delete mudlet::self();
  }

  // Per-test cleanup: reset the profile so each test starts from clean state.
  // Tests that exercise resetProfile() themselves may leave the profile already
  // reset, which is fine — a second reset is harmless.
  void cleanup() {
    if (mpHost && !mpHost->mResetProfile) {
      performReset();
    } else if (mpHost && mpHost->mResetProfile) {
      QCoreApplication::processEvents();
    }
  }

  // -----------------------------------------------------------------------
  // Group 1: Temporary Item Removal (#761)
  // -----------------------------------------------------------------------

  void test_tempTriggersRemovedAfterReset() {
    int id = mpHost->mLuaInterpreter.startTempTrigger(qsl("test_pattern"),
                                                      qsl(""), -1);
    QVERIFY(id > 0);
    QVERIFY(countTempTriggers() > 0);

    performReset();

    QCOMPARE(countTempTriggers(), 0);
  }

  void test_tempAliasesRemovedAfterReset() {
    int id =
        mpHost->mLuaInterpreter.startTempAlias(qsl("^test_alias$"), qsl(""));
    QVERIFY(id > 0);
    QVERIFY(mpHost->getAliasUnit()->getAlias(id) != nullptr);

    performReset();

    // Our specific temp alias should be gone (Lua scripts may create their own
    // via sysLoadEvent)
    QVERIFY2(mpHost->getAliasUnit()->getAlias(id) == nullptr,
             "Temp alias should be removed after reset");
  }

  void test_tempTimersRemovedAfterReset() {
    auto result = mpHost->mLuaInterpreter.startTempTimer(60.0, qsl(""));
    int id = result.first;
    QVERIFY(id > 0);
    QVERIFY(mpHost->getTimerUnit()->getTimer(id) != nullptr);

    performReset();

    QVERIFY2(mpHost->getTimerUnit()->getTimer(id) == nullptr,
             "Temp timer should be removed after reset");
  }

  void test_tempKeysRemovedAfterReset() {
    int modifier = Qt::NoModifier;
    int keycode = Qt::Key_F12;
    int id = mpHost->mLuaInterpreter.startTempKey(modifier, keycode, qsl(""));
    QVERIFY(id > 0);
    QVERIFY(countTempKeys() > 0);

    performReset();

    QCOMPARE(countTempKeys(), 0);
  }

  // -----------------------------------------------------------------------
  // Group 2: Permanent Item Survival
  // -----------------------------------------------------------------------

  void test_permanentTriggerSurvivesReset() {
    QStringList patterns;
    patterns << qsl("perm_test_pattern");
    auto [id, msg] = mpHost->mLuaInterpreter.startPermSubstringTrigger(
        qsl("perm_trigger"), qsl(""), patterns, qsl(""));
    QVERIFY2(id > 0, qPrintable(msg));

    performReset();

    bool found = false;
    for (auto *trigger : mpHost->getTriggerUnit()->getTriggerRootNodeList()) {
      if (trigger->getID() == id) {
        found = true;
        QVERIFY(trigger->isActive());
        break;
      }
    }
    QVERIFY2(found, "Permanent trigger not found after reset");
  }

  void test_permanentAliasSurvivesReset() {
    auto [id, msg] = mpHost->mLuaInterpreter.startPermAlias(
        qsl("perm_alias"), qsl(""), qsl("^perm_test$"), qsl(""));
    QVERIFY2(id > 0, qPrintable(msg));

    performReset();

    bool found = false;
    for (auto *alias : mpHost->getAliasUnit()->getAliasRootNodeList()) {
      if (alias->getID() == id) {
        found = true;
        QVERIFY(alias->isActive());
        break;
      }
    }
    QVERIFY2(found, "Permanent alias not found after reset");
  }

  void test_permanentTimerSurvivesReset() {
    auto [id, msg] = mpHost->mLuaInterpreter.startPermTimer(
        qsl("perm_timer"), qsl(""), 300.0, qsl(""));
    QVERIFY2(id > 0, qPrintable(msg));

    performReset();

    bool found = false;
    for (auto *timer : mpHost->getTimerUnit()->getTimerRootNodeList()) {
      if (timer->getID() == id) {
        found = true;
        break;
      }
    }
    QVERIFY2(found, "Permanent timer not found after reset");
  }

  // -----------------------------------------------------------------------
  // Group 3: Stopwatch Handling (#4715)
  // -----------------------------------------------------------------------

  void test_nonPersistentStopWatchRemovedAfterReset() {
    mpHost->mBlockStopWatchCreation = false;
    auto result = mpHost->createStopWatch(qsl("test_sw"));
    int id = result.first;
    QVERIFY2(id > 0, qPrintable(result.second));
    QVERIFY(mpHost->getStopWatch(id) != nullptr);

    performReset();

    QVERIFY2(mpHost->getStopWatch(id) == nullptr,
             "Non-persistent stopwatch should be removed after reset");
  }

  void test_persistentStopWatchSurvivesReset() {
    mpHost->mBlockStopWatchCreation = false;
    auto result = mpHost->createStopWatch(qsl("persistent_sw"));
    int id = result.first;
    QVERIFY2(id > 0, qPrintable(result.second));
    mpHost->makeStopWatchPersistent(id, true);

    performReset();

    auto *sw = mpHost->getStopWatch(id);
    QVERIFY2(sw != nullptr, "Persistent stopwatch should survive reset");
    QVERIFY(sw->persistent());
  }

  void test_stopWatchCreationBlockedDuringReset() {
    mpHost->mBlockStopWatchCreation = false;

    mpHost->resetProfile_phase1();
    // mResetProfile is now true — stopwatch creation should be blocked
    auto result = mpHost->createStopWatch(qsl("blocked_sw"));
    QCOMPARE(result.first, 0);

    QCoreApplication::processEvents();

    // After phase2, creation should work again
    mpHost->mBlockStopWatchCreation = false;
    auto result2 = mpHost->createStopWatch(qsl("unblocked_sw"));
    QVERIFY2(result2.first > 0,
             "Stopwatch creation should work after reset completes");
  }

  // -----------------------------------------------------------------------
  // Group 4: Event System Cleanup (#4970, #7698)
  // -----------------------------------------------------------------------

  void test_eventHandlerMapClearedAfterReset() {
    mpHost->mEventHandlerMap[qsl("testEvent")] = QList<TScript *>();
    QVERIFY(!mpHost->mEventHandlerMap.isEmpty());

    performReset();

    // mEventHandlerMap is cleared, then repopulated by compileAll
    // re-registering script handlers. Our manually-inserted "testEvent" entry
    // should be gone.
    QVERIFY2(!mpHost->mEventHandlerMap.contains(qsl("testEvent")),
             "Manually inserted event handler should be cleared after reset");
  }

  void test_anonymousEventHandlersSurviveResetButAreFunctionallyStale() {
    lua_State *L = mpHost->mLuaInterpreter.getLuaGlobalState();
    luaL_dostring(
        L, "function anonHandlerTestFunc() anonHandlerCalled = true end");
    mpHost->registerAnonymousEventHandler(qsl("testAnonymousEvent"),
                                          qsl("anonHandlerTestFunc"));

    performReset();

    // The Lua function no longer exists in the new state — the handler is stale
    lua_State *newL = mpHost->mLuaInterpreter.getLuaGlobalState();
    lua_getglobal(newL, "anonHandlerTestFunc");
    QVERIFY2(lua_isnil(newL, -1), "Lua function referenced by anonymous "
                                  "handler should not exist in new Lua state");
    lua_pop(newL, 1);
  }

  // -----------------------------------------------------------------------
  // Group 5: Lua State Reinitialization (#3692)
  // -----------------------------------------------------------------------

  void test_luaCustomGlobalsClearedAfterReset() {
    lua_State *L = mpHost->mLuaInterpreter.getLuaGlobalState();
    QVERIFY(L);
    luaL_dostring(L, "resetProfileTestGlobal = 42");

    lua_getglobal(L, "resetProfileTestGlobal");
    QVERIFY(!lua_isnil(L, -1));
    lua_pop(L, 1);

    performReset();

    lua_State *newL = mpHost->mLuaInterpreter.getLuaGlobalState();
    lua_getglobal(newL, "resetProfileTestGlobal");
    QVERIFY2(lua_isnil(newL, -1),
             "Custom Lua global should be gone after reset");
    lua_pop(newL, 1);
  }

  void test_luaStandardFunctionsAvailableAfterReset() {
    performReset();

    lua_State *L = mpHost->mLuaInterpreter.getLuaGlobalState();
    QVERIFY(L);

    lua_getglobal(L, "assert");
    QVERIFY2(!lua_isnil(L, -1), "assert should be available after reset");
    lua_pop(L, 1);

    lua_getglobal(L, "pcall");
    QVERIFY2(!lua_isnil(L, -1), "pcall should be available after reset");
    lua_pop(L, 1);

    lua_getglobal(L, "string");
    QVERIFY2(!lua_isnil(L, -1),
             "string library should be available after reset");
    lua_pop(L, 1);

    lua_getglobal(L, "table");
    QVERIFY2(!lua_isnil(L, -1),
             "table library should be available after reset");
    lua_pop(L, 1);
  }

  void test_luaStateIsNewAfterReset() {
    lua_State *oldL = mpHost->mLuaInterpreter.getLuaGlobalState();
    QVERIFY(oldL);

    performReset();

    lua_State *newL = mpHost->mLuaInterpreter.getLuaGlobalState();
    QVERIFY(newL);
    QVERIFY2(
        oldL != newL,
        "Lua state pointer should change after reset (new state allocated)");
  }

  // -----------------------------------------------------------------------
  // Group 6: sysLoadEvent (#4779, #5005)
  // -----------------------------------------------------------------------

  void test_sysLoadEventFiredExactlyOnceWithFalse() {
    auto *pScript = new TScript(qsl("sysLoadEventCounter"), mpHost);
    pScript->setScript(qsl("sysLoadCounter = (sysLoadCounter or 0) + 1"));
    QStringList events;
    events << qsl("sysLoadEvent");
    pScript->setEventHandlerList(events);
    mpHost->getScriptUnit()->registerScript(pScript);
    pScript->setIsActive(true);
    pScript->compile();

    performReset();

    lua_State *L = mpHost->mLuaInterpreter.getLuaGlobalState();
    lua_getglobal(L, "sysLoadCounter");
    QVERIFY2(!lua_isnil(L, -1),
             "sysLoadCounter should be set by the event handler");
    int counter = lua_tointeger(L, -1);
    lua_pop(L, 1);
    QCOMPARE(counter, 1);
  }

  // -----------------------------------------------------------------------
  // Group 7: Flag Lifecycle
  // -----------------------------------------------------------------------

  void test_mResetProfileFlagLifecycle() {
    QVERIFY2(!mpHost->mResetProfile,
             "mResetProfile should be false before reset");

    mpHost->resetProfile_phase1();
    QVERIFY2(mpHost->mResetProfile,
             "mResetProfile should be true after phase1");

    QCoreApplication::processEvents();
    QVERIFY2(!mpHost->mResetProfile,
             "mResetProfile should be false after phase2");
  }

  void test_mBlockScriptCompileFalseAfterReset() {
    mpHost->mBlockScriptCompile = true;

    performReset();

    QVERIFY2(!mpHost->mBlockScriptCompile,
             "mBlockScriptCompile should be false after reset");
  }

  void test_mEmergencyStopNotSetDuringReset() {
    QVERIFY2(!mpHost->mEmergencyStop,
             "mEmergencyStop should be false before reset");

    mpHost->resetProfile_phase1();
    QVERIFY2(!mpHost->mEmergencyStop,
             "mEmergencyStop should stay false during phase1 (phase1 calls "
             "unit-level stop, not Host::stopAllTriggers)");

    QCoreApplication::processEvents();
    QVERIFY2(!mpHost->mEmergencyStop,
             "mEmergencyStop should stay false after phase2");
  }

  // -----------------------------------------------------------------------
  // Group 8: UI Cleanup
  // -----------------------------------------------------------------------

  void test_miniConsolesRemovedAfterReset() {
    auto [ok, msg] =
        mpHost->createMiniConsole(qsl("main"), qsl("test_mc"), 0, 0, 100, 100);
    QVERIFY2(ok, qPrintable(msg));
    QVERIFY(mpHost->mpConsole->mSubConsoleMap.contains(qsl("test_mc")));

    performReset();

    QVERIFY2(!mpHost->mpConsole->mSubConsoleMap.contains(qsl("test_mc")),
             "Mini console should be removed after reset");
  }

  void test_labelsRemovedAfterReset() {
    auto [ok, msg] = mpHost->createLabel(qsl("main"), qsl("test_label"), 0, 0,
                                         100, 100, true, false);
    QVERIFY2(ok, qPrintable(msg));
    QVERIFY(mpHost->mpConsole->mLabelMap.contains(qsl("test_label")));

    performReset();

    QVERIFY2(!mpHost->mpConsole->mLabelMap.contains(qsl("test_label")),
             "Label should be removed after reset");
  }

  // -----------------------------------------------------------------------
  // Group 9: Two-Phase Correctness
  // -----------------------------------------------------------------------

  void test_phase2RunsDeferredNotImmediate() {
    int triggerId = mpHost->mLuaInterpreter.startTempTrigger(
        qsl("deferred_test"), qsl(""), -1);
    QVERIFY(triggerId > 0);

    mpHost->resetProfile_phase1();

    // Phase2 has NOT run yet
    QVERIFY2(mpHost->mResetProfile,
             "mResetProfile should be true before processEvents");
    QVERIFY2(countTempTriggers() > 0,
             "Temp triggers should still exist before processEvents");

    QCoreApplication::processEvents();

    QVERIFY2(!mpHost->mResetProfile,
             "mResetProfile should be false after processEvents");
    QCOMPARE(countTempTriggers(), 0);
  }

  // -----------------------------------------------------------------------
  // Group 10: ANSI Color Table
  // -----------------------------------------------------------------------

  void test_ansiColorTableUpdatedAfterReset() {
    mpHost->mBlack = QColor(10, 20, 30);

    performReset();

    lua_State *L = mpHost->mLuaInterpreter.getLuaGlobalState();
    QVERIFY(L);

    lua_getglobal(L, "color_table");
    QVERIFY2(!lua_isnil(L, -1), "color_table should exist after reset");

    lua_getfield(L, -1, "ansi_000");
    QVERIFY2(!lua_isnil(L, -1), "color_table.ansi_000 should exist");

    lua_rawgeti(L, -1, 1);
    int r = lua_tointeger(L, -1);
    lua_pop(L, 1);

    lua_rawgeti(L, -1, 2);
    int g = lua_tointeger(L, -1);
    lua_pop(L, 1);

    lua_rawgeti(L, -1, 3);
    int b = lua_tointeger(L, -1);
    lua_pop(L, 1);

    lua_pop(L, 2);

    QCOMPARE(r, 10);
    QCOMPARE(g, 20);
    QCOMPARE(b, 30);
  }

  // -----------------------------------------------------------------------
  // Group 11: Map System Survival
  // -----------------------------------------------------------------------

  void test_mapDataSurvivesReset() {
    QVERIFY(mpHost->mpMap);
    int roomId = mpHost->mpMap->createNewRoomID();
    QVERIFY(roomId > 0);
    QVERIFY(mpHost->mpMap->addRoom(roomId));

    performReset();

    QVERIFY2(mpHost->mpMap, "TMap pointer should survive reset");
    auto *room = mpHost->mpMap->mpRoomDB->getRoom(roomId);
    QVERIFY2(room != nullptr, "Room added before reset should still exist");
  }

  // -----------------------------------------------------------------------
  // Group 12: Telnet Connection Survives
  // -----------------------------------------------------------------------

  void test_telnetConnectionSurvivesReset() {
    auto stateBefore = mpHost->mTelnet.getConnectionState();
    QCOMPARE(stateBefore, QAbstractSocket::ConnectedState);

    performReset();

    auto stateAfter = mpHost->mTelnet.getConnectionState();
    QCOMPARE(stateAfter, QAbstractSocket::ConnectedState);
  }

  // -----------------------------------------------------------------------
  // Group 13: Geyser/LuaGlobal Framework Reload
  // -----------------------------------------------------------------------

  void test_geyserFrameworkAvailableAfterReset() {
    performReset();

    lua_State *L = mpHost->mLuaInterpreter.getLuaGlobalState();
    QVERIFY(L);

    lua_getglobal(L, "Geyser");
    QVERIFY2(!lua_isnil(L, -1),
             "Geyser framework should be loaded after reset");
    lua_pop(L, 1);
  }

  void test_gmcpTableFreshAfterReset() {
    lua_State *L = mpHost->mLuaInterpreter.getLuaGlobalState();
    luaL_dostring(L, "gmcp.testField = 'stale_data'");

    performReset();

    lua_State *newL = mpHost->mLuaInterpreter.getLuaGlobalState();
    lua_getglobal(newL, "gmcp");
    QVERIFY2(!lua_isnil(newL, -1), "gmcp table should exist after reset");

    lua_getfield(newL, -1, "testField");
    QVERIFY2(lua_isnil(newL, -1),
             "gmcp.testField should be nil after reset (fresh table)");
    lua_pop(newL, 2);
  }

  // -----------------------------------------------------------------------
  // Group 14: Double Reset Safety
  // -----------------------------------------------------------------------

  void test_doubleResetDoesNotCrash() {
    mpHost->mLuaInterpreter.startTempTrigger(qsl("double_reset_test"), qsl(""),
                                             -1);

    // Call phase1 twice before any processEvents
    mpHost->resetProfile_phase1();
    mpHost->resetProfile_phase1();

    QCoreApplication::processEvents();

    QVERIFY2(!mpHost->mResetProfile,
             "mResetProfile should be false after double reset");
    QCOMPARE(countTempTriggers(), 0);

    lua_State *L = mpHost->mLuaInterpreter.getLuaGlobalState();
    QVERIFY(L);
    lua_getglobal(L, "assert");
    QVERIFY(!lua_isnil(L, -1));
    lua_pop(L, 1);
  }

  // -----------------------------------------------------------------------
  // Group 15: Script Recompilation and Event Handler Re-registration
  // -----------------------------------------------------------------------

  void test_permanentScriptEventHandlerReregisteredAfterReset() {
    const QString eventName = qsl("customResetTestEvent");

    auto *pScript = new TScript(qsl("eventHandlerScript"), mpHost);
    pScript->setScript(qsl("-- handler for customResetTestEvent"));
    QStringList events;
    events << eventName;
    pScript->setEventHandlerList(events);
    mpHost->getScriptUnit()->registerScript(pScript);
    pScript->setIsActive(true);
    pScript->compile();

    QVERIFY2(mpHost->mEventHandlerMap.contains(eventName),
             "Event handler should be registered before reset");

    performReset();

    QVERIFY2(
        mpHost->mEventHandlerMap.contains(eventName),
        "Event handler should be re-registered after reset via compileAll");
  }

  // -----------------------------------------------------------------------
  // Group 16: Border Size Preservation (#8871)
  // -----------------------------------------------------------------------

  void test_borderSizesPreservedAfterReset() {
    QMargins bordersBefore = mpHost->borders();

    performReset();

    QMargins bordersAfter = mpHost->borders();
    QCOMPARE(bordersAfter, bordersBefore);
  }

  // -----------------------------------------------------------------------
  // Group 17: Multiple Temp Items Cleanup
  // -----------------------------------------------------------------------

  void test_multipleTempTriggersAllRemovedAfterReset() {
    for (int i = 0; i < 10; ++i) {
      int id = mpHost->mLuaInterpreter.startTempTrigger(
          qsl("multi_test_%1").arg(i), qsl(""), -1);
      QVERIFY(id > 0);
    }
    QVERIFY(countTempTriggers() >= 10);

    performReset();

    QCOMPARE(countTempTriggers(), 0);
  }

  // -----------------------------------------------------------------------
  // Group 18: Lua C Functions Re-registered
  // -----------------------------------------------------------------------

  void test_mudletLuaCFunctionsAvailableAfterReset() {
    performReset();

    lua_State *L = mpHost->mLuaInterpreter.getLuaGlobalState();
    QVERIFY(L);

    // Verify key Mudlet Lua API functions registered by initLuaGlobals()
    const char *functions[] = {
        "send",      "echo",       "tempTrigger",
        "tempAlias", "tempTimer",  "permSubstringTrigger",
        "permAlias", "permTimer",  "exists",
        "isActive",  "raiseEvent", "resetProfile"};

    for (const char *func : functions) {
      lua_getglobal(L, func);
      QVERIFY2(!lua_isnil(L, -1),
               qPrintable(qsl("Lua C function '%1' should be available after "
                              "reset")
                              .arg(func)));
      lua_pop(L, 1);
    }
  }

  // -----------------------------------------------------------------------
  // Group 19: Permanent Script Execution After Reset
  // -----------------------------------------------------------------------

  void test_permanentScriptExecutesAfterReset() {
    auto *pScript = new TScript(qsl("execTestScript"), mpHost);
    pScript->setScript(qsl("permScriptExecuted = true"));
    mpHost->getScriptUnit()->registerScript(pScript);
    pScript->setIsActive(true);
    pScript->compile();

    // Verify the script ran during compilation
    lua_State *L = mpHost->mLuaInterpreter.getLuaGlobalState();
    lua_getglobal(L, "permScriptExecuted");
    QVERIFY(!lua_isnil(L, -1));
    lua_pop(L, 1);

    performReset();

    // After reset, the script should have been recompiled and re-executed
    lua_State *newL = mpHost->mLuaInterpreter.getLuaGlobalState();
    lua_getglobal(newL, "permScriptExecuted");
    QVERIFY2(!lua_isnil(newL, -1),
             "Permanent script body should execute after reset");
    lua_pop(newL, 1);
  }

  // -----------------------------------------------------------------------
  // Group 20: Echo Flag Not Auto-Reset
  // -----------------------------------------------------------------------

  void test_echoFlagPreservedAcrossReset() {
    // The echo flag is telnet connection state — it should persist
    // across resetProfile since the connection stays alive
    mpHost->setRemoteEchoingActive(true);
    QVERIFY(mpHost->isRemoteEchoingActive());

    performReset();

    QVERIFY2(mpHost->isRemoteEchoingActive(),
             "Remote echoing flag should persist across reset (connection "
             "state, not profile state)");

    // Clean up
    mpHost->setRemoteEchoingActive(false);
  }

  // -----------------------------------------------------------------------
  // Helpers (reused from TOscTerminatorTest pattern)
  // -----------------------------------------------------------------------

  void startProfile(const QString &hostname, const QString &address,
                    const QString &port) {
    QTimer::singleShot(0, qApp, [hostname, address, port]() {
      mudlet::self()->startAutoLogin({});
      QTest::qWait(100);
      QTest::mouseClick(mudlet::self()->mpConnectionDialog->new_profile_button,
                        Qt::LeftButton);
      QTest::qWait(100);
      QTest::keyClicks(QApplication::focusWidget(), hostname);
      QTest::qWait(100);
      QTest::keyClick(QApplication::focusWidget(), Qt::Key_Tab);
      QTest::qWait(100);
      QTest::keyClicks(QApplication::focusWidget(), address);
      QTest::qWait(100);
      QTest::keyClick(QApplication::focusWidget(), Qt::Key_Tab);
      QTest::qWait(100);
      QTest::keyClicks(QApplication::focusWidget(), port);
      QTest::qWait(100);
      QTest::keyClick(QApplication::focusWidget(), Qt::Key_Return);
    });

    QSignalSpy spy(mudlet::self(), &mudlet::signal_profileLoaded);
    if (!spy.wait(1000)) {
      QFAIL("Profile took too long to load.");
    }
    auto host = mudlet::self()->getActiveHost();
    if (!host) {
      QFAIL("No active host available for the test.");
    }

    QSignalSpy spy2(&(host->mTelnet), &cTelnet::signal_connected);
    if (!spy2.wait(500)) {
      QFAIL("Could not connect with the host.");
    }
  }

  void deleteProfileDirectory(const QString &profileName) {
    const QString path =
        mudlet::getMudletPath(enums::profileHomePath, profileName);
    QDir dir(path);

    if (!dir.exists()) {
      return;
    }
    dir.removeRecursively();
  }
};

void initializeQRCResourcesForResetProfileTest() {
#ifdef INCLUDE_VARIABLE_SPLASH_SCREEN
  qInitResources_additional_splash_screens();
#endif
#ifdef INCLUDE_FONTS
  qInitResources_mudlet_fonts_common();
#if defined(Q_OS_LINUX) || defined(Q_OS_FREEBSD)
  qInitResources_mudlet_fonts_posix();
#endif
#endif
  qInitResources_mudlet();
  qInitResources_qm();
}

#include "ResetProfileTest.moc"
QTEST_MAIN(ResetProfileTest)
