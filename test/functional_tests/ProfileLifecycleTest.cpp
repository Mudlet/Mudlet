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
 * The profile lifecycle API - loadProfile(), setActiveProfile(),
 * closeProfile(), closeMudlet() and the cross-profile half of
 * raiseGlobalEvent() - is out of reach of the busted suite, which runs inside
 * one profile of an application it must leave standing. Each test here drives
 * the API from a profile's own Lua state and checks the application state that
 * follows, plus the refusals the three name-taking functions return.
 *
 * Left uncovered on purpose: closeProfile() reports true as soon as it has
 * asked for the close, so a close that Host::requestClose() then refuses still
 * reads as a success. Reaching that needs the modal save prompt the fixture
 * below is deliberately built to avoid.
 *
 * Run with: ctest -R ProfileLifecycleTest -V
 */

#include <QtTest/QtTest>
#include <chrono>

#include "ProfileTestHelper.h"
#include "Host.h"
#include "HostManager.h"
#include "MudletInstanceCoordinator.h"
#include "TLuaInterpreter.h"
#include "TMainConsole.h"
#include "TTabBar.h"
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

using namespace std::chrono_literals;

class ProfileLifecycleTest : public QObject
{
    Q_OBJECT

private:
    // What a Lua call returned: the type of the first value matters as much as
    // the value, since these functions disagree on whether a refusal is a nil
    // or a false
    struct LuaOutcome
    {
        QString error; // non-null when the chunk failed, at compile or at run time
        QString firstType;
        bool first = false;
        QString message;
    };

    // enough for a real connection to the local stub on a loaded machine
    static constexpr int csmConnectBudgetMs = 15000;
    // What an offline profile is given to prove it does not connect. Shorter
    // than the budget above deliberately - the online test is what shows a
    // connection is noticed well inside a wait of this size.
    static constexpr int csmStayOfflineBudgetMs = 3000;
    static constexpr int csmTeardownBudgetMs = 30000;

    QTemporaryDir mConfigDir;
    QByteArray mSavedXdgConfigHome;
    TelnetServerStub* mpServer = nullptr;
    QString mPort;
    Host* mpFirstHost = nullptr;

    const QString mLocalhost = qsl("localhost");
    const QString mFirstProfile = qsl("ProfileLifecycle-First");
    const QString mSecondProfile = qsl("ProfileLifecycle-Second");
    const QString mThirdProfile = qsl("ProfileLifecycle-Third");
    const QString mOnlineProfile = qsl("ProfileLifecycle-Online");
    const QString mUnloadedProfile = qsl("ProfileLifecycle-Unloaded");
    const QString mAbsentProfile = qsl("ProfileLifecycle-Absent");

    // setupConfig() consults portable.txt ahead of the XDG logic; skip rather
    // than run against an unexpected config dir (see ConfigDirOverrideTest)
    bool portableMarkerPresent() const
    {
        return QFileInfo::exists(qsl("%1/portable.txt").arg(QCoreApplication::applicationDirPath())) || QFileInfo::exists(qsl("%1/.config/mudlet/portable.txt").arg(QDir::homePath()));
    }

    Host* hostFor(const QString& profileName) const { return mudlet::self()->getHostManager().getHost(profileName); }

    bool profileHasATab(const QString& profileName) const { return mudlet::self()->mpTabBar->tabIndex(profileName) != -1; }

    // Compares against the pool itself rather than a name, since a Host that
    // has been closed cannot be asked for one
    bool stillInTheHostPool(Host* pHost) const
    {
        if (!pHost) {
            return false;
        }
        for (auto pLoadedHost : mudlet::self()->getHostManager()) {
            if (pLoadedHost == pHost) {
                return true;
            }
        }
        return false;
    }

    static bool reachedTheGame(Host* pHost)
    {
        const auto [address, port, connected] = pHost->mTelnet.getConnectionInfo();
        return connected;
    }

