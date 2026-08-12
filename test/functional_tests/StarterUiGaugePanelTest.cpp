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

// The starter UI's gauges can be dragged out of its dock and dropped over the
// game's text, which is remembered between sessions. This pins that the default
// layout is exactly what it was before they could move, and drives the drag
// through the same callbacks the mouse does.
//
// The self-test profile's Lua specs never run the starter UI's interface code
// (that profile has no base UI package), so nothing else covers the gauge
// panel: what a saved position does to the next session is only ever exercised
// here.

#include <QtTest/QtTest>

#include "Host.h"
#include "MudletInstanceCoordinator.h"
#include "TLuaInterpreter.h"
#include "TelnetServerStub.h"
#include "ctelnet.h"
#include "dlgConnectionProfiles.h"
#include "mudlet.h"

extern void qInitResources_mudlet();
extern void qInitResources_qm();
extern void qInitResources_additional_splash_screens();
extern void qInitResources_mudlet_fonts_common();
extern void qInitResources_mudlet_fonts_posix();
static void initializeQRCResources();

class StarterUiGaugePanelTest : public QObject
{
    Q_OBJECT

private:
    TelnetServerStub* mpServer = nullptr;
    const QString mHostname = qsl("Test-StarterUiGaugePanel");
    const QString mLocalhost = qsl("localhost");
    quint16 mPort = 0;

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

    // The bars moved into a panel of their own to be draggable as a group, so
    // the numbers a player sees have to come out where they always did: the
    // expectations here are the constraints the gauges themselves used to carry.
    void test_theBarsComeUpInTheDockWhereTheyAlwaysWere()
    {
        Host* host = startProfileWithGauges();
        QVERIFY(host);

        QVERIFY2(luaTrue(host, qsl("not BaseUI.gaugesFloating()")), "the gauges came up floating on a profile that never moved them");
        QVERIFY2(luaTrue(host, qsl("BaseUI.gaugePanel.container == BaseUI.container.Inside")), "the gauge panel is not inside the dock");
        QVERIFY2(luaTrue(host, qsl("BaseUI.gaugeBackdrop.hidden")), "the floating backdrop is drawn while the bars are docked");
        // the topmost of a gauge's three labels is the one the mouse reaches,
        // and every bar carries the handlers so the group can be grabbed anywhere
        QVERIFY2(luaTrue(host,
                         qsl("BaseUI.gauges.hp.text.clickCallback == 'BaseUI.gaugeDragStart'"
                             " and BaseUI.gauges.hp.text.moveCallback == 'BaseUI.gaugeDragMove'"
                             " and BaseUI.gauges.hp.text.releaseCallback == 'BaseUI.gaugeDragEnd'"
                             " and BaseUI.gauges.mp.text.clickCallback == 'BaseUI.gaugeDragStart'")),
                 "the bars are not wired to the drag handlers, so the mouse never reaches them");
        QVERIFY2(luaTrue(host, qsl("BaseUI.gaugeBackdrop.clickCallback == 'BaseUI.gaugeDragStart'")), "the gaps between floating bars are not draggable");

        QVERIFY(runLua(host,
                       qsl("local inside = BaseUI.container.Inside\n"
                           "__baseUi = { hpSlipY = math.abs(BaseUI.gauges.hp:get_y() - (inside:get_y() + 0.78 * inside:get_height())),\n"
                           "  hpSlipH = math.abs(BaseUI.gauges.hp:get_height() - 0.045 * inside:get_height()),\n"
                           "  mpSlipY = math.abs(BaseUI.gauges.mp:get_y() - (inside:get_y() + 0.835 * inside:get_height())),\n"
                           "  slipX = math.abs(BaseUI.gauges.hp:get_x() - inside:get_x()),\n"
                           "  slipW = math.abs(BaseUI.gauges.hp:get_width() - inside:get_width()) }")));
        QVERIFY2(luaTrue(host, qsl("__baseUi.hpSlipY <= 1 and __baseUi.hpSlipH <= 1 and __baseUi.mpSlipY <= 1")),
                 "a docked gauge no longer lands on the slot it used to, so the default interface has moved");
        QVERIFY(luaTrue(host, qsl("__baseUi.slipX <= 1 and __baseUi.slipW <= 1")));

        // two bars, so the panel is two slots tall (5.5% of the dock each, less
        // the 1% gap the last one does not have): a panel any taller is empty
        // backdrop once it floats
        QVERIFY(runLua(host, qsl("__baseUi.panelSlip = math.abs(BaseUI.gaugePanel:get_height() - 0.1 * BaseUI.container.Inside:get_height())")));
        QVERIFY2(luaTrue(host, qsl("__baseUi.panelSlip <= 1")), "the gauge panel is not the height of the bars it holds");
    }

