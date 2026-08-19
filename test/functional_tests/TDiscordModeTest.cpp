/***************************************************************************
 *   Copyright (C) 2025 by Mudlet Makers                                   *
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
 * Functional tests for Discord mode gating, server-origin tracking, and
 * the privacy filtering logic.
 *
 * Uses TelnetServerStub to create a Host with a live profile. GMCP Discord
 * messages are delivered via Host::processDiscordGMCP() which is the same
 * entry point the telnet layer calls.
 *
 * Run with: ctest -R TDiscordModeTest -V
 */

#include <QFileInfo>
#include <QTemporaryDir>
#include <QtTest/QtTest>
#include <chrono>

#include <QJsonDocument>
#include <QJsonObject>

#include <utility>

#include "PortableModeTestHelper.h"
#include "ProfileTestHelper.h"
#include "DiscordIpcServerStub.h"
#include "MudletInstanceCoordinator.h"
#include "TLuaInterpreter.h"
#include "TelnetServerStub.h"
#include "discord.h"
#include "dlgConnectionProfiles.h"
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

using namespace std::chrono_literals;

class TDiscordModeTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;
    TelnetServerStub* mpServer = nullptr;
    DiscordIpcServerStub* mpDiscordIpcStub = nullptr;
    Host* mpHost = nullptr;
    const QString mHostname = "Discord-Mode-Test";
    QString mPort; // assigned the stub's actual ephemeral port in initTestCase()
    const QString mLocalhost = "localhost";
    const QString mDiscordStubUserName = "StubDiscordUser";

    // Evaluates a Lua expression in the profile's global state. Returns the
    // first result (invalid QVariant for nil or a compile/runtime error) plus
    // the second result as a string: the error message on denial, or the Lua
    // error text when the expression fails to compile or run.
    std::pair<QVariant, QString> evalLua(const QString& expression)
    {
        lua_State* L = mpHost->mLuaInterpreter.getLuaGlobalState();
        const int base = lua_gettop(L);
        QVariant first;
        QString second;
        if (luaL_dostring(L, qsl("return %1").arg(expression).toUtf8().constData()) == 0) {
            if (lua_gettop(L) > base) {
                if (lua_isboolean(L, base + 1)) {
                    first = static_cast<bool>(lua_toboolean(L, base + 1));
                } else if (lua_isnumber(L, base + 1)) {
                    first = lua_tonumber(L, base + 1);
                } else if (lua_isstring(L, base + 1)) {
                    first = QString::fromUtf8(lua_tostring(L, base + 1));
                }
            }
            if (lua_gettop(L) > base + 1 && lua_isstring(L, base + 2)) {
                second = QString::fromUtf8(lua_tostring(L, base + 2));
            }
        } else if (lua_gettop(L) > base && lua_isstring(L, base + 1)) {
            // A compile or runtime error leaves the message on the stack;
            // surface it so a failing test reports the real Lua error.
            second = QString::fromUtf8(lua_tostring(L, base + 1));
        }
        lua_settop(L, base);
        return {first, second};
    }

    // Returns the activity object of the most recent SET_ACTIVITY frame the
    // IPC stub received, or an empty object if none arrived yet.
    QJsonObject lastSetActivity() const
    {
        const QStringList frames = mpDiscordIpcStub->framePayloads();
        for (auto it = frames.crbegin(); it != frames.crend(); ++it) {
            const QJsonObject frame = QJsonDocument::fromJson(it->toUtf8()).object();
            if (frame.value(qsl("cmd")).toString() == qsl("SET_ACTIVITY")) {
                return frame.value(qsl("args")).toObject().value(qsl("activity")).toObject();
            }
        }
        return {};
    }

    // Brings up a genuine discord-rpc connection to the IPC stub, or reuses one
    // that is already active and whose most recent READY came from the stub,
    // and waits for the READY handshake to populate Discord::smUserName.
    // Reusing an established login lets the end-to-end tests share a single
    // connection.
    //
    // The reuse check (mRpcActive && logged in as the stub) does not prove the
    // current socket completed a handshake - smUserName is never cleared on
    // disconnect - only that RPC is up and the last READY was the stub's, which
    // is all a test that just needs to be logged in requires.
    //
    // When a fresh handshake is unavoidable the timeout is larger than one full
    // backoff period (discord-rpc's reconnect backoff has a 60s ceiling). A
    // fresh handshake needs two backoff-gated Open() attempts (handshake write,
    // then READY read), so a pathologically inflated backoff can exceed even
    // this - the ctest TIMEOUT gives further headroom. That process-global
    // backoff, inflated by the suite's init/shutdown churn, together with a
    // fixed short (10s) timeout, is what used to make these tests flake on slow
    // runners.
    bool establishDiscordLogin()
    {
        auto& discord = mudlet::self()->mDiscord;
        if (discord.mRpcActive && Discord::getLoggedInUserName() == mDiscordStubUserName) {
            return true;
        }
        // Fresh handshake: drop any half-open connection and reconnect. Snapshot
        // the stub's handshake tally so this proves a real op-0 handshake (not a
        // cached READY) drove the login.
        const int handshakesBefore = mpDiscordIpcStub->handshakeCount();
        discord.shutdownRpc();
        Discord::smUserName.clear();
        discord.UpdatePresence();
        const bool loggedIn = QTest::qWaitFor(
                [this]() {
                    return Discord::getLoggedInUserName() == mDiscordStubUserName;
                },
                65000);
        return loggedIn && mpDiscordIpcStub->handshakeCount() > handshakesBefore;
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

        // The discord-rpc library connects to $XDG_RUNTIME_DIR/discord-ipc-0,
        // so the stub must be listening and the environment pointed at it
        // before the Discord instance (created inside mudlet::start()) first
        // initializes the RPC connection:
        mpDiscordIpcStub = new DiscordIpcServerStub(qApp);
        if (mpDiscordIpcStub->start(mDiscordStubUserName) && !mpDiscordIpcStub->runtimeDir().isEmpty()) {
            qputenv("XDG_RUNTIME_DIR", mpDiscordIpcStub->runtimeDir().toLocal8Bit());
        }

        mpServer = new TelnetServerStub(qApp);
        mpServer->start(mLocalhost, 0); // ephemeral OS-assigned port avoids collisions across concurrent test runs
        mPort = QString::number(mpServer->serverPort());
        mudlet::start();
        mudlet::self()->setupConfig();
        QCOMPARE(mudlet::getMudletPath(enums::mainPath), qsl("%1/mudlet").arg(mConfigDir.path()));
        mudlet::self()->takeOwnershipOfInstanceCoordinator(std::make_unique<MudletInstanceCoordinator>("MudletInstanceCoordinator"));
        mudlet::self()->init();
        mudlet::self()->setStorePasswordsSecurely(false);

        const QString path = mudlet::getMudletPath(enums::profileHomePath, mHostname);
        QDir(path).removeRecursively();

        mpHost = TestProfile::create(mHostname, mLocalhost, mPort);
        if (!mpHost) {
            QFAIL("No active host available for the test.");
        }

        QSignalSpy spy2(&(mpHost->mTelnet), &cTelnet::signal_connected);
        if (!spy2.wait(500)) {
            QFAIL("Could not connect with the host.");
        }

        // Warm up the real discord-rpc connection once, up front. A completed
        // handshake resets the library's internal reconnect backoff (capped at
        // 60s), which the per-test init/shutdown churn below can otherwise
        // inflate. The end-to-end tests reuse this connection where they can.
        auto& discord = mudlet::self()->mDiscord;
        if (discord.libraryLoaded() && mpDiscordIpcStub->listening()) {
            mpHost->mDiscordMode = Host::DiscordShowGameDetails;
            mpHost->mRequiredDiscordUserName.clear();
            QVERIFY2(establishDiscordLogin(), "Discord warm-up handshake did not complete in time");
        }
    }

    void init()
    {
        QVERIFY(mpHost);
        auto& discord = mudlet::self()->mDiscord;
        // Restore the default gating state BEFORE resetData(): resetData() calls
        // UpdatePresence() internally, and a Disabled mode or username mismatch
        // left over from the previous test would make that call tear the shared
        // RPC connection down.
        mpHost->mDiscordMode = Host::DiscordShowGameDetails;
        mpHost->mDiscordAccessFlags = Host::DiscordSetSubMask;
        mpHost->mRequiredDiscordUserName.clear();
        discord.resetData(mpHost);
        // Deliberately do NOT clear Discord::smUserName here: a completed
        // handshake populated it and the reuse check in establishDiscordLogin()
        // depends on it. Tests that need a specific logged-in identity set
        // Discord::smUserName themselves.
    }

    // -- Mode gating tests --

    void testGMCPIgnoredInDisabledMode()
    {
        mpHost->mDiscordMode = Host::DiscordDisabled;
        auto& discord = mudlet::self()->mDiscord;

        mpHost->processDiscordGMCP(qsl("External.Discord.Status"), qsl(R"({"details":"Exploring the forest","state":"Level 50 Mage"})"));

        QVERIFY2(discord.getDetailText(mpHost).isEmpty(), "GMCP details should be ignored in Disabled mode");
        QVERIFY2(discord.getStateText(mpHost).isEmpty(), "GMCP state should be ignored in Disabled mode");
    }

    void testGMCPIgnoredInMudletOnlyMode()
    {
        mpHost->mDiscordMode = Host::DiscordShowMudletOnly;
        auto& discord = mudlet::self()->mDiscord;

        mpHost->processDiscordGMCP(qsl("External.Discord.Status"), qsl(R"({"details":"Exploring the forest","state":"Level 50 Mage"})"));

        QVERIFY2(discord.getDetailText(mpHost).isEmpty(), "GMCP details should be ignored in MudletOnly mode");
        QVERIFY2(discord.getStateText(mpHost).isEmpty(), "GMCP state should be ignored in MudletOnly mode");
    }

    void testGMCPProcessedInGameDetailsMode()
    {
        mpHost->mDiscordMode = Host::DiscordShowGameDetails;
        auto& discord = mudlet::self()->mDiscord;

        mpHost->processDiscordGMCP(qsl("External.Discord.Status"), qsl(R"({"details":"Exploring the forest","state":"Level 50 Mage"})"));

        QCOMPARE(discord.getDetailText(mpHost), qsl("Exploring the forest"));
        QCOMPARE(discord.getStateText(mpHost), qsl("Level 50 Mage"));
    }

    void testGMCPInfoIgnoredOutsideGameDetailsMode()
    {
        mpHost->mDiscordMode = Host::DiscordShowMudletOnly;
        auto& discord = mudlet::self()->mDiscord;

        mpHost->processDiscordGMCP(qsl("External.Discord.Info"), qsl(R"({"applicationid":"123456789"})"));

        // Application ID should remain default (empty or Mudlet's) since Info was ignored
        QVERIFY2(discord.getApplicationId(mpHost).isEmpty() || discord.getApplicationId(mpHost) == Discord::mMudletApplicationId, "GMCP Info should be ignored outside GameDetails mode");
    }

    // -- Server-origin tracking tests --

    void testGMCPSetsServerOrigin()
    {
        mpHost->mDiscordMode = Host::DiscordShowGameDetails;
        auto& discord = mudlet::self()->mDiscord;

        QVERIFY2(!discord.isServerOrigin(mpHost, Host::DiscordSetDetail), "Detail should not be server-origin before GMCP");
        QVERIFY2(!discord.isServerOrigin(mpHost, Host::DiscordSetState), "State should not be server-origin before GMCP");

        mpHost->processDiscordGMCP(qsl("External.Discord.Status"), qsl(R"({"details":"Hunting","state":"In Combat"})"));

        QVERIFY2(discord.isServerOrigin(mpHost, Host::DiscordSetDetail), "Detail should be server-origin after GMCP set it");
        QVERIFY2(discord.isServerOrigin(mpHost, Host::DiscordSetState), "State should be server-origin after GMCP set it");
    }

    void testLuaSetterClearsServerOrigin()
    {
        auto& discord = mudlet::self()->mDiscord;

        // First, have the server set it
        mpHost->processDiscordGMCP(qsl("External.Discord.Status"), qsl(R"({"details":"Server set this"})"));
        QVERIFY(discord.isServerOrigin(mpHost, Host::DiscordSetDetail));

        // Now simulate what the Lua API does: clear server origin, then set
        discord.clearServerOrigin(mpHost, Host::DiscordSetDetail);
        discord.setDetailText(mpHost, qsl("Lua set this"));

        QVERIFY2(!discord.isServerOrigin(mpHost, Host::DiscordSetDetail), "Detail should not be server-origin after Lua cleared it");
        QCOMPARE(discord.getDetailText(mpHost), qsl("Lua set this"));
    }

    void testServerOriginNotSetForUnsentFields()
    {
        auto& discord = mudlet::self()->mDiscord;

        // Send GMCP with only detail, not state
        mpHost->processDiscordGMCP(qsl("External.Discord.Status"), qsl(R"({"details":"Only details"})"));

        QVERIFY2(discord.isServerOrigin(mpHost, Host::DiscordSetDetail), "Detail should be server-origin");
        QVERIFY2(!discord.isServerOrigin(mpHost, Host::DiscordSetState), "State should NOT be server-origin when server didn't send it");
    }

    // -- Privacy flag tests --

    void testPrivacyFlagBlocksServerField()
    {
        auto& discord = mudlet::self()->mDiscord;

        // Server sets detail
        mpHost->processDiscordGMCP(qsl("External.Discord.Status"), qsl(R"({"details":"Secret details","state":"Visible state"})"));

        // Disable the detail privacy flag (user chose to hide it)
        mpHost->mDiscordAccessFlags &= ~Host::DiscordSetDetail;

        // Detail is server-origin and privacy flag is off - shouldShow would return false
        QVERIFY2(discord.isServerOrigin(mpHost, Host::DiscordSetDetail), "Detail is server-origin");
        QVERIFY2(!(mpHost->mDiscordAccessFlags & Host::DiscordSetDetail), "Detail privacy flag should be disabled");

        // State privacy flag is still on - shouldShow would return true
        QVERIFY2(discord.isServerOrigin(mpHost, Host::DiscordSetState), "State is server-origin");
        QVERIFY2(mpHost->mDiscordAccessFlags & Host::DiscordSetState, "State privacy flag should still be enabled");
    }

    // -- GMCP Status field coverage --

    void testGMCPStatusSetsAllFields()
    {
        auto& discord = mudlet::self()->mDiscord;

        mpHost->processDiscordGMCP(qsl("External.Discord.Status"),
                                   qsl(R"({"details":"Hunting","state":"Level 50","smallimagetext":"Warrior","largeimagetext":"Achaea","starttime":1234567890,"partysize":3,"partymax":6})"));

        QCOMPARE(discord.getDetailText(mpHost), qsl("Hunting"));
        QCOMPARE(discord.getStateText(mpHost), qsl("Level 50"));
        QCOMPARE(discord.getSmallImageText(mpHost), qsl("Warrior"));
        QCOMPARE(discord.getLargeImageText(mpHost), qsl("Achaea"));
        QCOMPARE(discord.getTimeStamps(mpHost).first, static_cast<int64_t>(1234567890));
        QCOMPARE(discord.getParty(mpHost).first, 3);
        QCOMPARE(discord.getParty(mpHost).second, 6);

        // All set fields should be server-origin
        QVERIFY(discord.isServerOrigin(mpHost, Host::DiscordSetDetail));
        QVERIFY(discord.isServerOrigin(mpHost, Host::DiscordSetState));
        QVERIFY(discord.isServerOrigin(mpHost, Host::DiscordSetSmallIconText));
        QVERIFY(discord.isServerOrigin(mpHost, Host::DiscordSetLargeIconText));
        QVERIFY(discord.isServerOrigin(mpHost, Host::DiscordSetTimeInfo));
        QVERIFY(discord.isServerOrigin(mpHost, Host::DiscordSetPartyInfo));
    }

    void testResetDataClearsEverything()
    {
        auto& discord = mudlet::self()->mDiscord;

        // Set some data via GMCP
        mpHost->processDiscordGMCP(qsl("External.Discord.Status"), qsl(R"({"details":"Test","state":"Test"})"));
        QVERIFY(!discord.getDetailText(mpHost).isEmpty());
        QVERIFY(discord.isServerOrigin(mpHost, Host::DiscordSetDetail));

        // Reset
        discord.resetData(mpHost);

        QVERIFY2(discord.getDetailText(mpHost).isEmpty(), "Detail text should be cleared after resetData");
        QVERIFY2(discord.getStateText(mpHost).isEmpty(), "State text should be cleared after resetData");
        QVERIFY2(!discord.isServerOrigin(mpHost, Host::DiscordSetDetail), "Server-origin flags should be cleared after resetData");
    }

    // -- Lua API permission gating tests --
    // Setters and resetDiscordData() need write access (denied while the API
    // is read-only because the logged-in Discord user does not match the
    // profile's restriction); getters only need read access.

    void testLuaSetterDeniedWhenDisabled()
    {
        auto& discord = mudlet::self()->mDiscord;
        discord.setDetailText(mpHost, qsl("original"));
        mpHost->mDiscordMode = Host::DiscordDisabled;

        auto [result, error] = evalLua(qsl("setDiscordDetail(\"changed\")"));
        QVERIFY2(!result.isValid(), "setDiscordDetail() should be denied when Discord is disabled");
        QVERIFY2(!error.isEmpty(), "denial should come with an error message");
        QCOMPARE(discord.getDetailText(mpHost), qsl("original"));
    }

    void testLuaResetDeniedWhenDisabled()
    {
        auto& discord = mudlet::self()->mDiscord;
        discord.setDetailText(mpHost, qsl("keep me"));
        mpHost->mDiscordMode = Host::DiscordDisabled;

        auto [result, error] = evalLua(qsl("resetDiscordData()"));
        QVERIFY2(!result.isValid(), "resetDiscordData() should be denied when Discord is disabled");
        QVERIFY2(!error.isEmpty(), "denial should come with an error message");
        QCOMPARE(discord.getDetailText(mpHost), qsl("keep me"));
    }

    void testLuaSetterAndResetDeniedWhenReadOnly()
    {
        auto& discord = mudlet::self()->mDiscord;
        if (!discord.libraryLoaded()) {
            QSKIP("Discord RPC library not available - cannot test the read-only gate");
        }

        discord.setDetailText(mpHost, qsl("original"));
        // Simulate being logged into Discord with a different account than
        // the profile requires - that makes the Lua API read-only:
        Discord::smUserName = qsl("someone_else");
        mpHost->mRequiredDiscordUserName = qsl("profile_owner");

        auto [setResult, setError] = evalLua(qsl("setDiscordDetail(\"changed\")"));
        QVERIFY2(!setResult.isValid(), "setter should be denied while the API is read-only");
        QVERIFY2(setError.contains(qsl("read-only")), "denial should say the API is read-only");
        QCOMPARE(discord.getDetailText(mpHost), qsl("original"));

        auto [resetResult, resetError] = evalLua(qsl("resetDiscordData()"));
        QVERIFY2(!resetResult.isValid(), "resetDiscordData() should be denied while the API is read-only");
        QVERIFY2(resetError.contains(qsl("read-only")), "denial should say the API is read-only");
        QCOMPARE(discord.getDetailText(mpHost), qsl("original"));
    }

    void testLuaGettersAllowedWhenReadOnly()
    {
        auto& discord = mudlet::self()->mDiscord;
        if (!discord.libraryLoaded()) {
            QSKIP("Discord RPC library not available - cannot test the read-only gate");
        }

        discord.setSmallImage(mpHost, qsl("shield"));
        discord.setSmallImageText(mpHost, qsl("Guardian"));
        discord.setLargeImageText(mpHost, qsl("Achaea"));
        discord.setParty(mpHost, 2, 5);

        Discord::smUserName = qsl("someone_else");
        mpHost->mRequiredDiscordUserName = qsl("profile_owner");

        QCOMPARE(evalLua(qsl("getDiscordSmallIcon()")).first, QVariant(qsl("shield")));
        QCOMPARE(evalLua(qsl("getDiscordSmallIconText()")).first, QVariant(qsl("Guardian")));
        QCOMPARE(evalLua(qsl("getDiscordLargeIconText()")).first, QVariant(qsl("Achaea")));
        QCOMPARE(evalLua(qsl("getDiscordParty()")).first, QVariant(2.0));
    }

    void testLuaSetterAndResetAllowedWithWriteAccess()
    {
        auto& discord = mudlet::self()->mDiscord;
        if (!discord.libraryLoaded()) {
            QSKIP("Discord RPC library not available - cannot test the write-access path");
        }

        // No username restriction - write access is granted:
        auto [setResult, setError] = evalLua(qsl("setDiscordDetail(\"set from Lua\")"));
        QCOMPARE(setResult, QVariant(true));
        QCOMPARE(discord.getDetailText(mpHost), qsl("set from Lua"));

        auto [resetResult, resetError] = evalLua(qsl("resetDiscordData()"));
        QCOMPARE(resetResult, QVariant(true));
        QVERIFY2(discord.getDetailText(mpHost).isEmpty(), "resetDiscordData() should clear the data when permitted");
    }

    // -- End-to-end tests against the Discord IPC stub --
    // These drive the REAL discord-rpc library: it connects to
    // DiscordIpcServerStub over the discord-ipc-0 socket, performs the
    // genuine handshake, and its READY dispatch is what sets
    // Discord::smUserName - the friend-class seam is only used to clear
    // state, never to fake the logged-in user.

    void testIpcHandshakeMakesApiReadOnlyEndToEnd()
    {
        auto& discord = mudlet::self()->mDiscord;
        if (!discord.libraryLoaded()) {
            QSKIP("Discord RPC library not available - cannot test the IPC handshake");
        }
        if (!mpDiscordIpcStub->listening()) {
            QSKIP("Discord IPC stub is not listening - cannot test the IPC handshake");
        }

        discord.setSmallImage(mpHost, qsl("shield"));

        // In a full-suite run the preceding read-only tests leave a non-stub
        // logged-in name, so this deterministically drives a genuine op-0
        // handshake - establishDiscordLogin() only returns true once the stub's
        // handshake tally has advanced. The ready callback arrives via
        // Discord_RunCallbacks, pumped every 50ms by Discord's timer while the
        // event loop spins:
        QVERIFY2(establishDiscordLogin(), "the discord-rpc handshake did not complete in time");

        // The profile demands a different account, so the API turns read-only:
        mpHost->mRequiredDiscordUserName = qsl("profile_owner");

        // getDiscordSmallIcon() used to (wrongly) demand write access - it
        // must keep working while the API is read-only:
        QCOMPARE(evalLua(qsl("getDiscordSmallIcon()")).first, QVariant(qsl("shield")));

        auto [setResult, setError] = evalLua(qsl("setDiscordDetail(\"changed\")"));
        QVERIFY2(!setResult.isValid(), "setter should be denied while the API is read-only");
        QVERIFY2(setError.contains(qsl("read-only")), "denial should say the API is read-only");
        // No need to restore mRequiredDiscordUserName here: the next init()
        // resets the gating state before resetData()'s UpdatePresence() runs, so
        // the shared connection is not torn down on the way into the next test.
    }

    void testSetActivityReachesDiscordEndToEnd()
    {
        auto& discord = mudlet::self()->mDiscord;
        if (!discord.libraryLoaded()) {
            QSKIP("Discord RPC library not available - cannot test presence delivery");
        }
        if (!mpDiscordIpcStub->listening()) {
            QSKIP("Discord IPC stub is not listening - cannot test presence delivery");
        }

        // Reuse the genuine discord-rpc connection the preceding end-to-end test
        // established (seeded by the initTestCase warm-up) and kept alive across
        // init(); only fall back to a fresh (backoff-tolerant) handshake if it
        // was torn down. This is a real connection, not a faked login:
        QVERIFY2(establishDiscordLogin(), "the discord-rpc handshake did not complete in time");

        mpDiscordIpcStub->clearRecordedFrames();

        // No username restriction, so write access is granted:
        QCOMPARE(evalLua(qsl("setDiscordDetail(\"Exploring the IPC stub\")")).first, QVariant(true));
        QCOMPARE(evalLua(qsl("setDiscordState(\"end-to-end\")")).first, QVariant(true));

        // discord-rpc serializes SET_ACTIVITY on its own IO thread, so give
        // the frames generous time to arrive:
        QTRY_COMPARE_WITH_TIMEOUT(lastSetActivity().value(qsl("details")).toString(), qsl("Exploring the IPC stub"), 10000);
        QTRY_COMPARE_WITH_TIMEOUT(lastSetActivity().value(qsl("state")).toString(), qsl("end-to-end"), 10000);
    }

    void cleanupTestCase()
    {
        delete mpServer;
        mpServer = nullptr;
        mpHost = nullptr;
        // Null when initTestCase skipped or failed ahead of mudlet::start(), and
        // getMudletPath() dereferences the instance rather than checking it
        if (mudlet::self()) {
            const QString path = mudlet::getMudletPath(enums::profileHomePath, mHostname);
            QDir(path).removeRecursively();
            delete mudlet::self();
        }
        delete mpDiscordIpcStub;
        mpDiscordIpcStub = nullptr;
        mSavedXdg.isNull() ? qunsetenv("XDG_CONFIG_HOME") : qputenv("XDG_CONFIG_HOME", mSavedXdg);
    }
};

#include "TDiscordModeTest.moc"
MUDLET_GROUPED_TEST_MAIN(TDiscordModeTest)
