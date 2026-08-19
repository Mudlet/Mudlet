/***************************************************************************
 *   Copyright (C) 2026 by Mudlet Developers                               *
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
#include <QtTest/QtTest>

#include <chrono>

#include "ProfileTestHelper.h"
#include "Host.h"
#include "MudletInstanceCoordinator.h"
#include "TMainConsole.h"
#include "TelnetServerStub.h"
#include "ctelnet.h"
#include "dlgConnectionProfiles.h"
#include "mudlet.h"
#include "utils.h"

#include <QDialog>
#include <QDockWidget>
#include <QLabel>
#include <QTemporaryDir>

#include <zip.h>

#include "GroupedTest.h"

using namespace std::chrono_literals;

// Exercises the widget-free seams introduced when Host was de-widgeted: the
// dockable map widget is now created and owned by the profile's main console
// (TMainConsole), and the mapping-script reminder and package-unpacking dialogs
// are shown by the frontend in response to Host signals carrying already
// translated strings. These tests verify that ownership moved and that the
// signals drive the frontend widgets as expected.
class HostWidgetDecouplingTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;
    TelnetServerStub* mpServer = nullptr;
    const QString mHostname = "Test-Host-Widget-Decoupling";
    const QString mLocalhost = "localhost";
    QString mPort;

    // setupConfig() consults portable.txt before the XDG logic
    static bool portableMarkerPresent()
    {
        return QFileInfo::exists(qsl("%1/portable.txt").arg(QCoreApplication::applicationDirPath())) || QFileInfo::exists(qsl("%1/.config/mudlet/portable.txt").arg(QDir::homePath()));
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
        // Bind an ephemeral OS-assigned port so parallel test runs (e.g. across
        // git worktrees) do not collide on a shared fixed port.
        mpServer->start(mLocalhost, 0);
        mPort = QString::number(mpServer->serverPort());
        mudlet::start();
        mudlet::self()->setupConfig();
        QCOMPARE(mudlet::getMudletPath(enums::mainPath), qsl("%1/mudlet").arg(mConfigDir.path()));
        mudlet::self()->takeOwnershipOfInstanceCoordinator(std::make_unique<MudletInstanceCoordinator>("MudletInstanceCoordinator"));
        mudlet::self()->init();
        mudlet::self()->setStorePasswordsSecurely(false);
        deleteProfileDirectory(mHostname);
    }

    // The dockable map widget used to be a QDockWidget member of Host; it now
    // lives on (and is owned by) the profile's TMainConsole. Creating the mapper
    // must populate that console-owned pointer.
    void test_dockableMapperOwnedByConsole()
    {
        startProfile(mHostname, mLocalhost, mPort);
        auto host = mudlet::self()->getActiveHost();
        QVERIFY2(host, "No active host available for the test.");
        QVERIFY2(host->mpConsole, "The active host has no main console.");

        QVERIFY2(!host->mpConsole->mpDockableMapWidget, "A fresh profile must not have a dockable map widget yet.");

        host->showHideOrCreateMapper(true);

        QVERIFY2(host->mpConsole->mpDockableMapWidget, "Creating the mapper must give the console a dockable map widget it owns.");
        QCOMPARE(host->mpConsole->mpDockableMapWidget->objectName(), qsl("dockMap_%1").arg(host->getName()));
    }

    // setMapperTitle is still a Host-facing (Lua) call, but it now drives the
    // console-owned dock: it must fail when there is no dock and set the window
    // title on the console's dock once one exists.
    void test_setMapperTitleDrivesConsoleDock()
    {
        startProfile(mHostname, mLocalhost, mPort);
        auto host = mudlet::self()->getActiveHost();
        QVERIFY2(host, "No active host available for the test.");
        QVERIFY2(host->mpConsole, "The active host has no main console.");

        auto [okWithoutDock, messageWithoutDock] = host->setMapperTitle(qsl("anything"));
        QVERIFY2(!okWithoutDock, "setMapperTitle must fail when there is no dockable map widget.");

        host->showHideOrCreateMapper(true);
        QVERIFY2(host->mpConsole->mpDockableMapWidget, "The mapper dock was not created.");

        auto [okWithDock, messageWithDock] = host->setMapperTitle(qsl("Custom map title"));
        QVERIFY2(okWithDock, qPrintable(messageWithDock));
        QCOMPARE(host->mpConsole->mpDockableMapWidget->windowTitle(), qsl("Custom map title"));
    }

    // The mapping-script reminder used to be a QDialog built inside Host; it is
    // now shown by the frontend in response to signal_showMapperScriptReminder().
    // Verify the frontend handler actually raises a dialog parented on the main
    // window. (Whether Host emits the signal depends on the profile's script
    // state, which is Host-side logic unchanged by this refactor, so we drive
    // the handler directly here.)
    void test_mappingScriptReminderShownByConsole()
    {
        startProfile(mHostname, mLocalhost, mPort);
        auto host = mudlet::self()->getActiveHost();
        QVERIFY2(host, "No active host available for the test.");
        QVERIFY2(host->mpConsole, "The active host has no main console.");

        const int dialogsBefore = mudlet::self()->findChildren<QDialog*>().count();
        host->mpConsole->showMapperScriptReminder();
        const int dialogsAfter = mudlet::self()->findChildren<QDialog*>().count();
        QVERIFY2(dialogsAfter > dialogsBefore, "showMapperScriptReminder must raise a reminder dialog owned by the main window.");
    }

    // The package-unpacking progress dialog is now owned by the console and
    // shown from Host's signal payload. A second show must replace (not stack
    // on top of) the first, and closing must dispose of it.
    void test_unpackingProgressDialogReplacedAndClosed()
    {
        startProfile(mHostname, mLocalhost, mPort);
        auto host = mudlet::self()->getActiveHost();
        QVERIFY2(host, "No active host available for the test.");
        QVERIFY2(host->mpConsole, "The active host has no main console.");

        auto console = host->mpConsole;
        QVERIFY2(!console->mpUnpackingDialog, "There must be no unpacking dialog before one is requested.");

        console->showUnpackingProgress(qsl("Unpacking package:\n\"first\"\nplease wait..."), qsl("Unpacking"));
        QVERIFY2(console->mpUnpackingDialog, "showUnpackingProgress must create a dialog.");
        QCOMPARE(console->mpUnpackingDialog->windowTitle(), qsl("Unpacking"));
        if (auto* pLabel = console->mpUnpackingDialog->findChild<QLabel*>(qsl("label"))) {
            QVERIFY2(pLabel->text().contains(qsl("first")), "The dialog label did not carry the message payload.");
        }

        // Track the first dialog: a replacement must dispose of it, not leak it
        // (the dialog is parentless, so nothing else would ever delete it).
        QPointer<QDialog> firstDialog = console->mpUnpackingDialog;
        console->showUnpackingProgress(qsl("Unpacking package:\n\"second\"\nplease wait..."), qsl("Unpacking"));
        QVERIFY2(console->mpUnpackingDialog, "A replacement unpacking dialog must exist.");
        QVERIFY2(console->mpUnpackingDialog != firstDialog, "The replacement must be a distinct dialog.");
        if (auto* pLabel = console->mpUnpackingDialog->findChild<QLabel*>(qsl("label"))) {
            QVERIFY2(pLabel->text().contains(qsl("second")), "The replacement dialog did not carry the new message payload.");
        }
        QTest::qWait(50ms); // let the replaced dialog's queued deleteLater() run
        QVERIFY2(!firstDialog, "Replacing the unpacking dialog must dispose of the previous one, not leak it.");

        console->closeUnpackingProgress();
        QTest::qWait(50ms);
        QVERIFY2(!console->mpUnpackingDialog, "closeUnpackingProgress must dispose of the dialog.");
    }

    // Regression guard: showUnpackingProgress() spins the event loop via
    // processEvents(). A deferred install completion can deliver a re-entrant
    // close (or a second show) during that spin, disposing of the dialog and
    // clearing mpUnpackingDialog. The frame must not then dereference the member.
    // Before the fix it did (mpUnpackingDialog->raise() on a nulled member) and
    // crashed; now it drives a local pointer, so reaching this test's end without
    // a crash is the assertion.
    void test_reentrantUnpackingProgressDoesNotCrash()
    {
        startProfile(mHostname, mLocalhost, mPort);
        auto host = mudlet::self()->getActiveHost();
        QVERIFY2(host, "No active host available for the test.");
        QVERIFY2(host->mpConsole, "The active host has no main console.");
        auto console = host->mpConsole;

        // Queue a re-entrant close to fire while showUnpackingProgress() is inside
        // its first processEvents(), mimicking a deferred install completion.
        QMetaObject::invokeMethod(
                qApp,
                [console]() {
                    console->closeUnpackingProgress();
                },
                Qt::QueuedConnection);

        console->showUnpackingProgress(qsl("Unpacking package:\n\"reentrant\"\nplease wait..."), qsl("Unpacking"));

        QTest::qWait(50ms);
        QVERIFY2(!console->mpUnpackingDialog, "The re-entrant close should have left no unpacking dialog behind.");
    }

    // The tests above drive the console's handlers directly, so they would all
    // still pass if the Host -> console connections made in
    // mudlet::addConsoleForNewHost() were lost (that function is a merge-conflict
    // hot spot). This one installs a real package instead, so the show and hide
    // signals have to travel the production wiring to reach the dialog.
    void test_unpackingDialogDrivenByInstall()
    {
        startProfile(mHostname, mLocalhost, mPort);
        auto host = mudlet::self()->getActiveHost();
        QVERIFY2(host, "No active host available for the test.");
        QVERIFY2(host->mpConsole, "The active host has no main console.");
        auto console = host->mpConsole;

        QTemporaryDir packageDir;
        QVERIFY2(packageDir.isValid(), "Could not create a temporary directory for the test package.");
        const QString packageName = qsl("HostWidgetDecouplingPackage");
        const QString packagePath = packageDir.filePath(qsl("%1.zip").arg(packageName));
        QVERIFY2(writePackageArchive(packagePath, packageName), "Could not write the test package archive.");

        // installPackage() postpones the whole install (and so emits nothing) if a
        // profile save is still in flight from loading the profile.
        QTRY_VERIFY(!host->currentlySavingProfile());

        QSignalSpy showSpy(host, &Host::signal_showUnpackingProgress);
        QSignalSpy hideSpy(host, &Host::signal_hideUnpackingProgress);

        // Connected after the console's own handler, so it observes the dialog
        // that handler has just put up - if the wiring is intact.
        QObject captureContext;
        bool dialogUpWhileUnpacking = false;
        QPointer<QDialog> dialogWhileUnpacking;
        connect(host, &Host::signal_showUnpackingProgress, &captureContext, [&](const QString&, const QString&) {
            dialogWhileUnpacking = console->mpUnpackingDialog;
            dialogUpWhileUnpacking = !dialogWhileUnpacking.isNull();
        });

        auto [ok, message] = host->installPackage(packagePath, enums::PackageModuleType::Package, false);
        QVERIFY2(ok, qPrintable(message));

        QCOMPARE(showSpy.count(), 1);
        QCOMPARE(hideSpy.count(), 1);
        QVERIFY2(dialogUpWhileUnpacking, "Installing a package must put the unpacking dialog up via the Host signal.");
        QVERIFY2(!console->mpUnpackingDialog, "Finishing the install must take the unpacking dialog down again.");
        QTest::qWait(50ms); // let the dialog's queued deleteLater() run
        QVERIFY2(dialogWhileUnpacking.isNull(), "The unpacking dialog was taken down but never disposed of.");
    }

    // The map dock moved from Host to TMainConsole, so disposing of it is now the
    // console destructor's job. addDockWidget() reparents the dock onto the main
    // window, which outlives the profile, so nothing else would clean it up.
    void test_mapDockDestroyedOnProfileClose()
    {
        startProfile(mHostname, mLocalhost, mPort);
        auto host = mudlet::self()->getActiveHost();
        QVERIFY2(host, "No active host available for the test.");
        QVERIFY2(host->mpConsole, "The active host has no main console.");

        host->showHideOrCreateMapper(true);
        QPointer<QDockWidget> dock = host->mpConsole->mpDockableMapWidget;
        QVERIFY2(dock, "The mapper dock was not created.");

        // Forcing the close stops TMainConsole::closeEvent() asking whether the
        // profile should be saved, which would block on a modal dialog here.
        // requestClose() is the half of the profile-close path that disposes of
        // the console; the mudlet::closeHost() that normally follows it only
        // removes the tab and the Host, and would reopen the connection dialog
        // as the last profile went away.
        host->forceClose();
        QVERIFY2(host->requestClose(), "Closing the profile was refused.");

        // Two chained deferred deletes to get through: the console (it carries
        // WA_DeleteOnClose) and then, from its destructor, the dock.
        QTest::qWait(500ms);
        QVERIFY2(dock.isNull(), "Closing the profile must destroy the map dock the console owns.");
    }

    void cleanup()
    {
        delete mpServer;
        mpServer = nullptr;
        deleteProfileDirectory(mHostname);
        delete mudlet::self();
    }

    // Utility function to manually start a profile like a user would do via the
    // GUI
    void startProfile(const QString& hostname, const QString& address, const QString& port)
    {
        auto host = TestProfile::create(hostname, address, port);
        if (!host) {
            QFAIL("No active host available for the test.");
        }

        QSignalSpy spy2(&(host->mTelnet), &cTelnet::signal_connected);
        if (!spy2.wait(2s)) {
            QFAIL("Could not connect with the host.");
        }
    }

    // Utility function producing the smallest package archive that installs: a
    // zip holding one Mudlet package XML with nothing in it. An archive with no
    // package XML at all is refused (it would install nowhere and could never be
    // uninstalled), so the dialog wiring this test is about needs a real one.
    bool writePackageArchive(const QString& path, const QString& packageName)
    {
        static const char packageXml[] = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
                                         "<!DOCTYPE MudletPackage>\n"
                                         "<MudletPackage version=\"1.001\">\n"
                                         "<TriggerPackage /><TimerPackage /><AliasPackage /><ActionPackage />\n"
                                         "<ScriptPackage /><KeyPackage /><VariablePackage><HiddenVariables /></VariablePackage>\n"
                                         "</MudletPackage>\n";

        int errorCode = 0;
        zip* archive = zip_open(path.toUtf8().constData(), ZIP_CREATE | ZIP_TRUNCATE, &errorCode);
        if (!archive) {
            return false;
        }
        // sizeof - 1 to leave the terminating null out of the archived file
        zip_source* source = zip_source_buffer(archive, packageXml, sizeof(packageXml) - 1, 0);
        if (!source || zip_file_add(archive, qsl("%1.xml").arg(packageName).toUtf8().constData(), source, ZIP_FL_ENC_UTF_8) < 0) {
            zip_source_free(source);
            zip_discard(archive);
            return false;
        }
        return zip_close(archive) == 0;
    }

    // Utility function
    void deleteProfileDirectory(const QString& profileName)
    {
        const QString path = mudlet::getMudletPath(enums::profileHomePath, profileName);
        QDir dir(path);

        if (!dir.exists()) {
            qInfo() << "Profile directory does not exist:" << path;
            return;
        }
        dir.removeRecursively();
    }
};

#include "HostWidgetDecouplingTest.moc"
MUDLET_GROUPED_TEST_MAIN(HostWidgetDecouplingTest)
