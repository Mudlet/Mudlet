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
 * Regression guard for #9520: closing a profile while its map was being
 * imported or exported used to free the TMap while its own loop was still
 * running.
 *
 * Mudlet is single threaded, so nothing here is a race. The interleaving is
 * re-entrancy: TMap::readJsonMapFile() and TMap::writeJsonMapFile() call
 * qApp->processEvents() once per area to keep their progress display alive and
 * its Abort button clickable, and that pump delivers whatever else the event
 * loop is holding - including the zero-millisecond timer that
 * mudlet::slot_closeProfileByName() posts to run mudlet::closeHost(). That call
 * takes the profile's QSharedPointer<Host> out of the host pool, which destroys
 * the Host and, with it, the TMap whose loop is still on the stack. Everything
 * the reader touches after that is freed memory.
 *
 * These tests stage exactly that, through the same public slot the tab close
 * and closeProfile() use, and let the operation's own pump deliver the timer. A
 * QPointer to the map is how they tell: it goes null the moment the TMap is
 * destroyed, so the failure is reported rather than left to whatever the freed
 * memory happens to hold. Without the fix that assertion fails - and under ASan
 * the run additionally reports the use-after-free that follows it.
 *
 * The same pump also runs a script's deleteArea() or deleteMap() under an
 * export, which walks the live areas: the export has to stop rather than carry
 * on over the area it was writing.
 *
 * Run with: ctest -R MapCloseDuringImportTest -V
 */

#include <QtTest/QtTest>

#include <QDeadlineTimer>
#include <QPointer>
#include <QTemporaryDir>

#include <chrono>

#include "PortableModeTestHelper.h"
#include "Host.h"
#include "HostManager.h"
#include "MudletApp.h"
#include "MudletInstanceCoordinator.h"
#include "TArea.h"
#include "TMap.h"
#include "TRoomDB.h"
#include "mudlet.h"

#include "GroupedTest.h"

using namespace std::chrono_literals;

class MapCloseDuringImportTest : public QObject
{
    Q_OBJECT

private:
    const QString mSourceName = qsl("MapCloseDuringImportSource-Test");
    // A name of its own per test: a test that fails part way through can leave
    // its deferred close pending on a timer, and a later test reusing the name
    // would have that close land on its profile instead.
    const QString mImportTargetName = qsl("MapCloseDuringImportTarget-Test");
    const QString mExportTargetName = qsl("MapCloseDuringExportTarget-Test");
    QTemporaryDir mConfigDir;
    QTemporaryDir mSaveDir;
    QByteArray mSavedXdg;
    QString mMapFile;

    // Enough areas that the operation pumps the event loop many times over: the
    // progress increment that delivers the close is reached once per area.
    static constexpr int areaCount = 40;

    void buildMap(Host* pHost)
    {
        TMap* pMap = pHost->mpMap.data();
        TRoomDB* pDB = pMap->mpRoomDB.get();
        int roomId = 1;
        for (int area = 0; area < areaCount; ++area) {
            const int areaId = pDB->addArea(qsl("Area %1").arg(area));
            QVERIFY(areaId > 0);
            for (int room = 0; room < 5; ++room, ++roomId) {
                QVERIFY(pMap->addRoom(roomId));
                QVERIFY(pMap->setRoomArea(roomId, areaId));
                QVERIFY(pMap->setRoomCoordinates(roomId, room, area, 0));
            }
        }
    }

    Host* addProfile(const QString& name)
    {
        auto* hostManager = HostManager::self();
        if (!hostManager->addHost(name, qsl("23"), QString(), QString())) {
            return nullptr;
        }
        return hostManager->getHost(name);
    }

    static bool setModified(const QString& path, const QDateTime& when)
    {
        QFile file(path);
        return file.open(QIODevice::ReadWrite) && file.setFileTime(when, QFileDevice::FileModificationTime);
    }

    // A map folder with a binary map that loads, behind either the JSON one
    // from initTestCase(), which loads too, or ones a newer Mudlet wrote, which do not
    bool fillMapFolder(const QString& profileName, const QStringList& tooNewFiles = {})
    {
        const QString folder = MudletApp::getMudletPath(enums::profileMapsPath, profileName);
        Host* pSource = HostManager::self()->getHost(mSourceName);
        const QDateTime now = QDateTime::currentDateTime();
        if (!QDir().mkpath(folder) || !pSource || !pSource->saveMapFile(qsl("%1/older.dat").arg(folder)) || !setModified(qsl("%1/older.dat").arg(folder), now.addSecs(-600))) {
            return false;
        }
        if (tooNewFiles.isEmpty()) {
            return QFile::copy(mMapFile, qsl("%1/newest.json").arg(folder)) && setModified(qsl("%1/newest.json").arg(folder), now.addSecs(-60));
        }
        // A format version beyond any this Mudlet reads, so each is refused on its first four bytes
        int age = 60;
        for (const QString& fileName : tooNewFiles) {
            QFile file(qsl("%1/%2").arg(folder, fileName));
            if (!file.open(QIODevice::WriteOnly) || file.write(QByteArray::fromHex("0000007f")) != 4) {
                return false;
            }
            file.close();
            if (!setModified(file.fileName(), now.addSecs(-age))) {
                return false;
            }
            age += 60;
        }
        return true;
    }

