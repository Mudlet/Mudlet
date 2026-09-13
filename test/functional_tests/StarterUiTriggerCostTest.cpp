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

// The starter UI (mudlet-base-ui) is preinstalled into new profiles, so every
// always-active trigger it arms is matched against every line the game sends.
// This pins what that costs and that the capture layers still capture.

#include <QFileInfo>
#include <QTemporaryDir>
#include <QtTest/QtTest>

#include "PortableModeTestHelper.h"
#include "ProfileTestHelper.h"
#include "Host.h"
#include "MudletInstanceCoordinator.h"
#include "TLuaInterpreter.h"
#include "TTrigger.h"
#include "TelnetServerStub.h"
#include "TriggerUnit.h"
#include "ctelnet.h"
#include "dlgConnectionProfiles.h"
#include "dlgTriggerEditor.h"
#include <QTreeWidget>
#include "mudlet.h"

#include "GroupedTest.h"

class StarterUiTriggerCostTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;
    TelnetServerStub* mpServer = nullptr;
    const QString mHostname = qsl("Test-StarterUiTriggerCost");
    const QString mLocalhost = qsl("localhost");
    quint16 mPort = 0;

    // A full default-package profile measures 3: the starter UI's chat capture
    // tree and its vitals prefilter, plus one folder from another package.
    // Nesting hides growth from this count, so kMaxGatePatterns is what tracks
    // the per-line cost.
    static constexpr int kMaxRootTriggers = 5;

    // What every line of game text really pays for: the substrings the chat
    // gates scan for before any regex runs. Measures 20, the whole budget.
    // Raising this is a throughput change and wants measuring first.
    static constexpr int kMaxGatePatterns = 20;

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
    }

    void cleanupTestCase() { mSavedXdg.isNull() ? qunsetenv("XDG_CONFIG_HOME") : qputenv("XDG_CONFIG_HOME", mSavedXdg); }

    void init()
    {
        mpServer = new TelnetServerStub(qApp);
        // Ephemeral port so parallel worktree runs never collide.
        mpServer->start(mLocalhost, 0);
        mPort = mpServer->serverPort();
        mudlet::start();
        mudlet::self()->setupConfig();
        QCOMPARE(mudlet::getMudletPath(enums::mainPath), qsl("%1/mudlet").arg(mConfigDir.path()));
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

    void test_captureLayersArmAHandfulOfTriggersNotOnePerShape()
    {
        Host* host = startProfileWithStarterUi();
        QVERIFY(host);

        const int rootTriggers = static_cast<int>(host->getTriggerUnit()->getTriggerRootNodeList().size());
        QVERIFY2(rootTriggers > 0, "no triggers are registered at all - the profile did not finish loading its packages");
        QVERIFY2(rootTriggers <= kMaxRootTriggers,
                 qPrintable(qsl("a new user's profile arms %1 always-active root triggers, and every line of game "
                                "text is matched against all of them")
                                    .arg(rootTriggers)));

        QVERIFY(luaTrue(host, qsl("#BaseUI.vitalsTriggerIds == 1")));
        QVERIFY2(luaTrue(host, qsl("BaseUI.chatTriggersArmed()")), "the chat capture tree's gates did not come up armed");
    }

    // Miss a line here and that game's gauges silently never appear.
    //
    // The label list in the script is hand-written, not derived from the
    // package's label tables, so this catches a new shape whose spelling is
    // missing from the prefilter but not a new spelling bolted onto an existing
    // shape - add spellings to promptLabels and friends, not inline.
    void test_thePrefilterMatchesEveryLineTheShapesRead()
    {
        Host* host = startProfileWithStarterUi();
        QVERIFY(host);

        QVERIFY2(runLua(host, prefilterDifferentialScript()), "the prefilter differential did not run - see the profile's error console");
        QVERIFY2(luaTrue(host, qsl("__starterUi.misses == 0")), "the vitals prefilter drops lines the shapes read - the first few are in the error console");
        // Without this the assertion above holds vacuously.
        QVERIFY2(luaTrue(host, qsl("__starterUi.readable > 1500")), "the generated corpus stopped producing readings, so the prefilter check proved nothing");
        QVERIFY2(luaTrue(host, qsl("__starterUi.shapesFired == __starterUi.shapeCount")),
                 "the generated corpus no longer exercises every vitals shape - a new shape needs a layout or label "
                 "adding to the lists in prefilterDifferentialScript()");
    }

    // The fallback to a pattern string still works, so nothing else fails when
    // a shape is recompiled per line.
    void test_theShapesArePrecompiled()
    {
        Host* host = startProfileWithStarterUi();
        QVERIFY(host);
        QVERIFY2(luaTrue(host, qsl("BaseUI.shapesArePrecompiled()")), "a chat or vitals shape is not a compiled regex object, so it is recompiled on every line");
    }

    // A label-after-the-numbers prompt is read only by recurrence-gated shapes,
    // so this pins the gate as well: nothing until the third sighting.
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
        QVERIFY(luaTrue(host, qsl("BaseUI.vitalsData.mv ~= nil and BaseUI.vitalsData.mv.max == 100")));
    }

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

    // A current-only prompt cannot supply a maximum, so the layer sends "score"
    // once - the one capture path with a visible side effect on the game.
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

    // Score-screen rows are trusted on first sight: a score may only be shown once.
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

    // Every chat shape needs a line here. The tree's gates are
    // case-sensitive substrings, so a shape whose literal is spelled differently
    // in its gate silently never routes, and only a line exercising that shape
    // notices.
    void test_chatCaptureStillSortsLinesIntoTheirTabs()
    {
        Host* host = startProfileWithStarterUi();
        QVERIFY(host);

        // second is the tab besides All the line has to reach, empty for the
        // shapes that only ever reach All
        const QList<QPair<QString, QString>> corpus = {
                {qsl("Bob tells you, 'hello there'"), qsl("tells")},
                {qsl("You tell Ann, 'on my way'"), qsl("tells")},
                {qsl("You tell the formation you are ready."), qsl("tells")},
                {qsl("Ann whispers to you, 'psst'"), qsl("tells")},
                {qsl("Bob tells the group, 'incoming'"), qsl("tells")},
                {qsl("You gossip, 'test!'"), qsl("tells")},
                {qsl("Bob says, 'hello everyone'"), QString()},
                {qsl("Bob asks, 'where is the bank?'"), QString()},
                {qsl("Bob exclaims, 'at last!'"), QString()},
                {qsl("You say, 'hi there'"), QString()},
                {qsl("You ask, 'which way?'"), QString()},
                {qsl("You exclaim, 'finally!'"), QString()},
                {qsl("Bob yells, 'help!'"), QString()},
                {qsl("Bob shouts, 'to arms!'"), QString()},
                {qsl("You yell, 'wait for me!'"), QString()},
                {qsl("You shout, 'over here!'"), QString()},
                {qsl("Bob chats, 'hello everyone'"), qsl("channels")},
                {qsl("You chat, \"test.\""), qsl("channels")},
                {qsl("[tell] Ann: are you there?"), qsl("tells")},
                {qsl("[newbie] Ann: how do I get out of here?"), qsl("channels")},
                {qsl("(gossip) Ann: anyone around?"), qsl("channels")},
                {qsl("< chat | Ann: anyone around?"), qsl("channels")},
        };

        QHash<QString, int> expectedUnread;
        for (const auto& [text, family] : corpus) {
            feedLine(host, text);
            // routeChatLine() records what it copied, and counts the active tab
            // (All) as read, so this is what says the line reached the dock
            QVERIFY2(luaTrue(host, qsl("BaseUI.recentCaptures[#BaseUI.recentCaptures] ~= nil and BaseUI.recentCaptures[#BaseUI.recentCaptures].text == %1").arg(luaLiteral(text))),
                     qPrintable(qsl("the chat tree did not route: %1").arg(text)));
            if (family.isEmpty()) {
                continue;
            }
            expectedUnread[family] += 1;
            QVERIFY2(luaTrue(host, qsl("BaseUI.unread.%1 == %2").arg(family, QString::number(expectedUnread.value(family)))), qPrintable(qsl("did not reach the %1 tab: %2").arg(family, text)));
        }

        // BaseUI.chatChannelNames has the last word on a captured tag, so this
        // line reaches no tab at all rather than only missing Channels.
        // recentCaptures is capped at ten and the corpus above already filled
        // it, so its length would hold whatever happened.
        QVERIFY(runLua(host, qsl("__starterUiAllLines = getLineCount('BaseUI_chat_all')")));
        feedLine(host, qsl("[inventory] a rusty sword"));
        QVERIFY2(luaTrue(host, qsl("getLineCount('BaseUI_chat_all') == __starterUiAllLines")), "an unknown tag was captured as chat");

        // No line above matches two shapes, so none of them trips routeChatLine's
        // dedup on lastChatLine. "You tell the group, 'go now'" matches both the
        // "You tell <someone>" and the "You tell the group" shapes. A copied
        // line need not be one line in the tab, so the yardstick is measured
        // rather than assumed, and the two lines are the same length so wrapping
        // cannot separate them.
        QVERIFY(runLua(host, routeChatLineCounterScript()));
        QVERIFY(runLua(host, qsl("__starterUi.routed = 0\n__starterUiTellLines = getLineCount('BaseUI_chat_tells')")));
        feedLine(host, qsl("Bob tells you, 'go now dude'"));
        QVERIFY2(luaTrue(host, qsl("__starterUi.routed == 1")), "the one-shape line matched more than one shape, so it is no yardstick for a line that does");
        QVERIFY(runLua(host, qsl("__starterUiOneCapture = getLineCount('BaseUI_chat_tells') - __starterUiTellLines")));
        QVERIFY2(luaTrue(host, qsl("__starterUiOneCapture > 0")), "a line matching one chat shape did not reach the Tells tab, so the comparison below proves nothing");

        const int tellsBefore = expectedUnread.value(qsl("tells")) + 1;
        QVERIFY(runLua(host, qsl("__starterUi.routed = 0\n__starterUiTellLines = getLineCount('BaseUI_chat_tells')")));
        feedLine(host, qsl("You tell the group, 'go now'"));
        QVERIFY2(luaTrue(host, qsl("__starterUi.routed == 2")), "the tree no longer fires twice on this line, so the dedup below is never reached and proves nothing");
        QVERIFY2(luaTrue(host, qsl("getLineCount('BaseUI_chat_tells') - __starterUiTellLines == __starterUiOneCapture")), "a line matching two chat shapes was copied into the Tells tab twice");
        QVERIFY2(luaTrue(host, qsl("BaseUI.unread.tells == %1").arg(QString::number(tellsBefore + 1))), "a line matching two chat shapes counted twice against the Tells tab");

        QVERIFY(runLua(host, chatShapeCoverageScript(corpus)));
        QVERIFY2(luaTrue(host, qsl("__starterUi.uncoveredShapes == 0")),
                 "a chat shape has no line in the corpus above, so nothing would notice if its gate stopped matching - "
                 "the shapes are named in the error console");
        QVERIFY2(luaTrue(host, qsl("__starterUi.unroutedLines == 0")),
                 "a corpus line routes through the trigger tree but no chatPatterns shape recognises it, so the vitals "
                 "layer would harvest it - the lines are in the error console");

        assertCorpusCoversEveryGateLiteral(host, corpus);
    }

    // The gate layer is what every line pays for, so it has to stay substring
    // matching, and the shapes hanging off it have to stay the ones the script
    // knows about - chatLikeLine() reads that list to keep chat out of the
    // vitals layer, and a shape only the tree has would be harvested for gauges.
    void test_theChatTreeGatesOnSubstringsAndKeepsTheScriptsShapes()
    {
        Host* host = startProfileWithStarterUi();
        QVERIFY(host);

        TTrigger* tree = findTrigger(host, qsl("Mudlet base UI chat capture"));
        QVERIFY2(tree, "the chat capture tree is not in the profile under the name the script arms");
        QVERIFY2(tree->getPatternsList().isEmpty(), "the chat tree's root folder grew a pattern, so it is a chain now and no longer passes every line to its gates");

        int gatePatterns = 0;
        QStringList shapes;
        for (auto* gateNode : *tree->getChildrenList()) {
            auto* gate = static_cast<TTrigger*>(gateNode);
            const QList<int> kinds = gate->getRegexCodePropertyList();
            QVERIFY2(!kinds.isEmpty(), qPrintable(qsl("gate \"%1\" has no patterns, so it passes every line straight to its regexes").arg(gate->getName())));
            for (const int kind : kinds) {
                QVERIFY2(kind == REGEX_SUBSTRING || kind == REGEX_BEGIN_OF_LINE_SUBSTRING,
                         qPrintable(qsl("gate \"%1\" has a pattern of kind %2 - a gate must be substring matching only, or every line pays for a regex").arg(gate->getName(), QString::number(kind))));
            }
            gatePatterns += static_cast<int>(kinds.size());
            for (auto* shapeNode : *gate->getChildrenList()) {
                auto* shape = static_cast<TTrigger*>(shapeNode);
                shapes << shape->getPatternsList();
            }
        }

        QVERIFY2(gatePatterns <= kMaxGatePatterns,
                 qPrintable(qsl("the chat gates scan every line for %1 substrings, over the budget of %2").arg(QString::number(gatePatterns), QString::number(kMaxGatePatterns))));

        QVERIFY(runLua(host, treeShapeComparisonScript(shapes)));
        QVERIFY2(luaTrue(host, qsl("__starterUi.shapeMismatch == nil")),
                 "the tree's shapes and the script's chatPatterns have drifted apart - chatLikeLine() would stop recognising a "
                 "line the tree routes, and the vitals layer would read it as a prompt. The first difference is in the error console");
    }

    // Nothing else here would notice the guard in refreshChatTabs() going away:
    // BaseUI.unread and the tab text stay correct either way. What moves is the
    // number of repaints, and what the labels are left showing.
    void test_aCapturedLineRepaintsOnlyTheTabItChanged()
    {
        Host* host = startProfileWithStarterUi();
        QVERIFY(host);
        feedLine(host, qsl("Bob tells you, 'hello there'"));
        QVERIFY2(luaTrue(host, qsl("BaseUI.chatTabLabels ~= nil and BaseUI.chatTabLabels.tells ~= nil")), "a captured chat line did not build the chat dock");
        QVERIFY(runLua(host, chatTabRepaintCounterScript()));

        // Zero is also what wrappers that never took would report.
        QVERIFY(runLua(host, qsl("BaseUI.chatTabLabels.all:echo('probe')\nBaseUI.chatTabLabels.all:setStyleSheet('background-color: #ff0000;')")));
        QVERIFY2(luaTrue(host, qsl("__starterUi.echoes == 1")), "the echo counter never reached the labels, so every echo count below would read zero whatever happened");
        QVERIFY2(luaTrue(host, qsl("__starterUi.styles == 1")), "the setStyleSheet counter never reached the labels, so every style count below would read zero whatever happened");
        QVERIFY(runLua(host, qsl("BaseUI.refreshChatTabs()")));

        QVERIFY(runLua(host, qsl("__starterUi.echoes, __starterUi.styles = 0, 0\nBaseUI.refreshChatTabs()")));
        QVERIFY2(luaTrue(host, qsl("__starterUi.echoes == 0 and __starterUi.styles == 0")), "a refresh with nothing changed still repainted a tab");

        QVERIFY(runLua(host, qsl("__starterUi.echoes, __starterUi.styles = 0, 0")));
        feedLine(host, qsl("Ann whispers to you, 'psst'"));
        QVERIFY2(luaTrue(host, qsl("BaseUI.unread.tells == 2")), "the second tell did not reach the Tells tab");
        QVERIFY2(luaTrue(host, qsl("__starterUi.echoes == 1")), "a captured chat line repainted a tab besides the one whose unread count moved");
        QVERIFY2(luaTrue(host, qsl("__starterUi.styles == 0")), "a captured chat line restyled a tab, though which tab is active had not changed");
        QVERIFY2(luaTrue(host, qsl("getLabelText('BaseUI_tab_tells'):find('Tells (2)', 1, true) ~= nil")), "the Tells tab is not showing the unread count it counted");

        // A guard that skips the right calls can still leave the wrong thing on
        // screen, which only Qt's own copy shows. #4fc1e9 is theme.accent, the
        // underline marking the active tab; #e8eef4 is theme.text and #96a0ab
        // theme.textDim.
        QVERIFY2(luaTrue(host, qsl("getLabelStyleSheet('BaseUI_tab_all'):find('#4fc1e9', 1, true) ~= nil")), "the active tab is not wearing the accent that marks it active");
        QVERIFY2(luaTrue(host, qsl("getLabelStyleSheet('BaseUI_tab_tells'):find('#4fc1e9', 1, true) == nil")), "an inactive tab is wearing the active tab's accent");
        QVERIFY2(luaTrue(host, qsl("getLabelText('BaseUI_tab_all'):find('#e8eef4', 1, true) ~= nil")), "the active tab is not wearing theme.text");
        QVERIFY2(luaTrue(host, qsl("getLabelText('BaseUI_tab_tells'):find('#96a0ab', 1, true) ~= nil")), "an inactive tab is not wearing theme.textDim");

        // Two of the three, not all: the third was inactive and stays inactive.
        // The tab left behind keeps its text and only its colour moves, so the
        // colour clause is the one that has to fire for it.
        QVERIFY(runLua(host, qsl("__starterUi.echoes, __starterUi.styles = 0, 0\nBaseUI.selectChatTab('tells')")));
        QVERIFY2(luaTrue(host, qsl("__starterUi.styles == 2")), "switching tabs did not restyle both the tab left behind and the tab taken up");
        QVERIFY2(luaTrue(host, qsl("__starterUi.echoes == 2")), "switching tabs did not re-echo both of them, so one kept the colour of the state it left");
        QVERIFY2(luaTrue(host, qsl("BaseUI.chatTabLabels.tells.message == 'Tells'")), "the tab switched to kept its unread badge");
        QVERIFY2(luaTrue(host, qsl("BaseUI.chatTabLabels.all.message == 'All'")), "the tab switched away from grew an unread badge it had not earned");
        QVERIFY2(luaTrue(host, qsl("getLabelStyleSheet('BaseUI_tab_tells'):find('#4fc1e9', 1, true) ~= nil")), "the tab switched to did not take on the active tab's accent");
        QVERIFY2(luaTrue(host, qsl("getLabelStyleSheet('BaseUI_tab_all'):find('#4fc1e9', 1, true) == nil")), "the tab switched away from kept the active tab's accent");
        QVERIFY2(luaTrue(host, qsl("getLabelText('BaseUI_tab_tells'):find('#e8eef4', 1, true) ~= nil")), "the tab switched to did not take on theme.text");
        QVERIFY2(luaTrue(host, qsl("getLabelText('BaseUI_tab_all'):find('#96a0ab', 1, true) ~= nil")), "the tab switched away from kept theme.text");

        // The guard reading a cache the previous refresh wrote.
        QVERIFY(runLua(host, qsl("__starterUi.echoes, __starterUi.styles = 0, 0")));
        feedLine(host, qsl("Bob says, 'hello everyone'"));
        QVERIFY2(luaTrue(host, qsl("BaseUI.unread.all == 1")), "the line did not reach the All tab");
        QVERIFY2(luaTrue(host, qsl("__starterUi.echoes == 1 and __starterUi.styles == 0")), "a line arriving after a tab switch repainted more than the tab whose unread count moved");
        QVERIFY2(luaTrue(host, qsl("getLabelText('BaseUI_tab_all'):find('All (1)', 1, true) ~= nil")), "the All tab is not showing the unread count it grew after the switch");
    }

    // setLabelStyleSheet and echo reach these labels without telling Geyser, so
    // its cache still describes a tab that no longer looks like that.
    void test_aTabWrittenBehindGeysersBackIsPutRightAgain()
    {
        Host* host = startProfileWithStarterUi();
        QVERIFY(host);
        feedLine(host, qsl("Bob tells you, 'hello there'"));
        QVERIFY2(luaTrue(host, qsl("BaseUI.chatTabLabels ~= nil and BaseUI.chatTabLabels.all ~= nil")), "a captured chat line did not build the chat dock");

        // All is the active tab, so its unread count never moves and nothing else
        // would ever repaint it.
        QVERIFY(runLua(host, qsl("setLabelStyleSheet('BaseUI_tab_all', 'background-color: #ff0000;')\necho('BaseUI_tab_all', 'CLOBBERED')")));
        QVERIFY2(luaTrue(host, qsl("getLabelStyleSheet('BaseUI_tab_all') == 'background-color: #ff0000;'")), "the stylesheet never reached the label, so this proves nothing");
        QVERIFY2(luaTrue(host, qsl("getLabelText('BaseUI_tab_all'):find('CLOBBERED', 1, true) ~= nil")), "the text never reached the label, so this proves nothing");
        QVERIFY2(luaTrue(host, qsl("BaseUI.chatTabLabels.all.message == 'All'")), "Geyser noticed the write after all, so the guard would not need to read the label back");

        QVERIFY(runLua(host, qsl("BaseUI.refreshChatTabs()")));
        QVERIFY2(luaTrue(host, qsl("getLabelStyleSheet('BaseUI_tab_all'):find('#4fc1e9', 1, true) ~= nil")),
                 "a tab restyled behind Geyser's back was left that way - the guard trusted a cache that no longer described the label");
        QVERIFY2(luaTrue(host, qsl("getLabelText('BaseUI_tab_all'):find('CLOBBERED', 1, true) == nil")),
                 "a tab written behind Geyser's back kept the foreign text - the guard trusted a cache that no longer described the label");
        QVERIFY2(luaTrue(host, qsl("getLabelText('BaseUI_tab_all'):find('All', 1, true) ~= nil")), "the tab was not given its own text back");

        QVERIFY(runLua(host, chatTabRepaintCounterScript()));
        QVERIFY(runLua(host, qsl("__starterUi.echoes, __starterUi.styles = 0, 0\nBaseUI.refreshChatTabs()")));
        QVERIFY2(luaTrue(host, qsl("__starterUi.echoes == 0 and __starterUi.styles == 0")), "the tab was repainted on every refresh after being put right once");
    }

    // A game that sends chat over GMCP does not need the gates, but the next
    // connection might.
    void test_theChatLayerRetiresOnceGmcpChatAppears()
    {
        Host* host = startProfileWithStarterUi();
        QVERIFY(host);
        QVERIFY(luaTrue(host, qsl("BaseUI.chatTriggersArmed()")));

        QVERIFY(runLua(host,
                       qsl("gmcp = gmcp or {}\n"
                           "gmcp.Comm = { Channel = { Text = { channel = 'chat', text = 'hello there' } } }\n"
                           "BaseUI.addChatMessage()")));
        QVERIFY2(luaTrue(host, qsl("BaseUI.gmcpChat")), "a GMCP chat message did not retire the trigger layer");
        QVERIFY2(luaTrue(host, qsl("not BaseUI.chatTriggersArmed()")), "the chat gates stayed armed after GMCP chat arrived, so lines would be captured twice");

        QVERIFY(runLua(host, qsl("BaseUI.handleDisconnect()")));
        QVERIFY2(luaTrue(host, qsl("BaseUI.chatTriggersArmed()")), "the chat gates did not re-arm after a disconnect");

        QVERIFY(runLua(host, qsl("BaseUI.hide()")));
        QVERIFY2(luaTrue(host, qsl("not BaseUI.chatTriggersArmed()")), "\"baseui hide\" left the chat gates armed");
        QVERIFY(runLua(host, qsl("BaseUI.handleDisconnect()")));
        QVERIFY2(luaTrue(host, qsl("not BaseUI.chatTriggersArmed()")), "a disconnect re-armed the chat gates of a hidden UI");

        QVERIFY(runLua(host, qsl("BaseUI.show()")));
        QVERIFY2(luaTrue(host, qsl("BaseUI.chatTriggersArmed()")), "\"baseui show\" did not bring the chat gates back");
    }

    // Shipping the tree visible invites players to copy it, and the editor's
    // paste keeps every name. enableTrigger()/disableTrigger() can only name a
    // trigger, so a lifecycle built on a name a copy reproduces would reach
    // into the player's triggers - and their active copy would answer for ours
    // when we asked whether the layer was armed.
    void test_copyingTheTreeLeavesThePlayersTriggersAlone()
    {
        Host* host = startProfileWithStarterUi();
        QVERIFY(host);
        auto* tree = triggerTreeWidget(host);
        QVERIFY2(tree, "could not reach the editor's trigger tree");

        QVERIFY2(luaTrue(host, qsl("not BaseUI.chatTreeShared()")), "a fresh profile already has more than one chat capture tree");
        QVERIFY(runLua(host, qsl("BaseUI.disarmChatTriggers()")));
        QVERIFY2(luaTrue(host, qsl("not BaseUI.chatTriggersArmed()")), "the layer did not disarm on a profile with no copies");
        QVERIFY(runLua(host, qsl("BaseUI.armChatTriggers()")));
        QVERIFY(luaTrue(host, qsl("BaseUI.chatTriggersArmed()")));

        // the likely copy: one gate, to adapt for themselves
        pasteCopyOf(tree, qsl("BaseUI chat: tells"));
        QList<TTrigger*> gates;
        collectByName(host, qsl("BaseUI chat: tells"), gates);
        QCOMPARE(gates.size(), 2);
        QVERIFY(runLua(host, qsl("BaseUI.disarmChatTriggers()")));
        QVERIFY2(gates.at(1)->isActive(), "retiring the chat layer switched off the player's copy of a gate");
        QVERIFY2(luaTrue(host, qsl("not BaseUI.chatTriggersArmed()")), "the player's copy answered for ours when we asked whether the layer was armed");
        QVERIFY(runLua(host, qsl("BaseUI.armChatTriggers()")));

        // and the whole tree, name and all - now we cannot tell ours apart
        pasteCopyOf(tree, qsl("Mudlet base UI chat capture"));
        QVERIFY2(luaTrue(host, qsl("BaseUI.chatTreeShared()")), "a second tree with our folder's name went unnoticed");
        QList<TTrigger*> folders;
        collectByName(host, qsl("Mudlet base UI chat capture"), folders);
        QCOMPARE(folders.size(), 2);
        QVERIFY(runLua(host, qsl("BaseUI.disarmChatTriggers()")));
        QVERIFY2(folders.at(1)->isActive(), "retiring the chat layer switched off the player's copy of the whole tree");

        // giving up the switch is only safe because routeChatLine() re-checks
        QVERIFY(runLua(host, qsl("__starterUiCaptures = #BaseUI.recentCaptures\nBaseUI.gmcpChat = true")));
        feedLine(host, qsl("Ann whispers to you, 'psst'"));
        QVERIFY2(luaTrue(host, qsl("#BaseUI.recentCaptures == __starterUiCaptures")),
                 "with the switch given up, a chat line was still captured after GMCP chat took over - the per-line guard is what "
                 "keeps this correct and it did not hold");
    }

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

        // The next connection may have no protocol at all.
        QVERIFY(runLua(host, qsl("BaseUI.handleDisconnect()")));
        QVERIFY2(luaTrue(host, qsl("#BaseUI.vitalsTriggerIds == 1")), "the vitals layer did not re-arm after a disconnect");
    }

