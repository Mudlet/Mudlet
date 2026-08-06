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

// The starter UI (mudlet-base-ui) is preinstalled into new profiles, and its
// capture layers used to arm 77 always-active PCRE triggers - one per chat shape
// and one per vitals shape - which every line the game sent was then matched
// against. That roughly halved Mudlet's text throughput for every new user, and
// cost far more on a chatty game.
//
// The shapes themselves are unchanged; they are now fronted by a handful of
// triggers, with the full shape list run in Lua only on the lines that get
// through. This test pins both halves of that: the trigger count stays small,
// and the capture layers still capture.
//
// The package is installed by hand when the new-profile preinstall gate did not
// install it, so this stays about the package's own cost regardless of who is
// considered a new user (DefaultPackagesTest covers the gating).

#include <QtTest/QtTest>

#include "Host.h"
#include "MudletInstanceCoordinator.h"
#include "TLuaInterpreter.h"
#include "TelnetServerStub.h"
#include "TriggerUnit.h"
#include "ctelnet.h"
#include "dlgConnectionProfiles.h"
#include "mudlet.h"

extern void qInitResources_mudlet();
extern void qInitResources_qm();
extern void qInitResources_additional_splash_screens();
extern void qInitResources_mudlet_fonts_common();
extern void qInitResources_mudlet_fonts_posix();
static void initializeQRCResources();

class StarterUiTriggerCostTest : public QObject
{
    Q_OBJECT

private:
    TelnetServerStub* mpServer = nullptr;
    const QString mHostname = qsl("Test-StarterUiTriggerCost");
    const QString mLocalhost = qsl("localhost");
    quint16 mPort = 0;

    // Comfortably above the three chat groups plus the one vitals prefilter, and
    // far below the 77 the layers used to arm. A change that needs more than this
    // is a change that should be measured before it ships.
    static constexpr int kMaxRootTriggers = 8;

private slots:
    void initTestCase() { initializeQRCResources(); }

    void init()
    {
        mpServer = new TelnetServerStub(qApp);
        // Ephemeral port so parallel worktree runs never collide.
        mpServer->start(mLocalhost, 0);
        mPort = mpServer->serverPort();
        mudlet::start();
        mudlet::self()->setupConfig();
        mudlet::self()->takeOwnershipOfInstanceCoordinator(std::make_unique<MudletInstanceCoordinator>("MudletInstanceCoordinator"));
        mudlet::self()->init();
        mudlet::self()->setStorePasswordsSecurely(false);
        deleteProfileDirectory(mHostname);
    }

    void cleanup()
    {
        delete mpServer;
        mpServer = nullptr;
        deleteProfileDirectory(mHostname);
        delete mudlet::self();
    }

    // The regression guard proper: whatever the capture layers do, they may not
    // put a per-shape trigger on the hot path.
    void test_captureLayersArmAHandfulOfTriggersNotOnePerShape()
    {
        Host* host = startProfileWithStarterUi();
        QVERIFY(host);

        const int rootTriggers = static_cast<int>(host->getTriggerUnit()->getTriggerRootNodeList().size());
        QVERIFY2(rootTriggers > 0, "the starter UI armed no capture triggers at all - the fallback layers are dead");
        QVERIFY2(rootTriggers <= kMaxRootTriggers,
                 qPrintable(qsl("the starter UI armed %1 always-active triggers; every line of game text is matched "
                                "against all of them, which is what made 5.0 half the speed of 4.22.0")
                                    .arg(rootTriggers)));

        QVERIFY(luaTrue(host, qsl("#BaseUI.vitalsTriggerIds == 1")));
        QVERIFY(luaTrue(host, qsl("#BaseUI.chatTriggerIds == 3")));
    }

    // The prefilter is only a saving if the shapes behind it still read a prompt.
    // The cur/max prompt shapes are gated on three sightings, so feed it three
    // times the way a real prompt recurs.
    void test_aPlainTextPromptStillDrivesTheGauges()
    {
        Host* host = startProfileWithStarterUi();
        QVERIFY(host);

        feedLine(host, qsl("HP: 523/600 MP: 210/250"));
        feedLine(host, qsl("HP: 522/600 MP: 209/250"));
        feedLine(host, qsl("HP: 521/600 MP: 208/250"));

        QVERIFY2(luaTrue(host, qsl("BaseUI.vitalsData.hp ~= nil and BaseUI.vitalsData.hp.max == 600")),
                 "a recurring cur/max prompt no longer reaches the gauges - the prefilter is dropping lines the "
                 "vitals shapes read");
        QVERIFY(luaTrue(host, qsl("BaseUI.vitalsData.hp.current == 521")));
    }

    // Each chat group has to keep routing into the tab its shapes always did.
    void test_chatCaptureStillSortsLinesIntoTheirTabs()
    {
        Host* host = startProfileWithStarterUi();
        QVERIFY(host);

        feedLine(host, qsl("Bob tells you, 'hello there'"));
        QVERIFY2(luaTrue(host, qsl("BaseUI.chats ~= nil and BaseUI.unread ~= nil")), "a tell did not build the chat dock");
        // routeChatLine() counts the active tab as read, so the tell lands in
        // Tells' unread counter while All (the active tab) does not move.
        QVERIFY2(luaTrue(host, qsl("BaseUI.unread.tells == 1")), "a tell was not routed into the Tells tab");

        feedLine(host, qsl("[newbie] Ann: how do I get out of here?"));
        QVERIFY2(luaTrue(host, qsl("BaseUI.unread.channels == 1")),
                 "a tagged channel line was not routed into the Channels tab - the grouped tagged trigger is not "
                 "finding its capture group");

        // A tag that is not a known channel name is not chat, grouped or not.
        feedLine(host, qsl("[12:34] the clock strikes noon")); // NOLINT(readability-magic-numbers)
        QVERIFY(luaTrue(host, qsl("BaseUI.unread.channels == 1")));
    }

