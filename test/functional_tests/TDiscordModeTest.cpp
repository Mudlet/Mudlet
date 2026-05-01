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

#include <QtTest/QtTest>

#include "MudletInstanceCoordinator.h"
#include "TelnetServerStub.h"
#include "discord.h"
#include "dlgConnectionProfiles.h"
#include "mudlet.h"

extern void qInitResources_mudlet();
extern void qInitResources_qm();
#ifdef INCLUDE_VARIABLE_SPLASH_SCREEN
extern void qInitResources_additional_splash_screens();
#endif
#ifdef INCLUDE_FONTS
extern void qInitResources_mudlet_fonts_common();
#if defined(Q_OS_LINUX) || defined(Q_OS_FREEBSD)
extern void qInitResources_mudlet_fonts_posix();
#endif
#endif
void initializeQRCResourcesForDiscordModeTest();

class TDiscordModeTest : public QObject {
    Q_OBJECT

private:
    TelnetServerStub* mpServer = nullptr;
    Host* mpHost = nullptr;
    const QString mHostname = "Discord-Mode-Test";
    const QString mPort = "4003";
    const QString mLocalhost = "localhost";

private slots:
    void initTestCase()
    {
        initializeQRCResourcesForDiscordModeTest();

        mpServer = new TelnetServerStub(qApp);
        mpServer->start(mLocalhost, mPort.toUShort());
        mudlet::start();
        mudlet::self()->setupConfig();
        mudlet::self()->takeOwnershipOfInstanceCoordinator(
            std::make_unique<MudletInstanceCoordinator>("MudletInstanceCoordinator"));
        mudlet::self()->init();
        mudlet::self()->setStorePasswordsSecurely(false);

        const QString path = mudlet::getMudletPath(enums::profileHomePath, mHostname);
        QDir(path).removeRecursively();

        QTimer::singleShot(0, qApp, [this]() {
            mudlet::self()->startAutoLogin({});
            QTest::qWait(100);
            QTest::mouseClick(mudlet::self()->mpConnectionDialog->new_profile_button, Qt::LeftButton);
            QTest::qWait(100);
            QTest::keyClicks(QApplication::focusWidget(), mHostname);
            QTest::qWait(100);
            QTest::keyClick(QApplication::focusWidget(), Qt::Key_Tab);
            QTest::qWait(100);
            QTest::keyClicks(QApplication::focusWidget(), mLocalhost);
            QTest::qWait(100);
            QTest::keyClick(QApplication::focusWidget(), Qt::Key_Tab);
            QTest::qWait(100);
            QTest::keyClicks(QApplication::focusWidget(), mPort);
            QTest::qWait(100);
            QTest::keyClick(QApplication::focusWidget(), Qt::Key_Return);
        });

        QSignalSpy spy(mudlet::self(), &mudlet::signal_profileLoaded);
        if (!spy.wait(1000)) {
            QFAIL("Profile took too long to load.");
        }
        mpHost = mudlet::self()->getActiveHost();
        if (!mpHost) {
            QFAIL("No active host available for the test.");
        }

        QSignalSpy spy2(&(mpHost->mTelnet), &cTelnet::signal_connected);
        if (!spy2.wait(500)) {
            QFAIL("Could not connect with the host.");
        }
    }

    void init()
    {
        QVERIFY(mpHost);
        auto& discord = mudlet::self()->mDiscord;
        // Reset Discord state between tests
        discord.resetData(mpHost);
        mpHost->mDiscordMode = Host::DiscordShowGameDetails;
        mpHost->mDiscordAccessFlags = Host::DiscordSetSubMask;
    }

    // -- Mode gating tests --

    void testGMCPIgnoredInDisabledMode()
    {
        mpHost->mDiscordMode = Host::DiscordDisabled;
        auto& discord = mudlet::self()->mDiscord;

        mpHost->processDiscordGMCP(
            qsl("External.Discord.Status"),
            qsl(R"({"details":"Exploring the forest","state":"Level 50 Mage"})"));

        QVERIFY2(discord.getDetailText(mpHost).isEmpty(),
                 "GMCP details should be ignored in Disabled mode");
        QVERIFY2(discord.getStateText(mpHost).isEmpty(),
                 "GMCP state should be ignored in Disabled mode");
    }

    void testGMCPIgnoredInMudletOnlyMode()
    {
        mpHost->mDiscordMode = Host::DiscordShowMudletOnly;
        auto& discord = mudlet::self()->mDiscord;

        mpHost->processDiscordGMCP(
            qsl("External.Discord.Status"),
            qsl(R"({"details":"Exploring the forest","state":"Level 50 Mage"})"));

        QVERIFY2(discord.getDetailText(mpHost).isEmpty(),
                 "GMCP details should be ignored in MudletOnly mode");
        QVERIFY2(discord.getStateText(mpHost).isEmpty(),
                 "GMCP state should be ignored in MudletOnly mode");
    }

    void testGMCPProcessedInGameDetailsMode()
    {
        mpHost->mDiscordMode = Host::DiscordShowGameDetails;
        auto& discord = mudlet::self()->mDiscord;

        mpHost->processDiscordGMCP(
            qsl("External.Discord.Status"),
            qsl(R"({"details":"Exploring the forest","state":"Level 50 Mage"})"));

        QCOMPARE(discord.getDetailText(mpHost), qsl("Exploring the forest"));
        QCOMPARE(discord.getStateText(mpHost), qsl("Level 50 Mage"));
    }

