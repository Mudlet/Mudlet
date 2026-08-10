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
 * follows, along with the refusal each function returns for a name that is
 * unknown, already loaded or not loaded.
 *
 * Run with: ctest -R ProfileLifecycleTest -V
 */

#include <QtTest/QtTest>
#include <chrono>

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

using namespace std::chrono_literals;

extern void qInitResources_mudlet();
extern void qInitResources_qm();
extern void qInitResources_additional_splash_screens();
extern void qInitResources_mudlet_fonts_common();
extern void qInitResources_mudlet_fonts_posix();
void initializeQRCResourcesForProfileLifecycleTest();

class ProfileLifecycleTest : public QObject
{
    Q_OBJECT

private:
    // What a Lua call returned: the type of the first value matters as much as
    // the value, since these functions disagree on whether a refusal is a nil
    // or a false
    struct LuaOutcome
    {
        QString error; // the chunk itself would not run
        QString firstType;
        bool first = false;
        QString message;
    };

    QTemporaryDir mConfigDir;
    QByteArray mSavedXdgConfigHome;
    TelnetServerStub* mpServer = nullptr;
    QString mPort;
    Host* mpFirstHost = nullptr;

    const QString mLocalhost = qsl("localhost");
    const QString mFirstProfile = qsl("ProfileLifecycle-First");
    const QString mSecondProfile = qsl("ProfileLifecycle-Second");
    const QString mThirdProfile = qsl("ProfileLifecycle-Third");
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

    // What dlgConnectionProfiles leaves on disk for a profile that has been
    // named and given connection details but never played: the folder plus the
    // url/port files. That is enough for getCanonicalProfileName() to find it.
    void provisionProfileOnDisk(const QString& profileName) const
    {
        QVERIFY(QDir().mkpath(mudlet::getMudletPath(enums::profileHomePath, profileName)));
        QVERIFY(mudlet::self()->writeProfileData(profileName, qsl("url"), mLocalhost).first);
        QVERIFY(mudlet::self()->writeProfileData(profileName, qsl("port"), mPort).first);
    }

