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
 * The mapCenterSmallAreas option makes a paint centre the view on the area
 * rather than on the player, when the whole area fits in the viewport.
 *
 * Why not a spec: the option is read from Mudlet.ini, and where the view ended
 * up is mMapCenterX/mMapCenterY on the widget, which no Lua call reports.
 *
 * Run with: ctest -R MapSmallAreaCenteringTest -V
 */

#include <QPixmap>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QtTest/QtTest>

#include "Host.h"
#include "MudletInstanceCoordinator.h"
#include "MudletApp.h"
#include "PortableModeTestHelper.h"
#include "ProfileTestHelper.h"
#include "T2DMap.h"
#include "TArea.h"
#include "TMap.h"
#include "TRoomDB.h"
#include "TelnetServerStub.h"
#include "ctelnet.h"
#include "dlgMapper.h"
#include "mudlet.h"

#include "GroupedTest.h"

class MapSmallAreaCenteringTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;
    TelnetServerStub* mpServer = nullptr;
    Host* mpHost = nullptr;
    const QString mProfileName = qsl("MapSmallAreaCentering-Test");
    const QString mLocalhost = qsl("localhost");
    QString mPort;

    // Half again as wide as it is tall, so the two axes cannot be confused for
    // each other: at this zoom the viewport holds 30 map units across and 20
    // down.
    static constexpr int kWidgetWidth = 600;
    static constexpr int kWidgetHeight = 400;
    static constexpr double kZoom = 20.0;
    static constexpr double kViewportUnitsAcross = kWidgetWidth / kZoom;
    static constexpr double kViewportUnitsDown = kWidgetHeight / kZoom;
    static constexpr int kPlayerRoomId = 1;

    TMap* map() const { return mpHost->mpMap.data(); }

    void deleteProfileDirectory() const
    {
        QDir dir(MudletApp::getMudletPath(enums::profileHomePath, mProfileName));
        if (dir.exists()) {
            dir.removeRecursively();
        }
    }

    bool addRoomAt(const int id, const int areaId, const int x, const int y) const { return map()->addRoom(id) && map()->setRoomArea(id, areaId) && map()->setRoomCoordinates(id, x, y, 0); }

    // The player stands in the first room given, which is what decides where a
    // paint that does not re-centre on the area leaves the view.
    int buildArea(const QList<QPoint>& roomPositions) const
    {
        TMap* pMap = map();
        pMap->mapClear();
        const int areaId = pMap->mpRoomDB->addArea(qsl("Centering Area"));
        if (areaId <= 0) {
            return 0;
        }
        int roomId = kPlayerRoomId;
        for (const QPoint& position : roomPositions) {
            if (!addRoomAt(roomId++, areaId, position.x(), position.y())) {
                return 0;
            }
        }
        pMap->mRoomIdHash[pMap->mProfileName] = kPlayerRoomId;
        TArea* pArea = pMap->mpRoomDB->getArea(areaId);
        if (!pArea) {
            return 0;
        }
        pArea->set2DMapZoom(kZoom);
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
        // Both false is what lets paintEvent() centre on the player room and
        // pick up the area and zoom the rooms were given.
        p2dMap->mShiftMode = false;
        p2dMap->mPick = false;
        p2dMap->mMultiSelectionSet.clear();
        p2dMap->mMapCenterX = 0.0;
        p2dMap->mMapCenterY = 0.0;
        p2dMap->mMapCenterZ = 0;
        return p2dMap;
    }

    static void renderFrame(T2DMap* p2dMap)
    {
        QPixmap target(kWidgetWidth, kWidgetHeight);
        target.fill(Qt::black);
        p2dMap->render(&target, QPoint(), QRegion(), QWidget::DrawWindowBackground);
        // paintEvent() works the viewport extents out for itself, so pin them
        // against what the cases below assume rather than trusting the constants
        QCOMPARE(static_cast<double>(p2dMap->mRoomWidth), kWidgetWidth / kViewportUnitsAcross);
        QCOMPARE(static_cast<double>(p2dMap->mRoomHeight), kWidgetHeight / kViewportUnitsDown);
    }

    // The published span is in screen coordinates - the negated room y - so an
    // area's height is read off the same values the paint works from.
    void verifyAreaSpan(const int areaId, const double expectedWidth, const double expectedHeight) const
    {
        const TArea* pArea = map()->mpRoomDB->getArea(areaId);
        QVERIFY(pArea);
        QVERIFY(pArea->xminForZ.contains(0));
        QVERIFY(pArea->yminForZ.contains(0));
        QCOMPARE(static_cast<double>(pArea->xmaxForZ.value(0) - pArea->xminForZ.value(0)), expectedWidth);
        QCOMPARE(static_cast<double>(pArea->ymaxForZ.value(0) - pArea->yminForZ.value(0)), expectedHeight);
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
        QCOMPARE(MudletApp::getMudletPath(enums::mainPath), qsl("%1/mudlet").arg(mConfigDir.path()));
        mudlet::self()->takeOwnershipOfInstanceCoordinator(std::make_unique<MudletInstanceCoordinator>("MudletInstanceCoordinator"));
        mudlet::self()->init();
        mudlet::self()->setStorePasswordsSecurely(false);
        mudlet::self()->mSkipDefaultPackageInstall = true;
        deleteProfileDirectory();

        mpHost = TestProfile::create(mProfileName, mLocalhost, mPort);
        QVERIFY(mpHost);
        QSignalSpy connected(&(mpHost->mTelnet), &cTelnet::signal_connected);
        QVERIFY2(connected.wait(3000), "could not connect to the telnet stub");
        mpHost->mMapperCenterSmallAreas = true;
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

    // An area narrow enough to fit across a wide mapper but far too tall for it
    // used to be centred sideways anyway, which put the player hard against one
    // edge of a view that was still following them (#8869).
    void test_anAreaTallerThanTheViewportIsNotRecentredSideways()
    {
        static constexpr double kAreaWidth = 10.0;
        static constexpr double kAreaHeight = 40.0;
        static_assert(kAreaWidth <= kViewportUnitsAcross, "the area has to fit across the viewport for this case to arise");
        static_assert(kAreaHeight > kViewportUnitsDown, "the area has to be taller than the viewport for this case to arise");

        const int areaId = buildArea({QPoint(10, 0), QPoint(0, 0), QPoint(10, 40)});
        QVERIFY2(areaId > 0, "the area under test could not be built");
        verifyAreaSpan(areaId, kAreaWidth, kAreaHeight);

        T2DMap* p2dMap = preparedWidget();
        QVERIFY2(p2dMap, "the profile's mapper could not be created");
        // The expected centre is also where a paint that never centred at all
        // would leave it, so these say the centring really was on offer
        QVERIFY(mpHost->mMapperCenterSmallAreas);
        QVERIFY(!p2dMap->mMultiSelection);
        QVERIFY(!p2dMap->mRoomBeingMoved);
        renderFrame(p2dMap);

        // Where the player is, not the middle of the area's 0 to 10 span.
        QCOMPARE(p2dMap->mMapCenterX, 10.0);
        QCOMPARE(p2dMap->mMapCenterY, 0.0);
    }

    // TArea publishes its y bounds in screen coordinates, so paintEvent()
    // negates them into room coordinates and has to negate the midpoint back
    // again to land on a screen-coordinate centre. Leaving that last negation
    // off put an area that did not straddle the origin twice its own offset
    // away from where it belonged (#8814).
    void test_aSmallAreaAwayFromTheOriginCentresOnWhereItsRoomsAre()
    {
        const int areaId = buildArea({QPoint(0, 10), QPoint(4, 14)});
        QVERIFY2(areaId > 0, "the area under test could not be built");
        verifyAreaSpan(areaId, 4.0, 4.0);

        T2DMap* p2dMap = preparedWidget();
        QVERIFY2(p2dMap, "the profile's mapper could not be created");
        renderFrame(p2dMap);

        // Rooms at y 10 and 14 are drawn at screen y -10 and -14, so the middle
        // of them is -12 - and not the -10 that following the player alone
        // would have given, which is what says the centring ran at all.
        QCOMPARE(p2dMap->mMapCenterX, 2.0);
        QCOMPARE(p2dMap->mMapCenterY, -12.0);
    }
};

#include "MapSmallAreaCenteringTest.moc"
MUDLET_GROUPED_TEST_MAIN(MapSmallAreaCenteringTest)
