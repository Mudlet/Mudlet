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
 * pixel back out of the file it wrote.
 *
 * Run with: ctest -R MapAreaImageExportTest -V
 */

#include <QImage>
#include <QTemporaryDir>
#include <QtTest/QtTest>

#include "Host.h"
#include "HostManager.h"
#include "MudletInstanceCoordinator.h"
#include "MudletPaths.h"
#include "PortableModeTestHelper.h"
#include "T2DMap.h"
#include "TMap.h"
#include "TRoom.h"
#include "TRoomDB.h"
#include "mudlet.h"

#include "GroupedTest.h"

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
        QDir dir(MudletPaths::getMudletPath(enums::profileHomePath, mProfileName));
        if (dir.exists()) {
            dir.removeRecursively();
        }
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
        QCOMPARE(MudletPaths::getMudletPath(enums::mainPath), qsl("%1/mudlet").arg(mConfigDir.path()));
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

        // The export hands the actual file write to a QtConcurrent task
        QImage exportedImage;
        QTRY_VERIFY2_WITH_TIMEOUT(exportedImage.load(filePath), "the exported image could not be read back", 10000);
        // The file lands inside QPixmap::save(), before the task reports back,
        // and the watcher is only cleared once it has - so this is what says the
        // pool thread has let go of the widget the fixture is about to delete
        QTRY_VERIFY_WITH_TIMEOUT(!mp2dMap->mpExportWatcher, 10000);

        // Both halves of the outline - the stub's and the arrowhead's - are
        // white, so this covers the pair jointly rather than either alone
        const int outlinePixels = countPixels(exportedImage, QColor(Qt::white));
        QVERIFY2(outlinePixels > 0, "the area exit marker was drawn with no outline, so nothing of it is visible against a background of its own colour");
        // The setup above only arranges that; this is what checks it held
        QCOMPARE(countPixels(exportedImage, QColor(Qt::black)) + outlinePixels, exportedImage.width() * exportedImage.height());
    }
};

#include "MapAreaImageExportTest.moc"
MUDLET_GROUPED_TEST_MAIN(MapAreaImageExportTest)
