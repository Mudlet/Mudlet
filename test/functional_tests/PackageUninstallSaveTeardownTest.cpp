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
 * Regression test for the profile save that uninstalling a package puts off to
 * the next event loop pass outliving the profile (#9653).
 *
 * Uninstalling a package cannot save the profile there and then - the
 * asynchronous save mechanism would be handed a package that has just been
 * taken out of memory - so the save is deferred. Closing Mudlet right after an
 * uninstall then destroys the Host while that save is still owed, and the save
 * ran anyway: against a freed Host, reading its writer map. Under
 * AddressSanitizer that is a heap-use-after-free at
 * Host::pendingXmlSaveFutures(); in a release build it is a crash or silent
 * memory corruption on the way out, i.e. a "Mudlet crashed when I closed it"
 * report.
 *
 * The two tests here pin both halves of what the fix has to hold true: the
 * deferred save still happens for a profile that stays up, and nothing of it is
 * left to run once the profile has been closed and its Host destroyed. The
 * report itself needs the whole application to shut down (the queued call is
 * delivered by the event loop pass after mudlet::closeEvent() has returned),
 * which is what the busted package specs arrange; what this file adds is the
 * contract the fix rests on, and a sanitizer run over the uninstall/close/
 * destroy/pump sequence itself.
 *
 * Run with: ctest -R PackageUninstallSaveTeardownTest -V
 */

#include <QtTest/QtTest>

#include <QTemporaryDir>
#include <chrono>
#include <zip.h>

#include "Host.h"
#include "HostManager.h"
#include "MudletInstanceCoordinator.h"
#include "TelnetServerStub.h"
#include "dlgConnectionProfiles.h"
#include "mudlet.h"

using namespace std::chrono_literals;

extern void qInitResources_mudlet();
extern void qInitResources_qm();
extern void qInitResources_additional_splash_screens();
extern void qInitResources_mudlet_fonts_common();
extern void qInitResources_mudlet_fonts_posix();
void initializeQRCResourcesForPackageUninstallSaveTeardownTest();

class PackageUninstallSaveTeardownTest : public QObject
{
    Q_OBJECT

private:
    TelnetServerStub* mpServer = nullptr;
    Host* mpHost = nullptr;
    const QString mProfileName = qsl("PackageUninstallSaveTeardown-Test");
    const QString mLocalhost = qsl("localhost");
    QString mPort; // the stub's actual ephemeral port, assigned in initTestCase()
    // The refusal test below installs an archive that names itself ".." - that
    // has to happen nowhere near the developer's own profiles.
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;

    static void deleteProfileDirectory(const QString& profileName)
    {
        QDir dir(mudlet::getMudletPath(enums::profileHomePath, profileName));
        if (dir.exists()) {
            dir.removeRecursively();
        }
    }

    static QStringList savedProfileFiles(const QString& profileName) { return QDir(mudlet::getMudletPath(enums::profileXmlFilesPath, profileName)).entryList(QStringList{qsl("*.xml")}, QDir::Files); }

    // Whether needle appears in the profile that was saved last - what actually
    // landed on disk, rather than what a save signal says was attempted.
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
        QTimer::singleShot(0ms, qApp, [profileName, address, port]() {
            mudlet::self()->startAutoLogin({});
            QTest::qWait(100ms);
            QTest::mouseClick(mudlet::self()->mpConnectionDialog->new_profile_button, Qt::LeftButton);
            QTest::qWait(100ms);
            QTest::keyClicks(QApplication::focusWidget(), profileName);
            QTest::qWait(100ms);
            QTest::keyClick(QApplication::focusWidget(), Qt::Key_Tab);
            QTest::qWait(100ms);
            QTest::keyClicks(QApplication::focusWidget(), address);
            QTest::qWait(100ms);
            QTest::keyClick(QApplication::focusWidget(), Qt::Key_Tab);
            QTest::qWait(100ms);
            QTest::keyClicks(QApplication::focusWidget(), port);
            QTest::qWait(100ms);
            QTest::keyClick(QApplication::focusWidget(), Qt::Key_Return);
        });

        QSignalSpy spy(mudlet::self(), &mudlet::signal_profileLoaded);
        if (!spy.wait(5000)) {
            QFAIL("Profile took too long to load.");
        }

        mpHost = mudlet::self()->getActiveHost();
        if (!mpHost) {
            QFAIL("No active host available for the test.");
        }
    }

    // The package itself is beside the point here - what matters is that
    // uninstallPackage() has something to take away, and so owes the profile a
    // save afterwards.
    void uninstallPackageOwingASave(const QString& packageName)
    {
        mpHost->waitForProfileSave();
        mpHost->mInstalledPackages << packageName;
        QVERIFY2(mpHost->uninstallPackage(packageName, enums::PackageModuleType::Package), "The package could not be uninstalled");
        QVERIFY2(!mpHost->mInstalledPackages.contains(packageName), "The package is still installed");
        QVERIFY2(mpHost->hasPendingProfileSave(), "Uninstalling a package left the profile no save to do");
    }

    // Writes an archive holding nothing but a config.lua that names the package,
    // i.e. one installPackage() unpacks and then refuses, having registered
    // nothing from it.
    static bool writeConfigOnlyArchive(const QString& path, const QString& declaredName)
    {
        const QByteArray config = qsl("mpackage = \"%1\"\n").arg(declaredName).toUtf8();
        int errorCode = 0;
        zip* archive = zip_open(path.toUtf8().constData(), ZIP_CREATE | ZIP_TRUNCATE, &errorCode);
        if (!archive) {
            return false;
        }
        zip_source* source = zip_source_buffer(archive, config.constData(), config.size(), 0);
        if (!source || zip_file_add(archive, "config.lua", source, ZIP_FL_ENC_UTF_8) < 0) {
            zip_source_free(source);
            zip_discard(archive);
            return false;
        }
        return zip_close(archive) == 0;
    }

private slots:
    void initTestCase()
    {
        initializeQRCResourcesForPackageUninstallSaveTeardownTest();

        // Keep the test hermetic: point the config dir resolution at a temporary
        // directory instead of the user's real profiles - one of the tests below
        // drives an archive that tries to have the profiles folder deleted.
        QVERIFY(mConfigDir.isValid());
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

        startProfile(mProfileName, mLocalhost, mPort);
        QVERIFY2(mpHost, "No active host after profile creation");
    }

    void cleanupTestCase()
    {
        mpHost = nullptr;
        delete mpServer;
        mpServer = nullptr;
        deleteProfileDirectory(mProfileName);
        delete mudlet::self();
        mSavedXdg.isNull() ? qunsetenv("XDG_CONFIG_HOME") : qputenv("XDG_CONFIG_HOME", mSavedXdg);
    }

    // The save is deferred, not dropped: a profile that stays up has to end up
    // with the uninstall written out. Without this the test below could be
    // passed by never saving at all.
    void test_deferredSaveRunsWhileTheProfileIsUp()
    {
        QSignalSpy saveSpy(mpHost, &Host::profileSaveStarted);
        uninstallPackageOwingASave(qsl("uninstall-save-deferred"));
        QCOMPARE(saveSpy.count(), 0); // the point of the deferral: not saved on the spot

        QTRY_VERIFY_WITH_TIMEOUT(saveSpy.count() >= 1, 5000);
        mpHost->waitForProfileSave();
    }

    // A batch of uninstalls owes the profile one save between them, not one
    // each: restarting the timer is what the old "only one timer is running"
    // flag did, and a profile save is expensive enough that the package specs
    // are shaped around how many of them a run does.
    void test_aBatchOfUninstallsOwesOneSave()
    {
        QSignalSpy saveSpy(mpHost, &Host::profileSaveStarted);
        uninstallPackageOwingASave(qsl("uninstall-save-batch-one"));
        // no pumping in between, so all three land in the same event loop pass
        mpHost->mInstalledPackages << qsl("uninstall-save-batch-two") << qsl("uninstall-save-batch-three");
        QVERIFY(mpHost->uninstallPackage(qsl("uninstall-save-batch-two"), enums::PackageModuleType::Package));
        QVERIFY(mpHost->uninstallPackage(qsl("uninstall-save-batch-three"), enums::PackageModuleType::Package));

        QTRY_VERIFY_WITH_TIMEOUT(saveSpy.count() >= 1, 5000);
        mpHost->waitForProfileSave();
        QCOMPARE(saveSpy.count(), 1);
    }

    // Refusing an archive that installed nothing takes the folder it unpacked
    // away again (#9654) - and nothing else. The package name can be whatever an
    // untrusted archive's config.lua says, and ".." names the folder that holds
    // every profile the user has.
    void test_refusingAnArchiveOnlyRemovesItsOwnFolder()
    {
        QTemporaryDir archiveDir;
        QVERIFY2(archiveDir.isValid(), "Could not create a temporary directory for the test archive");
        const QString archivePath = archiveDir.filePath(qsl("uninstall-save-escape.mpackage"));
        QVERIFY2(writeConfigOnlyArchive(archivePath, qsl("..")), "Could not write the test archive");

        mpHost->waitForProfileSave(); // an install during a save is postponed and answered with a bare true
        const QString profileHome = mudlet::getMudletPath(enums::profileHomePath, mProfileName);
        const QString profilesDirectory = QFileInfo(profileHome).absolutePath();

        auto [ok, message] = mpHost->installPackage(archivePath, enums::PackageModuleType::Package, true);
        QVERIFY2(!ok, "An archive holding no package was installed");
        QVERIFY2(QDir(profilesDirectory).exists(), "Refusing the archive took the folder holding every profile with it");
        QVERIFY2(QDir(profileHome).exists(), "Refusing the archive took the profile with it");
        QVERIFY2(!savedProfileFiles(mProfileName).isEmpty(), "Refusing the archive took the saved profile with it");
    }

    // ...and closing the profile straight after an uninstall must leave nothing
    // of that save behind: it would run on a destroyed Host.
    void test_deferredSaveDoesNotOutliveTheProfile()
    {
        const QString packageName = qsl("uninstall-save-teardown");
        QSignalSpy saveSpy(mpHost, &Host::profileSaveStarted);
        uninstallPackageOwingASave(packageName);

        // The close path Mudlet takes when the application is closed
        // (mudlet::closeEvent): forceClose() keeps TMainConsole::closeEvent()
        // from asking whether to save, which would block on a modal dialog.
        // deleteHost() is the step of the mudlet::closeHost() that follows which
        // destroys the Host - the rest of it is tab and dock bookkeeping, and is
        // private to mudlet.
        mpHost->forceClose();
        QVERIFY2(mpHost->requestClose(), "Closing the profile was refused");
        QVERIFY2(!mpHost->hasPendingProfileSave(), "Closing the profile left a package save still owed");
        // Dropping that save is only right because the uninstall reached the disk
        // on the way out - by the close's own save, or by the deferred one going
        // first. Assert the profile that was written, not that a save was tried:
        QVERIFY2(saveSpy.count() >= 1, "Closing the profile after an uninstall saved it nowhere");
        QVERIFY2(!lastSavedProfileContains(mProfileName, packageName), "The saved profile still carries the uninstalled package");
        mpHost = nullptr;
        mudlet::self()->getHostManager().deleteHost(mProfileName);

        // Nothing the uninstall queued may reach the destroyed Host now. Under
        // AddressSanitizer a queued save that does reach it aborts the run here;
        // without the sanitizer, the save it writes is what gives it away.
        const QStringList savedBefore = savedProfileFiles(mProfileName);
        QTest::qWait(500ms);
        QCOMPARE(savedProfileFiles(mProfileName), savedBefore);
    }
};

void initializeQRCResourcesForPackageUninstallSaveTeardownTest()
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

#include "PackageUninstallSaveTeardownTest.moc"
QTEST_MAIN(PackageUninstallSaveTeardownTest)
