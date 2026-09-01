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

#include <QFileInfo>
#include <QTemporaryDir>
#include <QtTest/QtTest>
#include <chrono>
#include <QMouseEvent>

#include "PortableModeTestHelper.h"
#include "ProfileTestHelper.h"
#include "Host.h"
#include "LuaInterface.h"
#include "MudletInstanceCoordinator.h"
#include "TAlias.h"
#include "TEvent.h"
#include "TKey.h"
#include "TLabel.h"
#include "TLuaInterpreter.h"
#include "TMainConsole.h"
#include "TMap.h"
#include "TRoomDB.h"
#include "TScript.h"
#include "TTimer.h"
#include "TTrigger.h"
#include "TVar.h"
#include "TelnetServerStub.h"
#include "VarUnit.h"
#include "XMLexport.h"
#include "ctelnet.h"
#include "dlgConnectionProfiles.h"
#include "mapInfoContributorManager.h"
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

#include "GroupedTest.h"

using namespace std::chrono_literals;

class ResetProfileTest : public QObject {
  Q_OBJECT

private:
  QTemporaryDir mConfigDir;
  QByteArray mSavedXdg;
  TelnetServerStub *mpServer = nullptr;
  Host *mpHost = nullptr;
  const QString mHostname = "ResetProfile-Test";
  QString mPort; // assigned the stub's actual ephemeral port in initTestCase()
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
    if (portableMarkerPresent()) {
      QSKIP("portable.txt present - it takes precedence over XDG_CONFIG_HOME, "
            "so the config dir cannot be redirected");
    }

    // A config root of this process's own. Sharing the developer's
    // ~/.config/mudlet means sharing a profile list, so a second copy of this
    // test running at the same time is told the name it types is already in
    // use and never gets an enabled Connect button. Since #9712 the opt-in
    // that makes setupConfig() adopt a directory is
    // $XDG_CONFIG_HOME/mudlet/profiles, not the mudlet directory alone.
    QVERIFY(mConfigDir.isValid());
    QVERIFY(QDir().mkpath(qsl("%1/mudlet/profiles").arg(mConfigDir.path())));
    mSavedXdg = qgetenv("XDG_CONFIG_HOME");
    qputenv("XDG_CONFIG_HOME", mConfigDir.path().toUtf8());

    mpServer = new TelnetServerStub(qApp);
    mpServer->start(mLocalhost, 0); // ephemeral OS-assigned port avoids collisions across concurrent test runs
    mPort = QString::number(mpServer->serverPort());
    mudlet::start();
    mudlet::self()->setupConfig();
    QCOMPARE(mudlet::getMudletPath(enums::mainPath),
             qsl("%1/mudlet").arg(mConfigDir.path()));
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
    // Null when initTestCase skipped or failed ahead of mudlet::start(), and
    // getMudletPath() dereferences the instance rather than checking it
    if (mudlet::self()) {
      deleteProfileDirectory(mHostname);
      delete mudlet::self();
    }
    mSavedXdg.isNull() ? qunsetenv("XDG_CONFIG_HOME")
                       : qputenv("XDG_CONFIG_HOME", mSavedXdg);
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
    // preinstalled packages (the starter UI) register their own temp triggers
    // and re-register them when the profile comes back, so the count after a
    // reset returns to that baseline rather than zero
    const int baseline = countTempTriggers();
    int id = mpHost->mLuaInterpreter.startTempTrigger(qsl("test_pattern"),
                                                      qsl(""), -1);
    QVERIFY(id > 0);
    QVERIFY(countTempTriggers() > baseline);

    performReset();

    QCOMPARE(countTempTriggers(), baseline);
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

  void test_anonymousEventHandlersClearedAfterReset() {
    lua_State *L = mpHost->mLuaInterpreter.getLuaGlobalState();
    luaL_dostring(
        L, "function anonHandlerTestFunc() anonHandlerCalled = true end");
    mpHost->registerAnonymousEventHandler(qsl("testAnonymousEvent"),
                                          qsl("anonHandlerTestFunc"));

    performReset();

    // The Lua function no longer exists in the new state
    lua_State *newL = mpHost->mLuaInterpreter.getLuaGlobalState();
    lua_getglobal(newL, "anonHandlerTestFunc");
    QVERIFY2(lua_isnil(newL, -1), "Lua function referenced by anonymous "
                                  "handler should not exist in new Lua state");
    lua_pop(newL, 1);

    // Raise the event - if the anonymous handler map was properly cleared,
    // this won't attempt to call the now-nonexistent function
    TEvent event{};
    event.mArgumentList.append(qsl("testAnonymousEvent"));
    event.mArgumentTypeList.append(ARGUMENT_TYPE_STRING);
    mpHost->raiseEvent(event);
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
    lua_State *L = mpHost->mLuaInterpreter.getLuaGlobalState();
    QVERIFY(L);
    luaL_dostring(L, "resetTestSentinel = 42");

    performReset();

    lua_State *newL = mpHost->mLuaInterpreter.getLuaGlobalState();
    QVERIFY(newL);
    lua_getglobal(newL, "resetTestSentinel");
    QVERIFY2(lua_isnil(newL, -1), "Lua state should be fresh after reset");
    lua_pop(newL, 1);
  }

