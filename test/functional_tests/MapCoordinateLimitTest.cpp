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
 * The two ends of the mapper's viewport query that a room out at the edge of
 * the coordinate space reaches, and that a Lua spec cannot:
 *
 *   - viewportRoomBounds() for the zoom that leaves a room zero pixels across.
 *     Every bound is then a division by that zero, and which non-number comes
 *     back differs by processor, so the only way to pin the answer is to ask
 *     the function directly with each of the pan offsets a machine can produce.
 *     Nothing the mapper draws differs between the right answer and the wrong
 *     one that a spec could see: at that zoom the whole map is one pixel.
 *
 *   - the paint that follows loading a map file which holds such a room. That
 *     route needs no script in the session that freezes, and a spec cannot
 *     take it: loading a map replaces the map of the profile the specs share.
 *
 * Both of these hang rather than fail when they regress, so what reports them
 * is the 60 second cap every functional test carries rather than an assertion.
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
    // One pixel per room on a widget this tall, which is where the viewport's
    // right-hand bound lands on the coordinate limit. The scroll wheel reaches
    // it.
    static constexpr double kOnePixelPerRoomZoom = 400.0;
    // More occupied columns than the viewport at that zoom asks for, so the
    // scan probes the columns it wants instead of walking the ones the area
    // has - probing is the route that stopped terminating. The margin is wide
    // because how many columns the viewport wants is half the widget's width in
    // rooms, which platform chrome moves.
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

    // An area a few thousand columns wide with one room out at the coordinate
    // limit, the player standing in that room, and the zoom that draws it a
    // pixel per room already stored on the area - which is the state a saved
    // map file carries.
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

    // The widget the profile's own mapper holds, sized and left to centre
    // itself on the player room the way a repaint after a map load does.
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

        // mRX and mRY are qRound()ed from zero times the overflowed span, which
        // is a NaN, and int(NaN) is not the same everywhere: x86-64 lands on
        // INT_MIN and ARM64 on 0. The third row is the same zoom with the map
        // panned so that both ends of the division are on the same side.
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

    // The control for the case above: widening the range is only the right
    // answer where there is no range to work out, so an ordinary zoom has to
    // come back with the handful of coordinates that really can be on screen.
    void test_theViewportStaysNarrowAtAnOrdinaryZoom()
    {
        const QRect bounds = T2DMap::viewportRoomBounds(kWidgetWidth / 2.0f, kWidgetHeight / 2.0f, 20.0f, 20.0f, kWidgetWidth, kWidgetHeight);

        QCOMPARE(bounds.left(), -16);
        QCOMPARE(bounds.right(), 16);
        QCOMPARE(bounds.top(), -11);
        QCOMPARE(bounds.bottom(), 11);
    }

    // The route that needs no script in the session that freezes: the room out
    // at the limit, the area's zoom and the player's position all travel in the
    // map file, so the first paint after the file is read back is the one that
    // asks the index for a range ending at INT_MAX.
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

        // What the file has to have carried for the paint below to be the one
        // this covers, checked before the paint rather than after it - a paint
        // that never returns reports nothing at all.
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

        // Reached at all, which is the whole point: the scan this paint runs
        // used to count in an int and so never got past a range ending at
        // INT_MAX.
        QCOMPARE(p2dMap->mAreaID, pFarRoom->getArea());
    }
};

#include "MapCoordinateLimitTest.moc"
MUDLET_GROUPED_TEST_MAIN(MapCoordinateLimitTest)
