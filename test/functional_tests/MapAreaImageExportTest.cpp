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
 * T2DMap::exportAreaToImage(), which Lua reaches as exportAreaImage(), draws
 * an area into a PNG with the same marks the mapper shows, the stub and
 * arrowhead that stand for an exit leading out of the area among them.
 *
 * Why not a spec: Lua can ask for the export, but nothing in the API reads a
 * pixel back out of the file it wrote, lets a save finish without letting it
 * report back, or keeps a map view alive after its profile has closed.
 *
 * Run with: ctest -R MapAreaImageExportTest -V
 */

#include <QFutureWatcher>
#include <QImage>
#include <QScopeGuard>
#include <QTemporaryDir>
#include <QThreadPool>
#include <QtTest/QtTest>

#include "Host.h"
#include "HostManager.h"
#include "MudletInstanceCoordinator.h"
#include "MudletApp.h"
#include "PortableModeTestHelper.h"
#include "T2DMap.h"
#include "TMap.h"
#include "TRoom.h"
#include "TRoomDB.h"
#include "mudlet.h"

#include "GroupedTest.h"

using namespace std::chrono_literals;

class MapAreaImageExportTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;
    Host* mpHost = nullptr;
    T2DMap* mp2dMap = nullptr;
    const QString mProfileName = qsl("MapAreaImageExport-Test");
    static constexpr int scmRoomInArea = 1;
    static constexpr int scmRoomNextDoor = 2;
    static constexpr int scmBlackEnvironment = 300;

    TMap* map() const { return mpHost->mpMap.data(); }
    TRoomDB* roomDB() const { return mpHost->mpMap->mpRoomDB.get(); }

    void deleteProfileDirectory() const
    {
        QDir dir(MudletApp::getMudletPath(enums::profileHomePath, mProfileName));
        if (dir.exists()) {
            dir.removeRecursively();
        }
    }

    // Every export parents a watcher of its own to the map and deletes it once
    // the save has reported back, so none left means the pool threads are done
    static bool exportsInFlight(const T2DMap* pMap) { return !pMap->findChildren<QFutureWatcherBase*>(QString(), Qt::FindDirectChildrenOnly).isEmpty(); }

    // Waits for the saves themselves rather than for the whole global thread
    // pool, which other work can be using too; their handlers still wait for
    // the event loop
    static void waitForExportSaves(const T2DMap* pMap)
    {
        for (auto* pWatcher : pMap->findChildren<QFutureWatcherBase*>(QString(), Qt::FindDirectChildrenOnly)) {
            pWatcher->waitForFinished();
        }
    }

    static void addOneRoomArea(TMap* pMap, const QString& areaName, int& areaId)
    {
        areaId = pMap->mpRoomDB->addArea(areaName);
        QVERIFY(areaId > 0);
        QVERIFY(pMap->addRoom(scmRoomInArea));
        QVERIFY(pMap->setRoomArea(scmRoomInArea, areaId));
        QVERIFY(pMap->setRoomCoordinates(scmRoomInArea, 0, 0, 0));
    }

    static int countPixels(const QImage& image, const QColor& colour)
    {
        int found = 0;
        for (int y = 0; y < image.height(); ++y) {
            for (int x = 0; x < image.width(); ++x) {
                if (image.pixelColor(x, y) == colour) {
                    ++found;
                }
            }
        }
        return found;
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
        deleteProfileDirectory();

        auto* hostManager = HostManager::self();
        QVERIFY2(hostManager->addHost(mProfileName, qsl("23"), QString(), QString()), "failed to create the Host");
        mpHost = hostManager->getHost(mProfileName);
        QVERIFY(mpHost);
        QVERIFY(map());

        // The two things dlgMapper's constructor sets that this path needs; the
        // export paints into a pixmap of its own rather than onto the widget
        mp2dMap = new T2DMap();
        mp2dMap->mpMap = map();
        mp2dMap->mpHost = mpHost;
    }

    void cleanupTestCase()
    {
        delete mp2dMap;
        mp2dMap = nullptr;
        mpHost = nullptr;
        if (mudlet::self()) {
            deleteProfileDirectory();
            delete mudlet::self();
        }
        mSavedXdg.isNull() ? qunsetenv("XDG_CONFIG_HOME") : qputenv("XDG_CONFIG_HOME", mSavedXdg);
    }

    // The stub and arrowhead that mark an exit out of the area are drawn in the
    // destination room's environment colour, so on a map whose background is
    // that same colour they used to come out invisible (#8794)
    void anAreaExitMarkerIsOutlinedAgainstABackgroundOfItsOwnColour()
    {
        map()->mapClear();
        const int areaId = roomDB()->addArea(qsl("Exporting area"));
        const int nextAreaId = roomDB()->addArea(qsl("Area next door"));
        QVERIFY(areaId > 0);
        QVERIFY(nextAreaId > 0);
        QVERIFY(map()->addRoom(scmRoomInArea));
        QVERIFY(map()->setRoomArea(scmRoomInArea, areaId));
        QVERIFY(map()->setRoomCoordinates(scmRoomInArea, 0, 0, 0));
        QVERIFY(map()->addRoom(scmRoomNextDoor));
        QVERIFY(map()->setRoomArea(scmRoomNextDoor, nextAreaId));
        QVERIFY(map()->setRoomCoordinates(scmRoomNextDoor, 1, 0, 0));
        QVERIFY(map()->setExit(scmRoomInArea, scmRoomNextDoor, DIR_EAST));

        // This map holds nothing but the two rooms and the one area exit, and
        // every colour those can be drawn in is forced to black, so a white
        // pixel in the file can only be the outline under test.
        map()->mCustomEnvColors[scmBlackEnvironment] = QColor(Qt::black);
        TRoom* pRoomInArea = roomDB()->getRoom(scmRoomInArea);
        TRoom* pRoomNextDoor = roomDB()->getRoom(scmRoomNextDoor);
        QVERIFY(pRoomInArea);
        QVERIFY(pRoomNextDoor);
        pRoomInArea->environment = scmBlackEnvironment;
        pRoomNextDoor->environment = scmBlackEnvironment;
        mpHost->mBgColor_2 = QColor(Qt::black);
        mpHost->mFgColor_2 = QColor(Qt::black);
        mpHost->mRoomBorderColor = QColor(Qt::black);
        mpHost->mUpperLevelColor = QColor(Qt::black);
        mpHost->mLowerLevelColor = QColor(Qt::black);
        mp2dMap->mMapperUseAntiAlias = false;
        QCOMPARE(map()->getColor(scmRoomNextDoor), QColor(Qt::black));

        const QString exportDir = qsl("%1/exports").arg(mConfigDir.path());
        QVERIFY(QDir().mkpath(exportDir));
        const QString filePath = qsl("%1/area-exit.png").arg(exportDir);
        const auto [exported, message] = mp2dMap->exportAreaToImage(areaId, filePath);
        QVERIFY2(exported, qPrintable(message));
        QVERIFY(exportsInFlight(mp2dMap));

        // The export hands the actual file write to a QtConcurrent task
        QImage exportedImage;
        QTRY_VERIFY2_WITH_TIMEOUT(exportedImage.load(filePath), "the exported image could not be read back", 10s);
        // The file lands inside QPixmap::save(), before the task reports back,
        // and the watcher is only deleted once it has - so this is what says the
        // pool thread has let go of the widget the fixture is about to delete
        QTRY_VERIFY_WITH_TIMEOUT(!exportsInFlight(mp2dMap), 10s);

        // Both halves of the outline - the stub's and the arrowhead's - are
        // white, so this covers the pair jointly rather than either alone
        const int outlinePixels = countPixels(exportedImage, QColor(Qt::white));
        QVERIFY2(outlinePixels > 0, "the area exit marker was drawn with no outline, so nothing of it is visible against a background of its own colour");
        // The setup above only arranges that; this is what checks it held
        QCOMPARE(countPixels(exportedImage, QColor(Qt::black)) + outlinePixels, exportedImage.width() * exportedImage.height());
    }

    // The export draws rooms in the area's hash order. With two highlighted
    // rooms side by side, each highlight clear at its own centre and solid over
    // its neighbour's, the room drawn second must not cover the first one's (#7206)
    void aHighlightIsDrawnOverTheRoomsAroundIt()
    {
        map()->mapClear();
        const int areaId = roomDB()->addArea(qsl("Highlighted area"));
        QVERIFY(areaId > 0);
        constexpr int orangeEnvironment = scmBlackEnvironment + 1;
        map()->mCustomEnvColors[orangeEnvironment] = QColor(255, 128, 0);
        const QList<QPair<QPoint, QColor>> rooms{{QPoint(0, 0), QColor(255, 255, 0)}, {QPoint(1, 0), QColor(255, 0, 255)}};
        int roomId = scmRoomInArea;
        for (const auto& [position, colour] : rooms) {
            QVERIFY(map()->addRoom(roomId));
            QVERIFY(map()->setRoomArea(roomId, areaId));
            QVERIFY(map()->setRoomCoordinates(roomId, position.x(), position.y(), 0));
            TRoom* pRoom = roomDB()->getRoom(roomId);
            QVERIFY(pRoom);
            pRoom->environment = orangeEnvironment;
            pRoom->highlight = true;
            pRoom->highlightColor = colour;
            pRoom->highlightColor2 = QColor(0, 0, 0, 0);
            // Its edge colour starts at 85% of the radius, short of the room next door one room away
            pRoom->highlightRadius = 2.3f;
            ++roomId;
        }
        mpHost->mBgColor_2 = QColor(Qt::black);
        mp2dMap->mMapperUseAntiAlias = false;
        mp2dMap->mShowRoomID = false;

        const QString exportDir = qsl("%1/exports-highlight").arg(mConfigDir.path());
        QVERIFY(QDir().mkpath(exportDir));
        const QString filePath = qsl("%1/highlight.png").arg(exportDir);
        const auto [exported, message] = mp2dMap->exportAreaToImage(areaId, filePath, std::nullopt, 1.0);
        QVERIFY2(exported, qPrintable(message));
        QImage exportedImage;
        QTRY_VERIFY2_WITH_TIMEOUT(exportedImage.load(filePath), "the exported image could not be read back", 10s);
        QTRY_VERIFY_WITH_TIMEOUT(!exportsInFlight(mp2dMap), 10s);

        // A 2x1 area is drawn 50 pixels a room, with two rooms of padding round it
        constexpr int roomSize = 50;
        constexpr int padding = 2 * roomSize;
        QCOMPARE(exportedImage.size(), QSize(2 * roomSize + 2 * padding, roomSize + 2 * padding));
        const QColor overFirst = exportedImage.pixelColor(padding, padding);
        const QColor overSecond = exportedImage.pixelColor(padding + roomSize, padding);
        // A room's own highlight is all but transparent at its centre, not quite
        const auto nearly = [](const QColor& lhs, const QColor& rhs) {
            return qAbs(lhs.red() - rhs.red()) <= 8 && qAbs(lhs.green() - rhs.green()) <= 8 && qAbs(lhs.blue() - rhs.blue()) <= 8;
        };
        QVERIFY2(nearly(overFirst, rooms.at(1).second) && nearly(overSecond, rooms.at(0).second),
                 qPrintable(qsl("a room was drawn over its neighbour's highlight: its centre is %1 and %2 rather than each other's highlight").arg(overFirst.name(), overSecond.name())));
    }

    // Every export's handler used to read one shared watcher member, so the
    // first handler reported the second export's result, deleted that watcher
    // and left a null for the second handler to crash on (#10393)
    void twoExportsInQuickSuccessionBothFinish()
    {
        map()->mapClear();
        int areaId = 0;
        addOneRoomArea(map(), qsl("Exported twice"), areaId);
        if (QTest::currentTestFailed()) {
            return;
        }

        const QString exportDir = qsl("%1/exports-twice").arg(mConfigDir.path());
        QVERIFY(QDir().mkpath(exportDir));
        const QString firstPath = qsl("%1/first.png").arg(exportDir);
        const QString secondPath = qsl("%1/second.png").arg(exportDir);
        const auto [firstExported, firstMessage] = mp2dMap->exportAreaToImage(areaId, firstPath);
        QVERIFY2(firstExported, qPrintable(firstMessage));
        // Both saves finish before either reports back, as when a script
        // exports twice in one go and the event loop only runs once it returns
        waitForExportSaves(mp2dMap);
        const auto [secondExported, secondMessage] = mp2dMap->exportAreaToImage(areaId, secondPath);
        QVERIFY2(secondExported, qPrintable(secondMessage));
        waitForExportSaves(mp2dMap);
        // Or the wait below would pass without running a single handler
        QVERIFY(exportsInFlight(mp2dMap));

        QTRY_VERIFY_WITH_TIMEOUT(!exportsInFlight(mp2dMap), 10s);
        QVERIFY(!QImage(firstPath).isNull());
        QVERIFY(!QImage(secondPath).isNull());
    }

    // A map view can outlive its profile - closeHost() only deleteLater()s the
    // main window's map dock - and a failed save used to call postMessage()
    // on the null Host (#11019)
    void aFailedExportThatFinishesAfterItsProfileClosedIsLoggedInstead()
    {
        const QString closingProfile = qsl("MapAreaImageExport-Closing");
        auto* hostManager = HostManager::self();
        QVERIFY(hostManager->addHost(closingProfile, qsl("23"), QString(), QString()));
        // Declared ahead of the guard, so that on an early return the guard
        // waits for the save before the view it was given is destroyed
        auto survivingView = std::make_unique<T2DMap>();
        auto closeProfile = qScopeGuard([hostManager, closingProfile] {
            QThreadPool::globalInstance()->waitForDone(10000);
            if (hostManager->getHost(closingProfile)) {
                hostManager->deleteHost(closingProfile);
            }
        });
        QPointer<Host> pClosingHost = hostManager->getHost(closingProfile);
        QVERIFY(pClosingHost);
        TMap* pClosingMap = pClosingHost->mpMap.data();
        int areaId = 0;
        addOneRoomArea(pClosingMap, qsl("Closing area"), areaId);
        if (QTest::currentTestFailed()) {
            return;
        }

        survivingView->mpMap = pClosingMap;
        survivingView->mpHost = pClosingHost;
        // A name longer than file systems allow passes the checks made before the save
        const QString unsavablePath = qsl("%1/closing%2.png").arg(mConfigDir.path(), QString(300, QLatin1Char('n')));
        QTest::ignoreMessage(QtWarningMsg, QRegularExpression(qsl("profile \"%1\" closed before this export finished: Failed to save image to .*closingn+\\.png").arg(closingProfile)));
        const auto [exported, message] = survivingView->exportAreaToImage(areaId, unsavablePath);
        QVERIFY2(exported, qPrintable(message));

        hostManager->deleteHost(closingProfile);
        QVERIFY(pClosingHost.isNull());
        // deleteHost() freed the map this view points at
        survivingView->mpMap = nullptr;

        QTRY_VERIFY_WITH_TIMEOUT(!exportsInFlight(survivingView.get()), 10s);
    }
};

#include "MapAreaImageExportTest.moc"
MUDLET_GROUPED_TEST_MAIN(MapAreaImageExportTest)