  // -----------------------------------------------------------------------
  // Group 6: sysLoadEvent (#4779, #5005)
  // -----------------------------------------------------------------------

  void test_sysLoadEventFiredExactlyOnce() {
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
    const int baseline = countTempTriggers();
    int triggerId = mpHost->mLuaInterpreter.startTempTrigger(
        qsl("deferred_test"), qsl(""), -1);
    QVERIFY(triggerId > 0);

    mpHost->resetProfile_phase1();

    // Phase2 has NOT run yet
    QVERIFY2(mpHost->mResetProfile,
             "mResetProfile should be true before processEvents");
    QVERIFY2(countTempTriggers() > baseline,
             "Temp triggers should still exist before processEvents");

    QCoreApplication::processEvents();

    QVERIFY2(!mpHost->mResetProfile,
             "mResetProfile should be false after processEvents");
    QCOMPARE(countTempTriggers(), baseline);
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

  void test_doubleResetIsGuarded() {
    const int baseline = countTempTriggers();
    mpHost->mLuaInterpreter.startTempTrigger(qsl("double_reset_test"), qsl(""),
                                             -1);

    lua_State *L = mpHost->mLuaInterpreter.getLuaGlobalState();
    luaL_dostring(L, "doubleResetSentinel = 99");

    // Call phase1 twice - the second call should be a no-op
    QVERIFY2(mpHost->resetProfile_phase1(),
             "First resetProfile_phase1() should succeed");
    QVERIFY2(!mpHost->resetProfile_phase1(),
             "Second resetProfile_phase1() should be guarded");

    QCoreApplication::processEvents();

    // Verify reset actually happened by checking the Lua state is fresh
    lua_State *afterL = mpHost->mLuaInterpreter.getLuaGlobalState();
    QVERIFY(afterL);
    lua_getglobal(afterL, "doubleResetSentinel");
    QVERIFY2(lua_isnil(afterL, -1), "Lua state should be fresh after reset");
    lua_pop(afterL, 1);
    QVERIFY2(!mpHost->mResetProfile,
             "mResetProfile should be false after reset");
    QCOMPARE(countTempTriggers(), baseline);
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
  // Group 16: Lua Registry Corruption (TLabel destructor timing)
  // -----------------------------------------------------------------------

  // PR #9141 added a TLabel destructor that frees its 7 callback indices
  // via luaL_unref. resetMainConsole() destroys labels via deleteLater(),
  // so without an explicit drain those destructors fire AFTER phase2 has
  // swapped in a new Lua state via initLuaGlobals(). The luaL_unref calls
  // then write into the new state's registry — any slots already populated
  // by compileAll() during the same phase2 (e.g. Adjustable.Container's
  // many label callbacks) get overwritten with freelist-chain numbers,
  // surfacing as "attempt to call a number value" the first time the user
  // hovers a label.
  //
  // The fix in Host::resetProfile_phase2() drains DeferredDelete events
  // between resetMainConsole() and initLuaGlobals(), so the old destructors
  // run their unrefs against the still-live original state. This test
  // reproduces the conditions: a script that creates many labels with
  // click callbacks at top level, so compileAll re-executes it during
  // phase2 and populates the new registry with the exact slot indices the
  // queued old destructors would have corrupted.
  void test_labelCallbacksWorkAfterResetWithManyLabels() {
    const int kNumLabels = 50;

    auto *pScript = new TScript(qsl("labelCallbackRegressionScript"), mpHost);
    pScript->setScript(qsl("labelCallbackHits = 0\n"
                           "for i = 1, %1 do\n"
                           "  local n = 'cb_label_'..i\n"
                           "  createLabel(n, 0, 0, 10, 10, true)\n"
                           "  setLabelClickCallback(n, function()\n"
                           "    labelCallbackHits = labelCallbackHits + 1\n"
                           "  end)\n"
                           "end\n")
                           .arg(kNumLabels));
    mpHost->getScriptUnit()->registerScript(pScript);
    pScript->setIsActive(true);
    pScript->compile();

    performReset();
    // Force any DeferredDelete events still pending after phase2 to fire
    // before we trigger callbacks. With the fix, phase2 already drained
    // them; without the fix, this is when corruption would land.
    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);

    for (int i = 1; i <= kNumLabels; ++i) {
      auto *pL = mpHost->mpConsole->mLabelMap.value(qsl("cb_label_%1").arg(i));
      QVERIFY2(pL, qPrintable(qsl("post-reset label cb_label_%1 missing").arg(i)));
      QMouseEvent ev(QEvent::MouseButtonPress, QPointF(1, 1), QPointF(1, 1),
                     QPointF(1, 1), Qt::LeftButton, Qt::LeftButton,
                     Qt::NoModifier);
      QCoreApplication::sendEvent(pL, &ev);
    }

    lua_State *L = mpHost->mLuaInterpreter.getLuaGlobalState();
    lua_getglobal(L, "labelCallbackHits");
    QVERIFY2(lua_isnumber(L, -1),
             "labelCallbackHits should be a number after firing callbacks");
    int hits = lua_tointeger(L, -1);
    lua_pop(L, 1);
    QCOMPARE(hits, kNumLabels);

    // Neutralize the script so it doesn't keep recreating labels in
    // subsequent tests' compileAll passes.
    pScript->setScript(qsl(""));
    pScript->setIsActive(false);
  }

  // -----------------------------------------------------------------------
  // Group 17: VarUnit saved/hidden variable bookkeeping (#9430 follow-up)
  // -----------------------------------------------------------------------

  // The VarUnit's savedVars/hiddenByUser sets are name-keyed and independent
  // of the lua_State (VarUnit::clear() deliberately preserves them), and
  // nothing repopulates them after a reset - only XMLimport at profile open
  // does. If replacing the LuaInterface in resetProfile_phase2() loses them,
  // the first profile save after a reset silently drops every user-saved
  // variable and hidden-variable preference from the profile XML.
  void test_savedAndHiddenVarSetsSurviveReset() {
    lua_State *L = mpHost->mLuaInterpreter.getLuaGlobalState();
    luaL_dostring(L, "resetSavedTestVar = 'important'");

    LuaInterface *lI = mpHost->getLuaInterface();
    VarUnit *vu = lI->getVarUnit();
    lI->getVars(false);
    TVar *var = findGlobalVar(vu, qsl("resetSavedTestVar"));
    QVERIFY2(var, "test variable not found in the variable tree");
    // as the Variables view does when the user ticks the save checkbox:
    vu->addSavedVar(var);
    // and as it does when the user hides a variable:
    vu->addHidden(qsl("resetHiddenTestVar"));
    QVERIFY(vu->savedVars.contains(qsl("resetSavedTestVar")));
    QVERIFY(vu->hiddenByUser.contains(qsl("resetHiddenTestVar")));

    performReset();

    VarUnit *newVu = mpHost->getLuaInterface()->getVarUnit();
    QVERIFY2(newVu->savedVars.contains(qsl("resetSavedTestVar")),
             "user's saved-variable marking should survive resetProfile()");
    QVERIFY2(newVu->hiddenByUser.contains(qsl("resetHiddenTestVar")),
             "user's hidden-variable preference should survive resetProfile()");
  }

  // End-to-end version of the above: the saved variable must still be
  // written out to profile XML after a reset.
  void test_savedVariableExportedToXmlAfterReset() {
    lua_State *L = mpHost->mLuaInterpreter.getLuaGlobalState();
    luaL_dostring(L, "xmlSavedTestVar = 'survives'");

    LuaInterface *lI = mpHost->getLuaInterface();
    VarUnit *vu = lI->getVarUnit();
    lI->getVars(false);
    TVar *var = findGlobalVar(vu, qsl("xmlSavedTestVar"));
    QVERIFY2(var, "test variable not found in the variable tree");
    vu->addSavedVar(var);

    performReset();

    // the reset wiped the Lua value; user scripts recreate it on
    // sysLoadEvent, and opening the Variables view rebuilds the tree
    lua_State *newL = mpHost->mLuaInterpreter.getLuaGlobalState();
    luaL_dostring(newL, "xmlSavedTestVar = 'survives'");
    mpHost->getLuaInterface()->getVars(false);

    const QString xmlPath =
        mudlet::getMudletPath(enums::profileHomePath, mHostname) +
        qsl("/reset-var-test.xml");
    auto writer = std::make_shared<XMLexport>(mpHost);
    QVERIFY(writer->exportPackage(xmlPath, true, false));
    QFile file(xmlPath);
    QVERIFY(file.open(QIODevice::ReadOnly | QIODevice::Text));
    const QString xml = QString::fromUtf8(file.readAll());
    file.close();
    QFile::remove(xmlPath);
    QVERIFY2(xml.contains(qsl("xmlSavedTestVar")),
             "saved variable should still be exported to profile XML after "
             "a reset");
  }

  // A fresh profile load runs hideMudletsVariables() right after
  // loadGlobal() so the Variables view only shows the user's variables;
  // a reset must do the same or the view fills up with Mudlet's entire
  // internal Lua API.
  void test_mudletInternalVariablesHiddenAfterReset() {
    mpHost->hideMudletsVariables();
    VarUnit *vu = mpHost->getLuaInterface()->getVarUnit();
    QVERIFY(vu->hidden.contains(qsl("color_table")));
    QVERIFY(vu->hidden.contains(qsl("Geyser")));

    performReset();

    VarUnit *newVu = mpHost->getLuaInterface()->getVarUnit();
    QVERIFY2(newVu->hidden.contains(qsl("color_table")),
             "Mudlet's internal variables should be re-hidden after reset, "
             "as on profile load");
    QVERIFY2(newVu->hidden.contains(qsl("Geyser")),
             "Mudlet's internal variables should be re-hidden after reset, "
             "as on profile load");
  }

  // -----------------------------------------------------------------------
  // Group 18: MapInfoContributorManager stale lua_State (#10001)
  // -----------------------------------------------------------------------

  // registerMapInfo() stores the lua_State it was called on plus a registry
  // reference, and the callback captures that state too, while phase2's
  // initLuaGlobals() closes it. A contributor left registered across a reset
  // therefore holds a dangling state: the next killMapInfo()/registerMapInfo()
  // on that name unrefs into freed memory - which a package that does both in
  // its init hits from compileAll() later in the same phase2 - and one that
  // nothing re-registers is called on that state by every later map redraw.
  void test_luaMapInfoContributorsRemovedByReset() {
    auto *pManager = mpHost->mpMap->mMapInfoContributorManager;
    QVERIFY(pManager);
    QSignalSpy spy(pManager,
                   &MapInfoContributorManager::signal_contributorsUpdated);
    lua_State *L = mpHost->mLuaInterpreter.getLuaGlobalState();
    QCOMPARE(luaL_dostring(L, "registerMapInfo('reset.contrib', function() "
                              "return 'info' end)\n"
                              "registerMapInfo('reset.contrib2', function() "
                              "return 'info' end)"),
             0);
    QVERIFY(pManager->getContributorKeys().contains(qsl("reset.contrib")));
    QVERIFY(pManager->getContributorKeys().contains(qsl("reset.contrib2")));
    spy.clear();

    performReset();

    QCOMPARE(mpHost->mpMap->mMapInfoContributorManager, pManager);
    QVERIFY2(!pManager->getContributorKeys().contains(qsl("reset.contrib")),
             "Lua map info contributor should not outlive the lua_State it was "
             "registered on");
    QVERIFY2(!pManager->getContributorKeys().contains(qsl("reset.contrib2")),
             "every Lua map info contributor should go, not just one");
    QVERIFY2(pManager->getContributorKeys().contains(qsl("Short")),
             "built-in contributors have no Lua reference and must stay");
    QVERIFY2(pManager->getContributorKeys().contains(qsl("Full")),
             "built-in contributors have no Lua reference and must stay");
    QVERIFY2(
        !spy.isEmpty(),
        "the mapper rebuilds its info menu from signal_contributorsUpdated");
  }

  // The reporter's package kills a contributor before re-registering it, and
  // after a reset that kill is the call that unrefs into the closed state. The
  // dangling unref is not what fails here without the fix: liblua is linked as
  // a prebuilt system library and so is not ASan-instrumented, and ASan leaves
  // freed memory intact by default, so on Linux the read of the closed state
  // goes unnoticed (on Windows it crashes with an access violation). What goes
  // red is the kill still finding a contributor - which is also what a fix that
  // dropped the name from the ordering but left it in the contributor map would
  // do.
  void test_mapInfoKillAfterResetFindsNothingToRemove() {
    lua_State *L = mpHost->mLuaInterpreter.getLuaGlobalState();
    QCOMPARE(luaL_dostring(L, "registerMapInfo('reset.stale', function() "
                              "return 'info' end)"),
             0);

    performReset();

    lua_State *newL = mpHost->mLuaInterpreter.getLuaGlobalState();
    QCOMPARE(luaL_dostring(newL,
                           "resetStaleKilled, resetStaleMessage = "
                           "killMapInfo('reset.stale')\n"
                           "registerMapInfo('reset.stale', function() return "
                           "'info' end)"),
             0);
    lua_getglobal(newL, "resetStaleKilled");
    QVERIFY2(lua_isnil(newL, -1),
             "the reset should have left killMapInfo() nothing to remove");
    lua_pop(newL, 1);
    lua_getglobal(newL, "resetStaleMessage");
    QVERIFY2(
        lua_isstring(newL, -1),
        "killMapInfo() should report the label as missing, as it does on a "
        "profile that has just been loaded");
    lua_pop(newL, 1);
    QVERIFY2(mpHost->mpMap->mMapInfoContributorManager->getContributorKeys()
                 .contains(qsl("reset.stale")),
             "re-registering after a reset should work");

    QCOMPARE(luaL_dostring(newL, "killMapInfo('reset.stale')"), 0);
  }

  // registerMapInfo() can be given a built-in's name, which replaces the
  // built-in callback. Dropping that on reset must not leave the profile with
  // fewer contributors than a freshly loaded one has.
  void test_builtinMapInfoRestoredAfterShadowingContributorDropped() {
    auto *pManager = mpHost->mpMap->mMapInfoContributorManager;
    lua_State *L = mpHost->mLuaInterpreter.getLuaGlobalState();
    QCOMPARE(luaL_dostring(L, "registerMapInfo('Short', function() return "
                              "'shadowed' end)"),
             0);

    performReset();

    QVERIFY2(pManager->getContributorKeys().contains(qsl("Short")),
             "the built-in contributor should be back once the Lua one that "
             "replaced it is dropped");
    QColor color;
    auto info = pManager->getContributor(qsl("Short"))(0, 0, -1, -1, color);
    QVERIFY2(info.text.isEmpty(),
             "'Short' should be Mudlet's own contributor again, which reports "
             "nothing for a room that does not exist");
  }

  // Dropping the contributors is only acceptable because the script that
  // registered them runs again in the same reset, in compileAll(); the enabled
  // state is the user's saved choice and is deliberately not dropped with them,
  // so the contributor comes back exactly as it was.
  void test_scriptRegisteredMapInfoReturnsAfterReset() {
    auto *pScript = new TScript(qsl("mapInfoRegisteringScript"), mpHost);
    pScript->setScript(qsl("registerMapInfo('reset.pkg', function() return "
                           "'info' end)"));
    mpHost->getScriptUnit()->registerScript(pScript);
    pScript->setIsActive(true);
    pScript->compile();
    lua_State *L = mpHost->mLuaInterpreter.getLuaGlobalState();
    QCOMPARE(luaL_dostring(L, "enableMapInfo('reset.pkg')"), 0);
    QVERIFY(mpHost->mMapInfoContributors.contains(qsl("reset.pkg")));

    performReset();

    lua_State *newL = mpHost->mLuaInterpreter.getLuaGlobalState();
    QCOMPARE(luaL_dostring(newL, "resetPkgEnabled = getMapInfo()['reset.pkg']"),
             0);
    lua_getglobal(newL, "resetPkgEnabled");
    QVERIFY2(lua_isboolean(newL, -1),
             "the registering script should have put its contributor back");
    QVERIFY2(lua_toboolean(newL, -1),
             "the user's enabled choice should survive the reset");
    lua_pop(newL, 1);

    // stop the script re-registering into every later test's reset
    pScript->setScript(qsl(""));
    pScript->setIsActive(false);
    QCOMPARE(luaL_dostring(newL, "killMapInfo('reset.pkg')"), 0);
  }

  // -----------------------------------------------------------------------
  // Helpers (reused from TOscTerminatorTest pattern)
  // -----------------------------------------------------------------------

  TVar *findGlobalVar(VarUnit *vu, const QString &name) {
    for (auto *child : vu->getBase()->getChildren(false)) {
      if (child->getName() == name) {
        return child;
      }
    }
    return nullptr;
  }

  void startProfile(const QString &hostname, const QString &address,
                    const QString &port) {
    auto host = TestProfile::create(hostname, address, port);
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

#include "ResetProfileTest.moc"
MUDLET_GROUPED_TEST_MAIN(ResetProfileTest)
