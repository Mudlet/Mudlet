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
 * A temp trigger/timer/alias/key created with a function argument stores that
 * function as an anonymous callback in the Lua registry (keyed by the item
 * pointer) and flags mRegisteredAnonymousLuaFunction. Replacing its script via
 * setScript() must leave that callback mode: it has to release the old function
 * from the registry (otherwise the entry leaks, as the destructor's mScript-based
 * branch then deletes the compiled function rather than the callback) and clear
 * the flag (otherwise execute() keeps calling the stale function and the new
 * script never runs, for triggers/aliases/keys which gate execution on the flag).
 *
 * Run with: ctest -R SetScriptCallbackTest -V
 */

#include <QFileInfo>
#include <QTemporaryDir>
#include <QtTest/QtTest>

#include "ProfileTestHelper.h"
#include "AliasUnit.h"
#include "Host.h"
#include "KeyUnit.h"
#include "MudletInstanceCoordinator.h"
#include "TAlias.h"
#include "TKey.h"
#include "TLuaInterpreter.h"
#include "TTimer.h"
#include "TTrigger.h"
#include "TimerUnit.h"
#include "TriggerUnit.h"
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

#include "GroupedTest.h"

class SetScriptCallbackTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;
    TelnetServerStub* mpServer = nullptr;
    Host* mpHost = nullptr;
    const QString mHostname = "SetScriptCallback-Test";
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

    // The anonymous callback is stored in the Lua registry keyed by the item
    // pointer; a released callback leaves a nil entry there.
    bool registryEntryIsNil(void* item)
    {
        lua_State* L = mpHost->mLuaInterpreter.getLuaGlobalState();
        lua_pushlightuserdata(L, item);
        lua_rawget(L, LUA_REGISTRYINDEX);
        const bool result = lua_isnil(L, -1);
        lua_pop(L, 1);
        return result;
    }

    // setupConfig() consults portable.txt before the XDG logic
    static bool portableMarkerPresent()
    {
        return QFileInfo::exists(qsl("%1/portable.txt").arg(QCoreApplication::applicationDirPath())) || QFileInfo::exists(qsl("%1/.config/mudlet/portable.txt").arg(QDir::homePath()));
    }

