/***************************************************************************
 *   Copyright (C) 2026 by Mudlet Makers                                   *
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
 * Why not a spec: viewportRoomBounds() has to be asked directly, because at the
 * zoom these cases cover the whole map is one pixel and nothing drawn differs
 * between the right answer and the wrong one; and loading a map file replaces
 * the map of the profile the specs share.
 *
 * Both cases hang rather than fail when they regress, so what reports them is
 * the 60 second cap every functional test carries rather than an assertion.
 *
 * Run with: ctest -R MapCoordinateLimitTest -V
 */

#include <QPixmap>
#include <QSaveFile>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QtTest/QtTest>

#include <climits>

#include "Host.h"
#include "MudletInstanceCoordinator.h"
#include "PortableModeTestHelper.h"
#include "ProfileTestHelper.h"
#include "T2DMap.h"
#include "TArea.h"
#include "TMap.h"
#include "TRoom.h"
#include "TRoomDB.h"
#include "TelnetServerStub.h"
#include "ctelnet.h"
#include "dlgMapper.h"
#include "mudlet.h"

#include "GroupedTest.h"

class MapCoordinateLimitTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;
    TelnetServerStub* mpServer = nullptr;
    Host* mpHost = nullptr;
    const QString mProfileName = qsl("MapCoordinateLimit-Test");
    const QString mLocalhost = qsl("localhost");
    QString mPort;

    static constexpr int kWidgetWidth = 600;
    static constexpr int kWidgetHeight = 400;
    // One pixel per room on a widget this tall, which puts the viewport's
    // right-hand bound on the coordinate limit.
    static constexpr double kOnePixelPerRoomZoom = 400.0;
    // More occupied columns than the viewport at that zoom asks for, so the scan
    // probes the columns it wants instead of walking the ones the area has. The
    // margin is wide because how many it wants tracks the widget's width.
    static constexpr int kOccupiedColumns = 2000;
    static constexpr int kFarRoomId = 99999;

    TMap* map() const { return mpHost->mpMap.data(); }

    void deleteProfileDirectory() const
    {
        QDir dir(mudlet::getMudletPath(enums::profileHomePath, mProfileName));
        if (dir.exists()) {
            dir.removeRecursively();
        }
    }

    bool addRoomAt(const int id, const int areaId, const int x, const int y) const { return map()->addRoom(id) && map()->setRoomArea(id, areaId) && map()->setRoomCoordinates(id, x, y, 0); }

    // The player is left standing in the far room and the zoom is stored on the
    // area, which is the state a saved map file carries.
    int buildAreaWithARoomAtTheCoordinateLimit() const
    {
        TMap* pMap = map();
        pMap->mapClear();
        const int areaId = pMap->mpRoomDB->addArea(qsl("Coordinate Limit Area"));
        if (areaId <= 0) {
            return 0;
        }
        for (int i = 0; i < kOccupiedColumns; ++i) {
            if (!addRoomAt(i + 1, areaId, i, 0)) {
                return 0;
            }
        }
        if (!addRoomAt(kFarRoomId, areaId, INT_MAX, 0)) {
            return 0;
        }
        pMap->mRoomIdHash[pMap->mProfileName] = kFarRoomId;
        TArea* pArea = pMap->mpRoomDB->getArea(areaId);
        if (!pArea) {
            return 0;
        }
        pArea->set2DMapZoom(kOnePixelPerRoomZoom);
        return areaId;
    }

    T2DMap* preparedWidget() const
    {
        TMap* pMap = map();
        if (!pMap->mpMapper) {
            mpHost->showHideOrCreateMapper(false);
        }
        if (!pMap->mpMapper || !pMap->mpMapper->mp2dMap) {
            return nullptr;
        }
        T2DMap* p2dMap = pMap->mpMapper->mp2dMap;
        p2dMap->init();
        p2dMap->resize(kWidgetWidth, kWidgetHeight);
        // Both false is what makes paintEvent() centre on the player room and
        // pick up the area and zoom the map file brought with it.
        p2dMap->mShiftMode = false;
        p2dMap->mPick = false;
        p2dMap->mMultiSelectionSet.clear();
        return p2dMap;
    }

    static void renderFrame(T2DMap* p2dMap)
    {
        QPixmap target(kWidgetWidth, kWidgetHeight);
        target.fill(Qt::black);
        p2dMap->render(&target, QPoint(), QRegion(), QWidget::DrawWindowBackground);
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

        mpServer = new TelnetServerStub(qApp);
        mpServer->start(mLocalhost, 0);
        QVERIFY2(mpServer->serverPort() != 0, "the telnet stub did not start listening");
        mPort = QString::number(mpServer->serverPort());

        mudlet::start();
        mudlet::self()->setupConfig();
        QCOMPARE(mudlet::getMudletPath(enums::mainPath), qsl("%1/mudlet").arg(mConfigDir.path()));
        mudlet::self()->takeOwnershipOfInstanceCoordinator(std::make_unique<MudletInstanceCoordinator>("MudletInstanceCoordinator"));
        mudlet::self()->init();
        mudlet::self()->setStorePasswordsSecurely(false);
        mudlet::self()->mSkipDefaultPackageInstall = true;
        deleteProfileDirectory();

        mpHost = TestProfile::create(mProfileName, mLocalhost, mPort);
        QVERIFY(mpHost);
        QSignalSpy connected(&(mpHost->mTelnet), &cTelnet::signal_connected);
        QVERIFY2(connected.wait(3000), "could not connect to the telnet stub");
    }

    void cleanupTestCase()
    {
        mpHost = nullptr;
        delete mpServer;
        mpServer = nullptr;
        if (mudlet::self()) {
            deleteProfileDirectory();
            delete mudlet::self();
        }
        mSavedXdg.isNull() ? qunsetenv("XDG_CONFIG_HOME") : qputenv("XDG_CONFIG_HOME", mSavedXdg);
    }

    void test_theViewportCoversEveryCoordinateWhenARoomIsZeroPixelsAcross_data()
    {
        QTest::addColumn<float>("panOffset");

        // mRX and mRY are qRound()ed from a NaN, and int(NaN) is not the same
        // everywhere: x86-64 lands on INT_MIN and ARM64 on 0.
        QTest::newRow("the pan offset an x86-64 build produces") << static_cast<float>(INT_MIN);
        QTest::newRow("the pan offset an ARM64 build produces") << 0.0f;
        QTest::newRow("panned past the widget") << 1000.0f;
    }

    void test_theViewportCoversEveryCoordinateWhenARoomIsZeroPixelsAcross()
    {
        QFETCH(float, panOffset);

        const QRect bounds = T2DMap::viewportRoomBounds(panOffset, panOffset, 0.0f, 0.0f, kWidgetWidth, kWidgetHeight);

        // left/right/top/bottom rather than width() and height(), which the
        // whole range overflows
        QCOMPARE(bounds.left(), INT_MIN);
        QCOMPARE(bounds.right(), INT_MAX);
        QCOMPARE(bounds.top(), INT_MIN);
        QCOMPARE(bounds.bottom(), INT_MAX);
    }

    void test_theViewportStaysNarrowAtAnOrdinaryZoom()
    {
        const QRect bounds = T2DMap::viewportRoomBounds(kWidgetWidth / 2.0f, kWidgetHeight / 2.0f, 20.0f, 20.0f, kWidgetWidth, kWidgetHeight);

        QCOMPARE(bounds.left(), -16);
        QCOMPARE(bounds.right(), 16);
        QCOMPARE(bounds.top(), -11);
        QCOMPARE(bounds.bottom(), 11);
    }

    void test_aMapFileHoldingARoomAtTheCoordinateLimitIsPaintedWhenItIsLoadedBack()
    {
        const int areaId = buildAreaWithARoomAtTheCoordinateLimit();
        QVERIFY2(areaId > 0, "the area under test could not be built");

        const QString fileName = qsl("%1/coordinate-limit-map.dat").arg(mConfigDir.path());
        QSaveFile file(fileName);
        QVERIFY(file.open(QIODevice::WriteOnly));
        QDataStream out(&file);
        out.setVersion(QDataStream::Qt_5_12);
        QVERIFY2(map()->serialize(out, map()->mDefaultVersion), "the map under test could not be saved");
        QVERIFY(file.commit());

        map()->mapClear();
        QVERIFY2(map()->restore(fileName), "the map under test could not be read back");

        // Checked before the paint rather than after it: a paint that never
        // returns reports nothing at all.
        const TRoom* pFarRoom = map()->mpRoomDB->getRoom(kFarRoomId);
        QVERIFY2(pFarRoom, "the room at the coordinate limit did not survive the save and load");
        QCOMPARE(pFarRoom->x(), INT_MAX);
        QCOMPARE(map()->mRoomIdHash.value(map()->mProfileName), kFarRoomId);
        const TArea* pArea = map()->mpRoomDB->getArea(pFarRoom->getArea());
        QVERIFY(pArea);
        QCOMPARE(pArea->get2DMapZoom(), kOnePixelPerRoomZoom);

        T2DMap* p2dMap = preparedWidget();
        QVERIFY2(p2dMap, "the profile's mapper could not be created");
        renderFrame(p2dMap);

        // Reached at all is the point: a paint whose scan never returns never
        // gets here.
        QCOMPARE(p2dMap->mAreaID, pFarRoom->getArea());
    }
};

#include "MapCoordinateLimitTest.moc"
MUDLET_GROUPED_TEST_MAIN(MapCoordinateLimitTest)
