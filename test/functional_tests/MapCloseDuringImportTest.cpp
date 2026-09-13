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
#include "MudletInstanceCoordinator.h"
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
        auto& hostManager = mudlet::self()->getHostManager();
        if (!hostManager.addHost(name, qsl("23"), QString(), QString())) {
            return nullptr;
        }
        return hostManager.getHost(name);
    }

    // Runs the event loop until the profile is gone. The close is deferred
    // until the map operation has unwound, so this is where it lands.
    bool waitForProfileToClose(const QString& name)
    {
        QDeadlineTimer deadline(10s);
        while (mudlet::self()->getHostManager().getHost(name)) {
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
};

#include "MapCloseDuringImportTest.moc"
MUDLET_GROUPED_TEST_MAIN(MapCloseDuringImportTest)