    // The whole group is one drag target: pressing any bar and moving picks up
    // all of them, and only a real movement takes them out of the dock.
    void test_draggingABarPullsTheWholeGroupOutOfTheDock()
    {
        Host* host = startProfileWithGauges();
        QVERIFY(host);
        QVERIFY(runLua(host, qsl("__baseUi = { x = BaseUI.gaugePanel:get_x(), y = BaseUI.gaugePanel:get_y(), gap = BaseUI.gauges.mp:get_y() - BaseUI.gauges.hp:get_y() }")));

        QVERIFY(runLua(host, qsl("BaseUI.gaugeDragStart({ button = 'LeftButton', buttons = { 'LeftButton' }, globalX = 400, globalY = 300 })")));
        QVERIFY(runLua(host, qsl("BaseUI.gaugeDragMove({ button = 'NoButton', buttons = { 'LeftButton' }, globalX = 403, globalY = 302 })")));
        QVERIFY2(luaTrue(host, qsl("not BaseUI.gaugesFloating()")), "a three pixel wobble took the bars out of the dock, so a click on one moves them");

        QVERIFY(runLua(host, qsl("BaseUI.gaugeDragMove({ button = 'NoButton', buttons = { 'LeftButton' }, globalX = 300, globalY = 250 })")));
        QVERIFY2(luaTrue(host, qsl("BaseUI.gaugesFloating()")), "dragging a bar did not pull the gauges out of the dock");
        QVERIFY2(luaTrue(host, qsl("BaseUI.gaugePanel.container == Geyser")), "the gauges left the dock but are not in the main window");
        QVERIFY2(luaTrue(host, qsl("not BaseUI.gaugeBackdrop.hidden")), "the floating bars have no backdrop, so game text shows through them");

        QVERIFY(runLua(host,
                       qsl("__baseUi.slipX = math.abs(BaseUI.gaugePanel:get_x() - (__baseUi.x - 100))\n"
                           "__baseUi.slipY = math.abs(BaseUI.gaugePanel:get_y() - (__baseUi.y - 50))\n"
                           "__baseUi.slipGap = math.abs((BaseUI.gauges.mp:get_y() - BaseUI.gauges.hp:get_y()) - __baseUi.gap)")));
        QVERIFY2(luaTrue(host, qsl("__baseUi.slipX <= 1 and __baseUi.slipY <= 1")), "the panel did not follow the pointer by the distance it moved");
        QVERIFY2(luaTrue(host, qsl("__baseUi.slipGap <= 1")), "the bars lost their spacing on the way out of the dock");

        // getMousePosition() decides whether a drop landed on the dock, and the
        // test has no pointer to place - so this asserts what it reads rather
        // than pretending to drop there.
        QVERIFY2(luaTrue(host, qsl("not BaseUI.overDock(getMousePosition())")), "the test's pointer sits over the dock, so the drop below re-docks the bars");
        QVERIFY(runLua(host, qsl("BaseUI.gaugeDragEnd({ button = 'LeftButton', buttons = {}, globalX = 300, globalY = 250 })")));
        QVERIFY(luaTrue(host, qsl("BaseUI.gaugesFloating()")));

        QVERIFY(runLua(host,
                       qsl("__baseUi.saved = {}\n"
                           "table.load(getMudletHomeDir() .. '/base_ui_settings.lua', __baseUi.saved)\n"
                           "local winw, winh = getMainWindowSize()\n"
                           "__baseUi.savedSlip = __baseUi.saved.gaugePanel and\n"
                           "  math.abs(__baseUi.saved.gaugePanel.x * winw - BaseUI.gaugePanel:get_x())\n"
                           "  + math.abs(__baseUi.saved.gaugePanel.y * winh - BaseUI.gaugePanel:get_y())")));
        QVERIFY2(luaTrue(host, qsl("__baseUi.savedSlip ~= nil and __baseUi.savedSlip <= 2")), "where the bars were dropped did not reach the settings file, so the next session loses it");

        // a chat line builds the interface before any vitals arrive, so a
        // restored position can be applied to a panel holding nothing yet
        QVERIFY(runLua(host, qsl("__baseUi.slots = BaseUI.usedGaugeSlots\nBaseUI.usedGaugeSlots = 0\nBaseUI.placeGaugePanel()")));
        QVERIFY2(luaTrue(host, qsl("BaseUI.gaugeBackdrop.hidden")), "a floating panel with no bars in it paints an empty box over the game's text");
        QVERIFY(runLua(host, qsl("BaseUI.usedGaugeSlots = __baseUi.slots\nBaseUI.layoutGauges()")));
        QVERIFY(luaTrue(host, qsl("not BaseUI.gaugeBackdrop.hidden")));
    }

