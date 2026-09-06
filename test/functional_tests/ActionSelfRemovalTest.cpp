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

#include <QFileInfo>
#include <QTemporaryDir>
#include <QtTest/QtTest>

#include <QPushButton>

#include "PortableModeTestHelper.h"
#include "ProfileTestHelper.h"
#include "ActionUnit.h"
#include "Host.h"
#include "MudletInstanceCoordinator.h"
#include "TAction.h"
#include "TEasyButtonBar.h"
#include "TFlipButton.h"
#include "TLuaInterpreter.h"
#include "TMainConsole.h"
#include "TToolBar.h"
#include "TelnetServerStub.h"
#include "ctelnet.h"
#include "dlgConnectionProfiles.h"
#include "mudlet.h"

#include "GroupedTest.h"

// Regression tests for the self-uninstall use-after-free: a package toolbar
// button whose Lua script calls uninstallPackage() on its own package used to
// free the very TAction that TAction::execute() was running on, which then read
// this->mpHost after the Lua call returned (heap-use-after-free). ActionUnit now
// defers that delete until execute() has unwound.
class ActionSelfRemovalTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;
    TelnetServerStub* mpServer = nullptr;
    const QString mpHostname = "Test-ActionSelfRemoval";
    QString mpPort; // assigned the stub's actual ephemeral port in init()
    const QString mpLocalhost = "localhost";

    // Builds a package the way an installed package with a toolbar button is laid
    // out: a master-folder root carrying the package name, a toolbar under it (the
    // TEasyButtonBar, on the top bar via mLocation 0), and the actual button under
    // that. updateAllToolbars() then gives the button a real TFlipButton, letting a
    // test drive the genuine click dispatch path. registerAction() assigns the ids,
    // so it must run before setScript() (which bakes the id into the Lua funcname).
    // Returns the leaf button whose script uninstalls the package.
    TAction* buildSelfUninstallingPackage(Host* host, const QString& packageName)
    {
        auto* actionUnit = host->getActionUnit();

        auto* master = new TAction(packageName, host);
        master->mPackageName = packageName;
        master->mModuleMasterFolder = true;
        master->setIsFolder(true);
        master->setIsActive(true);
        actionUnit->registerAction(master);

        auto* toolbar = new TAction(master, host);
        toolbar->setName(qsl("selfUninstallToolbar"));
        toolbar->mLocation = 0;
        toolbar->setIsActive(true);
        actionUnit->registerAction(toolbar);

        auto* button = new TAction(toolbar, host);
        button->setName(qsl("selfUninstallButton"));
        button->setIsActive(true);
        actionUnit->registerAction(button);
        button->setScript(qsl("uninstallPackage([[%1]])").arg(packageName));

        host->mInstalledPackages << packageName;
        return button;
    }

    TFlipButton* findButtonWidget(Host* host, const TAction* action)
    {
        // TFlipButton has no Q_OBJECT, so findChildren<> it as its QPushButton base
        // and downcast.
        for (auto* pushButton : host->mpConsole->findChildren<QPushButton*>()) {
            auto* pB = dynamic_cast<TFlipButton*>(pushButton);
            if (pB && pB->mpTAction == action) {
                return pB;
            }
        }
        return nullptr;
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
    }

    void cleanupTestCase() { mSavedXdg.isNull() ? qunsetenv("XDG_CONFIG_HOME") : qputenv("XDG_CONFIG_HOME", mSavedXdg); }

    void init()
    {
        mpServer = new TelnetServerStub(qApp);
        mpServer->start(mpLocalhost, 0);
        mpPort = QString::number(mpServer->serverPort());
        mudlet::start();
        mudlet::self()->setupConfig();
        QCOMPARE(mudlet::getMudletPath(enums::mainPath), qsl("%1/mudlet").arg(mConfigDir.path()));
        mudlet::self()->takeOwnershipOfInstanceCoordinator(std::make_unique<MudletInstanceCoordinator>("MudletInstanceCoordinator"));
        mudlet::self()->init();
        mudlet::self()->setStorePasswordsSecurely(false);
        deleteProfileDirectory(mpHostname);
    }

    // A button whose script uninstalls its own package must not crash, must finish
    // removing the package, and its deferred TActions must be freed cleanly by
    // doCleanup() afterwards. Invokes the TAction::execute() path directly (the
    // entry the review brief asks for) and, because the package is given a live
    // toolbar first, also drives uninstallPackage()'s internal updateAllToolbars()
    // over the half-uninstalled (deactivated but still-linked) package.
    void test_selfUninstallingButtonDoesNotCrash()
    {
        startProfile(mpHostname, mpLocalhost, mpPort);
        auto* host = mudlet::self()->getActiveHost();
        QVERIFY2(host, "No active host available for the test.");
        host->mEchoLuaErrors = true;
        auto* actionUnit = host->getActionUnit();

        const QString packageName = qsl("TestActionUninstallPkg");
        auto* button = buildSelfUninstallingPackage(host, packageName);
        const int buttonId = button->getID();

        // Give the package a real TFlipButton/TEasyButtonBar so the uninstall runs
        // over a live toolbar, not an empty one.
        actionUnit->updateAllToolbars();
        QVERIFY2(findButtonWidget(host, button), "The button should have a real toolbar widget before the test");

        // The crash: pre-fix this frees `button` mid-call, then execute() reads
        // this->mpHost. If the guard works we return here with everything intact.
        button->execute();

        QCOMPARE(actionUnit->processingDepth(), 0);
        QVERIFY2(!host->mInstalledPackages.contains(packageName), "The package should have been uninstalled by the button's script");
        QVERIFY2(!bufferContains(qsl("Lua error")), "The button's uninstallPackage() script must not have errored");
        // Still alive but deferred - deletion was postponed until execute() unwound.
        QVERIFY2(actionUnit->getAction(buttonId), "The self-uninstalling button must not be freed while execute() is on the stack");

        // Flushing the deferred deletes (as the dispatchers and Host's per-line
        // cleanup do) must actually remove them, with no double free.
        actionUnit->doCleanup();
        QVERIFY2(!actionUnit->getAction(buttonId), "The button should be gone after doCleanup()");
        QVERIFY2(actionUnit->findItems(qsl("selfUninstallButton")).empty(), "No trace of the button should remain after cleanup");

        // uninstallPackage() defers its profile save to the next event-loop cycle
        // (QTimer::singleShot, see Host::uninstallPackage()) and that save runs its
        // XML serialization on a background thread. Fire the deferred timer, then
        // block until the save has fully finished, so no background save thread is
        // still running when cleanup() destroys the host: tearing the host down
        // underneath an in-flight save corrupted the heap and crashed on Windows.
        QTest::qWait(50);
        host->waitForProfileSave();
    }

    void cleanup()
    {
        // Defence for the failure path: if an assertion above aborted the test
        // before its own drain ran, a profile save uninstallPackage() deferred
        // could still be in flight. Let it finish before deleting the host, so
        // destruction never races a background save thread (a Windows crash).
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

    // installPackage() works out why an install failed and hands the reason back,
    // but most callers drop it: a GUI install used to fail with nothing on screen
    // and only a qWarning on a terminal no player sees. A non-quiet install of a
    // file that is not an archive has to say so in the profile's message area.
    // Script installs pass quiet and are deliberately left out of that - they get
    // the reason as a return value - which is why this drives
    // Host::installPackage() directly rather than the Lua installPackage().
    void test_aFailedInstallTellsTheUserWhy()
    {
        startProfile(mpHostname, mpLocalhost, mpPort);
        auto* host = mudlet::self()->getActiveHost();
        QVERIFY2(host, "Could not start a profile");

        // A file that exists but is not an archive, which is the failure a user
        // meets most often - unzip refuses it.
        const QString brokenPackage = qsl("%1/broken.mpackage").arg(mudlet::getMudletPath(enums::profileHomePath, mpHostname));
        QFile broken(brokenPackage);
        QVERIFY2(broken.open(QIODevice::WriteOnly), qPrintable(qsl("Could not create %1").arg(brokenPackage)));
        broken.write("this is not a zip archive");
        broken.close();

        auto [installed, reason] = host->installPackage(brokenPackage, enums::PackageModuleType::Package);
        QVERIFY2(!installed, "Installing a file that is not an archive must fail");
        QVERIFY2(!reason.isEmpty(), "A failed install must come back with a reason");

        QApplication::processEvents();
        QVERIFY2(bufferContains(qsl("Package install failed")), qPrintable(qsl("The failure must reach the profile's message area, but the buffer held: \"%1\"").arg(joinedBuffer())));
        QVERIFY2(bufferContains(reason), qPrintable(qsl("The message must carry the reason \"%1\", but the buffer held: \"%2\"").arg(reason, joinedBuffer())));
    }

    // ActionUnit only decides which bar an action gets; TMainConsole builds and
    // places the widgets. A top-bar, a left-bar and a right-bar TEasyButtonBar
    // have to end up in the console's matching side toolbar (the side ones are
    // what prove the attach step, since every TEasyButtonBar starts life on the
    // top bar), and a floating TToolBar has to be docked on the main window's
    // left, the area a bar that has never floated gets.
    void test_toolbarsArePlacedByTheMainConsole()
    {
        startProfile(mpHostname, mpLocalhost, mpPort);
        auto* host = mudlet::self()->getActiveHost();
        QVERIFY2(host, "No active host available for the test.");
        auto* actionUnit = host->getActionUnit();

        auto makeRoot = [&](const QString& name, int location) {
            auto* root = new TAction(name, host);
            root->setIsFolder(true);
            root->mLocation = location;
            root->mToolbarLastFloatingState = false;
            root->setIsActive(true);
            actionUnit->registerAction(root);
            return root;
        };
        auto* topBar = makeRoot(qsl("topBar"), 0);
        auto* leftBar = makeRoot(qsl("leftBar"), 2);
        auto* rightBar = makeRoot(qsl("rightBar"), 3);
        auto* floatingBar = makeRoot(qsl("floatingBar"), 4);

        actionUnit->updateAllToolbars();

        TMainConsole* console = host->mpConsole;
        QVERIFY2(topBar->mpEasyButtonBar, "The top-bar action should have been given a TEasyButtonBar");
        QCOMPARE(topBar->mpEasyButtonBar->parentWidget(), console->mpTopToolBar);
        QVERIFY2(leftBar->mpEasyButtonBar, "The left-bar action should have been given a TEasyButtonBar");
        QCOMPARE(leftBar->mpEasyButtonBar->parentWidget(), console->mpLeftToolBar);
        QVERIFY2(rightBar->mpEasyButtonBar, "The right-bar action should have been given a TEasyButtonBar");
        QCOMPARE(rightBar->mpEasyButtonBar->parentWidget(), console->mpRightToolBar);
        QVERIFY2(floatingBar->mpToolBar, "The floating action should have been given a TToolBar");
        QCOMPARE(floatingBar->mpToolBar->parentWidget(), static_cast<QWidget*>(mudlet::self()));
        QCOMPARE(mudlet::self()->dockWidgetArea(floatingBar->mpToolBar), Qt::LeftDockWidgetArea);
    }

    // The bars are the console's widgets now, so ActionUnit has to cope with being
    // asked to rebuild them for a profile that has no console - the view is
    // optional. A floating root is the case that used to survive it:
    // regenerateEasyButtonBars() skips those nodes, and before the bars moved to
    // the console regenerateToolBars() only touched the main window, so neither
    // half went near mpConsole.
    void test_rebuildingToolbarsWithoutAConsoleBuildsNothing()
    {
        startProfile(mpHostname, mpLocalhost, mpPort);
        auto* host = mudlet::self()->getActiveHost();
        QVERIFY2(host, "No active host available for the test.");
        auto* actionUnit = host->getActionUnit();

        auto* floatingBar = new TAction(qsl("floatingBar"), host);
        floatingBar->setIsFolder(true);
        floatingBar->mLocation = 4;
        floatingBar->mToolbarLastFloatingState = false;
        floatingBar->setIsActive(true);
        actionUnit->registerAction(floatingBar);

        QPointer<TMainConsole> console = host->mpConsole;
        host->mpConsole = nullptr;
        actionUnit->updateAllToolbars();
        host->mpConsole = console;

        QVERIFY2(!floatingBar->mpToolBar, "No bar should have been built while the console was away");

        actionUnit->updateAllToolbars();
        QVERIFY2(floatingBar->mpToolBar, "The bar should be built once the console is back");
    }

    // Removing a docked bar's action reaches the console to take the bar down.
    // With no console there is nothing to take it down from, so the removal has
    // to leave the widget alone rather than reach through a null pointer.
    void test_removingADockedBarActionWithoutAConsoleLeavesTheBarAlone()
    {
        startProfile(mpHostname, mpLocalhost, mpPort);
        auto* host = mudlet::self()->getActiveHost();
        QVERIFY2(host, "No active host available for the test.");
        auto* actionUnit = host->getActionUnit();

        auto* leftBar = new TAction(qsl("leftBar"), host);
        leftBar->setIsFolder(true);
        leftBar->mLocation = 2;
        leftBar->mToolbarLastFloatingState = false;
        leftBar->setIsActive(true);
        actionUnit->registerAction(leftBar);
        actionUnit->updateAllToolbars();
        QVERIFY2(leftBar->mpEasyButtonBar, "The left-bar action should have been given a TEasyButtonBar");
        QPointer<TEasyButtonBar> bar = leftBar->mpEasyButtonBar;

        QPointer<TMainConsole> console = host->mpConsole;
        host->mpConsole = nullptr;
        delete leftBar;
        host->mpConsole = console;

        QVERIFY2(bar, "The bar belongs to the console and has to outlive its action");
        QVERIFY2(bar->isHidden(), "The action hides its bar on the way out");
        QCOMPARE(bar->parentWidget(), console->mpLeftToolBar);
    }

    // Moving a bar's action under a folder reaches the console the same way, to
    // take the bar off its side toolbar. Without a console the move itself still
    // happens and the widget is left where it is.
    void test_reparentingABarActionWithoutAConsoleLeavesTheBarAlone()
    {
        startProfile(mpHostname, mpLocalhost, mpPort);
        auto* host = mudlet::self()->getActiveHost();
        QVERIFY2(host, "No active host available for the test.");
        auto* actionUnit = host->getActionUnit();

        auto* leftBar = new TAction(qsl("leftBar"), host);
        leftBar->setIsFolder(true);
        leftBar->mLocation = 2;
        leftBar->mToolbarLastFloatingState = false;
        leftBar->setIsActive(true);
        actionUnit->registerAction(leftBar);
        auto* folder = new TAction(qsl("folder"), host);
        folder->setIsFolder(true);
        folder->setIsActive(true);
        actionUnit->registerAction(folder);
        actionUnit->updateAllToolbars();
        QVERIFY2(leftBar->mpEasyButtonBar, "The left-bar action should have been given a TEasyButtonBar");
        QPointer<TEasyButtonBar> bar = leftBar->mpEasyButtonBar;

        QPointer<TMainConsole> console = host->mpConsole;
        host->mpConsole = nullptr;
        actionUnit->reParentAction(leftBar->getID(), 0, folder->getID());
        host->mpConsole = console;

        QCOMPARE(leftBar->getParent(), folder);
        QVERIFY2(bar, "The bar belongs to the console and has to outlive the move");
        QCOMPARE(bar->parentWidget(), console->mpLeftToolBar);
    }

    // With a console present, removing a bar's action takes the bar off its side
    // toolbar; a floating toolbar is only hidden and stays docked on the main
    // window. The widgets outlive the action either way.
    void test_removingBarActionsWithAConsoleTakesTheBarsDown()
    {
        startProfile(mpHostname, mpLocalhost, mpPort);
        auto* host = mudlet::self()->getActiveHost();
        QVERIFY2(host, "No active host available for the test.");
        auto* actionUnit = host->getActionUnit();

        auto makeRoot = [&](const QString& name, int location) {
            auto* root = new TAction(name, host);
            root->setIsFolder(true);
            root->mLocation = location;
            root->mToolbarLastFloatingState = false;
            root->setIsActive(true);
            actionUnit->registerAction(root);
            return root;
        };
        auto* leftBar = makeRoot(qsl("leftBar"), 2);
        auto* floatingBar = makeRoot(qsl("floatingBar"), 4);
        actionUnit->updateAllToolbars();

        TMainConsole* console = host->mpConsole;
        QPointer<TEasyButtonBar> bar = leftBar->mpEasyButtonBar;
        QVERIFY2(bar, "The left-bar action should have been given a TEasyButtonBar");
        QVERIFY2(console->mpLeftToolBar->layout()->indexOf(bar) != -1, "SETUP: the bar is not on the left toolbar to begin with");
        QPointer<TToolBar> toolbar = floatingBar->mpToolBar;
        QVERIFY2(toolbar, "The floating action should have been given a TToolBar");
        QVERIFY2(mudlet::self()->dockWidgetArea(toolbar) != Qt::NoDockWidgetArea, "SETUP: the toolbar is not docked to begin with");
        QVERIFY2(!toolbar->isHidden(), "SETUP: the toolbar is hidden to begin with");

        delete leftBar;
        delete floatingBar;

        QVERIFY2(bar, "The bar belongs to the console and has to outlive its action");
        QCOMPARE(console->mpLeftToolBar->layout()->indexOf(bar), -1);
        QVERIFY2(toolbar, "The toolbar belongs to the main window and has to outlive its action");
        QVERIFY2(toolbar->isHidden(), "The removed action's toolbar is still showing");
    }

    // Starts a profile the way a user would via the GUI (mirrors the helper in
    // TFeedTriggersRecursionTest).
    void startProfile(const QString& hostname, const QString& address, const QString& port)
    {
        auto host = TestProfile::create(hostname, address, port);
        if (!host) {
            QFAIL("No active host available for the test.");
        }

        QSignalSpy spy2(&(host->mTelnet), &cTelnet::signal_connected);
        if (!spy2.wait(2000)) {
            QFAIL("Could not connect with the host.");
        }
    }

    QString joinedBuffer()
    {
        auto console = mudlet::self()->getActiveHost()->mpConsole;
        QString allText;
        for (int i = 0; i <= console->buffer.getLastLineNumber(); ++i) {
            allText.append(console->buffer.line(i)).append(QChar::Space);
        }
        return allText.simplified();
    }

    bool bufferContains(const QString& needle) { return joinedBuffer().contains(needle); }

    void deleteProfileDirectory(const QString& profileName)
    {
        const QString path = mudlet::getMudletPath(enums::profileHomePath, profileName);
        QDir dir(path);
        if (!dir.exists()) {
            return;
        }
        dir.removeRecursively();
    }
};

#include "ActionSelfRemovalTest.moc"
MUDLET_GROUPED_TEST_MAIN(ActionSelfRemovalTest)