private:
    // Quotes and backslashes in a corpus line have to reach the Lua state
    // exactly as they were fed to the trigger engine.
    static QString luaLiteral(const QString& text) { return qsl("[==[%1]==]").arg(text); }

    // Drives the editor the way a player does: its copy/paste goes through the
    // same XML export/import as a package, so this covers both.
    QTreeWidget* triggerTreeWidget(Host* host)
    {
        dlgTriggerEditor* editor = host->mpEditorDialog;
        if (!editor) {
            return nullptr;
        }
        QMetaObject::invokeMethod(editor, "slot_showTriggers");
        QCoreApplication::processEvents();
        auto* tree = editor->findChild<QTreeWidget*>(qsl("treeWidget_triggers"));
        return (tree && tree->topLevelItemCount() > 0) ? tree : nullptr;
    }

    void pasteCopyOf(QTreeWidget* tree, const QString& name)
    {
        dlgTriggerEditor* editor = tree->window()->findChild<dlgTriggerEditor*>();
        editor = editor ? editor : qobject_cast<dlgTriggerEditor*>(tree->window());
        QVERIFY(editor);
        // the editor rebuilds the tree after a paste, so nothing may be cached
        QTreeWidgetItem* base = tree->topLevelItem(0);
        QVERIFY(base);
        QTreeWidgetItem* item = findItem(base, name);
        QVERIFY2(item, qPrintable(qsl("no editor tree item called %1").arg(name)));
        tree->setCurrentItem(item);
        QCoreApplication::processEvents();
        QMetaObject::invokeMethod(editor, "slot_copyXml");
        QCoreApplication::processEvents();
        tree->setCurrentItem(tree->topLevelItem(0));
        QCoreApplication::processEvents();
        QMetaObject::invokeMethod(editor, "slot_pasteXml");
        QCoreApplication::processEvents();
    }

    static QTreeWidgetItem* findItem(QTreeWidgetItem* parent, const QString& name)
    {
        if (!parent) {
            return nullptr;
        }
        if (parent->text(0) == name) {
            return parent;
        }
        for (int i = 0; i < parent->childCount(); ++i) {
            if (QTreeWidgetItem* found = findItem(parent->child(i), name)) {
                return found;
            }
        }
        return nullptr;
    }

    static void collectByName(Host* host, const QString& name, QList<TTrigger*>& found)
    {
        for (auto root : host->getTriggerUnit()->getTriggerRootNodeList()) {
            collectByNameIn(root, name, found);
        }
    }

    static void collectByNameIn(TTrigger* trigger, const QString& name, QList<TTrigger*>& found)
    {
        if (trigger->getName() == name) {
            found << trigger;
        }
        for (auto* childNode : *trigger->getChildrenList()) {
            auto* child = static_cast<TTrigger*>(childNode);
            collectByNameIn(child, name, found);
        }
    }

    static TTrigger* findTrigger(Host* host, const QString& name)
    {
        for (auto root : host->getTriggerUnit()->getTriggerRootNodeList()) {
            if (TTrigger* found = findTriggerIn(root, name)) {
                return found;
            }
        }
        return nullptr;
    }

    static TTrigger* findTriggerIn(TTrigger* trigger, const QString& name)
    {
        if (trigger->getName() == name) {
            return trigger;
        }
        for (auto* childNode : *trigger->getChildrenList()) {
            auto* child = static_cast<TTrigger*>(childNode);
            if (TTrigger* found = findTriggerIn(child, name)) {
                return found;
            }
        }
        return nullptr;
    }

    void assertCorpusCoversEveryGateLiteral(Host* host, const QList<QPair<QString, QString>>& corpus)
    {
        TTrigger* tree = findTrigger(host, qsl("Mudlet base UI chat capture"));
        QVERIFY(tree);
        for (auto* gateNode : *tree->getChildrenList()) {
            auto* gate = static_cast<TTrigger*>(gateNode);
            const QStringList patterns = gate->getPatternsList();
            const QList<int> kinds = gate->getRegexCodePropertyList();
            for (int i = 0; i < patterns.size() && i < kinds.size(); ++i) {
                const QString& literal = patterns.at(i);
                bool exercised = false;
                for (const auto& [text, family] : corpus) {
                    // matching the engine: substrings are indexOf, the rest startsWith
                    exercised = kinds.at(i) == REGEX_SUBSTRING ? text.contains(literal, Qt::CaseSensitive) : text.startsWith(literal, Qt::CaseSensitive);
                    if (exercised) {
                        break;
                    }
                }
                QVERIFY2(exercised,
                         qPrintable(qsl("no corpus line exercises \"%1\" in gate \"%2\", so that literal could be misspelled and "
                                        "the chat it gates would silently stop appearing")
                                            .arg(literal, gate->getName())));
            }
        }
    }

    // Only a line matching two shapes reaches routeChatLine twice, and only then
    // does the lastChatLine dedup have anything to do.
    static QString routeChatLineCounterScript()
    {
        return qsl(R"LUA(
__starterUi = __starterUi or {}
__starterUi.routed = 0
local routeChatLine = BaseUI.routeChatLine
BaseUI.routeChatLine = function(...)
  __starterUi.routed = __starterUi.routed + 1
  return routeChatLine(...)
end
)LUA");
    }

    // Anything not going through :echo() or :setStyleSheet() - a rawEcho, a
    // decho, a bare setLabelStyleSheet - is invisible here, which is why the
    // assertions that matter read the label back from Qt as well. The wrappers
    // go on the label instances, shadowing what they take from Geyser's class
    // table, so the labels still paint exactly as they would have.
    static QString chatTabRepaintCounterScript()
    {
        return qsl(R"LUA(
__starterUi = __starterUi or {}
__starterUi.echoes, __starterUi.styles = 0, 0
for _, label in pairs(BaseUI.chatTabLabels) do
  local echo, setStyleSheet = label.echo, label.setStyleSheet
  label.echo = function(self, ...)
    __starterUi.echoes = __starterUi.echoes + 1
    return echo(self, ...)
  end
  label.setStyleSheet = function(self, ...)
    __starterUi.styles = __starterUi.styles + 1
    return setStyleSheet(self, ...)
  end
end
)LUA");
    }

    static QString treeShapeComparisonScript(const QStringList& treeShapes)
    {
        QStringList entries;
        for (const QString& shape : treeShapes) {
            entries << luaLiteral(shape);
        }
        return qsl(R"LUA(
local tree = { %1 }
local script = BaseUI.chatShapeRegexes()
__starterUi = __starterUi or {}
__starterUi.shapeMismatch = nil
for i = 1, math.max(#tree, #script) do
  if tree[i] ~= script[i] then
    __starterUi.shapeMismatch = string.format("shape %d: tree has %s, chatPatterns has %s",
      i, tostring(tree[i]), tostring(script[i]))
    echo("\n[ chat shape drift ] " .. __starterUi.shapeMismatch .. "\n")
    break
  end
end
)LUA")
                .arg(entries.join(qsl(", ")));
    }

    // Holds the corpus fed above against the shapes chatLikeLine() walks: every
    // shape needs a line, and every line needs a shape.
    static QString chatShapeCoverageScript(const QList<QPair<QString, QString>>& corpus)
    {
        QStringList entries;
        for (const auto& entry : corpus) {
            entries << luaLiteral(entry.first);
        }
        return qsl(R"LUA(
local corpus = { %1 }
__starterUi = { uncoveredShapes = 0, unroutedLines = 0 }

for _, regex in ipairs(BaseUI.chatShapeRegexes()) do
  local covered = false
  for _, text in ipairs(corpus) do
    if rex.find(text, regex) then
      covered = true
      break
    end
  end
  if not covered then
    __starterUi.uncoveredShapes = __starterUi.uncoveredShapes + 1
    echo("\n[ chat shape with no corpus line ] " .. regex .. "\n")
  end
end

for _, text in ipairs(corpus) do
  if not BaseUI.chatLikeLine(text) then
    __starterUi.unroutedLines = __starterUi.unroutedLines + 1
    echo("\n[ corpus line no chat shape recognises ] " .. text .. "\n")
  end
end
)LUA")
                .arg(entries.join(qsl(", ")));
    }

    // Runs inside the profile's Lua state, against the real
    // BaseUI.parseVitalsLine and BaseUI.vitalsPrefilter.
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

__starterUi = { misses = 0, readable = 0, shapeSeen = {} }
local reported = 0

for _, label in ipairs(labels) do
  for _, template in ipairs(templates) do
    for _, spelling in ipairs({ label, label:sub(1, 1):upper() .. label:sub(2), label:upper() }) do
      local line = template:gsub("@", spelling)
      local readings = BaseUI.parseVitalsLine(line)
      if #readings > 0 then
        __starterUi.readable = __starterUi.readable + 1
        for _, reading in ipairs(readings) do
          __starterUi.shapeSeen[reading.pattern] = true
        end
        -- rex.find: rex.match returns false for an unset capture group
        if not rex.find(line, BaseUI.vitalsPrefilter) then
          __starterUi.misses = __starterUi.misses + 1
          if reported < 5 then
            reported = reported + 1
            echo("\n[ prefilter MISS ] " .. line .. "\n")
          end
        end
      end
    end
  end
end

__starterUi.shapesFired = 0
for _ in pairs(__starterUi.shapeSeen) do
  __starterUi.shapesFired = __starterUi.shapesFired + 1
end
__starterUi.shapeCount = BaseUI.vitalsShapeCount()
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
        // Installed by hand only when the preinstall gate did not, so this test
        // says nothing about who counts as a new user.
        if (!host->mInstalledPackages.contains(qsl("mudlet-base-ui"))) {
            auto [installed, message] = host->installPackage(qsl(":/packages/mudlet-base-ui/mudlet-base-ui.mpackage"), enums::PackageModuleType::Package, true);
            if (!installed) {
                qWarning("%s", qPrintable(qsl("could not install the starter UI: %1").arg(message)));
                return nullptr;
            }
        }
        // A hidden or stood-aside setting would suppress every capture trigger.
        if (!luaTrue(host, qsl("type(BaseUI) == 'table' and not BaseUI.dormant()"))) {
            qWarning("the starter UI did not load, or loaded dormant");
            return nullptr;
        }
        return host;
    }

    // Mirrors the helper the other functional tests use.
    void startProfile()
    {
        const QString port = QString::number(mPort);
        Host* host = TestProfile::create(mHostname, mLocalhost, port);
        if (!host) {
            QFAIL("No active host available for the test.");
        }
        QSignalSpy connected(&(host->mTelnet), &cTelnet::signal_connected);
        if (!connected.wait(3000)) {
            QFAIL("Could not connect to the stub.");
        }
    }

    void feedLine(Host* host, const QString& text)
    {
        QByteArray data = text.toUtf8() + "\r\n";
        data.reserve(data.size() + 16);
        host->mTelnet.loopbackTest(data);
    }

    bool runLua(Host* host, const QString& script) { return host->getLuaInterpreter()->compileAndExecuteScript(script); }

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

#include "StarterUiTriggerCostTest.moc"
MUDLET_GROUPED_TEST_MAIN(StarterUiTriggerCostTest)