    // What a new session does with that file: the settings are read, and the
    // panel is placed where they say - which is all build() does at the end.
    // The height is not in the file, because a bar arriving later has to make
    // the panel taller; it comes from the bar count and the saved scale.
    void test_aSavedPositionIsRestoredOnTheNextSession()
    {
        Host* host = startProfileWithGauges();
        QVERIFY(host);

        QVERIFY(runLua(host,
                       qsl("table.save(getMudletHomeDir() .. '/base_ui_settings.lua',\n"
                           "  { gaugePanel = { x = 0.05, y = 0.8, width = 0.3, scale = 1 } })\n"
                           "BaseUI.loadSettings()\n"
                           "BaseUI.placeGaugePanel()\n"
                           "local winw, winh = getMainWindowSize()\n"
                           "__baseUi = { slip = math.abs(BaseUI.gaugePanel:get_x() - 0.05 * winw)\n"
                           "  + math.abs(BaseUI.gaugePanel:get_y() - 0.8 * winh)\n"
                           "  + math.abs(BaseUI.gaugePanel:get_width() - 0.3 * winw)\n"
                           "  + math.abs(BaseUI.gaugePanel:get_height() - 0.1 * winh) }")));
        QVERIFY2(luaTrue(host, qsl("BaseUI.gaugesFloating()")), "a saved position did not survive being read back");
        QVERIFY2(luaTrue(host, qsl("__baseUi.slip <= 4")), "the bars did not come back where they were left");
        QVERIFY2(luaTrue(host, qsl("BaseUI.gauges.hp:get_x() >= BaseUI.gaugePanel:get_x() and BaseUI.gauges.hp:get_x() <= BaseUI.gaugePanel:get_x() + BaseUI.gaugePanel:get_width()")),
                 "the bars stayed behind when the panel was restored");

        // a third bar turns up while they are floating
        QVERIFY(runLua(host, qsl("__baseUi.twoBars = BaseUI.gaugePanel:get_height()")));
        feedLine(host, qsl("Moves:    800/800"));
        QVERIFY2(luaTrue(host, qsl("BaseUI.gauges.mv ~= nil and BaseUI.gauges.mv.container == BaseUI.gaugePanel")), "a gauge created while the bars floated did not join them");
        QVERIFY(runLua(host, qsl("__baseUi.grew = BaseUI.gaugePanel:get_height() / __baseUi.twoBars")));
        QVERIFY2(luaTrue(host, qsl("math.abs(__baseUi.grew - 1.55) <= 0.03")), "the floating panel did not grow by one slot for the new bar");
    }

