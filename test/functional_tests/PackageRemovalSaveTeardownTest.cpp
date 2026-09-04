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
 * Run with: ctest -R PackageRemovalSaveTeardownTest -V
 */

#include <QtTest/QtTest>

#include <QTemporaryDir>
#include <chrono>
#include <zip.h>

#include "PortableModeTestHelper.h"
#include "ProfileTestHelper.h"
#include "Host.h"
#include "AliasUnit.h"
#include "HostManager.h"
#include "MudletInstanceCoordinator.h"
#include "TelnetServerStub.h"
#include "dlgConnectionProfiles.h"
#include "mudlet.h"

#include "GroupedTest.h"

using namespace std::chrono_literals;

class PackageRemovalSaveTeardownTest : public QObject
{
    Q_OBJECT

private:
    TelnetServerStub* mpServer = nullptr;
    Host* mpHost = nullptr;
    const QString mProfileName = qsl("PackageRemovalSaveTeardown-Test");
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
        mpHost = TestProfile::create(profileName, address, port);
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

    // Writes an archive holding one file, i.e. one installPackage() unpacks and
    // then refuses, having registered nothing from it.
    static bool writeArchive(const QString& path, const QString& entryName, const QByteArray& contents)
    {
        int errorCode = 0;
        zip* archive = zip_open(path.toUtf8().constData(), ZIP_CREATE | ZIP_TRUNCATE, &errorCode);
        if (!archive) {
            return false;
        }
        zip_source* source = zip_source_buffer(archive, contents.constData(), contents.size(), 0);
        if (!source || zip_file_add(archive, entryName.toUtf8().constData(), source, ZIP_FL_ENC_UTF_8) < 0) {
            zip_source_free(source);
            zip_discard(archive);
            return false;
        }
        return zip_close(archive) == 0;
    }

    // ...specifically one whose config.lua renames the package to declaredName.
    static bool writeConfigOnlyArchive(const QString& path, const QString& declaredName) { return writeArchive(path, qsl("config.lua"), qsl("mpackage = \"%1\"\n").arg(declaredName).toUtf8()); }

    // ...and one that installs rather than being refused: an archive is only a
    // package if it holds a Mudlet package XML, empty though this one's units are.
    static bool writeInstallableArchive(const QString& path, const QString& packageName)
    {
        static const char packageXml[] = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
                                         "<!DOCTYPE MudletPackage>\n"
                                         "<MudletPackage version=\"1.001\">\n"
                                         "<TriggerPackage /><TimerPackage /><AliasPackage /><ActionPackage />\n"
                                         "<ScriptPackage /><KeyPackage /><VariablePackage><HiddenVariables /></VariablePackage>\n"
                                         "</MudletPackage>\n";
        return writeArchive(path, qsl("%1.xml").arg(packageName), QByteArray(packageXml, sizeof(packageXml) - 1));
    }