private slots:
    void initTestCase()
    {
        if (portableMarkerPresent()) {
            QSKIP("portable.txt present - it takes precedence over XDG_CONFIG_HOME, so the config dir cannot be redirected");
        }

        // A config root of this process's own. Sharing the developer's
        // ~/.config/mudlet means sharing a profile list, so a second copy of
        // this test running at the same time is told the name it types is
        // already in use and never gets an enabled Connect button. Since #9712
        // the opt-in that makes setupConfig() adopt a directory is
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
        QCOMPARE(mudlet::getMudletPath(enums::mainPath), qsl("%1/mudlet").arg(mConfigDir.path()));
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
        // Null when initTestCase skipped or failed ahead of mudlet::start(), and
        // getMudletPath() dereferences the instance rather than checking it
        if (mudlet::self()) {
            deleteProfileDirectory(mHostname);
            delete mudlet::self();
        }
        mSavedXdg.isNull() ? qunsetenv("XDG_CONFIG_HOME") : qputenv("XDG_CONFIG_HOME", mSavedXdg);
    }

    // Trigger: execute() gates on the flag, so before the fix the stale function
    // keeps firing (new script never runs) AND its registry entry leaks.
    void test_triggerSetScriptReleasesCallback()
    {
        runLua(qsl("trigOld = 0\n"
                   "trigNew = 0\n"
                   "cbTrigId = tempRegexTrigger('^setscript_trig$', function() trigOld = trigOld + 1 end)\n"));
        const int id = luaInt(qsl("cbTrigId"));
        QVERIFY2(id > 0, "temp trigger with a function callback should be created");
        auto* pTrigger = mpHost->getTriggerUnit()->getTrigger(id);
        QVERIFY(pTrigger);
        QVERIFY2(pTrigger->mRegisteredAnonymousLuaFunction, "callback trigger should start in registered-function mode");

        // Sanity: the registered function is what fires before we replace it.
        runLua(qsl("feedTriggers('setscript_trig\\n')"));
        QCOMPARE(luaInt(qsl("trigOld")), 1);

        QVERIFY(pTrigger->setScript(qsl("trigNew = trigNew + 1")));

        QVERIFY2(!pTrigger->mRegisteredAnonymousLuaFunction, "setScript() must leave registered-function mode");
        QVERIFY2(registryEntryIsNil(pTrigger), "setScript() must release the old function from the Lua registry (no leak)");

        // Firing now runs the new script; the stale function must not fire again.
        runLua(qsl("feedTriggers('setscript_trig\\n')"));
        QCOMPARE(luaInt(qsl("trigNew")), 1);
        QCOMPARE(luaInt(qsl("trigOld")), 1);
    }

    // Alias: execute() also gates on the flag, so the same stale-function symptom
    // applies. Fire it directly through execute() since aliases match user input.
    void test_aliasSetScriptReleasesCallback()
    {
        runLua(qsl("aliasOld = 0\n"
                   "aliasNew = 0\n"
                   "cbAliasId = tempAlias('^setscript_alias$', function() aliasOld = aliasOld + 1 end)\n"));
        const int id = luaInt(qsl("cbAliasId"));
        QVERIFY2(id > 0, "temp alias with a function callback should be created");
        auto* pAlias = mpHost->getAliasUnit()->getAlias(id);
        QVERIFY(pAlias);
        QVERIFY2(pAlias->mRegisteredAnonymousLuaFunction, "callback alias should start in registered-function mode");

        pAlias->execute();
        QCOMPARE(luaInt(qsl("aliasOld")), 1);

        QVERIFY(pAlias->setScript(qsl("aliasNew = aliasNew + 1")));

        QVERIFY2(!pAlias->mRegisteredAnonymousLuaFunction, "setScript() must leave registered-function mode");
        QVERIFY2(registryEntryIsNil(pAlias), "setScript() must release the old function from the Lua registry (no leak)");

        pAlias->execute();
        QCOMPARE(luaInt(qsl("aliasNew")), 1);
        QCOMPARE(luaInt(qsl("aliasOld")), 1);
    }

    // Key: execute() gates on the flag as well.
    void test_keySetScriptReleasesCallback()
    {
        runLua(qsl("keyOld = 0\n"
                   "keyNew = 0\n"
                   "cbKeyId = tempKey(65, function() keyOld = keyOld + 1 end)\n"));
        const int id = luaInt(qsl("cbKeyId"));
        QVERIFY2(id > 0, "temp key with a function callback should be created");
        auto* pKey = mpHost->getKeyUnit()->getKey(id);
        QVERIFY(pKey);
        QVERIFY2(pKey->mRegisteredAnonymousLuaFunction, "callback key should start in registered-function mode");

        pKey->execute();
        QCOMPARE(luaInt(qsl("keyOld")), 1);

        QVERIFY(pKey->setScript(qsl("keyNew = keyNew + 1")));

        QVERIFY2(!pKey->mRegisteredAnonymousLuaFunction, "setScript() must leave registered-function mode");
        QVERIFY2(registryEntryIsNil(pKey), "setScript() must release the old function from the Lua registry (no leak)");

        pKey->execute();
        QCOMPARE(luaInt(qsl("keyNew")), 1);
        QCOMPARE(luaInt(qsl("keyOld")), 1);
    }

    // Timer: execute() discriminates on mScript rather than the flag, so the
    // stale-function symptom does not surface - but the registry entry still leaks
    // without the fix, which is what this asserts. A long timeout keeps the timer
    // from firing on its own during the test.
    void test_timerSetScriptReleasesCallback()
    {
        runLua(qsl("cbTimerId = tempTimer(100, function() end)\n"));
        const int id = luaInt(qsl("cbTimerId"));
        QVERIFY2(id > 0, "temp timer with a function callback should be created");
        auto* pTimer = mpHost->getTimerUnit()->getTimer(id);
        QVERIFY(pTimer);
        QVERIFY2(pTimer->mRegisteredAnonymousLuaFunction, "callback timer should start in registered-function mode");

        QVERIFY(pTimer->setScript(qsl("noop = 1")));

        QVERIFY2(!pTimer->mRegisteredAnonymousLuaFunction, "setScript() must leave registered-function mode");
        QVERIFY2(registryEntryIsNil(pTimer), "setScript() must release the old function from the Lua registry (no leak)");
    }

    // Clearing a callback item's script with setScript("") must also release the
    // callback and stop it firing: for triggers/aliases/keys execute() then takes the
    // empty-mScript early-out.
    void test_triggerSetScriptEmptyReleasesCallback()
    {
        runLua(qsl("clearOld = 0\n"
                   "clearTrigId = tempRegexTrigger('^setscript_clear$', function() clearOld = clearOld + 1 end)\n"));
        const int id = luaInt(qsl("clearTrigId"));
        QVERIFY2(id > 0, "temp trigger with a function callback should be created");
        auto* pTrigger = mpHost->getTriggerUnit()->getTrigger(id);
        QVERIFY(pTrigger);
        QVERIFY(pTrigger->mRegisteredAnonymousLuaFunction);

        runLua(qsl("feedTriggers('setscript_clear\\n')"));
        QCOMPARE(luaInt(qsl("clearOld")), 1);

        QVERIFY(pTrigger->setScript(QString()));

        QVERIFY2(!pTrigger->mRegisteredAnonymousLuaFunction, "setScript(\"\") must leave registered-function mode");
        QVERIFY2(registryEntryIsNil(pTrigger), "setScript(\"\") must release the old function from the Lua registry (no leak)");

        // With no script and no callback, firing must do nothing - the stale function
        // must not run.
        runLua(qsl("feedTriggers('setscript_clear\\n')"));
        QCOMPARE(luaInt(qsl("clearOld")), 1);
    }

    // Guard the common real-world path (the script editor replacing a normal,
    // string-created item's code): a non-callback item must never be pushed into
    // callback mode, and setScript() must still update its behavior normally.
    void test_stringCreatedTriggerSetScriptIsUnaffected()
    {
        runLua(qsl("plainA = 0\n"
                   "plainB = 0\n"
                   "plainTrigId = tempRegexTrigger('^setscript_plain$', 'plainA = plainA + 1')\n"));
        const int id = luaInt(qsl("plainTrigId"));
        QVERIFY2(id > 0, "string-created temp trigger should be created");
        auto* pTrigger = mpHost->getTriggerUnit()->getTrigger(id);
        QVERIFY(pTrigger);
        QVERIFY2(!pTrigger->mRegisteredAnonymousLuaFunction, "a string-created trigger is never in registered-function mode");

        runLua(qsl("feedTriggers('setscript_plain\\n')"));
        QCOMPARE(luaInt(qsl("plainA")), 1);

        QVERIFY(pTrigger->setScript(qsl("plainB = plainB + 1")));
        QVERIFY2(!pTrigger->mRegisteredAnonymousLuaFunction, "setScript() must not push a string item into registered-function mode");

        runLua(qsl("feedTriggers('setscript_plain\\n')"));
        QCOMPARE(luaInt(qsl("plainB")), 1);
        QCOMPARE(luaInt(qsl("plainA")), 1);
    }

    // Helpers (reused from the TFeedTriggersRecursionTest pattern)

    void startProfile(const QString& hostname, const QString& address, const QString& port)
    {
        auto host = TestProfile::create(hostname, address, port);
        if (!host) {
            QFAIL("No active host available for the test.");
        }

        QSignalSpy spy2(&(host->mTelnet), &cTelnet::signal_connected);
        if (!spy2.wait(2000)) {
            QFAIL("Could not connect with the host.");
        }
    }

    void deleteProfileDirectory(const QString& profileName)
    {
        const QString path = mudlet::getMudletPath(enums::profileHomePath, profileName);
        QDir dir(path);

        if (!dir.exists()) {
            return;
        }
        dir.removeRecursively();
    }
};

#include "SetScriptCallbackTest.moc"
MUDLET_GROUPED_TEST_MAIN(SetScriptCallbackTest)