    // A position that cannot be read is not a position: Geyser raises on a size
    // it cannot parse, and that error lands inside build() - the player would
    // be left with no interface at all rather than with the bars in the dock.
    void test_aCorruptSavedPositionLeavesTheBarsInTheDock()
    {
        Host* host = startProfileWithGauges();
        QVERIFY(host);

        const QStringList nonsense = {
                qsl("'over there'"),
                qsl("{ x = 0.1 }"),
                qsl("{ x = 0.1, y = 0.1, width = 'wide' }"),
                qsl("{ x = 0.1, y = 0.1, width = 0.3, scale = 0 }"),
                qsl("{ x = 0.1, y = 0.1, width = 0.3, scale = -2 }"),
        };
        for (const QString& entry : nonsense) {
            QVERIFY(runLua(host,
                           qsl("table.save(getMudletHomeDir() .. '/base_ui_settings.lua', { gaugePanel = %1 })\n"
                               "BaseUI.loadSettings()\n"
                               "BaseUI.placeGaugePanel()")
                                   .arg(entry)));
            QVERIFY2(luaTrue(host, qsl("not BaseUI.gaugesFloating()")), qPrintable(qsl("a saved position of %1 was taken seriously").arg(entry)));
            QVERIFY2(luaTrue(host, qsl("BaseUI.gaugePanel.container == BaseUI.container.Inside")), qPrintable(qsl("the panel is not in the dock after reading %1").arg(entry)));
        }

        // table.save writes these as tokens Lua cannot read back, so they never
        // reach the file - only the reader itself can be held to them
        QVERIFY2(luaTrue(host, qsl("BaseUI.readGaugePlacement({ x = 0/0, y = 0.1, width = 0.3, scale = 1 }) == nil")), "a not-a-number position was accepted");
        QVERIFY2(luaTrue(host, qsl("BaseUI.readGaugePlacement({ x = 0.1, y = 0.1, width = 1/0, scale = 1 }) == nil")), "an infinite width was accepted");
        // 0 is a perfectly good coordinate, and the nan test must not catch it
        QVERIFY(luaTrue(host, qsl("BaseUI.readGaugePlacement({ x = 0, y = 0, width = 0.3 }) ~= nil")));
    }

    // A width or a corner from a bigger window, or from a hand-edited file, has
    // to end up somewhere the pointer can still reach.
    void test_aPositionOffTheEdgeIsPulledBackOnScreen()
    {
        Host* host = startProfileWithGauges();
        QVERIFY(host);

        QVERIFY(runLua(host,
                       qsl("table.save(getMudletHomeDir() .. '/base_ui_settings.lua',\n"
                           "  { gaugePanel = { x = 0.9, y = 0.95, width = 0.3, scale = 1 } })\n"
                           "BaseUI.loadSettings()\n"
                           "BaseUI.placeGaugePanel()\n"
                           "local winw, winh = getMainWindowSize()\n"
                           "__baseUi = { overRight = BaseUI.gaugePanel:get_x() + BaseUI.gaugePanel:get_width() - winw,\n"
                           "  overBottom = BaseUI.gaugePanel:get_y() + BaseUI.gaugePanel:get_height() - winh }")));
        QVERIFY(luaTrue(host, qsl("BaseUI.gaugesFloating()")));
        QVERIFY2(luaTrue(host, qsl("__baseUi.overRight <= 1")), "the panel hangs off the right of the window");
        QVERIFY2(luaTrue(host, qsl("__baseUi.overBottom <= 1")), "the panel hangs off the bottom of the window");
    }