    // Returns the Lua error, or a null QString when the chunk ran
    QString runLua(Host* pHost, const QString& code) const
    {
        lua_State* L = pHost->getLuaInterpreter()->getLuaGlobalState();
        if (luaL_dostring(L, code.toUtf8().constData()) == 0) {
            return QString();
        }
        const QString error = QString::fromUtf8(lua_tostring(L, -1));
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

    // The handler is killed again by forgetEvent() so repeated tests do not
    // stack up handlers that all write the same globals
    QString rememberEvent(Host* pHost, const QString& eventName) const
    {
        return runLua(pHost,
                      qsl("_lifecyclePayload, _lifecycleSender = nil, nil\n"
                          "_lifecycleHandler = registerAnonymousEventHandler('%1', function(_, payload, sender)\n"
                          "  _lifecyclePayload, _lifecycleSender = payload, sender\n"
                          "end)")
                              .arg(eventName));
    }

    QString forgetEvent(Host* pHost) const { return runLua(pHost, qsl("if _lifecycleHandler then killAnonymousEventHandler(_lifecycleHandler) _lifecycleHandler = nil end")); }

    void startFirstProfile()
    {
        const QString profileName = mFirstProfile;
        const QString address = mLocalhost;
        const QString port = mPort;
        QTimer::singleShot(0ms, qApp, [profileName, address, port]() {
            mudlet::self()->startAutoLogin({});
            QTest::qWait(100ms);
            QTest::mouseClick(mudlet::self()->mpConnectionDialog->new_profile_button, Qt::LeftButton);
            QTest::qWait(100ms);
            QTest::keyClicks(QApplication::focusWidget(), profileName);
            QTest::qWait(100ms);
            QTest::keyClick(QApplication::focusWidget(), Qt::Key_Tab);
            QTest::qWait(100ms);
            QTest::keyClicks(QApplication::focusWidget(), address);
            QTest::qWait(100ms);
            QTest::keyClick(QApplication::focusWidget(), Qt::Key_Tab);
            QTest::qWait(100ms);
            QTest::keyClicks(QApplication::focusWidget(), port);
            QTest::qWait(100ms);
            QTest::keyClick(QApplication::focusWidget(), Qt::Key_Return);
        });

        QSignalSpy spy(mudlet::self(), &mudlet::signal_profileLoaded);
        QVERIFY2(spy.wait(30000), "the first profile took too long to load");
        mpFirstHost = mudlet::self()->getActiveHost();
        QVERIFY2(mpFirstHost, "no active host after creating the first profile");
    }

    // Every test that needs a second profile brings it up itself, so the file
    // does not depend on the order QTest happens to run the tests in
    Host* loadProfileThroughLua(const QString& profileName)
    {
        if (auto* pHost = hostFor(profileName)) {
            return pHost;
        }
        provisionProfileOnDisk(profileName);
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

    // The close is finished off from a zero-timer, so nothing has gone until
    // the event loop has run
    bool waitForProfileToClose(const QString& profileName) const
    {
        QElapsedTimer timer;
        timer.start();
        while (timer.elapsed() < 30000) {
            if (!hostFor(profileName)) {
                return true;
            }
            QTest::qWait(50ms);
        }
        return false;
    }

private slots:
    void initTestCase()
    {
        if (portableMarkerPresent()) {
            QSKIP("portable.txt present - cannot redirect the config dir for this test");
        }
        initializeQRCResourcesForProfileLifecycleTest();

        QVERIFY(mConfigDir.isValid());
        // an existing $XDG_CONFIG_HOME/mudlet makes setupConfig() adopt it, so
        // the profiles these tests enumerate are only ever their own
        QVERIFY(QDir().mkpath(qsl("%1/mudlet").arg(mConfigDir.path())));
        mSavedXdgConfigHome = qgetenv("XDG_CONFIG_HOME");
        qputenv("XDG_CONFIG_HOME", mConfigDir.path().toUtf8());

        mpServer = new TelnetServerStub(qApp);
        mpServer->start(mLocalhost, 0); // ephemeral OS-assigned port avoids collisions across concurrent test runs
        // a stub that failed to bind only warns, and the profile would then
        // merely look slow to load
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
        // otherwise closing a profile asks whether to save it, and the modal
        // question would hang the test
        QVERIFY2(mpFirstHost->mFORCE_SAVE_ON_EXIT, "profiles must save without asking, or a close puts up a modal question");
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

        const LuaOutcome outcome = callLua(mpFirstHost, qsl("loadProfile('%1')").arg(mFirstProfile));

        QVERIFY2(outcome.error.isNull(), qPrintable(outcome.error));
        QCOMPARE(outcome.firstType, qsl("nil"));
        QVERIFY2(outcome.message.contains(qsl("already loaded")), qPrintable(qsl("unexpected message: %1").arg(outcome.message)));
        QCOMPARE(mudlet::self()->getHostManager().getHostCount(), loadedBefore);
    }

    void test_loadProfileOpensASecondProfile()
    {
        provisionProfileOnDisk(mSecondProfile);
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
        // offline was asked for, so the profile must not have reached the stub
        const auto [connectedAddress, connectedPort, connected] = pSecondHost->mTelnet.getConnectionInfo();
        QVERIFY2(!connected, "loadProfile(name, true) connected the profile despite being asked for offline");
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
        provisionProfileOnDisk(mUnloadedProfile);
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
        QCOMPARE(mudlet::self()->mpTabBar->currentIndex(), mudlet::self()->mpTabBar->tabIndex(mSecondProfile));

        // leave the profile the rest of the tests drive from in charge
        QVERIFY(callLua(mpFirstHost, qsl("setActiveProfile('%1')").arg(mFirstProfile)).first);
    }

    // A profile is told the name of the profile that raised the event, and
    // never hears its own raiseGlobalEvent() come back
    void test_raiseGlobalEventReachesTheOtherProfileOnly()
    {
        Host* pSecondHost = loadProfileThroughLua(mSecondProfile);
        QVERIFY(pSecondHost);
        const QString eventName = qsl("ProfileLifecycleGlobalEvent");
        QVERIFY(rememberEvent(mpFirstHost, eventName).isNull());
        QVERIFY(rememberEvent(pSecondHost, eventName).isNull());

        QVERIFY(runLua(mpFirstHost, qsl("raiseGlobalEvent('%1', 'from the first')").arg(eventName)).isNull());

        QCOMPARE(luaGlobalString(pSecondHost, qsl("_lifecyclePayload")), qsl("from the first"));
        QCOMPARE(luaGlobalString(pSecondHost, qsl("_lifecycleSender")), mFirstProfile);
        QVERIFY2(luaGlobalString(mpFirstHost, qsl("_lifecyclePayload")).isEmpty(), "the profile that raised the global event received it itself");

        QVERIFY(runLua(pSecondHost, qsl("raiseGlobalEvent('%1', 'from the second')").arg(eventName)).isNull());

        QCOMPARE(luaGlobalString(mpFirstHost, qsl("_lifecyclePayload")), qsl("from the second"));
        QCOMPARE(luaGlobalString(mpFirstHost, qsl("_lifecycleSender")), mSecondProfile);
        QCOMPARE(luaGlobalString(pSecondHost, qsl("_lifecyclePayload")), qsl("from the first"));

        QVERIFY(forgetEvent(mpFirstHost).isNull());
        QVERIFY(forgetEvent(pSecondHost).isNull());
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
        provisionProfileOnDisk(mUnloadedProfile);
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
        QVERIFY2(pSecondConsole.isNull(), "the closed profile's main console was left behind");
        QVERIFY2(!profileHasATab(mSecondProfile), "the closed profile kept its tab");
        QCOMPARE(mudlet::self()->getHostManager().getHostCount(), loadedBefore - 1);
        QVERIFY2(hostFor(mFirstProfile), "closing one profile took the other one with it");
        QCOMPARE(mudlet::self()->getActiveHost(), mpFirstHost);
    }

    // With no argument at all the profile whose Lua state is running closes
    // itself, which is the form scripts actually use
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

    // Last on purpose: this takes the main window with it, so nothing can run
    // after it
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
        QElapsedTimer timer;
        timer.start();
        while (!pMainWindow.isNull() && timer.elapsed() < 30000) {
            QTest::qWait(50ms);
        }

        QVERIFY2(pMainWindow.isNull(), "closeMudlet() left the main window standing");
        QVERIFY2(pFirstHost.isNull(), "closeMudlet() left a profile loaded");
        QVERIFY2(pSecondHost.isNull(), "closeMudlet() closed one profile but not the other");
        QVERIFY2(!mudlet::self(), "the main window went but mudlet::self() still hands it out");
    }
};

void initializeQRCResourcesForProfileLifecycleTest()
{
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

#include "ProfileLifecycleTest.moc"
QTEST_MAIN(ProfileLifecycleTest)