    // The folder is all getCanonicalProfileName() matches a name against; the
    // url and port are what the load then needs to reach the stub, as the Host
    // constructor reads both back out of the profile's data files.
    bool provisionProfileOnDisk(const QString& profileName) const
    {
        return QDir().mkpath(mudlet::getMudletPath(enums::profileHomePath, profileName)) && mudlet::self()->writeProfileData(profileName, qsl("url"), mLocalhost).first
               && mudlet::self()->writeProfileData(profileName, qsl("port"), mPort).first;
    }

    // Returns the Lua error, or a null QString when the chunk ran
    QString runLua(Host* pHost, const QString& code) const
    {
        lua_State* L = pHost->getLuaInterpreter()->getLuaGlobalState();
        if (luaL_dostring(L, code.toUtf8().constData()) == 0) {
            return QString();
        }
        // an error object that is neither a string nor a number gives back a
        // nullptr here, and QString::fromUtf8(nullptr) is null - which every
        // caller would read as "the chunk ran"
        const char* message = lua_tostring(L, -1);
        const QString error = message ? QString::fromUtf8(message) : qsl("(a Lua error that is not a string)");
        lua_pop(L, 1);
        return error;
    }

    LuaOutcome callLua(Host* pHost, const QString& expression) const
    {
        LuaOutcome outcome;
        outcome.error = runLua(pHost, qsl("_lifecycleResult, _lifecycleMessage = %1").arg(expression));
        if (!outcome.error.isNull()) {
            return outcome;
        }

        lua_State* L = pHost->getLuaInterpreter()->getLuaGlobalState();
        lua_getglobal(L, "_lifecycleResult");
        outcome.firstType = QString::fromUtf8(luaL_typename(L, -1));
        outcome.first = lua_toboolean(L, -1);
        lua_pop(L, 1);
        lua_getglobal(L, "_lifecycleMessage");
        if (lua_type(L, -1) == LUA_TSTRING) {
            outcome.message = QString::fromUtf8(lua_tostring(L, -1));
        }
        lua_pop(L, 1);
        return outcome;
    }

    QString luaGlobalString(Host* pHost, const QString& globalName) const
    {
        lua_State* L = pHost->getLuaInterpreter()->getLuaGlobalState();
        lua_getglobal(L, globalName.toUtf8().constData());
        QString value;
        if (lua_type(L, -1) == LUA_TSTRING) {
            value = QString::fromUtf8(lua_tostring(L, -1));
        }
        lua_pop(L, 1);
        return value;
    }

    int luaGlobalNumber(Host* pHost, const QString& globalName) const
    {
        lua_State* L = pHost->getLuaInterpreter()->getLuaGlobalState();
        lua_getglobal(L, globalName.toUtf8().constData());
        const int value = static_cast<int>(lua_tointeger(L, -1));
        lua_pop(L, 1);
        return value;
    }

    // _lifecycleHandler holds one handler at a time, so forgetEvent() has to
    // run before the next rememberEvent() or the previous handler is orphaned
    // and goes on writing the same globals
    QString rememberEvent(Host* pHost, const QString& eventName) const
    {
        return runLua(pHost,
                      qsl("_lifecyclePayload, _lifecycleSender, _lifecycleCalls = nil, nil, 0\n"
                          "_lifecycleHandler = registerAnonymousEventHandler('%1', function(_, payload, sender)\n"
                          "  _lifecyclePayload, _lifecycleSender = payload, sender\n"
                          "  _lifecycleCalls = _lifecycleCalls + 1\n"
                          "end)")
                              .arg(eventName));
    }

    QString forgetEvent(Host* pHost) const { return runLua(pHost, qsl("if _lifecycleHandler then killAnonymousEventHandler(_lifecycleHandler) _lifecycleHandler = nil end")); }

    void startFirstProfile()
    {
        const QString profileName = mFirstProfile;
        const QString address = mLocalhost;
        const QString port = mPort;
        mpFirstHost = TestProfile::create(profileName, address, port);
        QVERIFY2(mpFirstHost, "no active host after creating the first profile");
        // otherwise closing a profile asks whether to save it, and the modal
        // question would hang the test
        QVERIFY2(mpFirstHost->mFORCE_SAVE_ON_EXIT, "profiles must save without asking, or a close puts up a modal question");
    }

