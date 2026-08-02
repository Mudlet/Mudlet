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

/*
 * Tests the map-progress seam introduced for the libmudlet split (#8681,
 * #9011): the Qt-Widgets-free TMap no longer owns a QProgressDialog and instead
 * emits pre-translated payloads for the frontend to render, while cancellation
 * returns through TMap::slot_mapProgressDialogCancelled().
 *
 * These tests stand in for the frontend with plain signal recorders and drive
 * the engine directly, so they verify the engine half of the seam without any
 * widget:
 *   - the download/XML transfer-progress state machine emits the right signals
 *     and keeps its own maximum/active state (the old QProgressDialog read-backs)
 *   - a JSON export and re-import announce and close their progress dialogs and
 *     leave no stuck "operation already in progress" state
 *   - a cancel delivered through the seam mid-import makes the JSON reader abort,
 *     the exact behaviour that used to depend on QProgressDialog::wasCanceled()
 *
 * Run with: ctest -R MapProgressDialogSeamTest -V
 */

#include <QtTest/QtTest>

#include <QSignalSpy>
#include <QTemporaryDir>

#include "Host.h"
#include "HostManager.h"
#include "MudletInstanceCoordinator.h"
#include "TMap.h"
#include "TRoomDB.h"
#include "mudlet.h"

extern void qInitResources_mudlet();
extern void qInitResources_qm();
extern void qInitResources_additional_splash_screens();
extern void qInitResources_mudlet_fonts_common();
extern void qInitResources_mudlet_fonts_posix();
void initializeQRCResourcesForMapProgressDialogSeamTest();

class MapProgressDialogSeamTest : public QObject
{
    Q_OBJECT

private:
    Host* mpSource = nullptr;
    Host* mpTarget = nullptr;
    const QString mSourceName = qsl("MapProgressSeamSource-Test");
    const QString mTargetName = qsl("MapProgressSeamTarget-Test");
    QTemporaryDir mSaveDir;

    void buildSmallMap(Host* pHost)
    {
        TMap* pMap = pHost->mpMap.data();
        TRoomDB* pDB = pMap->mpRoomDB.get();
        const int areaA = pDB->addArea(qsl("Area A"));
        const int areaB = pDB->addArea(qsl("Area B"));
        QVERIFY(areaA > 0);
        QVERIFY(areaB > 0);
        int id = 1;
        for (const int areaId : {areaA, areaB}) {
            for (int i = 0; i < 3; ++i, ++id) {
                QVERIFY(pMap->addRoom(id));
                QVERIFY(pMap->setRoomArea(id, areaId, false));
                QVERIFY(pMap->setRoomCoordinates(id, i, i, 0));
            }
        }
    }

    void deleteProfileDirectory(const QString& profileName)
    {
        const QString path = mudlet::getMudletPath(enums::profileHomePath, profileName);
        QDir dir(path);
        if (dir.exists()) {
            dir.removeRecursively();
        }
    }

private slots:
    void initTestCase()
    {
        initializeQRCResourcesForMapProgressDialogSeamTest();

        mudlet::start();
        mudlet::self()->setupConfig();
        mudlet::self()->takeOwnershipOfInstanceCoordinator(std::make_unique<MudletInstanceCoordinator>("MudletInstanceCoordinator"));
        mudlet::self()->init();
        mudlet::self()->setStorePasswordsSecurely(false);
        deleteProfileDirectory(mSourceName);
        deleteProfileDirectory(mTargetName);

        QVERIFY(mSaveDir.isValid());

        auto& hostManager = mudlet::self()->getHostManager();
        QVERIFY2(hostManager.addHost(mSourceName, qsl("23"), QString(), QString()), "failed to create the source Host");
        mpSource = hostManager.getHost(mSourceName);
        QVERIFY(mpSource);
        QVERIFY2(hostManager.addHost(mTargetName, qsl("23"), QString(), QString()), "failed to create the target Host");
        mpTarget = hostManager.getHost(mTargetName);
        QVERIFY(mpTarget);

        buildSmallMap(mpSource);
        if (QTest::currentTestFailed()) {
            return;
        }
    }

    void cleanupTestCase()
    {
        mpSource = nullptr;
        mpTarget = nullptr;
        deleteProfileDirectory(mSourceName);
        deleteProfileDirectory(mTargetName);
        delete mudlet::self();
    }

