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
 * Covers the map autosave, which serialises the map on the main thread and
 * writes the file on a pool thread. The autosave runs off a timer measured in
 * minutes, with no Lua route to it, so the tests call Host::autoSaveMap().
 *
 * To catch a write while it is still running, every thread of the global pool
 * is held busy, which keeps the write queued until it is either released or
 * taken over by a wait on the main thread.
 *
 * Run with: ctest -R MapAutosaveTest -V
 */

#include <QAction>
#include <QDataStream>
#include <QFileInfo>
#include <QMenu>
#include <QPointer>
#include <QSemaphore>
#include <QTemporaryDir>
#include <QThread>
#include <QThreadPool>
#include <QTimer>
#include <QUrl>
#include <QtTest/QtTest>
#include <memory>

#include "Host.h"
#include "HostManager.h"
#include "MudletApp.h"
#include "MudletInstanceCoordinator.h"
#include "TMap.h"
#include "TRoom.h"
#include "TRoomDB.h"
#include "dlgProfilePreferences.h"
#include "mudlet.h"

#include "GroupedTest.h"

#if defined(Q_OS_UNIX)
#include <csignal>
#include <sys/resource.h>
#endif

using namespace std::chrono_literals;

namespace {
class PoolHold
{
public:
    PoolHold()
    : mThreads(QThreadPool::globalInstance()->maxThreadCount())
    {
        for (int i = 0; i < mThreads; ++i) {
            QThreadPool::globalInstance()->start([this]() {
                mStarted.release();
                // Bounded, so that a failing test cannot hang the run
                mRelease.tryAcquire(1, 30s);
            });
        }
    }
    ~PoolHold()
    {
        release();
        QThreadPool::globalInstance()->waitForDone();
    }
    bool holding() { return mStarted.tryAcquire(mThreads, 10s); }
    void release()
    {
        if (!mReleased) {
            mReleased = true;
            mRelease.release(mThreads);
        }
    }

private:
    const int mThreads;
    QSemaphore mStarted;
    QSemaphore mRelease;
    bool mReleased = false;
};
} // namespace

class MapAutosaveTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;
    Host* mpHost = nullptr;
    const QString mProfileName = qsl("MapAutosave-Test");
    const QString mClosingProfileName = qsl("MapAutosaveClosing-Test");
    const QString mCopyTargetProfileName = qsl("MapAutosaveCopyTarget-Test");

    static bool portableMarkerPresent()
    {
        return QFileInfo::exists(qsl("%1/portable.txt").arg(QCoreApplication::applicationDirPath())) || QFileInfo::exists(qsl("%1/.config/mudlet/portable.txt").arg(QDir::homePath()));
    }

    static void deleteProfileDirectory(const QString& profileName)
    {
        QDir dir(MudletApp::getMudletPath(enums::profileHomePath, profileName));
        if (dir.exists()) {
            dir.removeRecursively();
        }
    }

    static QString autosavePath(const QString& profileName) { return MudletApp::getMudletPath(enums::profileMapPathFileName, profileName, qsl("autosave.dat")); }

    static QByteArray readFile(const QString& pathFileName)
    {
        QFile file(pathFileName);
        return file.open(QIODevice::ReadOnly) ? file.readAll() : QByteArray();
    }

    // Serialised straight into memory, with no buffering of Mudlet's own in the way
    static QByteArray serialized(TMap* pMap)
    {
        QByteArray bytes;
        QDataStream out(&bytes, QIODevice::WriteOnly);
        out.setVersion(QDataStream::Qt_5_12);
        return pMap->serialize(out, 0) ? bytes : QByteArray();
    }

    // Large enough that the file crosses the 256 KiB chunks saveMapFile() writes in, and with one string
    // bigger than a chunk on its own
    static void buildMap(TMap* pMap)
    {
        pMap->mapClear();
        const int area = pMap->mpRoomDB->addArea(qsl("Autosave area"));
        QVERIFY(area > 0);
        for (int id = 1; id <= 2000; ++id) {
            QVERIFY(pMap->addRoom(id));
            QVERIFY(pMap->setRoomArea(id, area));
            QVERIFY(pMap->setRoomCoordinates(id, id % 50, id / 50, 0));
            pMap->mpRoomDB->getRoom(id)->name = qsl("Room %1 %2").arg(id).arg(QString(80, QChar(u'é')));
        }
        pMap->mUserData.insert(qsl("big"), QString(300'000, QChar(u'x')));
        pMap->setUnsaved(__func__);
    }

    // So that each write takes milliseconds, and two files written one after the other cannot share a time
    static void makeSlowToWrite(TMap* pMap) { pMap->mUserData.insert(qsl("bigger"), QString(10'000'000, QChar(u'x'))); }

    TMap* map() const { return mpHost->mpMap.data(); }

    void renameRoom(TMap* pMap, const QString& name) const
    {
        pMap->mpRoomDB->getRoom(1)->name = name;
        pMap->setUnsaved(__func__);
    }

    // Copies this profile's map to the target, whose autosave comes due the first time the dialog reports the
    // given progress. A copy lets events run as it goes, and the autosave runs off a timer.
    struct CopyRun
    {
        bool copied = false;
        bool autosaveDue = false;
        bool autosaveWrittenBeforeCopying = false;
    };
    void copyMapWithAutosaveDue(Host* pTarget, const QString& progress, CopyRun& run)
    {
        const std::unique_ptr<dlgProfilePreferences> pPreferences(new dlgProfilePreferences(mudlet::self(), mpHost));
        QAction* pTargetChoice = nullptr;
        for (auto* pAction : pPreferences->pushButton_chooseProfiles->menu()->actions()) {
            if (pAction->text() == pTarget->getName()) {
                pTargetChoice = pAction;
            }
        }
        QVERIFY(pTargetChoice);
        pTargetChoice->setChecked(true);
        QTimer ticker;
        bool sawCopying = false;
        connect(&ticker, &QTimer::timeout, this, [&]() {
            const QString shown = pPreferences->label_mapFileActionResult->text();
            if (!sawCopying && shown.startsWith(qsl("Copying over map to"))) {
                sawCopying = true;
                run.autosaveWrittenBeforeCopying = QFileInfo::exists(autosavePath(pTarget->getName()));
            }
            if (!run.autosaveDue && shown.startsWith(progress)) {
                run.autosaveDue = true;
                renameRoom(pTarget->mpMap.data(), qsl("Changed while copying"));
                pTarget->autoSaveMap();
            }
        });
        ticker.start(0);
        PoolHold hold;
        QVERIFY(hold.holding());
        run.copied = QMetaObject::invokeMethod(pPreferences.get(), "slot_copyMap");
        ticker.stop();
        QThread::sleep(50ms);
    }

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

        mudlet::start();
        mudlet::self()->setupConfig();
        QCOMPARE(MudletApp::getMudletPath(enums::mainPath), qsl("%1/mudlet").arg(mConfigDir.path()));
        mudlet::self()->takeOwnershipOfInstanceCoordinator(std::make_unique<MudletInstanceCoordinator>("MudletInstanceCoordinator"));
        mudlet::self()->init();
        mudlet::self()->setStorePasswordsSecurely(false);
        deleteProfileDirectory(mProfileName);
        deleteProfileDirectory(mClosingProfileName);
        deleteProfileDirectory(mCopyTargetProfileName);

        QVERIFY2(HostManager::self()->addHost(mProfileName, qsl("23"), QString(), QString()), "failed to create the test Host");
        mpHost = HostManager::self()->getHost(mProfileName);
        QVERIFY(mpHost);
    }

    void cleanupTestCase()
    {
        mpHost = nullptr;
        if (mudlet::self()) {
            deleteProfileDirectory(mProfileName);
            deleteProfileDirectory(mClosingProfileName);
            deleteProfileDirectory(mCopyTargetProfileName);
            delete mudlet::self();
        }
        mSavedXdg.isNull() ? qunsetenv("XDG_CONFIG_HOME") : qputenv("XDG_CONFIG_HOME", mSavedXdg);
    }

    void init()
    {
        mpHost->waitForMapAutosave();
        QDir dir(MudletApp::getMudletPath(enums::profileMapsPath, mProfileName));
        if (dir.exists()) {
            QVERIFY(dir.removeRecursively());
        }
        map()->setSaveError(false);
    }

    void test_autosaveWritesWhatSaveMapWrites()
    {
        buildMap(map());
        if (QTest::currentTestFailed()) {
            return;
        }
        const QByteArray expected = serialized(map());
        QVERIFY(expected.size() > 1024 * 1024);

        mpHost->autoSaveMap();
        QVERIFY(!map()->isUnsaved());
        QTRY_VERIFY(QFileInfo::exists(autosavePath(mProfileName)));
        QCOMPARE(readFile(autosavePath(mProfileName)), expected);

        const QString explicitPath = MudletApp::getMudletPath(enums::profileMapPathFileName, mProfileName, qsl("explicit.dat"));
        QVERIFY(mpHost->saveMapFile(explicitPath));
        QCOMPARE(readFile(explicitPath), expected);
        QVERIFY(!map()->hasSaveError());
    }

    // The newest map file is the one a profile loads, so a save made while an autosave is still writing must not
    // end up older than the autosave of what came before it
    void test_saveMadeDuringAnAutosaveWriteIsTheNewerFile()
    {
        buildMap(map());
        if (QTest::currentTestFailed()) {
            return;
        }
        makeSlowToWrite(map());
        QString explicitPath;
        {
            PoolHold hold;
            QVERIFY(hold.holding());
            mpHost->autoSaveMap();
            renameRoom(map(), qsl("Renamed after the autosave"));
            explicitPath = MudletApp::getMudletPath(enums::profileMapPathFileName, mProfileName, qsl("explicit.dat"));
            QVERIFY(mpHost->saveMapFile(explicitPath));
            QVERIFY2(QFileInfo::exists(autosavePath(mProfileName)), "the map was saved before the autosave had been written");
            QThread::sleep(50ms);
        }
        mpHost->waitForMapAutosave();

        const QDateTime autosaved = QFileInfo(autosavePath(mProfileName)).lastModified();
        const QDateTime saved = QFileInfo(explicitPath).lastModified();
        QVERIFY(autosaved.isValid());
        QVERIFY2(autosaved <= saved, qPrintable(qsl("the autosave (%1) landed after the later save (%2)").arg(autosaved.toString(Qt::ISODateWithMs), saved.toString(Qt::ISODateWithMs))));
    }

    void test_loadMadeDuringAnAutosaveWriteReadsIt()
    {
        buildMap(map());
        if (QTest::currentTestFailed()) {
            return;
        }
        renameRoom(map(), qsl("Only in the autosave"));
        {
            PoolHold hold;
            QVERIFY(hold.holding());
            mpHost->autoSaveMap();
            renameRoom(map(), qsl("Never saved"));
            QVERIFY(map()->restore(QString()));
        }
        QCOMPARE(map()->mpRoomDB->getRoom(1)->name, qsl("Only in the autosave"));
    }

    void test_secondAutosaveDuringAWriteIsPutOff()
    {
        buildMap(map());
        if (QTest::currentTestFailed()) {
            return;
        }
        {
            PoolHold hold;
            QVERIFY(hold.holding());
            mpHost->autoSaveMap();
            QVERIFY(!map()->isUnsaved());
            renameRoom(map(), qsl("Changed during the write"));
            mpHost->autoSaveMap();
            QVERIFY2(map()->isUnsaved(), "a second autosave started while the first was still writing");
        }
        mpHost->waitForMapAutosave();
        QVERIFY(map()->isUnsaved());

        const QByteArray expected = serialized(map());
        mpHost->autoSaveMap();
        mpHost->waitForMapAutosave();
        QVERIFY(!map()->isUnsaved());
        QCOMPARE(readFile(autosavePath(mProfileName)), expected);
    }

    void test_closingDuringAnAutosaveWriteLeavesTheWholeFile()
    {
        QVERIFY(HostManager::self()->addHost(mClosingProfileName, qsl("23"), QString(), QString()));
        QPointer<Host> closing = HostManager::self()->getHost(mClosingProfileName);
        QVERIFY(closing);
        buildMap(closing->mpMap.data());
        if (QTest::currentTestFailed()) {
            return;
        }
        const QByteArray expected = serialized(closing->mpMap.data());

        PoolHold hold;
        QVERIFY(hold.holding());
        closing->autoSaveMap();
        HostManager::self()->deleteHost(mClosingProfileName);
        QVERIFY2(closing.isNull(), "the closing profile outlived its deletion");
        QCOMPARE(readFile(autosavePath(mClosingProfileName)), expected);
    }

    void test_failedAutosaveWriteIsReportedAndRetried()
    {
        buildMap(map());
        if (QTest::currentTestFailed()) {
            return;
        }
        // A directory in its place fails the write whatever the file permissions, root included
        QVERIFY(QDir().mkpath(autosavePath(mProfileName)));

        mpHost->autoSaveMap();
        QTRY_VERIFY(map()->hasSaveError());
        QVERIFY2(map()->isUnsaved(), "a failed autosave left the map marked as saved");

        QVERIFY(QDir(autosavePath(mProfileName)).removeRecursively());
        mpHost->autoSaveMap();
        QTRY_VERIFY(!map()->hasSaveError());
        QVERIFY(!map()->isUnsaved());
        QCOMPARE(readFile(autosavePath(mProfileName)), serialized(map()));
    }

    // Each autosave is reported once, so a save made since must survive the next wait
    void test_autosaveFailureIsNotReportedAgainAfterASave()
    {
        buildMap(map());
        if (QTest::currentTestFailed()) {
            return;
        }
        QVERIFY(QDir().mkpath(autosavePath(mProfileName)));
        mpHost->autoSaveMap();
        mpHost->waitForMapAutosave();
        QVERIFY(map()->hasSaveError());

        QVERIFY(mpHost->saveMapFile(MudletApp::getMudletPath(enums::profileMapPathFileName, mProfileName, qsl("explicit.dat"))));
        mpHost->waitForMapAutosave();
        QVERIFY(!map()->hasSaveError());
        QVERIFY(!map()->isUnsaved());
    }

    // A downloaded map is loaded as already saved, so the autosave of the map it replaced must not land after it
    void test_downloadMadeDuringAnAutosaveWriteIsTheNewerFile()
    {
        buildMap(map());
        if (QTest::currentTestFailed()) {
            return;
        }
        makeSlowToWrite(map());
        renameRoom(map(), qsl("Downloaded"));
        const QString served = qsl("%1/served map.dat").arg(mConfigDir.path());
        {
            QFile file(served);
            QVERIFY(file.open(QIODevice::WriteOnly));
            QVERIFY(file.write(serialized(map())) > 0);
        }
        renameRoom(map(), qsl("Replaced by the download"));
        {
            PoolHold hold;
            QVERIFY(hold.holding());
            mpHost->autoSaveMap();
            map()->downloadMap(QUrl::fromLocalFile(served).toString(), QString());
            QTRY_VERIFY(map()->mpRoomDB->getRoom(1) && map()->mpRoomDB->getRoom(1)->name == qsl("Downloaded"));
        }
        mpHost->waitForMapAutosave();

        const QDateTime autosaved = QFileInfo(autosavePath(mProfileName)).lastModified();
        const QDateTime downloaded = QFileInfo(MudletApp::getMudletPath(enums::profileMapPathFileName, mProfileName, qsl("map.dat"))).lastModified();
        QVERIFY(autosaved.isValid());
        QVERIFY2(autosaved <= downloaded, qPrintable(qsl("the autosave (%1) landed after the download (%2)").arg(autosaved.toString(Qt::ISODateWithMs), downloaded.toString(Qt::ISODateWithMs))));
    }

    // On Windows and macOS a copy keeps the time of the map saved to make it, so that save has to come after the
    // target's autosave
    void test_mapCopiedToAProfileDuringItsAutosaveWriteIsTheNewerFile()
    {
        // Map files are named to the second, and a copy would not replace one an earlier test left
        deleteProfileDirectory(mCopyTargetProfileName);
        QVERIFY(HostManager::self()->addHost(mCopyTargetProfileName, qsl("23"), QString(), QString()));
        QPointer<Host> target = HostManager::self()->getHost(mCopyTargetProfileName);
        QVERIFY(target);
        buildMap(target->mpMap.data());
        buildMap(map());
        if (QTest::currentTestFailed()) {
            return;
        }
        makeSlowToWrite(target->mpMap.data());
        makeSlowToWrite(map());
        renameRoom(map(), qsl("Copied from the source"));
        CopyRun run;
        copyMapWithAutosaveDue(target, qsl("Backing up current map"), run);
        target->waitForMapAutosave();
        const QFileInfoList saves = QDir(MudletApp::getMudletPath(enums::profileMapsPath, mProfileName)).entryInfoList({qsl("*.dat")}, QDir::Files, QDir::Time);
        const QDateTime autosaved = QFileInfo(autosavePath(mCopyTargetProfileName)).lastModified();
        const bool restored = target->mpMap->restore(QString());
        const TRoom* pRoom = target->mpMap->mpRoomDB->getRoom(1);
        const QString loaded = pRoom ? pRoom->name : QString();
        HostManager::self()->deleteHost(mCopyTargetProfileName);

        QVERIFY(run.copied);
        QVERIFY(run.autosaveDue);
        QVERIFY2(run.autosaveWrittenBeforeCopying, "the map to copy was saved before the target's autosave had been written");
        QVERIFY(!saves.isEmpty());
        QVERIFY(autosaved.isValid());
        // Of two files with the same time, the dated one sorts first
        QVERIFY2(autosaved <= saves.first().lastModified(),
                 qPrintable(qsl("the target's autosave (%1) was not older than the map saved to copy (%2)")
                                    .arg(autosaved.toString(Qt::ISODateWithMs), saves.first().lastModified().toString(Qt::ISODateWithMs))));
        QVERIFY(restored);
        QCOMPARE(loaded, qsl("Copied from the source"));
    }

#if defined(Q_OS_LINUX)
    // Linux only: on Windows and macOS a copy keeps its source's time, so the autosave is newer whichever is written last
    void test_autosaveStartedWhileAMapIsCopiedToItsProfileIsOlderThanTheCopy()
    {
        deleteProfileDirectory(mCopyTargetProfileName);
        QVERIFY(HostManager::self()->addHost(mCopyTargetProfileName, qsl("23"), QString(), QString()));
        QPointer<Host> target = HostManager::self()->getHost(mCopyTargetProfileName);
        QVERIFY(target);
        buildMap(target->mpMap.data());
        buildMap(map());
        if (QTest::currentTestFailed()) {
            return;
        }
        makeSlowToWrite(target->mpMap.data());
        renameRoom(map(), qsl("Copied from the source"));
        CopyRun run;
        copyMapWithAutosaveDue(target, qsl("Copying over map to"), run);
        target->waitForMapAutosave();
        const bool restored = target->mpMap->restore(QString());
        const TRoom* pRoom = target->mpMap->mpRoomDB->getRoom(1);
        const QString loaded = pRoom ? pRoom->name : QString();
        HostManager::self()->deleteHost(mCopyTargetProfileName);

        QVERIFY(run.copied);
        QVERIFY(run.autosaveDue);
        QVERIFY(restored);
        QCOMPARE(loaded, qsl("Copied from the source"));
    }
#endif

    // A map copy reads where the player is in each profile it copies to from that profile's newest map file
    void test_mapFileStatsReadDuringAnAutosaveWriteAreTheAutosaves()
    {
        deleteProfileDirectory(mCopyTargetProfileName);
        QVERIFY(HostManager::self()->addHost(mCopyTargetProfileName, qsl("23"), QString(), QString()));
        QPointer<Host> target = HostManager::self()->getHost(mCopyTargetProfileName);
        QVERIFY(target);
        buildMap(target->mpMap.data());
        if (QTest::currentTestFailed()) {
            return;
        }
        makeSlowToWrite(target->mpMap.data());
        target->mpMap->mRoomIdHash[mCopyTargetProfileName] = 1;
        QVERIFY(target->saveMapFile(QString()));
        target->mpMap->mRoomIdHash[mCopyTargetProfileName] = 2;
        target->mpMap->setUnsaved(__func__);
        QString file;
        int version = 0;
        int room = 0;
        qsizetype areas = 0;
        qsizetype rooms = 0;
        bool read = false;
        {
            PoolHold hold;
            QVERIFY(hold.holding());
            target->autoSaveMap();
            read = map()->retrieveMapFileStats(mCopyTargetProfileName, &file, &version, &room, &areas, &rooms);
        }
        target->waitForMapAutosave();
        HostManager::self()->deleteHost(mCopyTargetProfileName);

        QVERIFY(read);
        QCOMPARE(QFileInfo(file).fileName(), qsl("autosave.dat"));
        QCOMPARE(room, 2);
    }

    // Until it has been written, an autosave is missing from the map folder, or there under a temporary name
    void test_mapHistoryFilledDuringAnAutosaveWriteListsTheAutosave()
    {
        buildMap(map());
        if (QTest::currentTestFailed()) {
            return;
        }
        QFile::remove(autosavePath(mProfileName));
        QStringList listed;
        {
            PoolHold hold;
            QVERIFY(hold.holding());
            mpHost->autoSaveMap();
            auto* pPreferences = new dlgProfilePreferences(mudlet::self(), mpHost);
            for (int i = 0; i < pPreferences->comboBox_mapHistory->count(); ++i) {
                listed << QFileInfo(pPreferences->comboBox_mapHistory->itemData(i).toString()).fileName();
            }
            delete pPreferences;
        }
        mpHost->waitForMapAutosave();

        QVERIFY2(listed.contains(qsl("autosave.dat")), qPrintable(listed.join(qsl(", "))));
    }

    // A script's handler can close a profile and open it again while something still holds the closed one
    void test_profileOpenedAgainWhileTheClosedOneIsHeldLoadsItsAutosave()
    {
        QVERIFY(HostManager::self()->addHost(mClosingProfileName, qsl("23"), QString(), QString()));
        QSharedPointer<Host> held;
        for (const auto& pHost : HostManager::self()->hostList()) {
            if (pHost->getName() == mClosingProfileName) {
                held = pHost;
            }
        }
        QVERIFY(held);
        buildMap(held->mpMap.data());
        if (QTest::currentTestFailed()) {
            return;
        }
        makeSlowToWrite(held->mpMap.data());
        QVERIFY(held->saveMapFile(QString()));
        renameRoom(held->mpMap.data(), qsl("Only in the autosave"));
        QString loaded;
        bool restored = false;
        {
            PoolHold hold;
            QVERIFY(hold.holding());
            held->autoSaveMap();
            HostManager::self()->deleteHost(mClosingProfileName);
            QVERIFY(HostManager::self()->addHost(mClosingProfileName, qsl("23"), QString(), QString()));
            Host* pReopened = HostManager::self()->getHost(mClosingProfileName);
            QVERIFY(pReopened);
            restored = pReopened->mpMap->restore(QString());
            const TRoom* pRoom = pReopened->mpMap->mpRoomDB->getRoom(1);
            loaded = pRoom ? pRoom->name : QString();
        }
        held.reset();
        HostManager::self()->deleteHost(mClosingProfileName);

        QVERIFY(restored);
        QCOMPARE(loaded, qsl("Only in the autosave"));
    }

    // Something can hold a closed profile long enough for its autosave timer to fire again
    void test_heldClosedProfileFinishesItsAutosaveWhenReleased()
    {
        QVERIFY(HostManager::self()->addHost(mClosingProfileName, qsl("23"), QString(), QString()));
        QSharedPointer<Host> held;
        for (const auto& pHost : HostManager::self()->hostList()) {
            if (pHost->getName() == mClosingProfileName) {
                held = pHost;
            }
        }
        QVERIFY(held);
        buildMap(held->mpMap.data());
        if (QTest::currentTestFailed()) {
            return;
        }
        HostManager::self()->deleteHost(mClosingProfileName);
        renameRoom(held->mpMap.data(), qsl("Changed after closing"));
        const QByteArray expected = serialized(held->mpMap.data());

        PoolHold hold;
        QVERIFY(hold.holding());
        held->autoSaveMap();
        held.reset();
        QCOMPARE(readFile(autosavePath(mClosingProfileName)), expected);
    }

    void test_jsonExportMadeDuringAnAutosaveWriteIsTheNewerFile()
    {
        buildMap(map());
        if (QTest::currentTestFailed()) {
            return;
        }
        makeSlowToWrite(map());
        std::pair<bool, QString> exported;
        {
            PoolHold hold;
            QVERIFY(hold.holding());
            mpHost->autoSaveMap();
            exported = map()->writeJsonMapFile(QString());
            QThread::sleep(50ms);
        }
        mpHost->waitForMapAutosave();
        const QFileInfoList exports = QDir(MudletApp::getMudletPath(enums::profileMapsPath, mProfileName)).entryInfoList({qsl("*.json")}, QDir::Files, QDir::Time);
        const QDateTime autosaved = QFileInfo(autosavePath(mProfileName)).lastModified();

        QVERIFY2(exported.first, qPrintable(exported.second));
        QVERIFY(!exports.isEmpty());
        QVERIFY(autosaved.isValid());
        QVERIFY2(autosaved <= exports.first().lastModified(),
                 qPrintable(qsl("the autosave (%1) landed after the export (%2)").arg(autosaved.toString(Qt::ISODateWithMs), exports.first().lastModified().toString(Qt::ISODateWithMs))));
    }

#if defined(Q_OS_UNIX)
    void test_writeCutShortIsReported_data()
    {
        QTest::addColumn<qint64>("bytesBeforeTheEnd");
        // Inside the user data value bigger than a chunk, which is written straight through
        QTest::newRow("in a direct write") << qint64(-1);
        // Inside the last chunk, after which nothing else is written that could fail
        QTest::newRow("in the last chunk") << qint64(1024);
    }

    // A full disk cuts a write short without an error that QSaveFile would see, so it would commit what it got
    void test_writeCutShortIsReported()
    {
        QFETCH(qint64, bytesBeforeTheEnd);
        buildMap(map());
        if (QTest::currentTestFailed()) {
            return;
        }
        const QString path = MudletApp::getMudletPath(enums::profileMapPathFileName, mProfileName, qsl("cut short.dat"));
        QVERIFY(QDir().mkpath(QFileInfo(path).absolutePath()));
        {
            QFile old(path);
            QVERIFY(old.open(QIODevice::WriteOnly));
            QCOMPARE(old.write("OLD"), 3);
        }
        rlimit uncapped{};
        QCOMPARE(getrlimit(RLIMIT_FSIZE, &uncapped), 0);
        rlimit capped = uncapped;
        capped.rlim_cur = bytesBeforeTheEnd < 0 ? 512 * 1024 : serialized(map()).size() - bytesBeforeTheEnd;
        const auto previousHandler = std::signal(SIGXFSZ, SIG_IGN);
        const bool cappedOk = setrlimit(RLIMIT_FSIZE, &capped) == 0;
        bool saved = true;
        if (cappedOk) {
            saved = mpHost->saveMapFile(path);
            mpHost->autoSaveMap();
            mpHost->waitForMapAutosave();
            setrlimit(RLIMIT_FSIZE, &uncapped);
        }
        std::signal(SIGXFSZ, previousHandler);

        QVERIFY(cappedOk);
        QVERIFY2(!saved, "a save that was cut short reported success");
        QCOMPARE(readFile(path), QByteArray("OLD"));
        QVERIFY(!QFileInfo::exists(autosavePath(mProfileName)));
        QVERIFY(map()->hasSaveError());
        QVERIFY2(map()->isUnsaved(), "a save that was cut short left the map marked as saved");
    }
#endif
};

#include "MapAutosaveTest.moc"
MUDLET_GROUPED_TEST_MAIN(MapAutosaveTest)