    // test_closeProfileNamedByAnotherProfileTearsItDown takes the second
    // profile away again, and any one test can also be run on its own with
    // -functions, so a test that needs another profile opens it itself. The
    // declaration order still matters for the closeMudlet test, which has to
    // stay last.
    Host* loadProfileThroughLua(const QString& profileName)
    {
        if (auto* pHost = hostFor(profileName)) {
            return pHost;
        }
        if (!provisionProfileOnDisk(profileName)) {
            qWarning() << "loadProfileThroughLua: could not provision" << profileName;
            return nullptr;
        }
        const LuaOutcome outcome = callLua(mpFirstHost, qsl("loadProfile('%1', true)").arg(profileName));
        if (!outcome.error.isNull()) {
            qWarning() << "loadProfileThroughLua:" << outcome.error;
            return nullptr;
        }
        if (!outcome.first) {
            qWarning() << "loadProfileThroughLua: loadProfile() refused:" << outcome.message;
            return nullptr;
        }
        return hostFor(profileName);
    }

    bool waitFor(const std::function<bool()>& condition) const
    {
        QElapsedTimer timer;
        timer.start();
        while (timer.elapsed() < csmTeardownBudgetMs) {
            if (condition()) {
                return true;
            }
            QTest::qWait(50ms);
        }
        return condition();
    }

    // Removal from the host pool is finished off from a zero-timer, so the
    // Host is still there until the event loop has run - even though the
    // console has been closed and the profile saved by the time closeProfile()
    // returns
    bool waitForProfileToClose(const QString& profileName) const
    {
        return waitFor([this, profileName]() {
            return !hostFor(profileName);
        });
    }

private slots:
    void initTestCase()
    {
        if (portableMarkerPresent()) {
            QSKIP("portable.txt present - cannot redirect the config dir for this test");
        }

        QVERIFY(mConfigDir.isValid());
        // $XDG_CONFIG_HOME/mudlet/profiles is the opt-in that makes setupConfig()
        // adopt it, so the profiles these tests enumerate are only ever their own
        QVERIFY(QDir().mkpath(qsl("%1/mudlet/profiles").arg(mConfigDir.path())));
        mSavedXdgConfigHome = qgetenv("XDG_CONFIG_HOME");
        qputenv("XDG_CONFIG_HOME", mConfigDir.path().toUtf8());

        mpServer = new TelnetServerStub(qApp);
        mpServer->start(mLocalhost, 0); // ephemeral OS-assigned port avoids collisions across concurrent test runs
        // TelnetServerStub::start() only logs a failed bind, so check the port
        // here: otherwise every profile is pointed at port 0 and the run fails
        // later on, nowhere near the stub
        QVERIFY2(mpServer->serverPort() != 0, "the telnet stub did not start listening");
        mPort = QString::number(mpServer->serverPort());

        mudlet::start();
        mudlet::self()->setupConfig();
        QCOMPARE(mudlet::getMudletPath(enums::mainPath), qsl("%1/mudlet").arg(mConfigDir.path()));
        // Written before init(), which is where a config with no keys at all
        // gets stamped as a first launch. A settings file that already holds
        // something is how mudletUsedBefore() recognises an existing player, so
        // this both suppresses the first-run UI tour and keeps the starter UI
        // package out of every profile these tests open.
        mudlet::getQSettings()->setValue(qsl("uiTourShown"), true);
        mudlet::getQSettings()->sync();
        mudlet::self()->takeOwnershipOfInstanceCoordinator(std::make_unique<MudletInstanceCoordinator>(qsl("MudletInstanceCoordinator")));
        mudlet::self()->init();
        mudlet::self()->setStorePasswordsSecurely(false);
        QVERIFY2(mudlet::self()->experiencedMudletPlayer(), "the first-run UI would open over these tests");

        startFirstProfile();
        // a failed assertion in there only returns from it, so stop the whole
        // run here rather than let every test dereference a null host
        QVERIFY(mpFirstHost);
    }