    // The download/XML transfer path: with no visible mapper the engine takes
    // the standalone-dialog branch, which must now be pure signals plus the
    // engine-side state that replaced the QProgressDialog read-backs.
    void test_transferProgressStateMachine()
    {
        TMap* pMap = mpSource->mpMap.data();
        QSignalSpy startSpy(pMap, &TMap::signal_mapTransferProgressStart);
        QSignalSpy rangeSpy(pMap, &TMap::signal_mapProgressSetRange);
        QSignalSpy valueSpy(pMap, &TMap::signal_mapProgressSetValue);
        QSignalSpy labelSpy(pMap, &TMap::signal_mapProgressSetLabel);
        QSignalSpy disableSpy(pMap, &TMap::signal_mapProgressDisableCancel);
        QSignalSpy closeSpy(pMap, &TMap::signal_mapProgressClose);
        QVERIFY(startSpy.isValid());

        QVERIFY(!pMap->hasActiveTransferProgress());

        pMap->createTransferProgress(qsl("A title"), qsl("A label"), true);
        QCOMPARE(startSpy.count(), 1);
        QCOMPARE(startSpy.at(0).at(0).toString(), qsl("A title"));
        QCOMPARE(startSpy.at(0).at(1).toString(), qsl("A label"));
        // cancelable == true carries the pre-translated Abort button text:
        QCOMPARE(startSpy.at(0).at(2).toString(), qsl("Abort"));
        QVERIFY(pMap->hasActiveTransferProgress());
        QCOMPARE(pMap->transferProgressMaximum(), 0);

        pMap->updateTransferProgressRange(0, 100);
        QCOMPARE(rangeSpy.count(), 1);
        QCOMPARE(rangeSpy.at(0).at(1).toInt(), 100);
        // Read-back must come from the engine's cached maximum, not a widget:
        QCOMPARE(pMap->transferProgressMaximum(), 100);

        pMap->updateTransferProgressValue(42);
        QCOMPARE(valueSpy.count(), 1);
        QCOMPARE(valueSpy.at(0).at(0).toInt(), 42);

        pMap->updateTransferProgressLabel(qsl("Working"));
        QCOMPARE(labelSpy.count(), 1);
        QCOMPARE(labelSpy.at(0).at(0).toString(), qsl("Working"));

        pMap->disableTransferProgressCancel();
        QCOMPARE(disableSpy.count(), 1);

        pMap->clearTransferProgress();
        QCOMPARE(closeSpy.count(), 1);
        QVERIFY(!pMap->hasActiveTransferProgress());
    }

    void test_jsonExportImportDrivesProgressSignals()
    {
        TMap* pSourceMap = mpSource->mpMap.data();
        const QString file = qsl("%1/seam.json").arg(mSaveDir.path());

        QSignalSpy exportStartSpy(pSourceMap, &TMap::signal_mapJsonProgressStart);
        QSignalSpy exportCloseSpy(pSourceMap, &TMap::signal_mapProgressClose);
        const auto [wrote, writeMsg] = pSourceMap->writeJsonMapFile(file);
        QVERIFY2(wrote, qPrintable(writeMsg));
        QCOMPARE(exportStartSpy.count(), 1);
        QCOMPARE(exportStartSpy.at(0).at(0).toString(), qsl("Map JSON export"));
        // Exactly one close: a second would mean the dialog was torn down twice:
        QCOMPARE(exportCloseSpy.count(), 1);
        // The engine must not stay "in progress" (that would reject the next op):
        QVERIFY(!pSourceMap->hasActiveTransferProgress());

        TMap* pTargetMap = mpTarget->mpMap.data();
        QSignalSpy importStartSpy(pTargetMap, &TMap::signal_mapJsonProgressStart);
        QSignalSpy importCloseSpy(pTargetMap, &TMap::signal_mapProgressClose);
        const auto [read, readMsg] = pTargetMap->readJsonMapFile(file);
        QVERIFY2(read, qPrintable(readMsg));
        QCOMPARE(importStartSpy.count(), 1);
        QCOMPARE(importStartSpy.at(0).at(0).toString(), qsl("Map JSON import"));
        QCOMPARE(importCloseSpy.count(), 1);
        QVERIFY(!pTargetMap->hasActiveTransferProgress());
    }

