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

#include <QLayout>
#include <QTemporaryDir>
#include <QtTest/QtTest>

#include "PortableModeTestHelper.h"
#include "ProfileTestHelper.h"
#include "ActionUnit.h"
#include "Host.h"
#include "MudletInstanceCoordinator.h"
#include "TAction.h"
#include "TEasyButtonBar.h"
#include "TMainConsole.h"
#include "TToolBar.h"
#include "TelnetServerStub.h"
#include "ctelnet.h"
#include "mudlet.h"

#include "GroupedTest.h"

// Where a button bar ends up: a docked bar in the console's top, left or right
// strip by its location, a floating toolbar (location 4) as a dock on the main
// window, and neither left behind once its action stops being a root one.
// Where it ends up is only visible in the widget tree, which Lua cannot read.
class ActionBarPlacementTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;
    TelnetServerStub* mpServer = nullptr;
    const QString mpHostname = "Test-ActionBarPlacement";
    QString mpPort; // assigned the stub's actual ephemeral port in init()
    const QString mpLocalhost = "localhost";

    TAction* makeRootBar(Host* host, const QString& name, const int location, const bool withButton = true)
    {
        auto* action = new TAction(name, host);
        action->setName(name);
        action->setIsFolder(true);
        action->setIsActive(true);
        action->mLocation = location;
        action->mToolbarLastDockArea = Qt::RightDockWidgetArea;
        host->getActionUnit()->registerAction(action);
        if (!withButton) {
            return action;
        }
        auto* button = new TAction(action, host);
        button->setName(name + qsl(" button"));
        button->setIsActive(true);
        host->getActionUnit()->registerAction(button);
        return action;
    }

    void startProfile(const QString& hostname, const QString& address, const QString& port)
    {
        auto* host = TestProfile::create(hostname, address, port);
        if (!host) {
            QFAIL("No active host available for the test.");
        }
        QSignalSpy connected(&(host->mTelnet), &cTelnet::signal_connected);
        if (!connected.wait(2000)) {
            QFAIL("Could not connect with the host.");
        }
    }

    static void deleteProfileDirectory(const QString& profileName)
    {
        QDir dir(MudletApp::getMudletPath(enums::profileHomePath, profileName));
        if (dir.exists()) {
            dir.removeRecursively();
        }
    }

    static bool laidOutIn(QWidget* strip, QWidget* bar) { return strip->layout()->indexOf(bar) >= 0; }