    void cleanupTestCase()
    {
        delete mpServer;
        mpServer = nullptr;
        // null once the closeMudlet test has run, and deleting that is a no-op
        delete mudlet::self();
        mSavedXdgConfigHome.isNull() ? qunsetenv("XDG_CONFIG_HOME") : qputenv("XDG_CONFIG_HOME", mSavedXdgConfigHome);
    }

    void test_loadProfileRefusesAnEmptyName()
    {
        const LuaOutcome outcome = callLua(mpFirstHost, qsl("loadProfile('')"));

        QVERIFY2(outcome.error.isNull(), qPrintable(outcome.error));
        QCOMPARE(outcome.firstType, qsl("nil"));
        QVERIFY2(outcome.message.contains(qsl("cannot be empty")), qPrintable(qsl("unexpected message: %1").arg(outcome.message)));
    }

    void test_loadProfileRefusesAProfileThatDoesNotExist()
    {
        QVERIFY2(!QDir(mudlet::getMudletPath(enums::profileHomePath, mAbsentProfile)).exists(), "the profile this test needs to be absent exists");

        const LuaOutcome outcome = callLua(mpFirstHost, qsl("loadProfile('%1')").arg(mAbsentProfile));

        QVERIFY2(outcome.error.isNull(), qPrintable(outcome.error));
        QCOMPARE(outcome.firstType, qsl("nil"));
        QVERIFY2(outcome.message.contains(qsl("does not exist")), qPrintable(qsl("unexpected message: %1").arg(outcome.message)));
        QVERIFY2(!hostFor(mAbsentProfile), "a profile that does not exist was loaded anyway");
    }

    void test_loadProfileRefusesAProfileThatIsAlreadyLoaded()
    {
        const int loadedBefore = mudlet::self()->getHostManager().getHostCount();

        // asked for in the wrong case, and the refusal names the profile as it
        // is spelt on disk: the case-insensitive lookup all three of these
        // functions share
        const LuaOutcome outcome = callLua(mpFirstHost, qsl("loadProfile('%1')").arg(mFirstProfile.toLower()));

        QVERIFY2(outcome.error.isNull(), qPrintable(outcome.error));
        QCOMPARE(outcome.firstType, qsl("nil"));
        QVERIFY2(outcome.message.contains(qsl("'%1' is already loaded").arg(mFirstProfile)), qPrintable(qsl("unexpected message: %1").arg(outcome.message)));
        QCOMPARE(mudlet::self()->getHostManager().getHostCount(), loadedBefore);
    }

    void test_loadProfileOpensASecondProfile()
    {
        QVERIFY(provisionProfileOnDisk(mSecondProfile));
        QVERIFY2(!hostFor(mSecondProfile), "the second profile was already loaded");
        const int loadedBefore = mudlet::self()->getHostManager().getHostCount();

        const LuaOutcome outcome = callLua(mpFirstHost, qsl("loadProfile('%1', true)").arg(mSecondProfile));

        QVERIFY2(outcome.error.isNull(), qPrintable(outcome.error));
        QCOMPARE(outcome.firstType, qsl("boolean"));
        QVERIFY2(outcome.first, qPrintable(qsl("loadProfile() refused: %1").arg(outcome.message)));

        Host* pSecondHost = hostFor(mSecondProfile);
        QVERIFY2(pSecondHost, "loadProfile() reported success but the profile is not in the host pool");
        QVERIFY2(pSecondHost->mpConsole, "the loaded profile has no main console, so nothing of it is on screen");
        QVERIFY2(profileHasATab(mSecondProfile), "the loaded profile got no tab");
        QCOMPARE(mudlet::self()->getHostManager().getHostCount(), loadedBefore + 1);

        // An online load reaches the game some event loop turns later, so the
        // socket is unconnected on the line after the call either way - only a
        // wait that runs out says the offline flag was honoured.
        // test_loadProfileConnectsWhenNotAskedForOffline is the control.
        QSignalSpy connectionSpy(&pSecondHost->mTelnet, &cTelnet::signal_connected);
        QVERIFY2(!connectionSpy.wait(csmStayOfflineBudgetMs) && !reachedTheGame(pSecondHost), "loadProfile(name, true) connected the profile despite being asked for offline");
    }