    // Runs the event loop until the profile is gone. The close is deferred
    // until the map operation has unwound, so this is where it lands.
    bool waitForProfileToClose(const QString& name)
    {
        QDeadlineTimer deadline(10s);
        while (HostManager::self()->getHost(name)) {
            if (deadline.hasExpired()) {
                return false;
            }
            qApp->processEvents(QEventLoop::AllEvents, 20);
        }
        return true;
    }

private slots:
    void initTestCase()
    {
        if (portableMarkerPresent()) {
            QSKIP("portable.txt present - it takes precedence over XDG_CONFIG_HOME, so the config dir cannot be redirected");
        }

        QVERIFY(mConfigDir.isValid());
        QVERIFY(mSaveDir.isValid());
        mSavedXdg = qgetenv("XDG_CONFIG_HOME");
        QVERIFY(QDir().mkpath(qsl("%1/mudlet/profiles").arg(mConfigDir.path())));
        qputenv("XDG_CONFIG_HOME", mConfigDir.path().toUtf8());

        mudlet::start();
        mudlet::self()->setupConfig();
        mudlet::self()->takeOwnershipOfInstanceCoordinator(std::make_unique<MudletInstanceCoordinator>("MudletInstanceCoordinator"));
        mudlet::self()->init();
        mudlet::self()->setStorePasswordsSecurely(false);

        // Kept for the whole run, so that closing the profile under test never
        // leaves Mudlet with no profiles at all and popping its connection
        // dialog at an offscreen test.
        Host* pSource = addProfile(mSourceName);
        QVERIFY2(pSource, "failed to create the source Host");
        buildMap(pSource);
        if (QTest::currentTestFailed()) {
            return;
        }

        mMapFile = qsl("%1/close-during-import.json").arg(mSaveDir.path());
        const auto [wrote, writeMessage] = pSource->mpMap->writeJsonMapFile(mMapFile);
        QVERIFY2(wrote, qPrintable(writeMessage));
    }

    void cleanupTestCase()
    {
        delete mudlet::self();
        mSavedXdg.isNull() ? qunsetenv("XDG_CONFIG_HOME") : qputenv("XDG_CONFIG_HOME", mSavedXdg);
    }

    void test_closingTheProfileDuringAJsonImportDoesNotFreeTheMap()
    {
        Host* pTarget = addProfile(mImportTargetName);
        QVERIFY2(pTarget, "failed to create the target Host");
        TMap* pTargetMap = pTarget->mpMap.data();
        const QPointer<TMap> mapWatch(pTargetMap);

        bool closeRequested = false;
        const QMetaObject::Connection closeOnProgress = connect(pTargetMap, &TMap::signal_mapProgressSetValue, pTargetMap, [&]() {
            if (closeRequested) {
                return;
            }
            closeRequested = true;
            // The same slot the tab's close button and closeProfile() use: it
            // posts closeHost() as a zero-millisecond timer, which the import's
            // own processEvents() then delivers with the import on the stack.
            mudlet::self()->slot_closeProfileByName(mImportTargetName);
        });
        const auto [read, readMessage] = pTargetMap->readJsonMapFile(mMapFile);
        disconnect(closeOnProgress);

        QVERIFY2(closeRequested, "the import never announced any progress, so no close was delivered into its pump");
        QVERIFY2(!mapWatch.isNull(), "the TMap was destroyed while its own import loop was still on the stack");
        // A close asked for mid-import stops it rather than reading a whole map
        // into a profile that is going away:
        QVERIFY2(!read, "the import was expected to stop once the close asked it to");
        QCOMPARE(readMessage, qsl("aborted by user"));
        // ...and deferring the close must not drop it:
        QVERIFY2(waitForProfileToClose(mImportTargetName), "the deferred close never completed once the import had unwound");
        QVERIFY2(mapWatch.isNull(), "the TMap outlived the profile it belongs to");
    }