private slots:
    void initTestCase()
    {
        if (portableMarkerPresent()) {
            QSKIP("portable.txt present - it takes precedence over XDG_CONFIG_HOME, so the config dir cannot be redirected");
        }

        QVERIFY(mConfigDir.isValid());
        QVERIFY(QDir().mkpath(qsl("%1/mudlet/profiles").arg(mConfigDir.path())));
        mSavedXdg = qgetenv("XDG_CONFIG_HOME");
        qputenv("XDG_CONFIG_HOME", mConfigDir.path().toUtf8());
    }

    void cleanupTestCase() { mSavedXdg.isNull() ? qunsetenv("XDG_CONFIG_HOME") : qputenv("XDG_CONFIG_HOME", mSavedXdg); }

    void init()
    {
        mpServer = new TelnetServerStub(qApp);
        mpServer->start(mpLocalhost, 0);
        mpPort = QString::number(mpServer->serverPort());
        mudlet::start();
        mudlet::self()->setupConfig();
        mudlet::self()->takeOwnershipOfInstanceCoordinator(std::make_unique<MudletInstanceCoordinator>("MudletInstanceCoordinator"));
        mudlet::self()->init();
        mudlet::self()->setStorePasswordsSecurely(false);
        deleteProfileDirectory(mpHostname);
    }

    void test_aButtonBarGoesInTheStripItsLocationNames()
    {
        startProfile(mpHostname, mpLocalhost, mpPort);
        auto* host = mudlet::self()->getActiveHost();
        QVERIFY2(host, "No active host available for the test.");
        auto* console = host->mpConsole.data();
        QVERIFY(console);

        auto* top = makeRootBar(host, qsl("placementTop"), 0);
        auto* left = makeRootBar(host, qsl("placementLeft"), 2);
        auto* right = makeRootBar(host, qsl("placementRight"), 3);
        host->getActionUnit()->updateAllToolbars();

        QVERIFY2(top->mpEasyButtonBar && left->mpEasyButtonBar && right->mpEasyButtonBar, "every docked location should have been given a button bar");
        QVERIFY(laidOutIn(console->mpTopToolBar, top->mpEasyButtonBar));
        QVERIFY(laidOutIn(console->mpLeftToolBar, left->mpEasyButtonBar));
        QVERIFY2(!laidOutIn(console->mpTopToolBar, left->mpEasyButtonBar), "a bar is made in the top strip, and should have moved out of it to its own");
        QVERIFY(laidOutIn(console->mpRightToolBar, right->mpEasyButtonBar));
        QVERIFY(!top->mpToolBar);
    }

    void test_aFloatingToolbarIsDockedWhereItWasLastDocked()
    {
        startProfile(mpHostname, mpLocalhost, mpPort);
        auto* host = mudlet::self()->getActiveHost();
        QVERIFY2(host, "No active host available for the test.");

        auto* floating = makeRootBar(host, qsl("placementFloating"), 4);
        host->getActionUnit()->updateAllToolbars();

        QVERIFY2(floating->mpToolBar, "a floating location should have been given a toolbar");
        QVERIFY(!floating->mpEasyButtonBar);
        QCOMPARE(mudlet::self()->dockWidgetArea(floating->mpToolBar), Qt::RightDockWidgetArea);
        QVERIFY(host->getActionUnit()->getToolBarList().size() == 1);
    }

    void test_aDeactivatedFloatingToolbarIsTakenOffTheWindow()
    {
        startProfile(mpHostname, mpLocalhost, mpPort);
        auto* host = mudlet::self()->getActiveHost();
        QVERIFY2(host, "No active host available for the test.");

        auto* floating = makeRootBar(host, qsl("placementDeactivated"), 4);
        host->getActionUnit()->updateAllToolbars();
        QVERIFY(floating->mpToolBar);
        QVERIFY2(mudlet::self()->dockWidgetArea(floating->mpToolBar) != Qt::NoDockWidgetArea, "the toolbar has to be docked first, or undocking it proves nothing");

        floating->setIsActive(false);
        floating->setDataChanged();
        host->getActionUnit()->updateAllToolbars();

        QVERIFY2(floating->mpToolBar, "a deactivated toolbar is taken down, not destroyed");
        QCOMPARE(mudlet::self()->dockWidgetArea(floating->mpToolBar), Qt::NoDockWidgetArea);
        QVERIFY(floating->mpToolBar->isHidden());
    }

    void test_aButtonBarMovedUnderAnotherActionLeavesItsStrip()
    {
        startProfile(mpHostname, mpLocalhost, mpPort);
        auto* host = mudlet::self()->getActiveHost();
        QVERIFY2(host, "No active host available for the test.");
        auto* console = host->mpConsole.data();
        QVERIFY(console);

        auto* moved = makeRootBar(host, qsl("placementMoved"), 0);
        auto* newParent = makeRootBar(host, qsl("placementNewParent"), 0);
        host->getActionUnit()->updateAllToolbars();
        QPointer<TEasyButtonBar> bar = moved->mpEasyButtonBar;
        QVERIFY(bar);
        QVERIFY(laidOutIn(console->mpTopToolBar, bar));

        host->getActionUnit()->reParentAction(moved->getID(), 0, newParent->getID());

        QVERIFY(bar);
        QVERIFY2(!laidOutIn(console->mpTopToolBar, bar), "a bar whose action is no longer a root one should be out of the strip");
        QVERIFY(laidOutIn(console->mpTopToolBar, newParent->mpEasyButtonBar));
    }

    void test_aFloatingToolbarMovedUnderAnotherActionIsUndocked()
    {
        startProfile(mpHostname, mpLocalhost, mpPort);
        auto* host = mudlet::self()->getActiveHost();
        QVERIFY2(host, "No active host available for the test.");

        auto* moved = makeRootBar(host, qsl("placementMovedFloating"), 4);
        auto* newParent = makeRootBar(host, qsl("placementFloatingParent"), 0);
        host->getActionUnit()->updateAllToolbars();
        QPointer<TToolBar> toolBar = moved->mpToolBar;
        QVERIFY(toolBar);
        toolBar->setFloating(true);

        host->getActionUnit()->reParentAction(moved->getID(), 0, newParent->getID());

        QVERIFY(toolBar);
        QVERIFY(!toolBar->isFloating());
        QCOMPARE(mudlet::self()->dockWidgetArea(toolBar), Qt::NoDockWidgetArea);
    }

    void test_aRemovedButtonBarLeavesItsStrip()
    {
        startProfile(mpHostname, mpLocalhost, mpPort);
        auto* host = mudlet::self()->getActiveHost();
        QVERIFY2(host, "No active host available for the test.");
        auto* console = host->mpConsole.data();
        QVERIFY(console);

        // Childless, so deleting it runs no unregisterAction() but its own
        auto* removed = makeRootBar(host, qsl("placementRemoved"), 0, false);
        host->getActionUnit()->updateAllToolbars();
        QPointer<TEasyButtonBar> bar = removed->mpEasyButtonBar;
        QVERIFY(bar);
        QVERIFY(laidOutIn(console->mpTopToolBar, bar));

        delete removed;

        QVERIFY2(bar, "removing the action takes the bar out of the strip without destroying it");
        QVERIFY(!laidOutIn(console->mpTopToolBar, bar));
    }

    void cleanup()
    {
        if (auto* self = mudlet::self()) {
            if (auto* host = self->getActiveHost()) {
                QTest::qWait(50);
                host->waitForProfileSave();
            }
        }
        delete mpServer;
        mpServer = nullptr;
        deleteProfileDirectory(mpHostname);
        delete mudlet::self();
    }
};

#include "ActionBarPlacementTest.moc"
MUDLET_GROUPED_TEST_MAIN(ActionBarPlacementTest)