    // Connecting is what loadProfile() does when it is not told otherwise
    void test_loadProfileConnectsWhenNotAskedForOffline()
    {
        QVERIFY(provisionProfileOnDisk(mOnlineProfile));
        QVERIFY2(!hostFor(mOnlineProfile), "the profile this test opens was already loaded");

        const LuaOutcome outcome = callLua(mpFirstHost, qsl("loadProfile('%1')").arg(mOnlineProfile));

        QVERIFY2(outcome.error.isNull(), qPrintable(outcome.error));
        QVERIFY2(outcome.first, qPrintable(qsl("loadProfile() refused: %1").arg(outcome.message)));
        Host* pOnlineHost = hostFor(mOnlineProfile);
        QVERIFY(pOnlineHost);
        QSignalSpy connectionSpy(&pOnlineHost->mTelnet, &cTelnet::signal_connected);
        QVERIFY2(reachedTheGame(pOnlineHost) || connectionSpy.wait(csmConnectBudgetMs), "loadProfile() with no offline argument did not connect the profile to the game");

        QVERIFY(callLua(mpFirstHost, qsl("closeProfile('%1')").arg(mOnlineProfile)).first);
        QVERIFY(waitForProfileToClose(mOnlineProfile));
    }

    void test_setActiveProfileRefusesAnEmptyName()
    {
        const LuaOutcome outcome = callLua(mpFirstHost, qsl("setActiveProfile('')"));

        QVERIFY2(outcome.error.isNull(), qPrintable(outcome.error));
        QCOMPARE(outcome.firstType, qsl("boolean"));
        QVERIFY(!outcome.first);
        QVERIFY2(outcome.message.contains(qsl("cannot be empty")), qPrintable(qsl("unexpected message: %1").arg(outcome.message)));
    }

    void test_setActiveProfileRefusesAProfileThatDoesNotExist()
    {
        const LuaOutcome outcome = callLua(mpFirstHost, qsl("setActiveProfile('%1')").arg(mAbsentProfile));

        QVERIFY2(outcome.error.isNull(), qPrintable(outcome.error));
        // unlike loadProfile()/closeProfile(), this one refuses with false
        QCOMPARE(outcome.firstType, qsl("boolean"));
        QVERIFY(!outcome.first);
        QVERIFY2(outcome.message.contains(qsl("does not exist")), qPrintable(qsl("unexpected message: %1").arg(outcome.message)));
    }

    void test_setActiveProfileRefusesAProfileThatIsNotLoaded()
    {
        QVERIFY(provisionProfileOnDisk(mUnloadedProfile));
        QVERIFY2(!hostFor(mUnloadedProfile), "the profile this test needs unloaded is loaded");
        Host* pActiveBefore = mudlet::self()->getActiveHost();

        const LuaOutcome outcome = callLua(mpFirstHost, qsl("setActiveProfile('%1')").arg(mUnloadedProfile));

        QVERIFY2(outcome.error.isNull(), qPrintable(outcome.error));
        QCOMPARE(outcome.firstType, qsl("boolean"));
        QVERIFY(!outcome.first);
        QVERIFY2(outcome.message.contains(qsl("is not loaded")), qPrintable(qsl("unexpected message: %1").arg(outcome.message)));
        QCOMPARE(mudlet::self()->getActiveHost(), pActiveBefore);
    }