    // Once a protocol owns the gauges the plain-text layer's readings are thrown
    // away by applyVitals anyway, so leaving it armed costs every line for
    // nothing.
    void test_theVitalsLayerRetiresOnceAProtocolOwnsTheGauges()
    {
        Host* host = startProfileWithStarterUi();
        QVERIFY(host);
        QVERIFY(luaTrue(host, qsl("#BaseUI.vitalsTriggerIds == 1")));

        QVERIFY(runLua(host,
                       qsl("gmcp = gmcp or {}\n"
                           "gmcp.Char = { Vitals = { hp = 500, maxhp = 600 } }\n"
                           "BaseUI.updateVitals()")));
        QVERIFY2(luaTrue(host, qsl("BaseUI.structuredVitalsOwnGauges()")), "GMCP vitals did not take the source lock");
        QVERIFY2(luaTrue(host, qsl("#BaseUI.vitalsTriggerIds == 0")), "the plain-text vitals triggers stayed armed after GMCP took the gauges over");

        // A reconnect may be to a game with no GMCP at all, so the layer has to
        // come back.
        QVERIFY(runLua(host, qsl("BaseUI.handleDisconnect()")));
        QVERIFY2(luaTrue(host, qsl("#BaseUI.vitalsTriggerIds == 1")), "the vitals layer did not re-arm after a disconnect");
    }

private:
    Host* startProfileWithStarterUi()
    {
        startProfile();
        Host* host = mudlet::self()->getActiveHost();
        if (!host) {
            return nullptr;
        }
        host->mEchoLuaErrors = true;
        // A brand-new profile normally gets the package from the preinstall list
        // already; install it here only when that gate did not, so this test says
        // nothing about who counts as a new user.
        if (!host->mInstalledPackages.contains(qsl("mudlet-base-ui"))) {
            auto [installed, message] = host->installPackage(qsl(":/packages/mudlet-base-ui/mudlet-base-ui.mpackage"), enums::PackageModuleType::Package, true);
            if (!installed) {
                qWarning("%s", qPrintable(qsl("could not install the starter UI: %1").arg(message)));
                return nullptr;
            }
        }
        // The package script arms its triggers as it loads; a hidden or
        // stood-aside setting left over from an earlier run would suppress them,
        // and there is no profile directory to carry one in, but be explicit.
        if (!luaTrue(host, qsl("type(BaseUI) == 'table' and not BaseUI.dormant()"))) {
            qWarning("the starter UI did not load, or loaded dormant");
            return nullptr;
        }
        return host;
    }

    // Starts a profile the way a user would via the GUI (mirrors the helper the
    // other functional tests use).
    void startProfile()
    {
        const QString port = QString::number(mPort);
        QTimer::singleShot(0, qApp, [this, port]() {
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
            QTest::keyClicks(QApplication::focusWidget(), port);
            QTest::qWait(100);
            QTest::keyClick(QApplication::focusWidget(), Qt::Key_Return);
        });

        QSignalSpy loaded(mudlet::self(), &mudlet::signal_profileLoaded);
        if (!loaded.wait(5000)) {
            QFAIL("Profile took too long to load.");
        }
        Host* host = mudlet::self()->getActiveHost();
        if (!host) {
            QFAIL("No active host available for the test.");
        }
        QSignalSpy connected(&(host->mTelnet), &cTelnet::signal_connected);
        if (!connected.wait(3000)) {
            QFAIL("Could not connect to the stub.");
        }
    }

    // Through the production socket path, so the triggers see the line exactly
    // as they would from a game.
    void feedLine(Host* host, const QString& text)
    {
        QByteArray data = text.toUtf8() + "\r\n";
        data.reserve(data.size() + 16);
        host->mTelnet.loopbackTest(data);
    }

    bool runLua(Host* host, const QString& script) { return host->getLuaInterpreter()->compileAndExecuteScript(script); }

    // Reports the truth of a Lua expression back through a global, so a false
    // result is a plain failure rather than a Lua error in the console.
    bool luaTrue(Host* host, const QString& expression)
    {
        if (!runLua(host, qsl("__starterUiProbe = not not (%1)").arg(expression))) {
            qWarning("%s", qPrintable(qsl("probe did not compile: %1").arg(expression)));
            return false;
        }
        const bool result = runLua(host, qsl("assert(__starterUiProbe)"));
        if (!result) {
            qWarning("%s", qPrintable(qsl("probe is false: %1").arg(expression)));
        }
        return result;
    }

    void deleteProfileDirectory(const QString& profileName)
    {
        QDir dir(mudlet::getMudletPath(enums::profileHomePath, profileName));
        if (dir.exists()) {
            dir.removeRecursively();
        }
    }
};

static void initializeQRCResources()
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

#include "StarterUiTriggerCostTest.moc"
QTEST_MAIN(StarterUiTriggerCostTest)