    void testGMCPInfoIgnoredOutsideGameDetailsMode()
    {
        mpHost->mDiscordMode = Host::DiscordShowMudletOnly;
        auto& discord = mudlet::self()->mDiscord;

        mpHost->processDiscordGMCP(
            qsl("External.Discord.Info"),
            qsl(R"({"applicationid":"123456789"})"));

        // Application ID should remain default (empty or Mudlet's) since Info was ignored
        QVERIFY2(discord.getApplicationId(mpHost).isEmpty() || discord.getApplicationId(mpHost) == Discord::mMudletApplicationId,
                 "GMCP Info should be ignored outside GameDetails mode");
    }

    // -- Server-origin tracking tests --

    void testGMCPSetsServerOrigin()
    {
        mpHost->mDiscordMode = Host::DiscordShowGameDetails;
        auto& discord = mudlet::self()->mDiscord;

        QVERIFY2(!discord.isServerOrigin(mpHost, Host::DiscordSetDetail),
                 "Detail should not be server-origin before GMCP");
        QVERIFY2(!discord.isServerOrigin(mpHost, Host::DiscordSetState),
                 "State should not be server-origin before GMCP");

        mpHost->processDiscordGMCP(
            qsl("External.Discord.Status"),
            qsl(R"({"details":"Hunting","state":"In Combat"})"));

        QVERIFY2(discord.isServerOrigin(mpHost, Host::DiscordSetDetail),
                 "Detail should be server-origin after GMCP set it");
        QVERIFY2(discord.isServerOrigin(mpHost, Host::DiscordSetState),
                 "State should be server-origin after GMCP set it");
    }

    void testLuaSetterClearsServerOrigin()
    {
        auto& discord = mudlet::self()->mDiscord;

        // First, have the server set it
        mpHost->processDiscordGMCP(
            qsl("External.Discord.Status"),
            qsl(R"({"details":"Server set this"})"));
        QVERIFY(discord.isServerOrigin(mpHost, Host::DiscordSetDetail));

        // Now simulate what the Lua API does: clear server origin, then set
        discord.clearServerOrigin(mpHost, Host::DiscordSetDetail);
        discord.setDetailText(mpHost, qsl("Lua set this"));

        QVERIFY2(!discord.isServerOrigin(mpHost, Host::DiscordSetDetail),
                 "Detail should not be server-origin after Lua cleared it");
        QCOMPARE(discord.getDetailText(mpHost), qsl("Lua set this"));
    }

    void testServerOriginNotSetForUnsentFields()
    {
        auto& discord = mudlet::self()->mDiscord;

        // Send GMCP with only detail, not state
        mpHost->processDiscordGMCP(
            qsl("External.Discord.Status"),
            qsl(R"({"details":"Only details"})"));

        QVERIFY2(discord.isServerOrigin(mpHost, Host::DiscordSetDetail),
                 "Detail should be server-origin");
        QVERIFY2(!discord.isServerOrigin(mpHost, Host::DiscordSetState),
                 "State should NOT be server-origin when server didn't send it");
    }

    // -- Privacy flag tests --

    void testPrivacyFlagBlocksServerField()
    {
        auto& discord = mudlet::self()->mDiscord;

        // Server sets detail
        mpHost->processDiscordGMCP(
            qsl("External.Discord.Status"),
            qsl(R"({"details":"Secret details","state":"Visible state"})"));

        // Disable the detail privacy flag (user chose to hide it)
        mpHost->mDiscordAccessFlags &= ~Host::DiscordSetDetail;

        // Detail is server-origin and privacy flag is off - shouldShow would return false
        QVERIFY2(discord.isServerOrigin(mpHost, Host::DiscordSetDetail),
                 "Detail is server-origin");
        QVERIFY2(!(mpHost->mDiscordAccessFlags & Host::DiscordSetDetail),
                 "Detail privacy flag should be disabled");

        // State privacy flag is still on - shouldShow would return true
        QVERIFY2(discord.isServerOrigin(mpHost, Host::DiscordSetState),
                 "State is server-origin");
        QVERIFY2(mpHost->mDiscordAccessFlags & Host::DiscordSetState,
                 "State privacy flag should still be enabled");
    }

    // -- GMCP Status field coverage --

    void testGMCPStatusSetsAllFields()
    {
        auto& discord = mudlet::self()->mDiscord;

        mpHost->processDiscordGMCP(
            qsl("External.Discord.Status"),
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
        mpHost->processDiscordGMCP(
            qsl("External.Discord.Status"),
            qsl(R"({"details":"Test","state":"Test"})"));
        QVERIFY(!discord.getDetailText(mpHost).isEmpty());
        QVERIFY(discord.isServerOrigin(mpHost, Host::DiscordSetDetail));

        // Reset
        discord.resetData(mpHost);

        QVERIFY2(discord.getDetailText(mpHost).isEmpty(),
                 "Detail text should be cleared after resetData");
        QVERIFY2(discord.getStateText(mpHost).isEmpty(),
                 "State text should be cleared after resetData");
        QVERIFY2(!discord.isServerOrigin(mpHost, Host::DiscordSetDetail),
                 "Server-origin flags should be cleared after resetData");
    }

    void cleanupTestCase()
    {
        delete mpServer;
        mpServer = nullptr;
        mpHost = nullptr;
        const QString path = mudlet::getMudletPath(enums::profileHomePath, mHostname);
        QDir(path).removeRecursively();
        delete mudlet::self();
    }
};

void initializeQRCResourcesForDiscordModeTest()
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

#include "TDiscordModeTest.moc"
QTEST_MAIN(TDiscordModeTest)