    void test_setActiveProfileSwitchesTheActiveProfile()
    {
        Host* pSecondHost = loadProfileThroughLua(mSecondProfile);
        QVERIFY(pSecondHost);

        LuaOutcome outcome = callLua(mpFirstHost, qsl("setActiveProfile('%1')").arg(mFirstProfile));
        QVERIFY2(outcome.error.isNull(), qPrintable(outcome.error));
        QVERIFY2(outcome.first, qPrintable(qsl("setActiveProfile() refused: %1").arg(outcome.message)));
        QCOMPARE(mudlet::self()->getActiveHost(), mpFirstHost);

        outcome = callLua(mpFirstHost, qsl("setActiveProfile('%1')").arg(mSecondProfile));
        QVERIFY2(outcome.error.isNull(), qPrintable(outcome.error));
        QCOMPARE(outcome.firstType, qsl("boolean"));
        QVERIFY2(outcome.first, qPrintable(qsl("setActiveProfile() refused: %1").arg(outcome.message)));
        QCOMPARE(mudlet::self()->getActiveHost(), pSecondHost);
        QVERIFY(profileHasATab(mSecondProfile));
        QCOMPARE(mudlet::self()->mpTabBar->currentIndex(), mudlet::self()->mpTabBar->tabIndex(mSecondProfile));

        // leave the profile the rest of the tests drive from in charge
        QVERIFY(callLua(mpFirstHost, qsl("setActiveProfile('%1')").arg(mFirstProfile)).first);
    }

    // Every other loaded profile is told the name of the profile that raised
    // the event, and the one that raised it never hears it come back
    void test_raiseGlobalEventReachesEveryOtherProfileOnly()
    {
        Host* pSecondHost = loadProfileThroughLua(mSecondProfile);
        Host* pThirdHost = loadProfileThroughLua(mThirdProfile);
        QVERIFY(pSecondHost && pThirdHost);
        const QString eventName = qsl("ProfileLifecycleGlobalEvent");
        QVERIFY(rememberEvent(mpFirstHost, eventName).isNull());
        QVERIFY(rememberEvent(pSecondHost, eventName).isNull());
        QVERIFY(rememberEvent(pThirdHost, eventName).isNull());

        QVERIFY(runLua(mpFirstHost, qsl("raiseGlobalEvent('%1', 'from the first')").arg(eventName)).isNull());

        for (Host* pReceiver : {pSecondHost, pThirdHost}) {
            QCOMPARE(luaGlobalString(pReceiver, qsl("_lifecyclePayload")), qsl("from the first"));
            QCOMPARE(luaGlobalString(pReceiver, qsl("_lifecycleSender")), mFirstProfile);
            QCOMPARE(luaGlobalNumber(pReceiver, qsl("_lifecycleCalls")), 1);
        }
        QCOMPARE(luaGlobalNumber(mpFirstHost, qsl("_lifecycleCalls")), 0);

        QVERIFY(runLua(pSecondHost, qsl("raiseGlobalEvent('%1', 'from the second')").arg(eventName)).isNull());

        QCOMPARE(luaGlobalString(mpFirstHost, qsl("_lifecyclePayload")), qsl("from the second"));
        QCOMPARE(luaGlobalString(mpFirstHost, qsl("_lifecycleSender")), mSecondProfile);
        QCOMPARE(luaGlobalNumber(mpFirstHost, qsl("_lifecycleCalls")), 1);
        QCOMPARE(luaGlobalNumber(pSecondHost, qsl("_lifecycleCalls")), 1);

        QVERIFY(forgetEvent(mpFirstHost).isNull());
        QVERIFY(forgetEvent(pSecondHost).isNull());
        QVERIFY(forgetEvent(pThirdHost).isNull());
    }