    // The highest-risk seam: the JSON reader used to poll
    // QProgressDialog::wasCanceled(); it now polls a flag set by
    // slot_mapProgressDialogCancelled(). Acting as the frontend, deliver a
    // cancel the instant the import announces its dialog and confirm the read
    // aborts with the user-cancel result and clears its state.
    void test_jsonImportCancellationAbortsViaSeam()
    {
        TMap* pSourceMap = mpSource->mpMap.data();
        const QString file = qsl("%1/cancel.json").arg(mSaveDir.path());
        // Spying on both ends of a dialog's life also stands in for a wired-up
        // frontend, so the engine's "nobody is showing this" warning stays quiet:
        QSignalSpy exportStartSpy(pSourceMap, &TMap::signal_mapJsonProgressStart);
        QSignalSpy exportCloseSpy(pSourceMap, &TMap::signal_mapProgressClose);
        const auto [wrote, writeMsg] = pSourceMap->writeJsonMapFile(file);
        QVERIFY2(wrote, qPrintable(writeMsg));
        QCOMPARE(exportStartSpy.count(), 1);
        QCOMPARE(exportCloseSpy.count(), 1);

        TMap* pTargetMap = mpTarget->mpMap.data();
        QSignalSpy importCloseSpy(pTargetMap, &TMap::signal_mapProgressClose);
        const QMetaObject::Connection cancelOnStart = connect(pTargetMap, &TMap::signal_mapJsonProgressStart, pTargetMap, [pTargetMap]() {
            pTargetMap->slot_mapProgressDialogCancelled();
        });
        const auto [read, readMsg] = pTargetMap->readJsonMapFile(file);
        disconnect(cancelOnStart);

        QVERIFY(!read);
        QCOMPARE(readMsg, qsl("aborted by user"));
        // An aborted import must still take its progress dialog down with it:
        QCOMPARE(importCloseSpy.count(), 1);
        QVERIFY(!pTargetMap->hasActiveTransferProgress());
    }

    // An XML map import started while a JSON operation owns the progress dialog
    // must be refused: readXmlMapFile() would otherwise mistake the JSON
    // operation's dialog for its own and mapClear() the map mid-import. The
    // re-entrancy is real - a Lua loadMap() from a timer lands in the
    // qApp->processEvents() the JSON reader pumps.
    void test_xmlImportRefusedWhileJsonOperationOwnsProgress()
    {
        TMap* pSourceMap = mpSource->mpMap.data();
        const QString file = qsl("%1/reentrancy.json").arg(mSaveDir.path());
        QSignalSpy exportStartSpy(pSourceMap, &TMap::signal_mapJsonProgressStart);
        QSignalSpy exportCloseSpy(pSourceMap, &TMap::signal_mapProgressClose);
        const auto [wrote, writeMsg] = pSourceMap->writeJsonMapFile(file);
        QVERIFY2(wrote, qPrintable(writeMsg));
        QCOMPARE(exportStartSpy.count(), 1);
        QCOMPARE(exportCloseSpy.count(), 1);

        TMap* pTargetMap = mpTarget->mpMap.data();
        QSignalSpy importCloseSpy(pTargetMap, &TMap::signal_mapProgressClose);
        bool importAttempted = false;
        bool importAccepted = true;
        QString importError;
        const QMetaObject::Connection reenter = connect(pTargetMap, &TMap::signal_mapJsonProgressStart, pTargetMap, [&]() {
            importAttempted = true;
            QFile xmlMap(qsl("%1/no-such-map.xml").arg(mSaveDir.path()));
            importAccepted = pTargetMap->importMap(xmlMap, &importError);
        });
        const auto [read, readMsg] = pTargetMap->readJsonMapFile(file);
        disconnect(reenter);

        QVERIFY(importAttempted);
        QVERIFY2(!importAccepted, "importMap() ran on top of an in-flight JSON import");
        // Refused by the in-progress guard, not by failing to read the file:
        QVERIFY2(importError.contains(qsl("already in progress")), qPrintable(importError));
        // ...and the JSON operation it interrupted still completed:
        QVERIFY2(read, qPrintable(readMsg));
        QCOMPARE(importCloseSpy.count(), 1);
        QVERIFY(!pTargetMap->hasActiveTransferProgress());
    }
};

void initializeQRCResourcesForMapProgressDialogSeamTest()
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

#include "MapProgressDialogSeamTest.moc"
QTEST_MAIN(MapProgressDialogSeamTest)
