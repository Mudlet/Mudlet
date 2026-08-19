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

/*
 * Regression test for the quit that ran from inside the profile save it was
 * meant to wait for (#9807).
 *
 * Host::saveProfile() marks the save as started and only afterwards makes the
 * watcher that will report it finished. It pumped the event loop in between, and
 * a quit is queued (closeMudlet() arms it on a zero timer), so the quit could be
 * delivered in that gap and run the whole application shutdown from inside the
 * save. The close's own Host::waitForProfileSave() then waited for a finish
 * notification whose watcher did not exist yet, gave up on it, and let teardown
 * destroy the Host that the rest of saveProfile() went on to use. The symptom is
 * an intermittent SIGSEGV at the end of an otherwise green run, on fast machines
 * only, because the cap it gave up on counted event loop passes rather than
 * time.
 *
 * Uninstalling a package is what makes this likely in practice: it queues its
 * save rather than starting it, so a save can begin only moments before a quit.
 *
 * Run with: ctest -R ProfileSaveShutdownRaceTest -V
 */

#include <QtTest/QtTest>

#include <QPointer>
#include <QTemporaryDir>
#include <chrono>

#include "PortableModeTestHelper.h"
#include "ProfileTestHelper.h"
#include "Host.h"
#include "HostManager.h"
#include "MudletInstanceCoordinator.h"
#include "TelnetServerStub.h"
#include "dlgConnectionProfiles.h"
#include "mudlet.h"

#include "GroupedTest.h"

using namespace std::chrono_literals;

// Counts the warning waitForProfileSave() logs when it stops waiting for a save it has
// not been told the end of. Nothing here may provoke it.
static QtMessageHandler previousMessageHandler = nullptr;
static int gaveUpWaitingWarnings = 0;

static void countGiveUpWarnings(QtMsgType type, const QMessageLogContext& context, const QString& message)
{
    if (message.contains(QLatin1String("waitForProfileSave() WARNING"))) {
        ++gaveUpWaitingWarnings;
    }
    if (previousMessageHandler) {
        previousMessageHandler(type, context, message);
    }
}

class ProfileSaveShutdownRaceTest : public QObject
{
    Q_OBJECT

private:
    TelnetServerStub* mpServer = nullptr;
    Host* mpHost = nullptr;
    const QString mProfileName = qsl("ProfileSaveShutdownRace-Test");
    const QString mLocalhost = qsl("localhost");
    QString mPort; // the stub's actual ephemeral port
    QTemporaryDir mConfigDir;
    QTemporaryDir mExportDir;
    QByteArray mSavedXdg;
    // A package the profile keeps, so that what a save writes can be told apart from a
    // profile XML some earlier save left on disk.
    const QString mKeptPackage = qsl("shutdown-race-kept");
    bool mCloseAccepted = false;

    static void deleteProfileDirectory(const QString& profileName)
    {
        QDir dir(mudlet::getMudletPath(enums::profileHomePath, profileName));
        if (dir.exists()) {
            dir.removeRecursively();
        }
    }

    // Whether needle appears in the profile saved last - what actually landed on disk,
    // rather than what a save signal says was attempted. Saves are named for the time
    // they were taken, so the last in name order is the newest.
    static bool lastSavedProfileContains(const QString& profileName, const QString& needle)
    {
        const QDir directory(mudlet::getMudletPath(enums::profileXmlFilesPath, profileName));
        const QStringList saved = directory.entryList(QStringList{qsl("*.xml")}, QDir::Files, QDir::Name);
        if (saved.isEmpty()) {
            return false;
        }
        QFile file(directory.absoluteFilePath(saved.last()));
        if (!file.open(QFile::ReadOnly | QFile::Text)) {
            return false;
        }
        return QString::fromUtf8(file.readAll()).contains(needle);
    }

