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

// Every section of the starter UI's dock - the map, the chat window and the
// health bars - sits in an adjustable container of its own that can be dragged
// out and dropped over the game's text. Geyser owns the dragging; what is pinned
// here is the glue around it: the header row a drag has to land on, the dock
// sharing itself out between the sections still in it, and the lifecycle of
// sections that are no longer the dock's children.
//
// The self-test profile's Lua specs never run the starter UI's interface code
// (that profile has no base UI package), so nothing else covers any of this.
//
// StarterUiGaugePanelTest covers the vitals section on its own, in the detail
// the bars need; this is about the three of them together.

#include <QFileInfo>
#include <QTemporaryDir>
#include <QtTest/QtTest>

#include "PortableModeTestHelper.h"
#include "ProfileTestHelper.h"
#include "Host.h"
#include "MudletInstanceCoordinator.h"
#include "TConsole.h"
#include "TLuaInterpreter.h"
#include "TMainConsole.h"
#include "TelnetServerStub.h"
#include "ctelnet.h"
#include "dlgConnectionProfiles.h"
#include "dlgMapper.h"
#include "mudlet.h"

#include "GroupedTest.h"

class StarterUiSectionsTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;
    TelnetServerStub* mpServer = nullptr;
    const QString mHostname = qsl("Test-StarterUiSections");
    const QString mLocalhost = qsl("localhost");
    quint16 mPort = 0;

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

    // The bands the dock hands out are what a player sees before they touch
    // anything, so they are pinned to the pixel: a player who never drags a
    // section must never see one move.
    void test_theDockComesUpLaidOutTheWayItAlwaysWas()
    {
        Host* host = startProfileWithDock();
        QVERIFY(host);

        for (const QString& key : {qsl("map"), qsl("chat"), qsl("vitals")}) {
            QVERIFY2(luaTrue(host, qsl("BaseUI.sections.%1 ~= nil").arg(key)), qPrintable(qsl("there is no %1 section at all").arg(key)));
            QVERIFY2(luaTrue(host, qsl("not BaseUI.sectionFloating('%1')").arg(key)), qPrintable(qsl("the %1 section came up floating on a profile that never moved it").arg(key)));
            QVERIFY2(luaTrue(host, qsl("BaseUI.sections.%1.container == BaseUI.container.Inside").arg(key)), qPrintable(qsl("the %1 section is not inside the dock").arg(key)));
            QVERIFY2(luaTrue(host, qsl("BaseUI.sections.%1.dragOut == true").arg(key)), qPrintable(qsl("the %1 section cannot be dragged out of the dock at all").arg(key)));
        }

        QVERIFY(runLua(host, measureSections()));
        // the bands a dock with all three sections in it hands out
        QVERIFY2(luaTrue(host, qsl("math.abs(__ui.map.top) <= 1 and math.abs(__ui.map.bottom - 37) <= 1")), "the map section no longer occupies the top 37% of the dock");
        QVERIFY2(luaTrue(host, qsl("math.abs(__ui.chat.top - 38.5) <= 1 and math.abs(__ui.chat.bottom - 76) <= 1")), "the chat section no longer occupies 38.5% to 76% of the dock");
        // the vitals panel's header row hangs up into the gap above it, so its
        // bars start at 78% of the dock rather than a header row lower down
        QVERIFY(runLua(host,
                       qsl("local inside = BaseUI.container.Inside\n"
                           "__ui.barsTop = math.abs(BaseUI.gauges.hp:get_y() - (inside:get_y() + 0.78 * inside:get_height()))")));
        QVERIFY2(luaTrue(host, qsl("__ui.barsTop <= 1")), "the bars no longer start at 78% of the dock, so the default interface has moved");

        QVERIFY(runLua(host,
                       qsl("__ui.mapFills = math.abs(BaseUI.mapPlaceholder:get_y() - (BaseUI.sections.map:get_y() + 20))\n"
                           "__ui.chatFills = math.abs(BaseUI.chatTabLabels.all:get_y() - (BaseUI.sections.chat:get_y() + 20))")));
        QVERIFY2(luaTrue(host, qsl("__ui.mapFills <= 1 and __ui.chatFills <= 1")), "a section's contents do not start one header row down");
    }

    // A section can only be dragged by the strip of its background label its
    // contents leave clear, and Geyser resizes rather than moves on the edges of
    // that strip. What is left is what a player has to hit, so it is measured
    // rather than assumed: hovering tells the label which it would be.
    void test_everySectionHasAHeaderRowToGrabItBy()
    {
        Host* host = startProfileWithDock();
        QVERIFY(host);

        for (const QString& key : {qsl("map"), qsl("chat"), qsl("vitals")}) {
            // Only the header row is scanned: below it the section's contents
            // cover the label, so Qt would never deliver those points to it even
            // though Geyser's own arithmetic would call them a move.
            QVERIFY(runLua(host,
                           qsl("local section = BaseUI.sections.%1\n"
                               "local label = section.adjLabel\n"
                               "-- what Qt sends the label on a hover: where in it the pointer is\n"
                               "local function cursorAt(x, y)\n"
                               "  Adjustable.Container.onMove(section, label, { button = 'NoButton', x = x, y = y })\n"
                               "  return label.cursorShape\n"
                               "end\n"
                               "local header = section.Inside:get_y() - section:get_y()\n"
                               "local middle = math.floor(label:get_width() / 2)\n"
                               "__ui = { header = header, width = label:get_width(), rows = 0, columns = 0, first = -1 }\n"
                               "for y = 0, header - 1 do\n"
                               "  if cursorAt(middle, y) == 'OpenHand' then\n"
                               "    if __ui.first < 0 then __ui.first = y end\n"
                               "    __ui.rows = __ui.rows + 1\n"
                               "  end\n"
                               "end\n"
                               "for x = 0, __ui.width - 1 do\n"
                               "  if cursorAt(x, header - 1) == 'OpenHand' then __ui.columns = __ui.columns + 1 end\n"
                               "end\n"
                               "__ui.topEdge = cursorAt(middle, 0)\n"
                               "__ui.leftEdge = cursorAt(0, header - 1)")
                                   .arg(key)));
            QVERIFY(runLua(
                    host,
                    qsl("__uiReport = string.format('%s: header %d px, label %s wide, grabbable %d x %d px from row %d', '%1', __ui.header, tostring(__ui.width), __ui.columns, __ui.rows, __ui.first)")
                            .arg(key)));
            qDebug("%s", qPrintable(luaString(host, qsl("__uiReport"))));
            QVERIFY2(luaTrue(host, qsl("__ui.header == 20")), qPrintable(qsl("the %1 section's contents do not leave a 20 pixel header row").arg(key)));
            // The probe reads a cursor the section also sets itself, so without
            // these two the whole scan could be answering OpenHand for reasons
            // of its own and every count below would still pass.
            QVERIFY2(luaTrue(host, qsl("__ui.topEdge == 'ResizeVertical' and __ui.leftEdge == 'ResizeHorizontal'")),
                     qPrintable(qsl("hovering the %1 section's resize edges does not say so, so the scan proves nothing").arg(key)));
            // Geyser's own resize handle takes the top four rows, the leftmost
            // eleven pixels and the rightmost ten, so these are exact rather
            // than a floor: anything else means the strip moved
            QVERIFY2(luaTrue(host, qsl("__ui.rows == __ui.header - 4 and __ui.first == 4")), qPrintable(qsl("the %1 section's header row is not grabbable from its fifth pixel row down").arg(key)));
            // to within a pixel, since a label's width is not a whole number of them
            QVERIFY2(luaTrue(host, qsl("math.abs(__ui.columns - (__ui.width - 21)) <= 1")),
                     qPrintable(qsl("the %1 section's header row is grabbable across less of its width than it looks").arg(key)));
        }

        // the line under a header row must not take any of that strip back
        for (const QString& key : {qsl("map"), qsl("chat"), qsl("vitals")}) {
            QVERIFY2(luaTrue(host, qsl("BaseUI.sections.%1.windowList[BaseUI.sections.%1.name .. '_rule'] ~= nil").arg(key)),
                     qPrintable(qsl("the %1 section has no line closing off its header row").arg(key)));
            QVERIFY2(luaTrue(host, qsl("BaseUI.sections.%1.windowList[BaseUI.sections.%1.name .. '_rule'].clickthrough == true").arg(key)),
                     qPrintable(qsl("the line under the %1 section's header row swallows clicks meant for the row").arg(key)));
        }
    }

    // The point of the whole thing: a section that leaves gives its band of the
    // dock to the sections still in it.
    // The first drag is finished by the event Geyser raises on the drop rather
    // than by calling the layout by hand, since that handler is the only thing
    // that reflows the dock for a real player.
    void test_aSectionOutOfTheDockLeavesItsSpaceToTheOthers()
    {
        Host* host = startProfileWithDock();
        QVERIFY(host);
        QVERIFY(runLua(host, measureSections() + qsl("\n__docked = __ui")));

        // the chat window goes first: the map has the whole of its space to grow into
        QVERIFY(runLua(host,
                       qsl("local chat = BaseUI.sections.chat\n"
                           "chat:dragOutOfParent(-200, 120)\n"
                           "raiseEvent('AdjustableContainerRepositionFinish', chat.name, chat:get_width(), chat:get_height(), chat:get_x(), chat:get_y())")
                               + qsl("\n") + measureSections()));
        QVERIFY2(luaTrue(host, qsl("BaseUI.sectionFloating('chat')")), "the chat section did not come out of the dock");
        QVERIFY2(luaTrue(host, qsl("__ui.map.bottom > __docked.map.bottom + 10")), "the map did not grow into the space the chat window left");
        QVERIFY2(luaTrue(host, qsl("math.abs(__ui.map.bottom - 76.5) <= 1")), "the map grew into the chat window's space by the wrong amount");
        QVERIFY2(luaTrue(host, qsl("math.abs(__ui.vitals.top - __docked.vitals.top) <= 1")), "the bars moved when the chat window left, which they had no space to gain");

        // everything back, then only the bars leave: the map and the chat window
        // have the whole dock between them
        QVERIFY(runLua(host, qsl("BaseUI.alias('dock')\nBaseUI.sections.vitals:dragOutOfParent(-200, 260)\nBaseUI.layoutDock()") + qsl("\n") + measureSections()));
        QVERIFY2(luaTrue(host, qsl("not BaseUI.sectionFloating('chat') and BaseUI.sectionFloating('vitals')")), "the sections are not in the state this measures");
        QVERIFY2(luaTrue(host, qsl("__ui.map.bottom > __docked.map.bottom + 5 and __ui.chat.bottom > __docked.chat.bottom + 5")),
                 "neither the map nor the chat window grew into the space the bars left");
        QVERIFY2(luaTrue(host, qsl("math.abs(__ui.chat.bottom - 98) <= 1")), "the sections did not fill the dock once the bars had left it");
        // in the ratio they have when everything is docked
        QVERIFY(runLua(host,
                       qsl("__ui.ratio = (__ui.map.bottom - __ui.map.top) / (__ui.chat.bottom - __ui.chat.top)\n"
                           "__ui.dockedRatio = (__docked.map.bottom - __docked.map.top) / (__docked.chat.bottom - __docked.chat.top)")));
        QVERIFY2(luaTrue(host, qsl("math.abs(__ui.ratio - __ui.dockedRatio) <= 0.01")), "the map and the chat window did not share the space the bars left in the ratio they had");

        // with only the bars left there is nothing to expand into, so they sit at
        // the top of the dock rather than at 78% of it
        QVERIFY(runLua(host,
                       qsl("BaseUI.alias('dock')\n"
                           "BaseUI.sections.map:dragOutOfParent(-200, 20)\n"
                           "BaseUI.sections.chat:dragOutOfParent(-200, 140)\n"
                           "BaseUI.layoutDock()")
                               + qsl("\n") + measureSections()));
        QVERIFY2(luaTrue(host, qsl("math.abs(__ui.vitals.top) <= 1")), "the bars were left alone at the bottom of an otherwise empty dock");
    }

    // The dock stacks its sections rather than letting them be arranged inside
    // it, so a drag that never left it has to end with the section back in its
    // own band - which is the same handler that does the reclaiming.
    void test_aDragThatNeverLeftTheDockPutsTheSectionBackInItsBand()
    {
        Host* host = startProfileWithDock();
        QVERIFY(host);
        QVERIFY(runLua(host, measureSections() + qsl("\n__docked = __ui")));

        QVERIFY(runLua(host,
                       qsl("local map = BaseUI.sections.map\n"
                           "map:move('0%', '50%')\n"
                           "__ui = { shoved = math.abs((map:get_y() - BaseUI.container.Inside:get_y()) - 0.5 * BaseUI.container.Inside:get_height()) }")));
        QVERIFY2(luaTrue(host, qsl("__ui.shoved <= 1")), "the map section did not move at all, so this proves nothing");

        QVERIFY(runLua(host,
                       qsl("local map = BaseUI.sections.map\n"
                           "raiseEvent('AdjustableContainerRepositionFinish', map.name, map:get_width(), map:get_height(), map:get_x(), map:get_y())")
                               + qsl("\n") + measureSections()));
        QVERIFY2(luaTrue(host, qsl("not BaseUI.sectionFloating('map')")), "shoving a docked section about took it out of the dock");
        QVERIFY2(luaTrue(host, qsl("math.abs(__ui.map.top - __docked.map.top) <= 1 and math.abs(__ui.map.bottom - __docked.map.bottom) <= 1")),
                 "a docked section shoved about was left where the drag put it rather than back in its band");
    }

    // Sections have to come back to a dock laid out exactly as they left it, or
    // dragging one out and back would walk the interface out of shape.
    void test_puttingASectionBackRestoresTheSharedLayout()
    {
        Host* host = startProfileWithDock();
        QVERIFY(host);
        QVERIFY(runLua(host, measureSections() + qsl("\n__docked = __ui")));

        QVERIFY(runLua(host,
                       qsl("BaseUI.sections.map:dragOutOfParent(-200, 20)\n"
                           "BaseUI.sections.chat:dragOutOfParent(-260, 140)\n"
                           "BaseUI.sections.vitals:dragOutOfParent(-320, 260)\n"
                           "BaseUI.layoutDock()")));
        QVERIFY2(luaTrue(host, qsl("BaseUI.sectionFloating('map') and BaseUI.sectionFloating('chat') and BaseUI.sectionFloating('vitals')")), "not every section came out of the dock");

        QVERIFY(runLua(host, qsl("__ui = { docked = BaseUI.dockAll() }")));
        QVERIFY2(luaTrue(host, qsl("__ui.docked == 3")), "\"baseui dock\" did not put all three sections back");
        QVERIFY(runLua(host, measureSections()));
        QVERIFY2(luaTrue(host,
                         qsl("math.abs(__ui.map.top - __docked.map.top) <= 1 and math.abs(__ui.map.bottom - __docked.map.bottom) <= 1"
                             " and math.abs(__ui.chat.top - __docked.chat.top) <= 1 and math.abs(__ui.chat.bottom - __docked.chat.bottom) <= 1"
                             " and math.abs(__ui.vitals.top - __docked.vitals.top) <= 1 and math.abs(__ui.vitals.bottom - __docked.vitals.bottom) <= 1")),
                 "the dock did not come back to the layout it had before its sections were dragged out");
        QVERIFY(runLua(host,
                       qsl("local inside = BaseUI.container.Inside\n"
                           "__ui.barsTop = math.abs(BaseUI.gauges.hp:get_y() - (inside:get_y() + 0.78 * inside:get_height()))")));
        QVERIFY2(luaTrue(host, qsl("__ui.barsTop <= 1")), "the bars came back to the dock but not into their slots");
    }

    // A section in the dock is drawn over the dock's own background, so it has
    // none of its own. Out of the dock there is nothing behind it but the
    // game's text, and a chat window or a set of bars read against that is the
    // most visible thing this feature could get wrong.
    void test_aSectionOutOfTheDockGetsABackgroundToBeReadAgainst()
    {
        Host* host = startProfileWithDock();
        QVERIFY(host);
        QVERIFY(runLua(host, qsl("__ui = { docked = BaseUI.sections.chat.adjLabelstyle, tip = BaseUI.sections.chat.adjLabel.toolTip }")));
        QVERIFY2(luaTrue(host, qsl("__ui.docked:find('transparent', 1, true) ~= nil")), "a docked section paints a background over the dock's own");

        QVERIFY(runLua(host,
                       qsl("BaseUI.sections.chat:dragOutOfParent(-200, 120)\nBaseUI.layoutDock()\n"
                           "__ui.floating = BaseUI.sections.chat.adjLabelstyle\n"
                           "__ui.floatingTip = BaseUI.sections.chat.adjLabel.toolTip")));
        QVERIFY2(luaTrue(host, qsl("__ui.floating ~= __ui.docked")), "a section out of the dock is styled exactly as one in it");
        QVERIFY2(luaTrue(host, qsl("__ui.floating:find('transparent', 1, true) == nil and __ui.floating:find('background-color', 1, true) ~= nil")),
                 "a section out of the dock has no background of its own, so it is read against the game's text");
        // and the tooltip says how to get it back, since a drag cannot
        QVERIFY2(luaTrue(host, qsl("__ui.floatingTip ~= __ui.tip and __ui.floatingTip:find('dock', 1, true) ~= nil")), "a floating section does not say how to put it back");

        QVERIFY(runLua(host, qsl("BaseUI.alias('dock')\n__ui.back = BaseUI.sections.chat.adjLabelstyle")));
        QVERIFY2(luaTrue(host, qsl("__ui.back == __ui.docked")), "a section put back in the dock kept its floating background");
    }

    // A game that sends chat but nothing the gauges can read builds the dock
    // with a vitals section that has no bars to show. That panel has a negative
    // natural height, which Geyser reads as one measured off the far edge.
    void test_aDockWithNoBarsYetKeepsItsOtherSectionsWhereTheyBelong()
    {
        startProfile();
        Host* host = mudlet::self()->getActiveHost();
        QVERIFY(host);
        host->mEchoLuaErrors = true;
        if (!host->mInstalledPackages.contains(qsl("mudlet-base-ui"))) {
            auto [installed, message] = host->installPackage(qsl(":/packages/mudlet-base-ui/mudlet-base-ui.mpackage"), enums::PackageModuleType::Package, true);
            QVERIFY2(installed, qPrintable(message));
        }
        // a chat line builds the whole dock without any vitals data behind it
        feedLine(host, qsl("Ithilwen tells you: no numbers for you"));
        QVERIFY2(luaTrue(host, qsl("BaseUI.container ~= nil and BaseUI.usedGaugeSlots == 0")), "this profile was expected to build a dock with no bars in it");

        QVERIFY2(luaTrue(host, qsl("BaseUI.sections.vitals.hidden")), "an empty vitals panel is on screen with nothing in it");
        QVERIFY(runLua(host, measureSections()));
        QVERIFY2(luaTrue(host, qsl("math.abs(__ui.map.top) <= 1 and math.abs(__ui.map.bottom - 37) <= 1")), "a dock with no bars does not give the map its usual band");
        QVERIFY2(luaTrue(host, qsl("math.abs(__ui.chat.top - 38.5) <= 1 and math.abs(__ui.chat.bottom - 76) <= 1")), "a dock with no bars does not give the chat window its usual band");
        // a height read off the far edge would make the panel nearly as tall as
        // the whole dock rather than an empty header row
        QVERIFY(runLua(host, qsl("__ui.panel = BaseUI.sections.vitals:get_height()")));
        QVERIFY2(luaTrue(host, qsl("__ui.panel <= 21")), "the bar-less vitals panel is far taller than the header row it has to show");
    }

    // The tabs and their unread counts are the chat section's children and ride
    // out of the dock with it.
    void test_theChatWindowKeepsCountingWhileItFloats()
    {
        Host* host = startProfileWithDock();
        QVERIFY(host);
        QVERIFY(runLua(host, qsl("BaseUI.sections.chat:dragOutOfParent(-200, 120)\nBaseUI.layoutDock()")));
        QVERIFY2(luaTrue(host, qsl("BaseUI.sectionFloating('chat')")), "the chat section did not come out of the dock");
        QVERIFY2(luaTrue(host, qsl("BaseUI.chats.all.container == BaseUI.sections.chat.Inside and BaseUI.chatTabLabels.tells.container == BaseUI.sections.chat.Inside")),
                 "the chat window's consoles or tabs stayed behind in the dock");
        QVERIFY(runLua(host, qsl("__ui = { before = BaseUI.unread.tells }")));

        feedLine(host, qsl("Ithilwen tells you: the dock is lighter without me"));
        QVERIFY2(luaTrue(host, qsl("BaseUI.unread.tells == __ui.before + 1")), "a tell arriving while the chat window floated was not counted as unread");
        QVERIFY(runLua(host, qsl("__ui.badge = BaseUI.chatTabLabels.tells.message")));
        QVERIFY2(luaTrue(host, qsl("__ui.badge == 'Tells (1)'")), "the unread badge on a floating chat window does not show the count");

        QVERIFY(runLua(host, qsl("BaseUI.selectChatTab('tells')")));
        QVERIFY2(luaTrue(host, qsl("BaseUI.unread.tells == 0 and not BaseUI.chats.tells.hidden")), "the tabs of a floating chat window no longer switch");
    }

    // The map section holds a real mapper widget rather than Geyser primitives,
    // and a widget that does not survive being reparented would leave the player
    // with an empty box where their map was.
    void test_theMapperSurvivesBeingTakenOutOfTheDockAndBack()
    {
        Host* host = startProfileWithDock();
        QVERIFY(host);
        // the placeholder holds the section until the profile has a map at all
        QVERIFY2(luaTrue(host, qsl("BaseUI.map == nil and BaseUI.mapPlaceholder ~= nil")), "this profile was expected to start with no map data");
        QVERIFY(runLua(host, qsl("BaseUI.sections.map:dragOutOfParent(-200, 20)\nBaseUI.layoutDock()")));
        QVERIFY2(luaTrue(host, qsl("BaseUI.mapPlaceholder.container == BaseUI.sections.map.Inside")), "the placeholder stayed behind in the dock");
        QVERIFY(runLua(host, qsl("__ui = { placeholderX = math.abs(BaseUI.mapPlaceholder:get_x() - BaseUI.sections.map:get_x()) }")));
        QVERIFY2(luaTrue(host, qsl("__ui.placeholderX <= 2")), "the placeholder did not follow the map section out of the dock");

        // a room turns up, so the real mapper takes the placeholder's place, out
        // of the dock where the section is
        QVERIFY(runLua(host, qsl("addRoom(1)\nBaseUI.checkMapData()")));
        QVERIFY2(luaTrue(host, qsl("BaseUI.map ~= nil and BaseUI.map.container == BaseUI.sections.map.Inside")), "the mapper was not created inside the floating map section");
        QVERIFY(host->mpConsole);
        QVERIFY2(host->mpConsole->mpMapper, "no mapper widget was created at all");
        QVERIFY(runLua(host,
                       qsl("__ui.mapperX = math.abs(BaseUI.map:get_x() - BaseUI.sections.map:get_x())\n"
                           "__ui.mapperY = math.abs(BaseUI.map:get_y() - (BaseUI.sections.map:get_y() + 20))")));
        QVERIFY2(luaTrue(host, qsl("__ui.mapperX <= 2 and __ui.mapperY <= 2")), "the mapper did not land inside the floating map section");
        // The mapper paints its own background over the whole of the section it
        // is in, so the section has no hole in it for the game's text to show
        // through. One that stopped filling its section would leave exactly
        // that, and only its width was ever checked.
        QVERIFY(runLua(host,
                       qsl("local inside = BaseUI.sections.map.Inside\n"
                           "__ui.fillW = math.abs(BaseUI.map:get_width() - inside:get_width())\n"
                           "__ui.fillH = math.abs(BaseUI.map:get_height() - inside:get_height())")));
        QVERIFY2(luaTrue(host, qsl("__ui.fillW <= 2 and __ui.fillH <= 2")), "the mapper does not fill the floating map section, so the panel has a hole in it");
        // Geyser.Mapper re-runs createMapper() on every reposition, so the widget
        // has to still be there after the section has been moved about
        QVERIFY(runLua(host, qsl("BaseUI.sections.map:move('30%', '30%')")));
        QVERIFY2(host->mpConsole->mpMapper, "moving the floating map section destroyed the mapper widget");
        QVERIFY2(host->mpConsole->mpMapper->width() > 0 && host->mpConsole->mpMapper->height() > 0, "moving the floating map section shrank the mapper widget to nothing");

        QVERIFY(runLua(host, qsl("BaseUI.alias('dock')")));
        QVERIFY2(luaTrue(host, qsl("not BaseUI.sectionFloating('map')")), "the map section did not go back into the dock");
        QVERIFY2(host->mpConsole->mpMapper, "putting the map section back destroyed the mapper widget");
        QVERIFY2(host->mpConsole->mpMapper->width() > 0 && host->mpConsole->mpMapper->height() > 0, "the mapper widget shrank to nothing when the map section came back to the dock");
        QVERIFY(runLua(host,
                       qsl("__ui.backX = math.abs(BaseUI.map:get_x() - BaseUI.container.Inside:get_x())\n"
                           "__ui.backY = math.abs(BaseUI.map:get_y() - (BaseUI.container.Inside:get_y() + 20))\n"
                           "__ui.backW = math.abs(BaseUI.map:get_width() - BaseUI.container.Inside:get_width())")));
        QVERIFY2(luaTrue(host, qsl("__ui.backX <= 2 and __ui.backY <= 2 and __ui.backW <= 2")), "the mapper did not come back to the top of the dock");
    }

    // Every floating section is parented to the window rather than to the dock,
    // so hiding the dock leaves them all behind unless each is hidden in its
    // own right.
    void test_hidingTheInterfaceHidesEveryFloatingSection()
    {
        Host* host = startProfileWithDock();
        QVERIFY(host);
        // with a real mapper in it: hiding and showing a section runs the
        // mapper's own hide and show, which resize the widget to nothing and back
        QVERIFY(runLua(host, qsl("addRoom(1)\nBaseUI.checkMapData()")));
        QVERIFY2(luaTrue(host, qsl("BaseUI.map ~= nil")), "the mapper was not created, so the lifecycle below proves less than it should");
        QVERIFY(runLua(host,
                       qsl("BaseUI.sections.map:dragOutOfParent(-200, 20)\n"
                           "BaseUI.sections.chat:dragOutOfParent(-260, 140)\n"
                           "BaseUI.sections.vitals:dragOutOfParent(-320, 260)\n"
                           "BaseUI.layoutDock()")));

        QVERIFY(runLua(host, qsl("BaseUI.hide()")));
        QVERIFY2(luaTrue(host, qsl("BaseUI.sections.map.hidden and BaseUI.sections.chat.hidden and BaseUI.sections.vitals.hidden")), "\"baseui hide\" left floating sections on screen");

        QVERIFY(runLua(host, qsl("BaseUI.show()")));
        QVERIFY2(luaTrue(host, qsl("not BaseUI.sections.map.hidden and not BaseUI.sections.chat.hidden and not BaseUI.sections.vitals.hidden")),
                 "\"baseui show\" did not bring the floating sections back");
        QVERIFY2(luaTrue(host, qsl("BaseUI.sectionFloating('map') and BaseUI.sectionFloating('chat') and BaseUI.sectionFloating('vitals')")),
                 "showing the interface again put the floating sections back in the dock");
        QVERIFY(host->mpConsole->mpMapper);
        QVERIFY2(host->mpConsole->mpMapper->width() > 0 && host->mpConsole->mpMapper->height() > 0, "hiding and showing the interface left the mapper widget with no size");

        QVERIFY(runLua(host, qsl("BaseUI.standAside(nil, 'some-game-gui')")));
        QVERIFY2(luaTrue(host, qsl("BaseUI.sections.map.hidden and BaseUI.sections.chat.hidden and BaseUI.sections.vitals.hidden")),
                 "standing aside for the game's own interface left floating sections on screen");
    }

    // Bars keep arriving as a game says more about the character, and the panel
    // grows for each one. That must not shuffle the sections above it: a map
    // that jumped down the dock the first time a fourth stat turned up would
    // look like a bug whether it was one or not.
    void test_aNewBarGrowsTheVitalsPanelWithoutMovingTheOtherSections()
    {
        Host* host = startProfileWithDock();
        QVERIFY(host);
        QVERIFY(runLua(host, measureSections() + qsl("\n__before = __ui\n__before.bars = BaseUI.usedGaugeSlots")));

        feedLine(host, qsl("Moves:    800/800"));
        QVERIFY2(luaTrue(host, qsl("BaseUI.usedGaugeSlots == __before.bars + 1")), "the third bar was not created, so this proves nothing");
        QVERIFY(runLua(host, measureSections()));
        QVERIFY2(luaTrue(host,
                         qsl("math.abs(__ui.map.top - __before.map.top) <= 1 and math.abs(__ui.map.bottom - __before.map.bottom) <= 1"
                             " and math.abs(__ui.chat.top - __before.chat.top) <= 1 and math.abs(__ui.chat.bottom - __before.chat.bottom) <= 1")),
                 "a new bar moved the map or the chat window");
        QVERIFY2(luaTrue(host, qsl("math.abs(__ui.vitals.top - __before.vitals.top) <= 1")), "a new bar moved the top of the vitals panel");
        QVERIFY2(luaTrue(host, qsl("__ui.vitals.bottom > __before.vitals.bottom + 3")), "the vitals panel did not grow for the new bar");

        // and the same holds with a section out of the dock, where the sections
        // left behind have a share of it that the bars are no part of
        QVERIFY(runLua(host, qsl("BaseUI.sections.vitals:dragOutOfParent(-200, 260)\nBaseUI.layoutDock()") + qsl("\n") + measureSections() + qsl("\n__before = __ui")));
        feedLine(host, qsl("Xp: 40/100"));
        QVERIFY2(luaTrue(host, qsl("BaseUI.gauges.xp ~= nil")), "the fourth bar was not created, so this proves nothing");
        QVERIFY(runLua(host, measureSections()));
        QVERIFY2(luaTrue(host, qsl("math.abs(__ui.map.bottom - __before.map.bottom) <= 1 and math.abs(__ui.chat.bottom - __before.chat.bottom) <= 1")),
                 "a bar arriving in a floating vitals panel moved the sections still in the dock");
    }

    // A section is a window in its own right rather than a child widget of the
    // dock, so bringing the sections back is not the same as bringing the dock
    // back: miss the dock and they are left over the game's text with nothing
    // behind them.
    void test_theInterfaceComesBackWhenTheGamesOwnIsUninstalled()
    {
        Host* host = startProfileWithDock();
        QVERIFY(host);
        QVERIFY(runLua(host, qsl("BaseUI.sections.map:dragOutOfParent(-200, 20)\nBaseUI.layoutDock()")));
        QVERIFY(runLua(host, qsl("BaseUI.standAside(nil, 'some-game-gui')")));
        QVERIFY2(luaTrue(host, qsl("BaseUI.container.hidden and BaseUI.sections.map.hidden")), "standing aside did not hide the dock and its floating section");

        QVERIFY(runLua(host, qsl("BaseUI.serverGuiRemoved(nil, 'some-game-gui')")));
        // it deliberately waits before looking, so that a version upgrade which
        // uninstalls only to reinstall a moment later is not taken for a removal
        QElapsedTimer waited;
        waited.start();
        while (waited.elapsed() < 30000 && !luaQuiet(host, qsl("not BaseUI.settings.standingAside"))) {
            QTest::qWait(250);
        }
        QVERIFY2(luaTrue(host, qsl("not BaseUI.settings.standingAside")), "the starter UI did not come back on duty at all");
        QVERIFY2(luaTrue(host, qsl("not BaseUI.container.hidden")), "the dock stayed hidden while its sections came back over the game's text");
        QVERIFY2(luaTrue(host, qsl("not BaseUI.sections.map.hidden and BaseUI.sectionFloating('map')")), "the floating map section did not come back where it was left");
        QVERIFY2(luaTrue(host, qsl("not BaseUI.sections.chat.hidden and not BaseUI.sections.vitals.hidden")), "the docked sections did not come back");
    }

    // Where each section was left is Adjustable's own save file, one per section:
    // this is the round trip a restart makes, through the real files.
    void test_whereEachSectionWasLeftSurvivesARestart()
    {
        Host* host = startProfileWithDock();
        QVERIFY(host);

        QVERIFY(runLua(host,
                       qsl("BaseUI.sections.map:dragOutOfParent(-200, 20)\n"
                           "BaseUI.sections.map:move('12%', '61%')\n"
                           "BaseUI.sectionDropped(nil, BaseUI.sections.map.name)\n"
                           "__ui = { x = BaseUI.sections.map:get_x(), y = BaseUI.sections.map:get_y(),\n"
                           "  chatBottom = BaseUI.sections.chat:get_y() + BaseUI.sections.chat:get_height() }")));

        // what the next session does: sections built into the dock, then loaded
        QVERIFY(runLua(host,
                       qsl("BaseUI.sections.map:changeContainer(BaseUI.container.Inside)\n"
                           "BaseUI.layoutDock()\n"
                           "for _, key in ipairs({ 'map', 'chat', 'vitals' }) do BaseUI.loadSection(key) end\n"
                           "BaseUI.layoutDock()\n"
                           "__ui.slip = math.abs(BaseUI.sections.map:get_x() - __ui.x) + math.abs(BaseUI.sections.map:get_y() - __ui.y)")));
        QVERIFY2(luaTrue(host, qsl("BaseUI.sectionFloating('map')")), "a section that was out of the dock came back docked");
        QVERIFY2(luaTrue(host, qsl("BaseUI.sections.map.container == Geyser")), "the restored section is not in the main window");
        QVERIFY2(luaTrue(host, qsl("__ui.slip <= 2")), "the map did not come back where it was left");
        QVERIFY2(luaTrue(host, qsl("not BaseUI.sectionFloating('chat') and not BaseUI.sectionFloating('vitals')")), "loading took sections out of the dock that were never dragged out");
        // and the dock is shared out for the sections it actually has
        QVERIFY(runLua(host, qsl("__ui.chatSlip = math.abs((BaseUI.sections.chat:get_y() + BaseUI.sections.chat:get_height()) - __ui.chatBottom)")));
        QVERIFY2(luaTrue(host, qsl("__ui.chatSlip <= 1")), "a restored session did not share the dock out the way the session that saved it had");
        QVERIFY2(luaTrue(host, qsl("BaseUI.sections.map.exitLabel.hidden and BaseUI.sections.map.minimizeLabel.hidden")), "loading put a section's buttons back, so it can be closed with no way back");
    }

    // A saved position that cannot be applied raises inside Geyser, and that
    // would land in the middle of building the interface: the player would be
    // left with no interface at all rather than with a section in the dock.
    void test_anUnreadableSavedPositionLeavesASectionInTheDock()
    {
        Host* host = startProfileWithDock();
        QVERIFY(host);

        for (const QString& key : {qsl("map"), qsl("chat"), qsl("vitals")}) {
            QVERIFY(runLua(host,
                           qsl("__ui = { path = getMudletHomeDir() .. '/AdjustableContainer/' .. BaseUI.sections.%1.name .. '.lua' }\n"
                               // draggedOut so the load takes the section out of the dock before it
                               // reaches the value it cannot apply - otherwise it never left and
                               // "back in the dock" would be true whatever the recovery did
                               "table.save(__ui.path, { draggedOut = true, x = 'over there', y = '0%', width = '50%', height = '20%' })\n"
                               "BaseUI.loadSection('%1')\n"
                               "BaseUI.layoutDock()\n"
                               "__ui.fileLeft = io.exists(__ui.path)")
                                   .arg(key)));
            QVERIFY2(luaTrue(host, qsl("not BaseUI.sectionFloating('%1')").arg(key)), qPrintable(qsl("a position that could not be applied left the %1 section out of the dock").arg(key)));
            QVERIFY2(luaTrue(host, qsl("BaseUI.sections.%1.container == BaseUI.container.Inside").arg(key)),
                     qPrintable(qsl("the %1 section is not back in the dock after an unreadable save").arg(key)));
            QVERIFY2(luaTrue(host, qsl("not __ui.fileLeft")), qPrintable(qsl("the %1 section's unreadable save file was kept, so it fails again every session").arg(key)));
        }

        // and the dock is laid out as if none of that had happened
        QVERIFY(runLua(host, measureSections()));
        QVERIFY2(luaTrue(host, qsl("math.abs(__ui.map.top) <= 1 and math.abs(__ui.map.bottom - 37) <= 1 and math.abs(__ui.chat.top - 38.5) <= 1")),
                 "the dock is not laid out the way it starts out after unreadable saves were dropped");
    }

    // A save file fails in two ways that look nothing alike from Lua: a value
    // Geyser cannot apply raises out of load(), while a file it cannot parse at
    // all only makes load() return false - and `not pcall(...)` reads that
    // second one as a success. Both have to land the section back in the dock.
    void test_aSaveFileThatWillNotParseAlsoLeavesASectionInTheDock()
    {
        Host* host = startProfileWithDock();
        QVERIFY(host);

        QVERIFY(runLua(host,
                       qsl("__ui = { path = getMudletHomeDir() .. '/AdjustableContainer/' .. BaseUI.sections.map.name .. '.lua' }\n"
                           "BaseUI.sections.map:dragOutOfParent(-200, 20)\n"
                           "BaseUI.sections.map:save()\n"
                           "local file = io.open(__ui.path, 'w')\n"
                           "file:write('this is not a lua chunk at all {{{')\n"
                           "file:close()\n"
                           "__ui.loadSaysFalse = select(2, pcall(BaseUI.sections.map.load, BaseUI.sections.map)) == false")));
        QVERIFY2(luaTrue(host, qsl("__ui.loadSaysFalse")), "Geyser's load() now raises on a file it cannot parse, so loadSection could go back to trusting pcall alone");

        // and again, since the probe above consumed the file Geyser deletes
        QVERIFY(runLua(host,
                       qsl("BaseUI.sections.map:dragOutOfParent(-200, 20)\n"
                           "BaseUI.sections.map:save()\n"
                           "local file = io.open(__ui.path, 'w')\n"
                           "file:write('this is not a lua chunk at all {{{')\n"
                           "file:close()\n"
                           "BaseUI.loadSection('map')\n"
                           "BaseUI.layoutDock()\n"
                           "__ui.fileLeft = io.exists(__ui.path)")));
        QVERIFY2(luaTrue(host, qsl("not BaseUI.sectionFloating('map')")), "a save file that would not parse left the map out of the dock");
        QVERIFY2(luaTrue(host, qsl("BaseUI.sections.map.container == BaseUI.container.Inside")), "the map section is not back in the dock");
        QVERIFY2(luaTrue(host, qsl("not __ui.fileLeft")), "the unparseable save file was kept, so it fails again every session");
    }

    // A save file naming a state the right-click menu no longer offers has to be
    // undone rather than merely not offered again: a locked container cannot be
    // dragged, and the item that would unlock it is one of the ones taken away.
    void test_aSectionSavedLockedDoesNotComeBackUndraggable()
    {
        Host* host = startProfileWithDock();
        QVERIFY(host);

        QVERIFY(runLua(host,
                       qsl("BaseUI.sections.chat:lockContainer()\n"
                           "BaseUI.sections.chat:save()\n"
                           "__ui = { lockedOnDisk = {} }\n"
                           "table.load(getMudletHomeDir() .. '/AdjustableContainer/' .. BaseUI.sections.chat.name .. '.lua', __ui.lockedOnDisk)")));
        QVERIFY2(luaTrue(host, qsl("__ui.lockedOnDisk.locked == true")), "a locked container is no longer saved as locked, so this proves nothing");

        QVERIFY(runLua(host, qsl("BaseUI.loadSection('chat')\nBaseUI.layoutDock()")));
        QVERIFY2(luaTrue(host, qsl("not BaseUI.sections.chat.locked")), "the chat section came back locked, so it can never be dragged again");
        QVERIFY2(luaTrue(host, qsl("BaseUI.sections.chat.exitLabel.hidden and BaseUI.sections.chat.minimizeLabel.hidden")), "unlocking the section put its close and minimise buttons back");
        QVERIFY(runLua(host, qsl("__ui.header = BaseUI.sections.chat.Inside:get_y() - BaseUI.sections.chat:get_y()")));
        QVERIFY2(luaTrue(host, qsl("__ui.header == 20")), "unlocking the section left its contents somewhere other than under its header row");
    }

    // Everything that could put a section somewhere with no obvious way back is
    // taken off it.
    void test_theChromeThatCouldWedgeASectionIsGone()
    {
        Host* host = startProfileWithDock();
        QVERIFY(host);

        for (const QString& key : {qsl("map"), qsl("chat"), qsl("vitals")}) {
            QVERIFY2(luaTrue(host, qsl("BaseUI.sections.%1.exitLabel.hidden and BaseUI.sections.%1.minimizeLabel.hidden").arg(key)),
                     qPrintable(qsl("the %1 section still has its close and minimise buttons, which hide it with no visible way back").arg(key)));
            QVERIFY(runLua(host,
                           qsl("__ui = { menu = {} }\n"
                               "for _, item in ipairs({ 'lockLabel', 'minLabel', 'saveLabel', 'loadLabel', 'lockStylesLabel' }) do\n"
                               "  local element = BaseUI.sections.%1.adjLabel:findMenuElement(item)\n"
                               "  __ui.menu[item] = element ~= nil and element.ignore == true\n"
                               "end\n"
                               "__ui.dockItem = BaseUI.sections.%1.adjLabel:findMenuElement('customItemsLabel.Back to the dock') ~= nil")
                                   .arg(key)));
            QVERIFY2(luaTrue(host,
                             qsl("__ui.menu.lockLabel and __ui.menu.minLabel and __ui.menu.saveLabel"
                                 " and __ui.menu.loadLabel and __ui.menu.lockStylesLabel")),
                     qPrintable(qsl("a right-click menu item that can wedge the %1 section is still there").arg(key)));
            QVERIFY2(luaTrue(host, qsl("__ui.dockItem")), qPrintable(qsl("the %1 section's right-click menu has no way back into the dock").arg(key)));
        }
    }

    // Uninstalling the package takes the alias, the chat trigger tree and the
    // script with it - and nothing else. Everything the script built is made at
    // runtime and belongs to the profile: the dock and its sections are Geyser
    // objects, the vitals prefilter is a temporary trigger and the handlers are
    // anonymous ones. Left behind, the interface sits on screen still repainting
    // from the game's text, and the "baseui hide" that could have dismissed it
    // went with the alias.
    void test_uninstallingThePackageTakesTheInterfaceWithIt()
    {
        Host* host = startProfileWithDock();
        QVERIFY(host);
        // A section out of the dock is no longer the dock's child, so nothing
        // done to the dock reaches it. A look with a tint of its own puts the
        // colour behind the game's own text in the package's hands as well.
        QVERIFY(runLua(host,
                       qsl("BaseUI.sections.chat:dragOutOfParent(-200, 120)\n"
                           "BaseUI.sectionDropped(nil, BaseUI.sections.chat.name)\n"
                           "BaseUI.setTheme('ember')\n"
                           "__ui = { dock = BaseUI.container.name, gauge = BaseUI.gauges.hp.name,\n"
                           "  trigger = tostring(BaseUI.vitalsTriggerIds[1]),\n"
                           "  settings = getMudletHomeDir() .. '/base_ui_settings.lua',\n"
                           "  savedSection = getMudletHomeDir() .. '/AdjustableContainer/' .. BaseUI.sections.chat.name .. '.lua',\n"
                           "  sections = {}, border = getBorderRight(), tint = select(1, getBackgroundColor('main')) }\n"
                           "for _, key in ipairs({ 'map', 'chat', 'vitals' }) do __ui.sections[key] = BaseUI.sections[key].name end")));
        QVERIFY2(luaTrue(host,
                         qsl("__ui.trigger ~= 'nil' and __ui.border > 0 and __ui.tint == 18"
                             " and io.exists(__ui.settings) and io.exists(__ui.savedSection)"
                             " and getLabelStyleSheet(__ui.gauge .. '_back') ~= nil")),
                 "the interface is not in the state this uninstalls, so it would prove nothing");

        QVERIFY2(host->uninstallPackage(qsl("mudlet-base-ui"), enums::PackageModuleType::Package), "the starter UI package would not uninstall");

        QVERIFY2(luaTrue(host, qsl("BaseUI == nil")), "the interface's state outlived the package that built it, so a reinstall starts on this session's leftovers");
        QVERIFY2(luaTrue(host, qsl("Adjustable.Container.all[__ui.dock] == nil and Geyser.windowList[__ui.dock] == nil")), "the dock is still there after the package that built it was uninstalled");
        QVERIFY2(luaTrue(host, qsl("getLabelStyleSheet(__ui.dock .. 'adjLabel') == nil")), "the dock's own window is still on screen");
        for (const QString& key : {qsl("map"), qsl("chat"), qsl("vitals")}) {
            QVERIFY2(luaTrue(host, qsl("Adjustable.Container.all[__ui.sections.%1] == nil and Geyser.windowList[__ui.sections.%1] == nil").arg(key)),
                     qPrintable(qsl("the %1 section outlived the uninstall").arg(key)));
            QVERIFY2(luaTrue(host, qsl("getLabelStyleSheet(__ui.sections.%1 .. 'adjLabel') == nil").arg(key)), qPrintable(qsl("the %1 section's window is still on screen").arg(key)));
        }
        QVERIFY2(luaTrue(host, qsl("getLabelStyleSheet(__ui.gauge .. '_back') == nil")), "the health bar is still on screen");
        // killTrigger says whether there was one to kill: a live prefilter here
        // is the whole of the "still repainting the gauges" half of the bug
        QVERIFY2(luaTrue(host, qsl("killTrigger(__ui.trigger) == false")), "the vitals trigger outlived the package and goes on reading the game's text");
        QVERIFY2(luaTrue(host, qsl("getBorderRight() == 0")), "the dock is gone but the screen space it was attached to was never given back");
        QVERIFY2(luaTrue(host, qsl("select(1, getBackgroundColor('main')) == 0")), "the look's tint was left behind with nothing to explain it");
        QVERIFY2(luaTrue(host, qsl("not io.exists(__ui.settings) and not io.exists(__ui.savedSection)")), "the interface's saved state was left on disk for a reinstall to pick up");

        // and the game goes on talking to a profile that no longer has any of it
        feedLine(host, qsl("Ithilwen tells you: still here?"));
        feedLine(host, qsl("Health:   321/400"));
        QVERIFY2(luaTrue(host, qsl("BaseUI == nil and getLabelStyleSheet(__ui.gauge .. '_back') == nil")), "game text after the uninstall brought part of the interface back");
    }

    // Nothing is built until the game sends something to show, so a player who
    // removes the package before that has no dock on screen - but the vitals
    // trigger and the event handlers are live from the first line, and a
    // teardown written around a dock that is not there would raise half way
    // through and leave them running.
    void test_uninstallingBeforeTheInterfaceWasBuiltIsAlsoClean()
    {
        startProfile();
        Host* host = mudlet::self()->getActiveHost();
        QVERIFY(host);
        host->mEchoLuaErrors = true;
        if (!host->mInstalledPackages.contains(qsl("mudlet-base-ui"))) {
            auto [installed, message] = host->installPackage(qsl(":/packages/mudlet-base-ui/mudlet-base-ui.mpackage"), enums::PackageModuleType::Package, true);
            QVERIFY2(installed, qPrintable(message));
        }
        QVERIFY(runLua(host, qsl("__ui = { trigger = tostring(BaseUI.vitalsTriggerIds[1]) }")));
        QVERIFY2(luaTrue(host, qsl("BaseUI.container == nil and __ui.trigger ~= 'nil'")), "this profile was expected to have the trigger layer up but no dock built yet");

        QVERIFY2(host->uninstallPackage(qsl("mudlet-base-ui"), enums::PackageModuleType::Package), "the starter UI package would not uninstall");
        QVERIFY2(luaTrue(host, qsl("BaseUI == nil")), "an uninstall before the interface was built left its state behind");
        QVERIFY2(luaTrue(host, qsl("killTrigger(__ui.trigger) == false")), "an uninstall before the interface was built left the vitals trigger reading the game's text");

        // and the line that would have built it builds nothing
        feedLine(host, qsl("Health:   3600/3600     Mana:     3400/3400"));
        QVERIFY2(luaTrue(host, qsl("BaseUI == nil")), "game text after the uninstall started the interface up again");
    }

    // The introduction is held back while the first-run tour is open, on a
    // one-shot handler registered long after the ones the script sets up. An
    // uninstall in that window has to take that one too, or closing the tour
    // calls into a BaseUI that is no longer there.
    void test_uninstallingWhileTheFirstRunTourIsOpenTakesItsHandlerToo()
    {
        startProfile();
        Host* host = mudlet::self()->getActiveHost();
        QVERIFY(host);
        host->mEchoLuaErrors = true;
        // what mudlet::loadProfile() sets before the default packages install
        QVERIFY(runLua(host, qsl("mudlet = mudlet or {}\nmudlet.uiTourPending = true")));
        if (!host->mInstalledPackages.contains(qsl("mudlet-base-ui"))) {
            auto [installed, message] = host->installPackage(qsl(":/packages/mudlet-base-ui/mudlet-base-ui.mpackage"), enums::PackageModuleType::Package, true);
            QVERIFY2(installed, qPrintable(message));
        }
        feedLine(host, qsl("Health:   3600/3600     Mana:     3400/3400"));
        // an interface that has not introduced itself is one that took the tour
        // branch, which is the only thing that registers that handler
        QVERIFY2(luaTrue(host, qsl("BaseUI.container ~= nil and not BaseUI.settings.announced")), "the dock was not built with its introduction held back for the tour, so there is no handler to leak");

        QVERIFY2(host->uninstallPackage(qsl("mudlet-base-ui"), enums::PackageModuleType::Package), "the starter UI package would not uninstall");
        QVERIFY2(luaTrue(host, qsl("BaseUI == nil")), "the uninstall left the interface's state behind");

        // a stand-in under the same name: a handler that outlived the package
        // calls announce on this rather than on a nil global, so what would have
        // been a Lua error is something this can assert on
        QVERIFY(runLua(host, qsl("__uiTourReached = false\nBaseUI = { announce = function() __uiTourReached = true end }")));
        QVERIFY(runLua(host, qsl("raiseEvent('sysUiTourFinished')")));
        QVERIFY(runLua(host, qsl("BaseUI = nil")));
        QVERIFY2(luaTrue(host, qsl("__uiTourReached == false")), "closing the first-run tour after the uninstall still called into the interface");
    }

