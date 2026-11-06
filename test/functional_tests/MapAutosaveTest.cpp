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

#include <QDataStream>
#include <QFileInfo>
#include <QPointer>
#include <QSemaphore>
#include <QTemporaryDir>
#include <QThread>
#include <QThreadPool>
#include <QtTest/QtTest>

#include "Host.h"
#include "HostManager.h"
#include "MudletApp.h"
#include "MudletInstanceCoordinator.h"
#include "TMap.h"
#include "TRoom.h"
#include "TRoomDB.h"
#include "mudlet.h"

#include "GroupedTest.h"

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

    TMap* map() const { return mpHost->mpMap.data(); }

    void renameRoom(TMap* pMap, const QString& name) const
    {
        pMap->mpRoomDB->getRoom(1)->name = name;
        pMap->setUnsaved(__func__);
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
        QString explicitPath;
        {
            PoolHold hold;
            QVERIFY(hold.holding());
            mpHost->autoSaveMap();
            renameRoom(map(), qsl("Renamed after the autosave"));
            explicitPath = MudletApp::getMudletPath(enums::profileMapPathFileName, mProfileName, qsl("explicit.dat"));
            QVERIFY(mpHost->saveMapFile(explicitPath));
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
};

#include "MapAutosaveTest.moc"
MUDLET_GROUPED_TEST_MAIN(MapAutosaveTest)