    // Arguments cross the profile boundary as strings and are rebuilt on the
    // other side, so their types have to survive the trip - and the sending
    // profile's name is appended after all of them, however many there are
    void test_raiseGlobalEventKeepsArgumentTypesAndPutsTheSenderLast()
    {
        Host* pSecondHost = loadProfileThroughLua(mSecondProfile);
        QVERIFY(pSecondHost);
        const QString eventName = qsl("ProfileLifecycleTypedEvent");
        QVERIFY(runLua(pSecondHost,
                       qsl("_lifecycleTypes, _lifecycleValues = nil, nil\n"
                           "_lifecycleHandler = registerAnonymousEventHandler('%1', function(_, ...)\n"
                           "  local given, types = {...}, {}\n"
                           "  for i = 1, select('#', ...) do types[i] = type(given[i]) end\n"
                           "  _lifecycleTypes = table.concat(types, ',')\n"
                           "  _lifecycleValues = table.concat({tostring(given[1]), tostring(given[2]), tostring(given[4]), tostring(given[5])}, '|')\n"
                           "end)")
                               .arg(eventName))
                        .isNull());

        QVERIFY(runLua(mpFirstHost, qsl("raiseGlobalEvent('%1', 3.5, true, nil, 'text')").arg(eventName)).isNull());

        QCOMPARE(luaGlobalString(pSecondHost, qsl("_lifecycleTypes")), qsl("number,boolean,nil,string,string"));
        QCOMPARE(luaGlobalString(pSecondHost, qsl("_lifecycleValues")), qsl("3.5|true|text|%1").arg(mFirstProfile));

        QVERIFY(forgetEvent(pSecondHost).isNull());
    }

    // Unlike raiseEvent(), which can hand a handler in the same profile a
    // table through the Lua registry, nothing survives the trip to another
    // profile that cannot be turned into a string.
    //
    // Refusing an argument is a lua_error(), which longjmps straight out of the
    // C function, so a TEvent being filled in on the stack there would never be
    // destroyed and whatever it had collected would leak. The named event with a
    // table after it is the case that would strand a partly filled one if the
    // arguments stopped being vetted first, and LeakSanitizer fails the whole
    // binary if it ever does.
    void test_raiseGlobalEventRefusesArgumentsItCannotCarry()
    {
        const QString tableError = runLua(mpFirstHost, qsl("raiseGlobalEvent({})"));
        QVERIFY2(!tableError.isNull(), "raiseGlobalEvent() accepted a table");
        QVERIFY2(tableError.contains(qsl("bad argument type #1")), qPrintable(tableError));

        const QString lateTableError = runLua(mpFirstHost, qsl("raiseGlobalEvent('lifecycleUncarryable', 'text', {})"));
        QVERIFY2(!lateTableError.isNull(), "raiseGlobalEvent() accepted a table after the event name");
        QVERIFY2(lateTableError.contains(qsl("bad argument type #3")), qPrintable(lateTableError));

        const QString noNameError = runLua(mpFirstHost, qsl("raiseGlobalEvent()"));
        QVERIFY2(!noNameError.isNull(), "raiseGlobalEvent() accepted a call with no event name");
        QVERIFY2(noNameError.contains(qsl("missing argument #1")), qPrintable(noNameError));
    }

    void test_closeProfileRefusesAProfileThatDoesNotExist()
    {
        const LuaOutcome outcome = callLua(mpFirstHost, qsl("closeProfile('%1')").arg(mAbsentProfile));

        QVERIFY2(outcome.error.isNull(), qPrintable(outcome.error));
        QCOMPARE(outcome.firstType, qsl("nil"));
        QVERIFY2(outcome.message.contains(qsl("does not exist")), qPrintable(qsl("unexpected message: %1").arg(outcome.message)));
    }

    void test_closeProfileRefusesAProfileThatIsNotLoaded()
    {
        QVERIFY(provisionProfileOnDisk(mUnloadedProfile));
        QVERIFY2(!hostFor(mUnloadedProfile), "the profile this test needs unloaded is loaded");

        const LuaOutcome outcome = callLua(mpFirstHost, qsl("closeProfile('%1')").arg(mUnloadedProfile));

        QVERIFY2(outcome.error.isNull(), qPrintable(outcome.error));
        QCOMPARE(outcome.firstType, qsl("nil"));
        QVERIFY2(outcome.message.contains(qsl("is not loaded")), qPrintable(qsl("unexpected message: %1").arg(outcome.message)));
    }