    // The settings are read at script load, long before the game sends anything
    // to build the interface from, so "baseui dock" has to clear a saved
    // position whether or not there is a panel to move yet.
    void test_dockingBeforeTheInterfaceIsBuiltStillClearsThePosition()
    {
        Host* host = startProfileWithGauges();
        QVERIFY(host);

        QVERIFY(runLua(host,
                       qsl("BaseUI.settings.gaugePanel = BaseUI.readGaugePlacement({ x = 0.1, y = 0.1, width = 0.3 })\n"
                           "__baseUi = { panel = BaseUI.gaugePanel }\n"
                           "BaseUI.gaugePanel = nil\n"
                           "BaseUI.alias('dock')\n"
                           "BaseUI.gaugePanel = __baseUi.panel\n"
                           "__baseUi.saved = {}\n"
                           "table.load(getMudletHomeDir() .. '/base_ui_settings.lua', __baseUi.saved)")));
        QVERIFY2(luaTrue(host, qsl("not BaseUI.gaugesFloating()")), "\"baseui dock\" said the bars were back while the setting still floated them");
        QVERIFY2(luaTrue(host, qsl("__baseUi.saved.gaugePanel == nil")), "the position stayed in the settings file, so the next session floats the bars again");
    }

    // Two ways back into the dock: dropping the bars on it, and "baseui dock".
    void test_theBarsGoBackIntoTheDock()
    {
        Host* host = startProfileWithGauges();
        QVERIFY(host);
        QVERIFY(runLua(host, qsl("BaseUI.floatGauges({ x = 0.02, y = 0.85, width = 0.3, scale = 1 })")));
        QVERIFY(luaTrue(host, qsl("BaseUI.gaugesFloating()")));

        // what a drop over the dock is decided by
        QVERIFY2(luaTrue(host, qsl("BaseUI.overDock(BaseUI.container:get_x() + 5, BaseUI.container:get_y() + 5)")), "a drop just inside the dock does not count as one");
        QVERIFY2(luaTrue(host, qsl("not BaseUI.overDock(BaseUI.container:get_x() - 5, BaseUI.container:get_y() + 5)")), "a drop outside the dock counts as one, so the bars could never leave");
        // a minimized dock is its title bar: the bars would go into a collapsed
        // container and "baseui dock" could not fetch them back out
        QVERIFY(runLua(host, qsl("BaseUI.container.minimized = true")));
        QVERIFY2(luaTrue(host, qsl("not BaseUI.overDock(BaseUI.container:get_x() + 5, BaseUI.container:get_y() + 5)")), "a drop on a minimized dock was accepted, which loses the bars");
        QVERIFY(runLua(host, qsl("BaseUI.container.minimized = false")));

        QVERIFY(runLua(host, qsl("BaseUI.alias('dock')")));
        QVERIFY2(luaTrue(host, qsl("not BaseUI.gaugesFloating()")), "\"baseui dock\" did not put the bars back");
        QVERIFY(luaTrue(host, qsl("BaseUI.gaugePanel.container == BaseUI.container.Inside")));
        QVERIFY2(luaTrue(host, qsl("BaseUI.gaugeBackdrop.hidden")), "the floating backdrop stayed on after the bars went back into the dock");

        QVERIFY(runLua(host, qsl("__baseUi = { saved = {} }\ntable.load(getMudletHomeDir() .. '/base_ui_settings.lua', __baseUi.saved)")));
        QVERIFY2(luaTrue(host, qsl("__baseUi.saved.gaugePanel == nil")), "the settings file still holds a position, so the bars float again next session");
    }

    // Hiding the interface has to take gauges that are no longer the dock's
    // children with it, or "baseui hide" leaves bars behind over the game text.
    // Standing aside for a game's own interface hides just the same.
    void test_hidingTheInterfaceHidesFloatingBarsToo()
    {
        Host* host = startProfileWithGauges();
        QVERIFY(host);
        QVERIFY(runLua(host, qsl("BaseUI.floatGauges({ x = 0.02, y = 0.85, width = 0.3, scale = 1 })")));

        QVERIFY(runLua(host, qsl("BaseUI.hide()")));
        QVERIFY2(luaTrue(host, qsl("BaseUI.gaugePanel.hidden")), "\"baseui hide\" left the floating bars on screen");

        QVERIFY(runLua(host, qsl("BaseUI.show()")));
        QVERIFY2(luaTrue(host, qsl("not BaseUI.gaugePanel.hidden")), "\"baseui show\" did not bring the floating bars back");
        QVERIFY(luaTrue(host, qsl("BaseUI.gaugesFloating()")));

        QVERIFY(runLua(host, qsl("BaseUI.standAside(nil, 'some-game-gui')")));
        QVERIFY2(luaTrue(host, qsl("BaseUI.gaugePanel.hidden")), "standing aside for the game's own interface left the floating bars on screen");
    }