    // The export half of the same loop, which pumps the event loop the same way.
    void test_closingTheProfileDuringAJsonExportDoesNotFreeTheMap()
    {
        Host* pTarget = addProfile(mExportTargetName);
        QVERIFY2(pTarget, "failed to create the target Host");
        TMap* pTargetMap = pTarget->mpMap.data();
        buildMap(pTarget);
        if (QTest::currentTestFailed()) {
            return;
        }
        const QPointer<TMap> mapWatch(pTargetMap);

        bool closeRequested = false;
        const QMetaObject::Connection closeOnProgress = connect(pTargetMap, &TMap::signal_mapProgressSetValue, pTargetMap, [&]() {
            if (closeRequested) {
                return;
            }
            closeRequested = true;
            mudlet::self()->slot_closeProfileByName(mExportTargetName);
        });
        const auto [wrote, writeMessage] = pTargetMap->writeJsonMapFile(qsl("%1/close-during-export.json").arg(mSaveDir.path()));
        disconnect(closeOnProgress);

        QVERIFY2(closeRequested, "the export never announced any progress, so no close was delivered into its pump");
        QVERIFY2(!mapWatch.isNull(), "the TMap was destroyed while its own export loop was still on the stack");
        // As with the import: the close stops the operation rather than writing
        // a whole map out of a profile that is going away.
        QVERIFY2(!wrote, "the export was expected to stop once the close asked it to");
        QCOMPARE(writeMessage, qsl("aborted by user"));
        QVERIFY2(waitForProfileToClose(mExportTargetName), "the deferred close never completed once the export had unwound");
        QVERIFY2(mapWatch.isNull(), "the TMap outlived the profile it belongs to");
    }

    // With no file named, as at profile start, restore() falls back to an
    // older map file when the newest does not load. A JSON one the user
    // aborted, or a closing profile stopped, is not damaged, and an older map
    // is not what either asked for.
    void test_stoppingTheNewestJsonMapDoesNotFallBack_data()
    {
        QTest::addColumn<QString>("how");
        QTest::newRow("abort") << qsl("abort");
        QTest::newRow("close") << qsl("close");
    }

    void test_stoppingTheNewestJsonMapDoesNotFallBack()
    {
        QFETCH(QString, how);
        const QString name = qsl("MapFallbackStopped-%1-Test").arg(how);
        Host* pTarget = addProfile(name);
        QVERIFY2(pTarget, "failed to create the target Host");
        QVERIFY2(fillMapFolder(name), "failed to write the profile's map files");
        TMap* pMap = pTarget->mpMap.data();
        // The file has no user data, so a load that got as far as the swap would leave none
        pMap->mUserData.insert(qsl("kept"), qsl("from the map before the load"));

        bool stopped = false;
        const QMetaObject::Connection stopOnProgress = connect(pMap, &TMap::signal_mapProgressSetValue, pMap, [&]() {
            if (stopped) {
                return;
            }
            stopped = true;
            if (how == qsl("abort")) {
                // What the progress dialog's Abort button runs
                pMap->slot_mapProgressDialogCancelled();
            } else {
                mudlet::self()->slot_closeProfileByName(name);
            }
        });
        const bool loaded = pMap->restore(QString());
        disconnect(stopOnProgress);

        QVERIFY2(stopped, "the JSON load never announced any progress, so it was never stopped");
        QVERIFY2(!loaded, "restore() reported loading a map after the newest map file was stopped");
        QVERIFY2(pMap->mpRoomDB->isEmpty(), "restore() went on to load the older map file after the newest one was stopped");
        QVERIFY2(pMap->mUserData.value(qsl("kept")) == qsl("from the map before the load"), "the stopped JSON load replaced the map user data");
        if (how == qsl("close")) {
            QVERIFY2(waitForProfileToClose(name), "the deferred close never completed once restore() had unwound");
        }
    }

    // A close that lands between two map files that would not load stops
    // restore() trying the rest, rather than holding the close up until it has
    void test_closingTheProfileStopsTheFallbackBetweenFiles()
    {
        const QString name = qsl("MapFallbackClosed-Test");
        Host* pTarget = addProfile(name);
        QVERIFY2(pTarget, "failed to create the target Host");
        QVERIFY2(fillMapFolder(name, {qsl("new1.dat"), qsl("new2.dat"), qsl("new3.dat")}), "failed to write the profile's map files");
        TMap* pMap = pTarget->mpMap.data();

        // Delivered by the processEvents() restore() runs after the first file fails
        QTimer::singleShot(0, pMap, [name]() {
            mudlet::self()->slot_closeProfileByName(name);
        });
        const bool loaded = pMap->restore(QString());

        QVERIFY2(!loaded, "restore() went on to load the older map file after the profile was asked to close");
        QVERIFY2(pMap->mpRoomDB->isEmpty(), "restore() went on to load the older map file after the profile was asked to close");
        QVERIFY2(waitForProfileToClose(name), "the deferred close never completed once restore() had unwound");
    }