    QString profileFilePath(const QString& relativePath) const { return qsl("%1/%2").arg(mudlet::getMudletPath(enums::profileHomePath, mProfileName), relativePath); }

private slots:
    void initTestCase()
    {
        if (portableMarkerPresent()) {
            QSKIP("portable.txt present - it takes precedence over XDG_CONFIG_HOME, so the config dir cannot be redirected");
        }

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
        // Null when initTestCase skipped or failed ahead of mudlet::start(), and
        // getMudletPath() dereferences the instance rather than checking it
        if (mudlet::self()) {
            deleteProfileDirectory(mProfileName);
            delete mudlet::self();
        }
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

    // ...and it may only remove a folder it made itself. The package name is the
    // archive's own file name, and then whatever its config.lua says, so it can
    // just as well be "map" - the folder the profile keeps the user's maps in.
    void test_refusingAnArchiveLeavesFoldersItDidNotMake()
    {
        const QString mapFolder = profileFilePath(qsl("map"));
        const QString mapFile = qsl("%1/spec-map.dat").arg(mapFolder);
        QVERIFY2(QDir().mkpath(mapFolder), "Could not create the map folder the profile would have");
        QFile map(mapFile);
        QVERIFY2(map.open(QFile::WriteOnly), "Could not write the map file this test is about");
        map.write("map data that was here before any package was installed");
        map.close();

        QTemporaryDir archiveDir;
        QVERIFY2(archiveDir.isValid(), "Could not create a temporary directory for the test archives");
        mpHost->waitForProfileSave(); // an install during a save is postponed and answered with a bare true

        // named through config.lua, from an archive called something harmless
        const QString viaConfig = archiveDir.filePath(qsl("uninstall-save-mapgrab.mpackage"));
        QVERIFY2(writeConfigOnlyArchive(viaConfig, qsl("map")), "Could not write the test archive");
        auto [configOk, configMessage] = mpHost->installPackage(viaConfig, enums::PackageModuleType::Package, true);
        QVERIFY2(!configOk, "An archive holding no package was installed");
        QVERIFY2(QFile::exists(mapFile), "Refusing the archive took the profile's map folder with it");
        // the folder the install did make is this one, and it does have to go
        QVERIFY2(!QDir(profileFilePath(qsl("uninstall-save-mapgrab"))).exists(), "Refusing the archive left the folder it unpacked behind");

        // ...and the same through the archive's file name alone, no config.lua
        mpHost->waitForProfileSave();
        const QString viaFileName = archiveDir.filePath(qsl("map.mpackage"));
        QVERIFY2(writeArchive(viaFileName, qsl("readme.txt"), QByteArray("no package in here")), "Could not write the test archive");
        auto [fileNameOk, fileNameMessage] = mpHost->installPackage(viaFileName, enums::PackageModuleType::Package, true);
        QVERIFY2(!fileNameOk, "An archive holding no package was installed");
        QVERIFY2(QFile::exists(mapFile), "Refusing the archive took the profile's map folder with it");
    }

    // The Package Manager's repository install deletes each archive as soon as
    // installPackage() returns (dlgPackageManager::slot_installPackageFromRepository), and
    // its first pass leaves a save in flight. During a save an uninstall is refused
    // outright and an install is put off, so a second pass that updates an existing
    // package would keep the old copy and then wait on a file already deleted. This
    // replays the loop's order for two packages, the second of them an update.
    void test_aSecondRepositoryInstallIsNotLeftWaitingOnItsDeletedArchive()
    {
        QTemporaryDir archiveDir;
        QVERIFY2(archiveDir.isValid(), "Could not create a temporary directory for the test archives");
        const QString firstName = qsl("uninstall-save-repository-first");
        const QString secondName = qsl("uninstall-save-repository-second");
        const QString firstPath = archiveDir.filePath(qsl("%1.mpackage").arg(firstName));
        const QString secondPath = archiveDir.filePath(qsl("%1.mpackage").arg(secondName));
        QVERIFY2(writeInstallableArchive(firstPath, firstName), "Could not write the first test archive");
        QVERIFY2(writeInstallableArchive(secondPath, secondName), "Could not write the second test archive");

        // The older copy the loop's second pass is going to update.
        mpHost->waitForProfileSave();
        auto [older, olderMessage] = mpHost->installPackage(secondPath, enums::PackageModuleType::Package, true);
        QVERIFY2(older, qPrintable(olderMessage));
        mpHost->waitForProfileSave();
        QVERIFY2(mpHost->mInstalledPackages.contains(secondName), "SETUP: there is no older copy for the second pass to update");

        // The loop's first pass, handled the way the dialog handles it.
        mpHost->waitForProfileSave();
        auto [first, firstMessage] = mpHost->installPackage(firstPath, enums::PackageModuleType::Package, true);
        QVERIFY2(first, qPrintable(firstMessage));
        QVERIFY2(QFile::remove(firstPath), "Could not delete the first archive the way the loop does");
        QVERIFY2(mpHost->currentlySavingProfile(), "SETUP: the first pass left no save for the second to run into");

        // ...and its second pass, which is the one at risk.
        mpHost->waitForProfileSave();
        QVERIFY2(mpHost->uninstallPackage(secondName, enums::PackageModuleType::Package), "The update's removal was refused, so the older copy would have been kept");
        auto [second, secondMessage] = mpHost->installPackage(secondPath, enums::PackageModuleType::Package, true);
        QVERIFY2(second, qPrintable(secondMessage));
        QVERIFY2(QFile::remove(secondPath), "Could not delete the second archive the way the loop does");

        // Long enough that an install merely waiting its turn would have had it.
        QTest::qWait(1000);
        QVERIFY2(mpHost->mInstalledPackages.contains(secondName), "The update was left waiting on an archive the repository loop had already deleted");

        mpHost->waitForProfileSave();
        QVERIFY2(mpHost->uninstallPackage(firstName, enums::PackageModuleType::Package), "the first package could not be uninstalled");
        mpHost->waitForProfileSave();
        QVERIFY2(mpHost->uninstallPackage(secondName, enums::PackageModuleType::Package), "the second package could not be uninstalled");
        mpHost->waitForProfileSave();
    }

    // An install put off until a save finishes must run at the bottom of the event
    // loop, not from inside the profileSaveFinished emission that releases it. That
    // install starts a save of its own, and a directly connected handler is
    // therefore re-entered from its own body once per operation still waiting -
    // deep enough on macOS to exhaust the stack (#10320).
    void test_anInstallDeferredByASaveRunsOutsideTheAnnouncement()
    {
        QTemporaryDir archiveDir;
        QVERIFY2(archiveDir.isValid(), "Could not create a temporary directory for the test archive");
        const QString packageName = qsl("uninstall-save-deferred-install");
        const QString archivePath = archiveDir.filePath(qsl("%1.mpackage").arg(packageName));
        QVERIFY2(writeInstallableArchive(archivePath, packageName), "Could not write the test archive");

        mpHost->waitForProfileSave();
        QVERIFY2(!mpHost->mInstalledPackages.contains(packageName), "SETUP: the package this installs is already installed");

        mpHost->saveProfile();
        QVERIFY2(mpHost->currentlySavingProfile(), "SETUP: saveProfile() left no save for the install to be put off behind");

        auto [ok, message] = mpHost->installPackage(archivePath, enums::PackageModuleType::Package, true);
        QVERIFY2(ok, qPrintable(message));
        QVERIFY2(!mpHost->mInstalledPackages.contains(packageName), "SETUP: the install was not put off, so there is nothing here to observe");

        // Connected after the Host's own handler, so Qt runs it second and it sees
        // whatever that handler has done by the time the announcement returns. Only
        // the first announcement can tell the two connection types apart: the install
        // starts a save of its own, and by the time that second one is announced the
        // package is installed either way.
        QObject observerContext;
        int announcements = 0;
        bool installedFromInsideTheAnnouncement = false;
        connect(mpHost, &Host::profileSaveFinished, &observerContext, [&]() {
            if (++announcements == 1) {
                installedFromInsideTheAnnouncement = mpHost->mInstalledPackages.contains(packageName);
            }
        });

        QTRY_VERIFY_WITH_TIMEOUT(mpHost->mInstalledPackages.contains(packageName), 10000);
        QVERIFY2(announcements > 0, "SETUP: no save was announced, so the observer never ran");
        QVERIFY2(!installedFromInsideTheAnnouncement, "The put-off install ran from inside the profileSaveFinished emission that released it");

        mpHost->waitForProfileSave();
        QVERIFY2(mpHost->uninstallPackage(packageName, enums::PackageModuleType::Package), "the package could not be uninstalled");
        mpHost->waitForProfileSave();
    }

    // The refusal is about archives nothing could be read out of, not about
    // archives whose XML turns out to be no good - those are a different case,
    // and one this deliberately leaves alone.
    void test_anArchiveWithABadXmlIsStillARemovablePackage()
    {
        QTemporaryDir archiveDir;
        QVERIFY2(archiveDir.isValid(), "Could not create a temporary directory for the test archives");

        // 1. well-formed XML that is not a Mudlet package at all. XMLimport only
        //    reports the XML reader's own errors, so the import of this one
        //    SUCCEEDS - checking the import result would not refuse it either.
        mpHost->waitForProfileSave();
        const QString notAPackage = archiveDir.filePath(qsl("spec-notapackage.mpackage"));
        QVERIFY2(writeArchive(notAPackage, qsl("spec-notapackage.xml"), QByteArray("<?xml version=\"1.0\"?>\n<something-else/>\n")), "Could not write the test archive");
        auto [notAPackageOk, notAPackageMessage] = mpHost->installPackage(notAPackage, enums::PackageModuleType::Package, true);
        QVERIFY2(notAPackageOk, qPrintable(notAPackageMessage));
        QVERIFY2(mpHost->mInstalledPackages.contains(qsl("spec-notapackage")), "The package was not registered");
        mpHost->waitForProfileSave(); // installing a package saves, and an uninstall during a save is refused
        QVERIFY2(mpHost->uninstallPackage(qsl("spec-notapackage"), enums::PackageModuleType::Package), "The package could not be uninstalled");
        QVERIFY2(!QDir(profileFilePath(qsl("spec-notapackage"))).exists(), "Uninstalling left the package folder behind");

        // 2. XML the reader does fail on, after it has already read items out of
        //    it. The import answers false, but the alias it created is in the
        //    profile - refusing the archive here would delete the folder and
        //    strand what was imported, and the package is registered either way,
        //    so it is listed and can be uninstalled. That is what #9654 was about.
        mpHost->waitForProfileSave();
        const QString truncated = archiveDir.filePath(qsl("spec-truncatedxml.mpackage"));
        const QByteArray truncatedXml = QByteArray("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
                                                   "<!DOCTYPE MudletPackage>\n"
                                                   "<MudletPackage version=\"1.001\">\n"
                                                   "<AliasPackage>\n"
                                                   "<Alias isActive=\"yes\" isFolder=\"no\">\n"
                                                   "<name>spec-truncatedxml alias</name>\n"
                                                   "<script>send(\"hello\")</script>\n"
                                                   "<command></command>\n"
                                                   "<packageName></packageName>\n"
                                                   "<regex>^spec-truncatedxml$</regex>\n"
                                                   "</Alias>\n"
                                                   "</AliasPackage>\n"
                                                   "<ActionPackage");
        QVERIFY2(writeArchive(truncated, qsl("spec-truncatedxml.xml"), truncatedXml), "Could not write the test archive");
        auto [truncatedOk, truncatedMessage] = mpHost->installPackage(truncated, enums::PackageModuleType::Package, true);
        QVERIFY2(truncatedOk, qPrintable(truncatedMessage));
        QVERIFY2(mpHost->getAliasUnit()->findFirstAlias(qsl("spec-truncatedxml alias")), "The alias read before the XML gave out was not created");
        QVERIFY2(mpHost->mInstalledPackages.contains(qsl("spec-truncatedxml")), "The package was not registered");
        mpHost->waitForProfileSave();
        QVERIFY2(mpHost->uninstallPackage(qsl("spec-truncatedxml"), enums::PackageModuleType::Package), "The package could not be uninstalled");
        QVERIFY2(!QDir(profileFilePath(qsl("spec-truncatedxml"))).exists(), "Uninstalling left the package folder behind");
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

#include "PackageRemovalSaveTeardownTest.moc"
MUDLET_GROUPED_TEST_MAIN(PackageRemovalSaveTeardownTest)