    // A save that fails is what loses a dropped position, so it has to say so.
    // table.save reports a path it cannot write by returning a message rather
    // than by raising, so the pcall around it never sees the failure by itself.
    void test_aFailedSaveIsNotSilent()
    {
        Host* host = startProfileWithGauges();
        QVERIFY(host);

        // a directory where the settings file goes, so io.open cannot open it
        QVERIFY(runLua(host,
                       qsl("local path = getMudletHomeDir() .. '/base_ui_settings.lua'\n"
                           "os.remove(path)\n"
                           "__baseUi = { madeDir = lfs.mkdir(path) }\n"
                           "local ok, _, message = pcall(table.save, path, { a = 1 })\n"
                           "__baseUi.pcallOk, __baseUi.message = ok, message")));
        QVERIFY2(luaTrue(host, qsl("__baseUi.madeDir")), "could not make the settings path unwritable, so this proves nothing");
        QVERIFY2(luaTrue(host, qsl("__baseUi.pcallOk and __baseUi.message ~= nil")), "table.save now raises on a path it cannot write, so saveSettings could go back to trusting pcall alone");

        QVERIFY(runLua(host, qsl("BaseUI.floatGauges({ x = 0.02, y = 0.85, width = 0.3, scale = 1 })")));
        QVERIFY2(luaTrue(host, qsl("BaseUI.warnedSaveFailed")), "a failed save said nothing, so a dropped position vanishes with no explanation");
    }

    // A bar hidden under the pointer takes Qt's mouse grab with it, so the
    // release never arrives: whatever hid it has to end the drag, or the next
    // click carries on from an anchor several seconds old.
    void test_hidingTheBarsMidDragEndsTheDrag()
    {
        Host* host = startProfileWithGauges();
        QVERIFY(host);

        QVERIFY(runLua(host, qsl("BaseUI.gaugeDragStart({ button = 'LeftButton', buttons = { 'LeftButton' }, globalX = 400, globalY = 300 })")));
        QVERIFY(luaTrue(host, qsl("BaseUI.gaugeDrag ~= nil")));
        QVERIFY(runLua(host, qsl("BaseUI.hide()")));
        QVERIFY2(luaTrue(host, qsl("BaseUI.gaugeDrag == nil")), "the interface was hidden mid-drag and the drag is still live");
    }

private:
    Host* startProfileWithGauges()
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
        // A score screen is read on first sight, so one line builds the dock
        // and the two gauges everything below measures.
        feedLine(host, qsl("Health:   3600/3600     Mana:     3400/3400"));
        if (!luaTrue(host, qsl("BaseUI.gaugePanel ~= nil and BaseUI.gauges.hp ~= nil and BaseUI.gauges.mp ~= nil"))) {
            qWarning("the starter UI did not build its gauges");
            return nullptr;
        }
        return host;
    }

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

    void feedLine(Host* host, const QString& text)
    {
        QByteArray data = text.toUtf8() + "\r\n";
        data.reserve(data.size() + 16);
        host->mTelnet.loopbackTest(data);
    }

    bool runLua(Host* host, const QString& script) { return host->getLuaInterpreter()->compileAndExecuteScript(script); }

    bool luaTrue(Host* host, const QString& expression)
    {
        if (!runLua(host, qsl("__baseUiProbe = not not (%1)").arg(expression))) {
            qWarning("%s", qPrintable(qsl("probe did not compile: %1").arg(expression)));
            return false;
        }
        const bool result = runLua(host, qsl("assert(__baseUiProbe)"));
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

#include "StarterUiGaugePanelTest.moc"
QTEST_MAIN(StarterUiGaugePanelTest)