    // restore() can run nested inside another map operation, whose scope keeps
    // a close asked for during it pending: no file should be read then either
    void test_aCloseAskedForBeforeTheFallbackStopsItBeforeTheFirstFile()
    {
        const QString name = qsl("MapFallbackClosedBeforeStart-Test");
        Host* pTarget = addProfile(name);
        QVERIFY2(pTarget, "failed to create the target Host");
        buildMap(pTarget);
        if (QTest::currentTestFailed()) {
            return;
        }
        TMap* pMap = pTarget->mpMap.data();
        // Not in the map file, so it is gone if the file was loaded
        QVERIFY(pMap->addRoom(99999));
        const QString folder = MudletApp::getMudletPath(enums::profileMapsPath, name);
        QVERIFY(QDir().mkpath(folder));
        QVERIFY(HostManager::self()->getHost(mSourceName)->saveMapFile(qsl("%1/only.dat").arg(folder)));

        bool closeRequested = false;
        bool restoreRan = false;
        bool abortPendingAtRestore = false;
        bool loaded = false;
        const QMetaObject::Connection closeOnProgress = connect(pMap, &TMap::signal_mapProgressSetValue, pMap, [&]() {
            if (closeRequested) {
                return;
            }
            closeRequested = true;
            mudlet::self()->slot_closeProfileByName(name);
            // Queued behind the close, as a script's timer running loadMap() would be
            QTimer::singleShot(0, pMap, [&]() {
                restoreRan = true;
                abortPendingAtRestore = pMap->mapOperationAbortRequested();
                loaded = pMap->restore(QString());
            });
        });
        pMap->writeJsonMapFile(qsl("%1/close-before-fallback.json").arg(mSaveDir.path()));
        disconnect(closeOnProgress);

        QVERIFY2(closeRequested, "the export never announced any progress, so no close was asked for during it");
        QVERIFY2(restoreRan, "the export's event pump never ran the queued restore()");
        QVERIFY2(abortPendingAtRestore, "the close had not asked the map operation to stop by the time restore() ran");
        QVERIFY2(!loaded, "restore() loaded a map file after the profile was asked to close");
        QVERIFY2(pMap->mpRoomDB->getRoom(99999), "restore() replaced the map after the profile was asked to close");
        QVERIFY2(waitForProfileToClose(name), "the deferred close never completed once the export had unwound");
    }

    void test_deletingAnAreaDuringAJsonExportStopsIt_data()
    {
        QTest::addColumn<QString>("how");
        QTest::newRow("deleteArea") << qsl("deleteArea");
        QTest::newRow("deleteMap") << qsl("deleteMap");
        // What restore() does to an area of the same id when createMapper() loads a map over one with no rooms
        QTest::newRow("replaceArea") << qsl("replaceArea");
    }

    void test_deletingAnAreaDuringAJsonExportStopsIt()
    {
        QFETCH(QString, how);
        Host* pHost = addProfile(qsl("MapAreaDeletedDuringExport-%1-Test").arg(QLatin1String(QTest::currentDataTag())));
        QVERIFY2(pHost, "failed to create the Host");
        buildMap(pHost);
        if (QTest::currentTestFailed()) {
            return;
        }
        TMap* pMap = pHost->mpMap.data();
        const int firstAreaWithRooms = pMap->mpRoomDB->getRoom(1)->getArea();

        bool deleted = false;
        const QMetaObject::Connection deleteOnProgress = connect(pMap, &TMap::signal_mapProgressSetValue, pMap, [&](int roomsWritten) {
            if (deleted) {
                return;
            }
            if (!roomsWritten) {
                // Outlasts the progress throttle, so the step after the first
                // area's rooms runs the event loop too
                QTest::qSleep(60);
                return;
            }
            deleted = true;
            if (how == qsl("deleteMap")) {
                pMap->mapClear();
            } else if (how == qsl("deleteArea")) {
                QVERIFY(pMap->mpRoomDB->removeArea(firstAreaWithRooms));
            } else {
                pMap->mpRoomDB->restoreSingleArea(firstAreaWithRooms, new TArea(pMap, pMap->mpRoomDB.get()));
            }
        });
        const auto [wrote, writeMessage] = pMap->writeJsonMapFile(qsl("%1/area-deleted-during-export.json").arg(mSaveDir.path()));
        disconnect(deleteOnProgress);

        QVERIFY2(deleted, "the export never ran the event loop part way through an area");
        QVERIFY2(!wrote, "the export carried on over the area deleted under it");
        QCOMPARE(writeMessage, qsl("an area was deleted while the map was being exported"));
    }
};

#include "MapCloseDuringImportTest.moc"
MUDLET_GROUPED_TEST_MAIN(MapCloseDuringImportTest)
