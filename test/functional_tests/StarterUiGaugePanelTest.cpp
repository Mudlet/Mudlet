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

// The starter UI's gauges sit in an adjustable container of their own, which can
// be dragged out of the dock and dropped over the game's text and remembers
// where it was left. Geyser owns the dragging itself; what is pinned here is the
// glue around it - the default layout, the panel fitting the bars in it, the
// chrome that is deliberately taken away, and the way back into the dock.
//
// The self-test profile's Lua specs never run the starter UI's interface code
// (that profile has no base UI package), so nothing else covers the gauge
// panel: what a saved position does to the next session is only ever exercised
// here.

#include <QFileInfo>
#include <QTemporaryDir>
#include <QtTest/QtTest>

#include "PortableModeTestHelper.h"
#include "ProfileTestHelper.h"
#include "Host.h"
#include "MudletInstanceCoordinator.h"
#include "TLuaInterpreter.h"
#include "TelnetServerStub.h"
#include "ctelnet.h"
#include "dlgConnectionProfiles.h"
#include "mudlet.h"

#include "GroupedTest.h"

class StarterUiGaugePanelTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;
    TelnetServerStub* mpServer = nullptr;
    const QString mHostname = qsl("Test-StarterUiGaugePanel");
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

    // The bars moved into a panel of their own to be draggable as a group, so
    // the numbers a player sees have to come out where they always did: the
    // expectations here are the constraints the gauges themselves used to carry.
    // The bars moved into a panel of their own to be draggable as a group, so
    // the numbers a player sees have to come out where they always did: the
    // expectations here are the constraints the gauges themselves used to carry.
    // The panel's title bar sits in the gap above them, so they do not move.
    void test_theBarsComeUpInTheDockWhereTheyAlwaysWere()
    {
        Host* host = startProfileWithGauges();
        QVERIFY(host);

        QVERIFY2(luaTrue(host, qsl("not BaseUI.sections.vitals.hidden and not BaseUI.sections.vitals.auto_hidden")), "the bars are in the dock but not on screen");
        QVERIFY2(luaTrue(host, qsl("not BaseUI.sectionFloating('vitals')")), "the gauges came up floating on a profile that never moved them");
        QVERIFY2(luaTrue(host, qsl("BaseUI.sections.vitals.container == BaseUI.container.Inside")), "the gauge panel is not inside the dock");
        QVERIFY2(luaTrue(host, qsl("BaseUI.gauges.hp.container == BaseUI.sections.vitals.Inside")), "the bars are not inside the gauge panel");
        QVERIFY2(luaTrue(host, qsl("BaseUI.sections.vitals.dragOut == true")), "the panel cannot be dragged out of the dock at all");

        QVERIFY(runLua(host,
                       qsl("local inside = BaseUI.container.Inside\n"
                           "__baseUi = { hpSlipY = math.abs(BaseUI.gauges.hp:get_y() - (inside:get_y() + 0.78 * inside:get_height())),\n"
                           "  hpSlipH = math.abs(BaseUI.gauges.hp:get_height() - 0.045 * inside:get_height()),\n"
                           "  mpSlipY = math.abs(BaseUI.gauges.mp:get_y() - (inside:get_y() + 0.835 * inside:get_height())),\n"
                           "  slipX = math.abs(BaseUI.gauges.hp:get_x() - inside:get_x()),\n"
                           "  slipW = math.abs(BaseUI.gauges.hp:get_width() - inside:get_width()) }")));
        QVERIFY2(luaTrue(host, qsl("__baseUi.hpSlipY <= 1 and __baseUi.hpSlipH <= 1 and __baseUi.mpSlipY <= 1")),
                 "a docked gauge no longer lands on the slot it used to, so the default interface has moved");
        QVERIFY2(luaTrue(host, qsl("__baseUi.slipX <= 1 and __baseUi.slipW <= 1")), "the bars no longer span the dock the way they used to");

        // two bars, so the panel is two slots tall (5.5% of the dock each, less
        // the 1% gap the last one does not have) plus its own title bar
        QVERIFY(runLua(host, qsl("__baseUi.panelSlip = math.abs(BaseUI.sections.vitals:get_height() - (0.1 * BaseUI.container.Inside:get_height() + 20))")));
        QVERIFY2(luaTrue(host, qsl("__baseUi.panelSlip <= 1")), "the gauge panel is not the height of the bars it holds plus its title bar");
    }

    // Everything that could put the bars somewhere with no obvious way back is
    // taken off the panel: closing it, collapsing it, locking it in place, and
    // loading a stale save over it.
    void test_theChromeThatCouldWedgeTheBarsIsGone()
    {
        Host* host = startProfileWithGauges();
        QVERIFY(host);

        QVERIFY2(luaTrue(host, qsl("BaseUI.sections.vitals.exitLabel.hidden and BaseUI.sections.vitals.minimizeLabel.hidden")),
                 "the panel still has its close and minimise buttons, which hide the bars with no visible way back");
        QVERIFY(runLua(host,
                       qsl("__baseUi = { menu = {} }\n"
                           "for _, item in ipairs({ 'lockLabel', 'minLabel', 'saveLabel', 'loadLabel', 'lockStylesLabel' }) do\n"
                           "  local element = BaseUI.sections.vitals.adjLabel:findMenuElement(item)\n"
                           "  __baseUi.menu[item] = element ~= nil and element.ignore == true\n"
                           "end\n"
                           "__baseUi.dockItem = BaseUI.sections.vitals.adjLabel:findMenuElement('customItemsLabel.Back to the dock') ~= nil")));
        QVERIFY2(luaTrue(host,
                         qsl("__baseUi.menu.lockLabel and __baseUi.menu.minLabel and __baseUi.menu.saveLabel"
                             " and __baseUi.menu.loadLabel and __baseUi.menu.lockStylesLabel")),
                 "a right-click menu item that can wedge the bars is still there");
        QVERIFY2(luaTrue(host, qsl("__baseUi.dockItem")), "the right-click menu has no way back into the dock");
    }

    // Geyser does the dragging; this is the glue reacting to it - the panel ends
    // up in the main window, keeps the bars, and knows it is out.
    void test_theBarsCanBeTakenOutOfTheDock()
    {
        Host* host = startProfileWithGauges();
        QVERIFY(host);
        QVERIFY(runLua(host, qsl("__baseUi = { height = BaseUI.sections.vitals:get_height(), gap = BaseUI.gauges.mp:get_y() - BaseUI.gauges.hp:get_y() }")));

        // what Adjustable.Container does once a drag leaves the dock behind
        QVERIFY(runLua(host, qsl("BaseUI.sections.vitals:dragOutOfParent(-200, 120)\nBaseUI.layoutDock()")));
        QVERIFY2(luaTrue(host, qsl("BaseUI.sectionFloating('vitals')")), "the panel did not come out of the dock");
        QVERIFY2(luaTrue(host, qsl("BaseUI.sections.vitals.container == Geyser")), "the panel left the dock but is not in the main window");
        QVERIFY2(luaTrue(host, qsl("BaseUI.gauges.hp.container == BaseUI.sections.vitals.Inside")), "the bars stayed behind in the dock");

        QVERIFY(runLua(host,
                       qsl("__baseUi.slipH = math.abs(BaseUI.sections.vitals:get_height() - __baseUi.height)\n"
                           "__baseUi.slipGap = math.abs((BaseUI.gauges.mp:get_y() - BaseUI.gauges.hp:get_y()) - __baseUi.gap)")));
        QVERIFY2(luaTrue(host, qsl("__baseUi.slipH <= 1")), "the panel changed height on its way out of the dock");
        QVERIFY2(luaTrue(host, qsl("__baseUi.slipGap <= 1")), "the bars lost their spacing on the way out of the dock");
        // dragOutOfParent keeps the panel inside the window it lands in
        QVERIFY(runLua(host,
                       qsl("local winw, winh = getMainWindowSize()\n"
                           "__baseUi.overRight = BaseUI.sections.vitals:get_x() + BaseUI.sections.vitals:get_width() - winw\n"
                           "__baseUi.offLeft = -BaseUI.sections.vitals:get_x()")));
        QVERIFY2(luaTrue(host, qsl("__baseUi.overRight <= 1 and __baseUi.offLeft <= 1")), "a drag that left the dock to the left put the panel off screen");

        // a third bar turns up while they are floating
        QVERIFY(runLua(host, qsl("__baseUi.twoBars = BaseUI.sections.vitals:get_height()")));
        feedLine(host, qsl("Moves:    800/800"));
        QVERIFY2(luaTrue(host, qsl("BaseUI.gauges.mv ~= nil and BaseUI.gauges.mv.container == BaseUI.sections.vitals.Inside")), "a gauge created while the bars floated did not join them");
        QVERIFY(runLua(host, qsl("__baseUi.grew = (BaseUI.sections.vitals:get_height() - 20) / (__baseUi.twoBars - 20)")));
        QVERIFY2(luaTrue(host, qsl("math.abs(__baseUi.grew - 1.55) <= 0.03")), "the floating panel did not grow by one slot for the new bar");
    }

    // Where the panel was left is Adjustable's own save file, not the package's
    // settings: this is the round trip a restart makes, through the real file.
    void test_whereTheBarsWereLeftSurvivesARestart()
    {
        Host* host = startProfileWithGauges();
        QVERIFY(host);

        QVERIFY(runLua(host,
                       qsl("BaseUI.sections.vitals:dragOutOfParent(-200, 120)\n"
                           "BaseUI.sections.vitals:move('12%', '61%')\n"
                           "BaseUI.sectionDropped(nil, BaseUI.sections.vitals.name)\n"
                           "__baseUi = { x = BaseUI.sections.vitals:get_x(), y = BaseUI.sections.vitals:get_y() }")));

        // what the next session does: a panel built into the dock, then loaded
        QVERIFY(runLua(host,
                       qsl("BaseUI.sections.vitals:changeContainer(BaseUI.container.Inside)\n"
                           "BaseUI.layoutDock()\n"
                           "BaseUI.loadSection('vitals')\n"
                           "BaseUI.layoutDock()\n"
                           "__baseUi.slip = math.abs(BaseUI.sections.vitals:get_x() - __baseUi.x) + math.abs(BaseUI.sections.vitals:get_y() - __baseUi.y)")));
        QVERIFY2(luaTrue(host, qsl("BaseUI.sectionFloating('vitals')")), "a panel that was out of the dock came back docked");
        QVERIFY2(luaTrue(host, qsl("BaseUI.sections.vitals.container == Geyser")), "the restored panel is not in the main window");
        QVERIFY2(luaTrue(host, qsl("__baseUi.slip <= 2")), "the bars did not come back where they were left");
        QVERIFY2(luaTrue(host, qsl("BaseUI.sections.vitals.exitLabel.hidden and BaseUI.sections.vitals.minimizeLabel.hidden")),
                 "loading put the panel's buttons back, so the bars can be closed with no way back");
    }

    // A saved position that cannot be applied raises inside Geyser, and that
    // would land in the middle of building the interface: the player would be
    // left with no interface at all rather than with the bars in the dock.
    void test_anUnreadableSavedPositionLeavesTheBarsInTheDock()
    {
        Host* host = startProfileWithGauges();
        QVERIFY(host);

        QVERIFY(runLua(host,
                       qsl("__baseUi = { path = getMudletHomeDir() .. '/AdjustableContainer/' .. BaseUI.sections.vitals.name .. '.lua' }\n"
                           // draggedOut so the load takes the panel out of the dock before it
                           // reaches the value it cannot apply - otherwise it never left and
                           // "back in the dock" would be true whatever the recovery did
                           "table.save(__baseUi.path, { draggedOut = true, x = 'over there', y = '0%', width = '50%', height = '20%' })\n"
                           "BaseUI.loadSection('vitals')\n"
                           "BaseUI.layoutDock()\n"
                           "__baseUi.fileLeft = io.exists(__baseUi.path)")));
        QVERIFY2(luaTrue(host, qsl("not BaseUI.sectionFloating('vitals')")), "a position that could not be applied left the bars out of the dock");
        QVERIFY2(luaTrue(host, qsl("BaseUI.sections.vitals.container == BaseUI.container.Inside")), "the panel is not back in the dock after an unreadable save");
        QVERIFY2(luaTrue(host, qsl("not __baseUi.fileLeft")), "the unreadable save file was kept, so it fails again every session");
        // and the bars are still where they belong
        QVERIFY(runLua(host,
                       qsl("local inside = BaseUI.container.Inside\n"
                           "__baseUi.slip = math.abs(BaseUI.gauges.hp:get_y() - (inside:get_y() + 0.78 * inside:get_height()))")));
        QVERIFY(luaTrue(host, qsl("__baseUi.slip <= 1")));
    }

    // Geyser has no dragging back in, so "baseui dock" and the panel's own menu
    // item are the whole of the way back.
    void test_theBarsGoBackIntoTheDock()
    {
        Host* host = startProfileWithGauges();
        QVERIFY(host);
        QVERIFY(runLua(host, qsl("BaseUI.sections.vitals:dragOutOfParent(-200, 120)\nBaseUI.layoutDock()")));
        QVERIFY(luaTrue(host, qsl("BaseUI.sectionFloating('vitals')")));

        QVERIFY(runLua(host, qsl("BaseUI.alias('dock')")));
        QVERIFY2(luaTrue(host, qsl("not BaseUI.sectionFloating('vitals')")), "\"baseui dock\" did not put the bars back");
        QVERIFY(luaTrue(host, qsl("BaseUI.sections.vitals.container == BaseUI.container.Inside")));
        QVERIFY(runLua(host,
                       qsl("local inside = BaseUI.container.Inside\n"
                           "__baseUi = { slip = math.abs(BaseUI.gauges.hp:get_y() - (inside:get_y() + 0.78 * inside:get_height())) }")));
        QVERIFY2(luaTrue(host, qsl("__baseUi.slip <= 1")), "the bars came back into the dock but not into their slots");

        // docking is saved at once, so a session that ends badly still has them docked
        QVERIFY(runLua(host,
                       qsl("__baseUi.saved = {}\n"
                           "table.load(getMudletHomeDir() .. '/AdjustableContainer/' .. BaseUI.sections.vitals.name .. '.lua', __baseUi.saved)")));
        QVERIFY2(luaTrue(host, qsl("not __baseUi.saved.draggedOut")), "the save file still says the bars are out of the dock");
    }

    // Hiding the interface has to take a panel that is no longer the dock's
    // child with it, or "baseui hide" leaves bars behind over the game text.
    // Standing aside for a game's own interface hides just the same.
    void test_hidingTheInterfaceHidesFloatingBarsToo()
    {
        Host* host = startProfileWithGauges();
        QVERIFY(host);
        QVERIFY(runLua(host, qsl("BaseUI.sections.vitals:dragOutOfParent(-200, 120)\nBaseUI.layoutDock()")));

        QVERIFY(runLua(host, qsl("BaseUI.hide()")));
        QVERIFY2(luaTrue(host, qsl("BaseUI.sections.vitals.hidden")), "\"baseui hide\" left the floating bars on screen");

        QVERIFY(runLua(host, qsl("BaseUI.show()")));
        QVERIFY2(luaTrue(host, qsl("not BaseUI.sections.vitals.hidden")), "\"baseui show\" did not bring the floating bars back");
        QVERIFY(luaTrue(host, qsl("BaseUI.sectionFloating('vitals')")));

        QVERIFY(runLua(host, qsl("BaseUI.standAside(nil, 'some-game-gui')")));
        QVERIFY2(luaTrue(host, qsl("BaseUI.sections.vitals.hidden")), "standing aside for the game's own interface left the floating bars on screen");
    }

    // A save that fails is what loses the player's own settings, so it has to
    // say so. table.save reports a path it cannot write by returning a message
    // rather than by raising, so the pcall around it never sees that by itself.
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

        QVERIFY(runLua(host, qsl("BaseUI.hide()")));
        QVERIFY2(luaTrue(host, qsl("BaseUI.warnedSaveFailed")), "a failed save said nothing, so a setting vanishes with no explanation");
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
        if (!luaTrue(host, qsl("BaseUI.sections.vitals ~= nil and BaseUI.gauges.hp ~= nil and BaseUI.gauges.mp ~= nil"))) {
            qWarning("the starter UI did not build its gauges");
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

#include "StarterUiGaugePanelTest.moc"
MUDLET_GROUPED_TEST_MAIN(StarterUiGaugePanelTest)
