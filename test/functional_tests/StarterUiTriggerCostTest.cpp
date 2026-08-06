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

    // The one contract the prefilter has: it must match every line one of the 65
    // shapes reads a value from. Miss one and the gauges silently never appear
    // for whichever game writes its prompt that way.
    //
    // Rather than pin a handful of sample lines, this crosses every label
    // spelling the shapes accept with every layout they describe and asserts the
    // implication over the lot - so a label added to a shape without being added
    // to the prefilter's union is caught by construction rather than by whoever
    // remembers to add a sample.
    void test_thePrefilterMatchesEveryLineTheShapesRead()
    {
        Host* host = startProfileWithStarterUi();
        QVERIFY(host);

        QVERIFY2(runLua(host, prefilterDifferentialScript()), "the prefilter differential did not run - see the profile's error console");
        QVERIFY2(luaTrue(host, qsl("__starterUiMisses == 0")), "the vitals prefilter drops lines the shapes read - the first few are in the error console");
        // Guards the corpus itself: if a refactor stopped the generated lines
        // producing readings, the implication above would hold vacuously.
        QVERIFY2(luaTrue(host, qsl("__starterUiReadable > 1500")), "the generated corpus stopped producing readings, so the prefilter check proved nothing");
        QVERIFY2(luaTrue(host, qsl("__starterUiShapesFired == __starterUiShapeCount")),
                 "the generated corpus no longer exercises every vitals shape - a new shape needs a layout or label "
                 "adding to the lists in prefilterDifferentialScript()");
    }

    // The precompilation falls back to the pattern string if rex.new ever fails,
    // which still works and would leave every test green while silently
    // restoring the per-line recompilation it replaced.
    void test_theShapesArePrecompiled()
    {
        Host* host = startProfileWithStarterUi();
        QVERIFY(host);
        QVERIFY2(luaTrue(host, qsl("BaseUI.shapesArePrecompiled()")), "a chat or vitals shape is not a compiled regex object, so it is recompiled on every line");
    }

    // The prefilter is only a saving if the shapes behind it still read a prompt.
    // Uses a label-after-the-numbers prompt, which only the recurrence-gated
    // shapes read, so this also pins the gate: nothing may reach the gauges until
    // the third sighting.
    void test_aPlainTextPromptStillDrivesTheGauges()
    {
        Host* host = startProfileWithStarterUi();
        QVERIFY(host);

        feedLine(host, qsl("<523/600hp 210/250m 80/100mv>"));
        QVERIFY2(luaTrue(host, qsl("BaseUI.vitalsData.hp == nil")), "a gated prompt shape drove the gauges on first sight");
        feedLine(host, qsl("<522/600hp 209/250m 79/100mv>"));
        QVERIFY(luaTrue(host, qsl("BaseUI.vitalsData.hp == nil")));

        feedLine(host, qsl("<521/600hp 208/250m 78/100mv>"));
        QVERIFY2(luaTrue(host, qsl("BaseUI.vitalsData.hp ~= nil and BaseUI.vitalsData.hp.max == 600")),
                 "a recurring cur/max prompt no longer reaches the gauges - the prefilter is dropping lines the "
                 "vitals shapes read");
        QVERIFY(luaTrue(host, qsl("BaseUI.vitalsData.hp.current == 521")));
        // mv is only read by shapes nothing else in this test touches.
        QVERIFY(luaTrue(host, qsl("BaseUI.vitalsData.mv ~= nil and BaseUI.vitalsData.mv.max == 100")));
    }

    // A percentage prompt needs no maximum at all, and is the one family with no
    // cur/max shape behind it.
    void test_aPercentagePromptStillDrivesTheGauges()
    {
        Host* host = startProfileWithStarterUi();
        QVERIFY(host);

        for (int i = 0; i < 3; ++i) {
            feedLine(host, qsl("<87%hp 80%m>"));
        }
        QVERIFY2(luaTrue(host, qsl("BaseUI.vitalsData.hp ~= nil and BaseUI.vitalsData.hp.percent == 87")), "a recurring percentage prompt no longer reaches the gauges");
        QVERIFY(luaTrue(host, qsl("BaseUI.vitalsData.mp.percent == 80")));
    }

    // Current-only prompts are the family with a user-visible side effect: they
    // cannot invent a maximum, so the layer asks the game for its score screen
    // once. That only happens if the prefilter lets the prompt through.
    void test_aCurrentOnlyPromptStillAsksTheGameForItsScoreScreen()
    {
        Host* host = startProfileWithStarterUi();
        QVERIFY(host);
        QVERIFY(luaTrue(host, qsl("not BaseUI.scoreRequested")));

        for (int i = 0; i < 3; ++i) {
            feedLine(host, qsl("<523hp 210m 80mv>"));
        }
        QVERIFY2(luaTrue(host, qsl("BaseUI.scoreRequested")),
                 "a current-only prompt no longer reaches BaseUI.maybeRequestScore, so games whose prompt carries no "
                 "maximum never get gauges");
    }

    // A different shape family through the same prefilter: score-screen rows are
    // trusted on first sight, and a score screen may only ever be shown once.
    void test_aScoreScreenIsStillReadOnFirstSight()
    {
        Host* host = startProfileWithStarterUi();
        QVERIFY(host);

        feedLine(host, qsl("Health:   3600/3600     Mana:     3400/3400"));
        QVERIFY2(luaTrue(host, qsl("BaseUI.vitalsData.hp ~= nil and BaseUI.vitalsData.hp.max == 3600")), "a score-screen row was not read on first sight");
        QVERIFY(luaTrue(host, qsl("BaseUI.vitalsData.mp ~= nil and BaseUI.vitalsData.mp.max == 3400")));

        // Chat is conversation, not a prompt: a tell quoting numbers must not
        // move the gauges, which is the chat shapes being consulted from the
        // vitals path.
        feedLine(host, qsl("Bob tells you, 'I am somehow alive at 11/12 hp'"));
        QVERIFY2(luaTrue(host, qsl("BaseUI.vitalsData.hp.max == 3600")), "a tell was harvested for vitals");
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

        // The three tagged shapes are one alternation, so each writes its tag
        // into a different capture group and the unmatched branches come back
        // empty. Exercise all three: only the first would be found by a handler
        // that still read matches[2] and nothing else.
        feedLine(host, qsl("[newbie] Ann: how do I get out of here?"));
        QVERIFY2(luaTrue(host, qsl("BaseUI.unread.channels == 1")),
                 "a [tag] channel line was not routed into the Channels tab - the grouped tagged trigger is not "
                 "finding its capture group");
        feedLine(host, qsl("(gossip) Ann: anyone around?"));
        QVERIFY2(luaTrue(host, qsl("BaseUI.unread.channels == 2")), "a (tag) channel line was not routed");
        feedLine(host, qsl("< chat | Ann: anyone around?"));
        QVERIFY2(luaTrue(host, qsl("BaseUI.unread.channels == 3")), "a < tag | channel line was not routed");

        // A tag the shape captures but that is not a known channel name is not
        // chat, grouped or not - BaseUI.chatChannelNames is still consulted.
        feedLine(host, qsl("[inventory] a rusty sword"));
        QVERIFY2(luaTrue(host, qsl("BaseUI.unread.channels == 3")), "an unknown tag was routed as a channel");
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
    // Crosses every label spelling the shapes accept with every layout they
    // describe, in three casings, and checks the prefilter's contract over the
    // result. Kept as one script so it runs inside the profile's own Lua state,
    // against the real BaseUI.parseVitalsLine and BaseUI.vitalsPrefilter.
    static QString prefilterDifferentialScript()
    {
        return qsl(R"LUA(
local labels = {
  "hp", "health", "hit", "hits", "hitpoint", "hitpoints", "hit point", "hit points", "h",
  "mp", "mana", "sp", "magic", "energy", "blood", "spell point", "spell points", "spellpoints", "m",
  "mv", "move", "moves", "movement", "movements", "move point", "move points", "movement points",
  "stamina", "st", "endurance", "end", "vitality",
  "xp", "exp", "experience", "experience point", "experience points", "exp points", "tnl",
}
local templates = {
  "@: 100/120", "@ 100/120", "@100/120", "100/120 @", "100/120@", "100 / 120 @",
  "@: 87%", "87% @", "@ 87%", "87%@", "@87%",
  "@: 100", "100 @", "100@",
  "| @: 100/120 |", "| @ : 100/120 |", "@ : 100 of 120", "@: 100(120)", "@ 100 ( 120 )",
  "You have 100/120 @.", "You have 100(120) @.", "You have 100/120 @points.",
  "You have 100/120 @ and 50/60 mana.", "You have 100/120 @ left.",
  "Level: 5   @: 100/120   Pager ( )", "@   :  [ 100/120 ]", "@: 12,345/23,456",
  "PRACT: 005   @: 90    of    90", "  @: 3600/3600     Mana:     3400/3400",
  "#### @ 100/120 ####", "50 @(50).",
  "| @: 100(120) |", "| @ : 100 of 120 |", "| Race: Undead | @: 4252/4252 |",
}

__starterUiMisses = 0
__starterUiReadable = 0
__starterUiShapeSeen = {}
local reported = 0

for _, label in ipairs(labels) do
  for _, template in ipairs(templates) do
    for _, spelling in ipairs({ label, label:sub(1, 1):upper() .. label:sub(2), label:upper() }) do
      local line = template:gsub("@", spelling)
      local readings = BaseUI.parseVitalsLine(line)
      if #readings > 0 then
        __starterUiReadable = __starterUiReadable + 1
        for _, reading in ipairs(readings) do
          __starterUiShapeSeen[reading.pattern] = true
        end
        -- rex.find, not rex.match: an unset first capture group comes back as
        -- false, which a truthiness test would read as "no match"
        if not rex.find(line, BaseUI.vitalsPrefilter) then
          __starterUiMisses = __starterUiMisses + 1
          if reported < 5 then
            reported = reported + 1
            echo("\n[ prefilter MISS ] " .. line .. "\n")
          end
        end
      end
    end
  end
end

__starterUiShapesFired = 0
for _ in pairs(__starterUiShapeSeen) do
  __starterUiShapesFired = __starterUiShapesFired + 1
end
__starterUiShapeCount = BaseUI.vitalsShapeCount()
)LUA");
    }

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