    void test_closeProfileNamedByAnotherProfileTearsItDown()
    {
        QPointer<Host> pSecondHost = loadProfileThroughLua(mSecondProfile);
        QVERIFY(pSecondHost);
        QPointer<TMainConsole> pSecondConsole = pSecondHost->mpConsole;
        QVERIFY(pSecondConsole);
        const int loadedBefore = mudlet::self()->getHostManager().getHostCount();

        const LuaOutcome outcome = callLua(mpFirstHost, qsl("closeProfile('%1')").arg(mSecondProfile));

        QVERIFY2(outcome.error.isNull(), qPrintable(outcome.error));
        QCOMPARE(outcome.firstType, qsl("boolean"));
        QVERIFY(outcome.first);
        QVERIFY2(waitForProfileToClose(mSecondProfile), "the profile was still in the host pool long after closeProfile() said it was closing");
        QVERIFY2(pSecondHost.isNull(), "the closed profile's Host outlived its removal from the host pool");
        // the console goes on a deferred delete of its own, so give it the
        // same budget rather than assume it landed inside the wait above
        QVERIFY2(waitFor([&pSecondConsole]() {
                     return pSecondConsole.isNull();
                 }),
                 "the closed profile's main console was left behind");
        QVERIFY2(!profileHasATab(mSecondProfile), "the closed profile kept its tab");
        QCOMPARE(mudlet::self()->getHostManager().getHostCount(), loadedBefore - 1);
        QVERIFY2(hostFor(mFirstProfile), "closing one profile took the other one with it");
        QVERIFY2(stillInTheHostPool(mudlet::self()->getActiveHost()), "closing a profile left the active profile pointing at a Host that has been destroyed");
    }

    void test_closeProfileWithNoArgumentClosesTheCallingProfile()
    {
        QPointer<Host> pThirdHost = loadProfileThroughLua(mThirdProfile);
        QVERIFY(pThirdHost);

        const LuaOutcome outcome = callLua(pThirdHost, qsl("closeProfile()"));

        QVERIFY2(outcome.error.isNull(), qPrintable(outcome.error));
        QCOMPARE(outcome.firstType, qsl("boolean"));
        QVERIFY(outcome.first);
        QVERIFY2(waitForProfileToClose(mThirdProfile), "a profile that closed itself was still in the host pool long afterwards");
        QVERIFY2(pThirdHost.isNull(), "the self-closed profile's Host outlived its removal from the host pool");
        QVERIFY2(!profileHasATab(mThirdProfile), "the self-closed profile kept its tab");
        QVERIFY2(hostFor(mFirstProfile), "a profile closing itself took another profile with it");
    }

    // Last on purpose: this takes the main window with it, so no test slot can
    // run after it - only cleanupTestCase(), which is written for a Mudlet
    // that has already gone
    void test_closeMudletShutsDownEveryProfileAndTheMainWindow()
    {
        QPointer<Host> pSecondHost = loadProfileThroughLua(mSecondProfile);
        QVERIFY(pSecondHost);
        QPointer<Host> pFirstHost = mpFirstHost;
        QPointer<mudlet> pMainWindow = mudlet::self();

        QVERIFY(runLua(mpFirstHost, qsl("closeMudlet()")).isNull());
        mpFirstHost = nullptr;

        // the main window is WA_DeleteOnClose, so it goes on a deferred delete
        // once the close it arranges for has been accepted
        QVERIFY2(waitFor([&pMainWindow]() {
                     return pMainWindow.isNull();
                 }),
                 "closeMudlet() left the main window standing");
        QVERIFY2(pFirstHost.isNull(), "closeMudlet() left a profile loaded");
        QVERIFY2(pSecondHost.isNull(), "closeMudlet() closed one profile but not the other");
    }
};

#include "ProfileLifecycleTest.moc"
MUDLET_GROUPED_TEST_MAIN(ProfileLifecycleTest)
