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
 * Coverage for the background half of a profile save that has modules to write,
 * and for that write outliving the profile that ordered it.
 *
 * A profile save hands the modules that are set to sync to a thread pool task and
 * returns. Two ways of closing a profile then wait for nothing: answering "No" to
 * "Save profile?", and any close that finds the main console already gone. Either
 * destroys the Host with the write still going, and the write went on reading the
 * Host it was queued from - its XMLexport, and then its name. Under
 * AddressSanitizer that kills the run inside Host::writeModuleFiles(); in a
 * release build it is a crash or a corrupted .mpackage on the way out. The watcher
 * that reports the write finished was unowned in the same window, so nothing was
 * left to delete it once the profile it reported to had gone.
 *
 * This is the same shape as #9653 "uninstalling a package then quitting is a
 * use-after-free", but it stayed hidden because no test profile had a module in
 * it: with none installed the save's module list comes out empty and the write
 * returns at its first line. So the point of this file is a profile that genuinely
 * carries a synced module, which is what makes the hazardous path run at all.
 *
 * test_aSyncedModuleIsWrittenOutOnSave() is that coverage - the module really is
 * serialized and its archive really is rewritten. The teardown test after it is
 * the regression: it holds the thread pool so the write is provably still queued
 * when the Host is destroyed, and then lets it run.
 *
 * Run with: ctest -R ModuleSaveTeardownTest -V
 */

#include <QtTest/QtTest>

#include <QFutureWatcher>
#include <QMessageBox>
#include <QRunnable>
#include <QScopeGuard>
#include <QSemaphore>
#include <QTemporaryDir>
#include <QThreadPool>
#include <chrono>
#include <zip.h>

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

class ModuleSaveTeardownTest : public QObject
{
    Q_OBJECT

private:
    TelnetServerStub* mpServer = nullptr;
    Host* mpHost = nullptr;
    const QString mProfileName = qsl("ModuleSaveTeardown-Test");
    const QString mModuleName = qsl("module-save-teardown");
    const QString mLocalhost = qsl("localhost");
    QString mPort; // the stub's actual ephemeral port, assigned in initTestCase()
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;
    // A synced module has its own archive rewritten by every profile save, so it
    // lives in a scratch directory rather than anywhere the repository can see.
    QTemporaryDir mArchiveDir;
    QString mModuleArchivePath;
    int mHeldPoolThreads = 0;
    int mOriginalMaxPoolThreads = 0;
    QSemaphore mPoolBlockersStarted;
    QSemaphore mPoolRelease;

    static void deleteProfileDirectory(const QString& profileName)
    {
        QDir dir(mudlet::getMudletPath(enums::profileHomePath, profileName));
        if (dir.exists()) {
            dir.removeRecursively();
        }
    }

    // What the module archive is built from. Deliberately nothing like the module
    // XML Mudlet writes: that one carries a <HelpPackage> element and this one does
    // not, which is how the tests below tell "the module has been written out" from
    // "this is still the file the archive was unpacked with".
    static QByteArray sourceModuleXml()
    {
        return QByteArray("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
                          "<!DOCTYPE MudletPackage>\n"
                          "<MudletPackage version=\"1.001\">\n"
                          "<AliasPackage>\n"
                          "<Alias isActive=\"yes\" isFolder=\"no\">\n"
                          "<name>module-save-teardown alias</name>\n"
                          "<script>send(\"hello\")</script>\n"
                          "<command></command>\n"
                          "<packageName></packageName>\n"
                          "<regex>^module-save-teardown$</regex>\n"
                          "</Alias>\n"
                          "</AliasPackage>\n"
                          "</MudletPackage>\n");
    }

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

    static QByteArray archiveEntry(const QString& path, const QString& entryName)
    {
        int errorCode = 0;
        zip* archive = zip_open(path.toUtf8().constData(), ZIP_RDONLY, &errorCode);
        if (!archive) {
            return {};
        }
        zip_stat_t entryStat;
        QByteArray contents;
        if (zip_stat(archive, entryName.toUtf8().constData(), 0, &entryStat) == 0) {
            if (zip_file* file = zip_fopen(archive, entryName.toUtf8().constData(), 0); file) {
                contents.resize(static_cast<qsizetype>(entryStat.size));
                if (zip_fread(file, contents.data(), entryStat.size) != static_cast<zip_int64_t>(entryStat.size)) {
                    contents.clear();
                }
                zip_fclose(file);
            }
        }
        zip_discard(archive);
        return contents;
    }