private:
    // every section's band of the dock, in percentages of it, which is what the
    // layout is written in and what survives a window of any size
    QString measureSections() const
    {
        return qsl("local inside = BaseUI.container.Inside\n"
                   "__ui = {}\n"
                   "for _, key in ipairs({ 'map', 'chat', 'vitals' }) do\n"
                   "  local section = BaseUI.sections[key]\n"
                   "  __ui[key] = {\n"
                   "    top = 100 * (section:get_y() - inside:get_y()) / inside:get_height(),\n"
                   "    bottom = 100 * (section:get_y() + section:get_height() - inside:get_y()) / inside:get_height(),\n"
                   "  }\n"
                   "end");
    }

    Host* startProfileWithDock()
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
        // A score screen is read on first sight, so one line builds the dock and
        // the two gauges the vitals section is sized from.
        feedLine(host, qsl("Health:   3600/3600     Mana:     3400/3400"));
        if (!luaTrue(host, qsl("BaseUI.container ~= nil and BaseUI.sections.map ~= nil and BaseUI.gauges.hp ~= nil"))) {
            qWarning("the starter UI did not build its dock");
            return nullptr;
        }
        return host;
    }

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

    QString luaString(Host* host, const QString& variable)
    {
        if (!runLua(host, qsl("__uiText = tostring(%1)").arg(variable))) {
            return QString();
        }
        lua_State* L = host->getLuaInterpreter()->getLuaGlobalState();
        lua_getglobal(L, "__uiText");
        const QString text = lua_isstring(L, -1) ? QString::fromUtf8(lua_tostring(L, -1)) : QString();
        lua_pop(L, 1);
        return text;
    }

    // the same probe without the warning, for polling a condition that is
    // expected to be false until it is not
    bool luaQuiet(Host* host, const QString& expression) { return runLua(host, qsl("__uiProbe = not not (%1)").arg(expression)) && runLua(host, qsl("assert(__uiProbe)")); }

    bool luaTrue(Host* host, const QString& expression)
    {
        if (!runLua(host, qsl("__uiProbe = not not (%1)").arg(expression))) {
            qWarning("%s", qPrintable(qsl("probe did not compile: %1").arg(expression)));
            return false;
        }
        const bool result = runLua(host, qsl("assert(__uiProbe)"));
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

#include "StarterUiSectionsTest.moc"
MUDLET_GROUPED_TEST_MAIN(StarterUiSectionsTest)
