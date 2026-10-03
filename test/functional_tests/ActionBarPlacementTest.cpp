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
#include <QMoveEvent>
#include <QResizeEvent>
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

using namespace std::chrono_literals;

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
        if (!connected.wait(2s)) {
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

        QVERIFY2(console->actionEasyButtonBar(top) && console->actionEasyButtonBar(left) && console->actionEasyButtonBar(right), "every docked location should have been given a button bar");
        QVERIFY(laidOutIn(console->mpTopToolBar, console->actionEasyButtonBar(top)));
        QVERIFY(laidOutIn(console->mpLeftToolBar, console->actionEasyButtonBar(left)));
        QVERIFY2(!laidOutIn(console->mpTopToolBar, console->actionEasyButtonBar(left)), "a bar is made in the top strip, and should have moved out of it to its own");
        QVERIFY(laidOutIn(console->mpRightToolBar, console->actionEasyButtonBar(right)));
        QVERIFY(!console->actionToolBar(top));
    }

    void test_aFloatingToolbarIsDockedWhereItWasLastDocked()
    {
        startProfile(mpHostname, mpLocalhost, mpPort);
        auto* host = mudlet::self()->getActiveHost();
        QVERIFY2(host, "No active host available for the test.");

        auto* floating = makeRootBar(host, qsl("placementFloating"), 4);
        host->getActionUnit()->updateAllToolbars();

        QVERIFY2(host->mpConsole->actionToolBar(floating), "a floating location should have been given a toolbar");
        QVERIFY(!host->mpConsole->actionEasyButtonBar(floating));
        QCOMPARE(mudlet::self()->dockWidgetArea(host->mpConsole->actionToolBar(floating)), Qt::RightDockWidgetArea);
        QVERIFY(host->mpConsole->actionToolBars().size() == 1);
    }

    void test_aDeactivatedFloatingToolbarIsTakenOffTheWindow()
    {
        startProfile(mpHostname, mpLocalhost, mpPort);
        auto* host = mudlet::self()->getActiveHost();
        QVERIFY2(host, "No active host available for the test.");

        auto* floating = makeRootBar(host, qsl("placementDeactivated"), 4);
        host->getActionUnit()->updateAllToolbars();
        QVERIFY(host->mpConsole->actionToolBar(floating));
        QVERIFY2(mudlet::self()->dockWidgetArea(host->mpConsole->actionToolBar(floating)) != Qt::NoDockWidgetArea, "the toolbar has to be docked first, or undocking it proves nothing");

        floating->setIsActive(false);
        floating->setDataChanged();
        host->getActionUnit()->updateAllToolbars();

        QVERIFY2(host->mpConsole->actionToolBar(floating), "a deactivated toolbar is taken down, not destroyed");
        QCOMPARE(mudlet::self()->dockWidgetArea(host->mpConsole->actionToolBar(floating)), Qt::NoDockWidgetArea);
        QVERIFY(host->mpConsole->actionToolBar(floating)->isHidden());
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
        QPointer<TEasyButtonBar> bar = console->actionEasyButtonBar(moved);
        QVERIFY(bar);
        QVERIFY(laidOutIn(console->mpTopToolBar, bar));

        host->getActionUnit()->reParentAction(moved->getID(), 0, newParent->getID());

        QVERIFY(bar);
        QVERIFY2(!laidOutIn(console->mpTopToolBar, bar), "a bar whose action is no longer a root one should be out of the strip");
        QVERIFY(laidOutIn(console->mpTopToolBar, console->actionEasyButtonBar(newParent)));
    }

    void test_aFloatingToolbarMovedUnderAnotherActionIsUndocked()
    {
        startProfile(mpHostname, mpLocalhost, mpPort);
        auto* host = mudlet::self()->getActiveHost();
        QVERIFY2(host, "No active host available for the test.");

        auto* moved = makeRootBar(host, qsl("placementMovedFloating"), 4);
        auto* newParent = makeRootBar(host, qsl("placementFloatingParent"), 0);
        host->getActionUnit()->updateAllToolbars();
        QPointer<TToolBar> toolBar = host->mpConsole->actionToolBar(moved);
        QVERIFY(toolBar);
        toolBar->setFloating(true);

        host->getActionUnit()->reParentAction(moved->getID(), 0, newParent->getID());

        QVERIFY(toolBar);
        QVERIFY(!toolBar->isFloating());
        QCOMPARE(mudlet::self()->dockWidgetArea(toolBar), Qt::NoDockWidgetArea);
    }

    // It is drawn as a menu on its new parent's bar from then on, so its own
    // bar must not stay where it was
    void test_aButtonBarMovedIntoAnotherBarIsTakenOffTheWindow()
    {
        startProfile(mpHostname, mpLocalhost, mpPort);
        auto* host = mudlet::self()->getActiveHost();
        QVERIFY2(host, "No active host available for the test.");
        auto* console = host->mpConsole.data();
        QVERIFY(console);

        auto* moved = makeRootBar(host, qsl("placementGhost"), 0);
        auto* newParent = makeRootBar(host, qsl("placementGhostParent"), 0);
        host->getActionUnit()->updateAllToolbars();
        QPointer<TEasyButtonBar> bar = console->actionEasyButtonBar(moved);
        QVERIFY(bar && !bar->isHidden());

        // As the editor's drop does
        host->getActionUnit()->reParentAction(moved->getID(), 0, newParent->getID(), -1, -1);
        host->getActionUnit()->updateAllToolbars();

        QVERIFY2(!bar || bar->isHidden(), "the moved bar's old bar is still showing");
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        QVERIFY2(!bar, "the moved bar's old bar was kept");
        QVERIFY(console->actionEasyButtonBar(newParent) && !console->actionEasyButtonBar(newParent)->isHidden());
    }

    void test_aButtonBarMovedOutOfAPackageIntoAnotherBarIsTakenOffTheWindow()
    {
        startProfile(mpHostname, mpLocalhost, mpPort);
        auto* host = mudlet::self()->getActiveHost();
        QVERIFY2(host, "No active host available for the test.");
        auto* console = host->mpConsole.data();
        QVERIFY(console);

        auto* package = makeRootBar(host, qsl("placementGhostPackage"), 0, false);
        package->mPackageName = qsl("placementGhostPackage");
        auto* moved = new TAction(package, host);
        moved->setName(qsl("placementPackagedGhost"));
        moved->setIsFolder(true);
        moved->setIsActive(true);
        host->getActionUnit()->registerAction(moved);
        auto* button = new TAction(moved, host);
        button->setName(qsl("placementPackagedGhost button"));
        button->setIsActive(true);
        host->getActionUnit()->registerAction(button);
        auto* newParent = makeRootBar(host, qsl("placementPackagedGhostParent"), 0);
        host->getActionUnit()->updateAllToolbars();
        QPointer<TEasyButtonBar> bar = console->actionEasyButtonBar(moved);
        QVERIFY2(bar && !bar->isHidden(), "a bar directly in a package has to be showing first, or moving it proves nothing");
        QVERIFY(laidOutIn(console->mpTopToolBar, bar));

        host->getActionUnit()->reParentAction(moved->getID(), package->getID(), newParent->getID(), -1, -1);
        host->getActionUnit()->updateAllToolbars();
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);

        QVERIFY2(!bar, "the bar it had in the package was kept");
        QVERIFY(console->actionEasyButtonBar(newParent) && !console->actionEasyButtonBar(newParent)->isHidden());
    }

    void test_aPackageMovedIntoABarTakesItsBarsOffTheWindow()
    {
        startProfile(mpHostname, mpLocalhost, mpPort);
        auto* host = mudlet::self()->getActiveHost();
        QVERIFY2(host, "No active host available for the test.");
        auto* console = host->mpConsole.data();
        QVERIFY(console);

        auto* package = makeRootBar(host, qsl("placementMovedPackage"), 0, false);
        package->mPackageName = qsl("placementMovedPackage");
        auto* packaged = new TAction(package, host);
        packaged->setName(qsl("placementMovedPackageBar"));
        packaged->setIsFolder(true);
        packaged->setIsActive(true);
        host->getActionUnit()->registerAction(packaged);
        auto* button = new TAction(packaged, host);
        button->setName(qsl("placementMovedPackageBar button"));
        button->setIsActive(true);
        host->getActionUnit()->registerAction(button);
        auto* newParent = makeRootBar(host, qsl("placementMovedPackageParent"), 0);
        host->getActionUnit()->updateAllToolbars();
        QPointer<TEasyButtonBar> bar = console->actionEasyButtonBar(packaged);
        QVERIFY2(bar && !bar->isHidden(), "a bar directly in a package has to be showing first, or moving the package proves nothing");

        host->getActionUnit()->reParentAction(package->getID(), 0, newParent->getID(), -1, -1);
        host->getActionUnit()->updateAllToolbars();
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);

        QVERIFY2(!bar, "the bar the package's toolbar had was kept");
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
        QPointer<TEasyButtonBar> bar = console->actionEasyButtonBar(removed);
        QVERIFY(bar);
        QVERIFY(laidOutIn(console->mpTopToolBar, bar));

        delete removed;

        QVERIFY2(bar, "removing the action takes the bar out of the strip without destroying it");
        QVERIFY(!laidOutIn(console->mpTopToolBar, bar));
    }

    // Titled after its action at creation, and kept in step with its renames.
    void test_renamingAFloatingToolbarsActionRetitlesIt()
    {
        startProfile(mpHostname, mpLocalhost, mpPort);
        auto* host = mudlet::self()->getActiveHost();
        QVERIFY2(host, "No active host available for the test.");
        auto* console = host->mpConsole.data();
        QVERIFY(console);

        auto* floating = makeRootBar(host, qsl("placementNamed"), 4);
        host->getActionUnit()->updateAllToolbars();
        QPointer<TToolBar> toolBar = console->actionToolBar(floating);
        QVERIFY(toolBar);
        QCOMPARE(toolBar->objectName(), qsl("dockToolBar_%1_placementNamed").arg(host->getName()));

        floating->setName(qsl("placementRenamed"));

        QCOMPARE(toolBar->objectName(), qsl("dockToolBar_%1_placementRenamed").arg(host->getName()));
        QVERIFY(toolBar->windowTitle().endsWith(qsl(" - placementRenamed")));
    }

    // What the trigger editor asks for when an action is switched off, has a
    // script error, or has moved between docked and floating.
    void test_theBarsOfAnActionCanBeHiddenAndShownByAction()
    {
        startProfile(mpHostname, mpLocalhost, mpPort);
        auto* host = mudlet::self()->getActiveHost();
        QVERIFY2(host, "No active host available for the test.");
        auto* console = host->mpConsole.data();
        QVERIFY(console);

        auto* floating = makeRootBar(host, qsl("placementShownFloating"), 4);
        auto* docked = makeRootBar(host, qsl("placementShownDocked"), 0);
        host->getActionUnit()->updateAllToolbars();
        QPointer<TToolBar> toolBar = console->actionToolBar(floating);
        QPointer<TEasyButtonBar> bar = console->actionEasyButtonBar(docked);
        QVERIFY(toolBar && bar);
        QVERIFY2(!toolBar->isHidden() && !bar->isHidden(), "both bars have to be showing first, or hiding them proves nothing");

        console->setActionToolBarVisible(floating, false);
        console->hideActionEasyButtonBar(docked);
        QVERIFY(toolBar->isHidden());
        QVERIFY(bar->isHidden());

        console->setActionToolBarVisible(floating, true);
        QVERIFY(!toolBar->isHidden());
    }

    // The console keeps an action's bars by the action's address, which a later
    // action can be given once this one is freed.
    void test_aDeletedActionsBarsAreNotPassedOnToItsAddress()
    {
        startProfile(mpHostname, mpLocalhost, mpPort);
        auto* host = mudlet::self()->getActiveHost();
        QVERIFY2(host, "No active host available for the test.");
        auto* console = host->mpConsole.data();
        QVERIFY(console);

        auto* removed = makeRootBar(host, qsl("placementForgotten"), 4, false);
        host->getActionUnit()->updateAllToolbars();
        QVERIFY(console->actionToolBar(removed));

        delete removed;

        // The freed pointer is only looked up as a key, never followed
        QVERIFY(!console->actionToolBar(removed));
    }

    // A group on a floating toolbar is drawn as a menu on that toolbar and is
    // recorded against it; moving the group out to the top level must not take
    // the toolbar with it.
    void test_aGroupMovedOffAFloatingToolbarLeavesTheToolbarAlone()
    {
        startProfile(mpHostname, mpLocalhost, mpPort);
        auto* host = mudlet::self()->getActiveHost();
        QVERIFY2(host, "No active host available for the test.");
        auto* console = host->mpConsole.data();
        QVERIFY(console);

        auto* floating = makeRootBar(host, qsl("placementGroupHome"), 4);
        auto* group = new TAction(floating, host);
        group->setName(qsl("placementGroup"));
        group->setIsFolder(true);
        group->setIsActive(true);
        host->getActionUnit()->registerAction(group);
        auto* entry = new TAction(group, host);
        entry->setName(qsl("placementGroupEntry"));
        entry->setIsActive(true);
        host->getActionUnit()->registerAction(entry);
        host->getActionUnit()->updateAllToolbars();
        QPointer<TToolBar> toolBar = console->actionToolBar(floating);
        QVERIFY(toolBar);
        QVERIFY2(console->actionToolBar(group) == toolBar, "the group has to be recorded against the toolbar first, or moving it proves nothing");

        host->getActionUnit()->reParentAction(group->getID(), floating->getID(), 0);
        host->getActionUnit()->updateAllToolbars();
        // A toolbar taken down is only queued for deletion
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);

        QVERIFY(!console->actionToolBar(group));
        QVERIFY2(toolBar, "moving the group out destroyed the toolbar it had been drawn on");
        QCOMPARE(console->actionToolBar(floating), toolBar.data());
    }

    // Undoing the addition of a group deletes it with its host still set. Its
    // children then go too, and each one redraws the bar the group is a menu
    // on - which must not happen once the group itself is half destroyed.
    void test_aGroupDeletedWithItsEntriesIsNotRecordedAgain()
    {
        startProfile(mpHostname, mpLocalhost, mpPort);
        auto* host = mudlet::self()->getActiveHost();
        QVERIFY2(host, "No active host available for the test.");
        auto* console = host->mpConsole.data();
        QVERIFY(console);

        auto* root = makeRootBar(host, qsl("placementMenuHome"), 0, false);
        auto* group = new TAction(root, host);
        group->setName(qsl("placementMenu"));
        group->setIsFolder(true);
        group->setIsActive(true);
        host->getActionUnit()->registerAction(group);
        auto* entry = new TAction(group, host);
        entry->setName(qsl("placementMenuEntry"));
        entry->setIsActive(true);
        host->getActionUnit()->registerAction(entry);
        host->getActionUnit()->updateAllToolbars();
        QVERIFY2(console->actionEasyButtonBar(group), "the group has to be recorded against the bar first, or deleting it proves nothing");

        // As EditorAddItemCommand::undo() does
        host->getActionUnit()->unregisterAction(group);
        delete group;

        // The freed pointer is only looked up as a key, never followed
        QVERIFY(!console->actionEasyButtonBar(group));
        QVERIFY(console->actionEasyButtonBar(root));
    }

    // Redoing a deletion from the editor that was undone deletes the restored
    // group while it is still switched on, with its host cleared first but not
    // its children's, so they still redraw the bar the group is a menu on.
    void test_aGroupDeletedFromTheEditorIsNotRecordedAgain()
    {
        startProfile(mpHostname, mpLocalhost, mpPort);
        auto* host = mudlet::self()->getActiveHost();
        QVERIFY2(host, "No active host available for the test.");
        auto* console = host->mpConsole.data();
        QVERIFY(console);

        auto* root = makeRootBar(host, qsl("placementDeletedMenuHome"), 0, false);
        auto* group = new TAction(root, host);
        group->setName(qsl("placementDeletedMenu"));
        group->setIsFolder(true);
        group->setIsActive(true);
        host->getActionUnit()->registerAction(group);
        auto* entry = new TAction(group, host);
        entry->setName(qsl("placementDeletedMenuEntry"));
        entry->setIsActive(true);
        host->getActionUnit()->registerAction(entry);
        host->getActionUnit()->updateAllToolbars();
        QVERIFY2(console->actionEasyButtonBar(group), "the group has to be recorded against the bar first, or deleting it proves nothing");

        // As EditorDeleteItemCommand::redo() does - the editor switches an
        // action off before deleting it only the first time
        host->getActionUnit()->unregisterAction(group);
        group->mpHost = nullptr;
        delete group;

        // The freed pointer is only looked up as a key, never followed
        QVERIFY(!console->actionEasyButtonBar(group));
        QVERIFY(console->actionEasyButtonBar(root));
    }

    // Moving or resizing a floating toolbar raises its layout-changed flag, and
    // committing the layout clears it and tells the window there is a layout to
    // save again.
    void test_aMovedOrResizedFloatingToolbarMarksTheLayoutChanged()
    {
        startProfile(mpHostname, mpLocalhost, mpPort);
        auto* host = mudlet::self()->getActiveHost();
        QVERIFY2(host, "No active host available for the test.");

        auto* floating = makeRootBar(host, qsl("placementLayout"), 4);
        host->getActionUnit()->updateAllToolbars();
        QPointer<TToolBar> toolBar = host->mpConsole->actionToolBar(floating);
        QVERIFY(toolBar);
        QCoreApplication::processEvents();
        host->commitLayoutUpdates();
        QVERIFY2(!host->commitLayoutUpdates(), "nothing should be left to commit once the toolbar has settled");

        QMoveEvent moved(QPoint(7, 9), QPoint(0, 0));
        QCoreApplication::sendEvent(toolBar, &moved);
        QVERIFY(toolBar->property("layoutChanged").toBool());
        mudlet::self()->mHasSavedLayout = true;
        mudlet::self()->commitLayoutUpdates();
        QVERIFY2(!mudlet::self()->mHasSavedLayout, "a moved toolbar should leave the window layout needing a save");
        QVERIFY(!toolBar->property("layoutChanged").toBool());
        QVERIFY(!host->commitLayoutUpdates());

        QResizeEvent resized(QSize(60, 40), QSize(50, 30));
        QCoreApplication::sendEvent(toolBar, &resized);
        QVERIFY(toolBar->property("layoutChanged").toBool());
        QVERIFY(host->commitLayoutUpdates());
        QVERIFY(!toolBar->property("layoutChanged").toBool());
    }

    // A layout just restored is not a change: flushing drops what was counted
    // without clearing the flags.
    void test_flushingTheLayoutUpdatesDropsAToolbarsChange()
    {
        startProfile(mpHostname, mpLocalhost, mpPort);
        auto* host = mudlet::self()->getActiveHost();
        QVERIFY2(host, "No active host available for the test.");

        auto* floating = makeRootBar(host, qsl("placementFlushed"), 4);
        host->getActionUnit()->updateAllToolbars();
        QPointer<TToolBar> toolBar = host->mpConsole->actionToolBar(floating);
        QVERIFY(toolBar);
        QCoreApplication::processEvents();
        host->commitLayoutUpdates();

        QMoveEvent moved(QPoint(7, 9), QPoint(0, 0));
        QCoreApplication::sendEvent(toolBar, &moved);
        QVERIFY(toolBar->property("layoutChanged").toBool());

        QVERIFY(!host->commitLayoutUpdates(true));
        QVERIFY(toolBar->property("layoutChanged").toBool());
        QVERIFY2(!host->commitLayoutUpdates(), "a flushed change should no longer be counted");

        QCoreApplication::sendEvent(toolBar, &moved);
        QVERIFY2(host->commitLayoutUpdates(), "a change after the flush should be counted again");
    }

    // A toolbar deleted after it was marked is skipped, not followed.
    void test_aDeletedToolbarsLayoutChangeIsNotCounted()
    {
        startProfile(mpHostname, mpLocalhost, mpPort);
        auto* host = mudlet::self()->getActiveHost();
        QVERIFY2(host, "No active host available for the test.");

        auto* floating = makeRootBar(host, qsl("placementLayoutGone"), 4);
        host->getActionUnit()->updateAllToolbars();
        QPointer<TToolBar> toolBar = host->mpConsole->actionToolBar(floating);
        QVERIFY(toolBar);
        QCoreApplication::processEvents();
        host->commitLayoutUpdates();

        QMoveEvent moved(QPoint(7, 9), QPoint(0, 0));
        QCoreApplication::sendEvent(toolBar, &moved);
        delete toolBar.data();

        QVERIFY(!host->commitLayoutUpdates());
    }

    void cleanup()
    {
        if (auto* self = mudlet::self()) {
            if (auto* host = self->getActiveHost()) {
                QTest::qWait(50ms);
                host->waitForProfileSave();
            }
        }
        delete mpServer;
        mpServer = nullptr;
        delete mudlet::self();
        deleteProfileDirectory(mpHostname);
    }
};

#include "ActionBarPlacementTest.moc"
MUDLET_GROUPED_TEST_MAIN(ActionBarPlacementTest)