    QString moduleXmlPath() const { return mudlet::getMudletPath(enums::profilePackagePathFileName, mProfileName, mModuleName); }

    static QByteArray readFile(const QString& path)
    {
        QFile file(path);
        if (!file.open(QFile::ReadOnly)) {
            return {};
        }
        return file.readAll();
    }

    static bool moduleWasWrittenOut(const QByteArray& xml) { return xml.contains("<HelpPackage"); }

    // Puts the module back the way installing it left it - both the unpacked XML and
    // the archive - so that "has the module been written out yet?" has an answer again
    // after an earlier save. Resetting only the unpacked XML would leave the archive
    // carrying the previous save's work, and every assertion about it vacuously true.
    bool resetModuleOnDisk() const
    {
        if (!writeArchive(mModuleArchivePath, qsl("%1.xml").arg(mModuleName), sourceModuleXml())) {
            return false;
        }
        QFile file(moduleXmlPath());
        if (!file.open(QFile::WriteOnly | QFile::Truncate)) {
            return false;
        }
        const QByteArray xml = sourceModuleXml();
        const bool written = file.write(xml) == xml.size();
        file.close();
        return written;
    }

    QStringList moduleBackupFiles() const { return QDir(mudlet::getMudletPath(enums::moduleBackupsPath)).entryList(QStringList{qsl("%1*").arg(mModuleName)}, QDir::Files); }

    // A backup is named after the second it was taken in, and QFile::copy() will not
    // overwrite, so counting backups across two saves in the same second proves
    // nothing. Clearing them first makes "was one taken?" a plain yes or no.
    void clearModuleBackups() const
    {
        QDir backups(mudlet::getMudletPath(enums::moduleBackupsPath));
        for (const auto& backup : moduleBackupFiles()) {
            backups.remove(backup);
        }
    }

    int profileOwnedWatchers() const { return mpHost->findChildren<QFutureWatcherBase*>(QString(), Qt::FindDirectChildrenOnly).count(); }

    // Occupies every thread the global pool has until it is let go. QThreadPool's own
    // reserveThread() is not enough - a thread woken by a newly queued task takes it
    // regardless of the reservation - so this holds the threads with actual work.
    class PoolBlocker : public QRunnable
    {
    public:
        PoolBlocker(QSemaphore* started, QSemaphore* release)
        : mpStarted(started)
        , mpRelease(release)
        {
            setAutoDelete(true);
        }
        void run() override
        {
            mpStarted->release();
            mpRelease->acquire();
        }

    private:
        QSemaphore* mpStarted = nullptr;
        QSemaphore* mpRelease = nullptr;
    };

    // Holds the pool so that a task queued after this cannot start until releasePool().
    // That is what turns "the profile was destroyed while the module write was still
    // going" from a race into something a test can state plainly.
    bool holdPool()
    {
        auto* pool = QThreadPool::globalInstance();
        mOriginalMaxPoolThreads = pool->maxThreadCount();
        // The module write waits on the profile XML save before it starts, so leave the
        // pool room to run both once they are let go. The count goes back only after
        // the pool has drained, so that room is actually there when they run.
        mHeldPoolThreads = qMax(4, mOriginalMaxPoolThreads);
        pool->setMaxThreadCount(mHeldPoolThreads);
        for (int i = 0; i < mHeldPoolThreads; ++i) {
            pool->start(new PoolBlocker(&mPoolBlockersStarted, &mPoolRelease));
        }
        // Every thread has to be taken before anything else is queued, or the save
        // below could still find one free. Bounded so that a pool thread left busy by
        // something else fails the test rather than wedging it until ctest's timeout.
        return mPoolBlockersStarted.tryAcquire(mHeldPoolThreads, 10000);
    }

    void releasePoolBlockers()
    {
        mPoolRelease.release(mHeldPoolThreads);
        mHeldPoolThreads = 0;
    }

    void restorePoolThreadCount()
    {
        if (mOriginalMaxPoolThreads) {
            QThreadPool::globalInstance()->setMaxThreadCount(mOriginalMaxPoolThreads);
            mOriginalMaxPoolThreads = 0;
        }
    }