    // Utility function to manually start a profile like a user would do via the GUI
    void startProfile(const QString& profileName, const QString& address, const QString& port)
    {
        mpHost = TestProfile::create(profileName, address, port);
        if (!mpHost) {
            QFAIL("No active host available for the test.");
        }
    }

private slots:
    void initTestCase()
    {
        if (portableMarkerPresent()) {
            QSKIP("portable.txt present - it takes precedence over XDG_CONFIG_HOME, so the config dir cannot be redirected");
        }

        // Keep the test hermetic: point the config dir resolution at a temporary
        // directory instead of the user's real profiles.
        QVERIFY(mConfigDir.isValid());
        QVERIFY(mExportDir.isValid());
        mSavedXdg = qgetenv("XDG_CONFIG_HOME");
        QVERIFY(QDir().mkpath(qsl("%1/mudlet/profiles").arg(mConfigDir.path())));
        qputenv("XDG_CONFIG_HOME", mConfigDir.path().toUtf8());

        mpServer = new TelnetServerStub(qApp);
        mpServer->start(mLocalhost, 0); // ephemeral OS-assigned port avoids collisions across concurrent test runs
        mPort = QString::number(mpServer->serverPort());
        mudlet::start();
        mudlet::self()->setupConfig();
        mudlet::self()->takeOwnershipOfInstanceCoordinator(std::make_unique<MudletInstanceCoordinator>(qsl("MudletInstanceCoordinator")));
        mudlet::self()->init();
        mudlet::self()->setStorePasswordsSecurely(false);
        deleteProfileDirectory(mProfileName);

        previousMessageHandler = qInstallMessageHandler(countGiveUpWarnings);
        startProfile(mProfileName, mLocalhost, mPort);
        QVERIFY2(mpHost, "No active host after profile creation");
        mpHost->mInstalledPackages << mKeptPackage;
    }

    void cleanupTestCase()
    {
        qInstallMessageHandler(previousMessageHandler);
        previousMessageHandler = nullptr;
        mpHost = nullptr;
        delete mpServer;
        mpServer = nullptr;
        // Null when initTestCase skipped or failed ahead of mudlet::start(), and
        // getMudletPath() dereferences the instance rather than checking it
        if (mudlet::self()) {
            deleteProfileDirectory(mProfileName);
            delete mudlet::self();
        }
        mSavedXdg.isNull() ? qunsetenv("XDG_CONFIG_HOME") : qputenv("XDG_CONFIG_HOME", mSavedXdg);
    }

    // A "Save Profile As" that is refused because a save is already running may not
    // announce one: the matching profileSaveFinished() only ever comes from a save that
    // really runs, so the editor would be left with its Save Profile action disabled
    // and captioned "Saving…" for good.
    void test_aRefusedSaveAsAnnouncesNoSave()
    {
        mpHost->waitForProfileSave();
        auto [ok, filename, error] = mpHost->saveProfile();
        QVERIFY2(ok, qPrintable(error));

        QSignalSpy saveSpy(mpHost, &Host::profileSaveStarted);
        const QString exportPath = mExportDir.filePath(qsl("refused-save-as.xml"));
        auto [exported, exportedTo, exportError] = mpHost->saveProfileAs(exportPath);

        QVERIFY2(!exported, "Save As went ahead while a save was already running");
        QCOMPARE(saveSpy.count(), 0);
        mpHost->waitForProfileSave();
        QVERIFY2(!QFile::exists(exportPath), "The refused Save As wrote a file anyway");
        QCOMPARE(gaveUpWaitingWarnings, 0);
    }

    // ...and a quit, which Mudlet always queues, may not be delivered while a save is
    // marked as started but has no watcher yet. This one destroys the profile, so it
    // stays last: anything after it would run without one.
    void test_aQueuedQuitDoesNotRunFromInsideTheSave()
    {
        mpHost->waitForProfileSave();
        Host* host = mpHost;
        const QPointer<Host> hostGuard(host);

        // Queued on a zero timer, as mudlet::armForceClose() does, and doing to this
        // one profile what mudlet::closeEvent() and the mudlet::closeHost() after it
        // do: forceClose() keeps TMainConsole::closeEvent() from asking whether to
        // save, which would block on a modal dialog, and deleteHost() is the step that
        // destroys the Host.
        QTimer::singleShot(0ms, qApp, [this, host]() {
            host->forceClose();
            mCloseAccepted = host->requestClose();
            mudlet::self()->getHostManager().deleteHost(mProfileName);
        });

        auto [ok, filename, error] = mpHost->saveProfile();
        QVERIFY2(ok, qPrintable(error));
        QVERIFY2(!hostGuard.isNull(), "The event loop ran the quit from inside the save and destroyed the profile it was saving");
        QCOMPARE(gaveUpWaitingWarnings, 0);

        // Now let the quit run where it belongs, with the save it has to wait for
        // already properly under way.
        mpHost = nullptr;
        QTRY_VERIFY_WITH_TIMEOUT(hostGuard.isNull(), 10000);
        QVERIFY2(mCloseAccepted, "Closing the profile was refused");
        QCOMPARE(gaveUpWaitingWarnings, 0);
        QVERIFY2(lastSavedProfileContains(mProfileName, mKeptPackage), "The save the quit waited for reached no profile on disk");
    }
};

#include "ProfileSaveShutdownRaceTest.moc"
MUDLET_GROUPED_TEST_MAIN(ProfileSaveShutdownRaceTest)