    // Installs the fixture module and turns syncing on for it - only a synced module
    // is written out by a profile save, so only a synced one reaches the background
    // write this file is about.
    bool installSyncedModule()
    {
        if (!mArchiveDir.isValid()) {
            return false;
        }
        mModuleArchivePath = mArchiveDir.filePath(qsl("%1.mpackage").arg(mModuleName));
        if (!writeArchive(mModuleArchivePath, qsl("%1.xml").arg(mModuleName), sourceModuleXml())) {
            return false;
        }
        mpHost->waitForProfileSave(); // an install during a save is postponed and answered with a bare true
        auto [installed, message] = mpHost->installPackage(mModuleArchivePath, enums::PackageModuleType::ModuleFromScript, true);
        if (!installed) {
            qWarning().noquote() << "installing the fixture module failed:" << message;
            return false;
        }
        if (!mpHost->mModulesLoadedOk.contains(mModuleName)) {
            return false;
        }
        auto [synced, syncMessage] = mpHost->changeModuleSync(mModuleName, QLatin1String("1"));
        if (!synced) {
            qWarning().noquote() << "enabling sync on the fixture module failed:" << syncMessage;
            return false;
        }
        return true;
    }

    // Closes the profile the way a user does who has turned the "save profile on
    // exit" preference off and then answers "No" to "Save profile?". That branch of
    // TMainConsole::closeEvent() waits for nothing - which is the point: a module
    // write queued beforehand is still going once the close is over. (The close that
    // finds the main console already gone waits for nothing either, and needs no
    // dialog at all, but a profile can only be closed once, so one test gets one of
    // the two.)
    bool closeProfileWithoutSaving()
    {
        mpHost->mFORCE_SAVE_ON_EXIT = false;
        QTimer answerNo;
        int ticks = 0;
        connect(&answerNo, &QTimer::timeout, qApp, [&ticks]() {
            auto* modal = QApplication::activeModalWidget();
            if (!modal) {
                return;
            }
            if (auto* box = qobject_cast<QMessageBox*>(modal); box) {
                if (auto* no = box->button(QMessageBox::No); no) {
                    no->click();
                    return;
                }
            }
            // Whatever this dialog is, it is not the one expected, and requestClose()
            // is blocked in its event loop: shut it so the test can fail and say so
            // rather than hang until ctest gives up on it.
            if (++ticks > 40) {
                modal->close();
            }
        });
        answerNo.start(50ms);
        const bool closed = mpHost->requestClose();
        answerNo.stop();
        return closed;
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
        QVERIFY2(installSyncedModule(), "The fixture module could not be installed");
        // installing a module owes the profile a save; let it come and go
        QTRY_VERIFY_WITH_TIMEOUT(!mpHost->hasPendingProfileSave(), 5000);
        mpHost->waitForProfileSave();
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

    // The coverage this file exists for: with a synced module installed, a profile
    // save really does serialize it and really does rewrite its archive. Without a
    // module in the profile the whole background write returns immediately, and
    // nothing below it can go wrong in a way a test would notice.
    void test_aSyncedModuleIsWrittenOutOnSave()
    {
        QVERIFY2(resetModuleOnDisk(), "Could not put the module back the way it was installed");
        QVERIFY2(!moduleWasWrittenOut(readFile(moduleXmlPath())), "The module XML looked written out before the save");
        clearModuleBackups();

        auto [ok, filename, error] = mpHost->saveProfile();
        QVERIFY2(ok, qPrintable(error));
        mpHost->waitForProfileSave();

        const QByteArray writtenXml = readFile(moduleXmlPath());
        QVERIFY2(moduleWasWrittenOut(writtenXml), "The profile save did not write the module out");
        // The document handed to the background write is a copy taken element by
        // element, so check the parts that live outside the root element survived it -
        // a module XML without them is not one Mudlet can read back in.
        QVERIFY2(writtenXml.startsWith("<?xml"), "The written module XML lost its declaration");
        QVERIFY2(writtenXml.contains("<!DOCTYPE MudletPackage>"), "The written module XML lost its doctype");
        QVERIFY2(moduleWasWrittenOut(archiveEntry(mModuleArchivePath, qsl("%1.xml").arg(mModuleName))), "The profile save did not update the module's archive");
        QVERIFY2(!moduleBackupFiles().isEmpty(), "The profile save did not back the module up before overwriting it");
        // The watcher the save made has to go once it has reported, or a long-lived
        // profile collects one per save.
        QTRY_COMPARE(profileOwnedWatchers(), 0);
    }

    // ...and an autosave deliberately does not back the module up, or every autosave
    // tick would leave another timestamped copy of every synced module behind.
    void test_anAutosaveWritesTheModuleWithoutBackingItUp()
    {
        QVERIFY2(resetModuleOnDisk(), "Could not put the module back the way it was installed");
        clearModuleBackups();

        auto [ok, filename, error] = mpHost->saveProfile(QString(), qsl("autosave"));
        QVERIFY2(ok, qPrintable(error));
        mpHost->waitForProfileSave();

        QVERIFY2(moduleWasWrittenOut(readFile(moduleXmlPath())), "The autosave did not write the module out");
        QVERIFY2(moduleBackupFiles().isEmpty(), "The autosave backed the module up, which every tick of it would then do");
    }

    // ...and closing the profile while that write is still going may neither reach
    // the destroyed Host nor abandon the write. This one destroys the Host, so it has
    // to stay last: anything after it would run without a profile.
    void test_theModuleWriteOutlivesTheProfile()
    {
        QVERIFY2(resetModuleOnDisk(), "Could not put the module back the way it was installed");

        QVERIFY2(holdPool(), "The thread pool could not be held - a pool thread was busy with something else");
        auto releaseGuard = qScopeGuard([this]() {
            if (mHeldPoolThreads) {
                releasePoolBlockers();
            }
            restorePoolThreadCount();
        });

        auto [ok, filename, error] = mpHost->saveProfile();
        QVERIFY2(ok, qPrintable(error));

        // Checked after the teardown below, but they have to be taken hold of here.
        // Every watcher this save made is expected to belong to the profile, so that
        // destroying it takes them too; before the fix none of them did.
        QList<QPointer<QObject>> watcherGuards;
        for (auto* watcher : mpHost->findChildren<QFutureWatcherBase*>(QString(), Qt::FindDirectChildrenOnly)) {
            watcherGuards.append(QPointer<QObject>(watcher));
        }

        // If this fails the pool was not actually held, and there is no teardown
        // race left for the rest of the test to be about.
        QVERIFY2(!moduleWasWrittenOut(readFile(moduleXmlPath())), "The module write ran before the profile was closed - the thread pool was not held");

        QVERIFY2(closeProfileWithoutSaving(), "Closing the profile was refused");
        QVERIFY2(!moduleWasWrittenOut(readFile(moduleXmlPath())), "Closing the profile waited for the module write - this test needs a close that does not");

        mpHost = nullptr;
        mudlet::self()->getHostManager().deleteHost(mProfileName);

        // Now let the write run against a profile that is gone. Before the fix this
        // kills the run under AddressSanitizer, reaching through the destroyed Host
        // for its XMLexport and then reading its name.
        releasePoolBlockers();
        QThreadPool::globalInstance()->waitForDone();

        // Abandoning the write instead would be no fix: the module's changes would
        // be lost, and its archive left half-rewritten.
        QVERIFY2(moduleWasWrittenOut(readFile(moduleXmlPath())), "The module write was dropped when the profile went away");
        QVERIFY2(moduleWasWrittenOut(archiveEntry(mModuleArchivePath, qsl("%1.xml").arg(mModuleName))), "The module's archive was left un-updated when the profile went away");

        // Nothing but the profile owns these, so destroying it has to have taken them:
        // the deleteLater() they are also wired to needs an event loop that is still
        // running to be delivered, and on the way out there is not one.
        QVERIFY2(!watcherGuards.isEmpty(), "The save made no watcher the profile owns");
        for (const auto& watcherGuard : watcherGuards) {
            QVERIFY2(watcherGuard.isNull(), "A save watcher outlived the profile it belongs to, with nothing left to delete it");
        }
    }
};

#include "ModuleSaveTeardownTest.moc"
MUDLET_GROUPED_TEST_MAIN(ModuleSaveTeardownTest)
