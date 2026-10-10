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
 * Switching the mapper between the 2D map and the modern 3D view hands the
 * shown area and center from one to the other.
 *
 * Why not a spec: where each view is centered is mMapCenterX/Y on the 2D map
 * and on the 3D widget, which no Lua call reports.
 *
 * Run with: ctest -R Map3DViewSyncTest -V
 */

#include <QSignalSpy>
#include <QTemporaryDir>
#include <QtTest/QtTest>

#include "Host.h"
#include "MudletApp.h"
#include "MudletInstanceCoordinator.h"
#include "PortableModeTestHelper.h"
#include "ProfileTestHelper.h"
#include "T2DMap.h"
#include "TMap.h"
#include "TRoomDB.h"
#include "TelnetServerStub.h"
#include "ctelnet.h"
#include "dlgMapper.h"
#include "modern_glwidget.h"
#include "mudlet.h"

#include "GroupedTest.h"

using namespace std::chrono_literals;

class Map3DViewSyncTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;
    TelnetServerStub* mpServer = nullptr;
    Host* mpHost = nullptr;
    const QString mProfileName = qsl("Map3DViewSync-Test");
    const QString mLocalhost = qsl("localhost");
    QString mPort;
    int mPlayerAreaId = 0;
    int mOtherAreaId = 0;
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

    dlgMapper* mapperShowing2DAtThePlayer() const
    {
        dlgMapper* pMapper = map()->mapper();
        if (!pMapper || !pMapper->mp2dMap) {
            return nullptr;
        }
        pMapper->show3DView(false);
        T2DMap* p2dMap = pMapper->mp2dMap;
        p2dMap->switchArea(mPlayerAreaId);
        p2dMap->mRoomID = kPlayerRoomId;
        p2dMap->mShiftMode = false;
        p2dMap->mMapCenterX = 0.0;
        p2dMap->mMapCenterY = 0.0;
        p2dMap->mMapCenterZ = 0;
        map()->mNewMove = false;
        return pMapper;
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
        QVERIFY2(connected.wait(3s), "could not connect to the telnet stub");
        QVERIFY(mpHost->setExperimentEnabled(qsl("experiment.3dmap.modernmapper"), true).first);

        mPlayerAreaId = map()->mpRoomDB->addArea(qsl("Player Area"));
        mOtherAreaId = map()->mpRoomDB->addArea(qsl("Other Area"));
        QVERIFY(mPlayerAreaId > 0 && mOtherAreaId > 0);
        QVERIFY(addRoomAt(kPlayerRoomId, mPlayerAreaId, 0, 0));
        // Far from where the 3D view is panned to below, so switching to this area cannot land there by chance
        QVERIFY(addRoomAt(2, mOtherAreaId, 20, 20));
        map()->mRoomIdHash[map()->mProfileName] = kPlayerRoomId;

        mpHost->showHideOrCreateMapper(false);
        QVERIFY2(map()->mapper(), "the profile's mapper could not be created");
        dlgMapper* pMapper = mapperShowing2DAtThePlayer();
        QVERIFY(pMapper);
        pMapper->show3DView(true);
        QVERIFY2(qobject_cast<ModernGLWidget*>(pMapper->glWidget), "the 3D view is not the modern one");
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

    void test_a3DViewPannedToAnotherAreaCarriesOverTo2D()
    {
        dlgMapper* pMapper = mapperShowing2DAtThePlayer();
        QVERIFY(pMapper);
        pMapper->show3DView(true);
        auto* pModern = qobject_cast<ModernGLWidget*>(pMapper->glWidget);
        QVERIFY(pModern);
        pModern->setViewCenter(mOtherAreaId, 3, 7, 0);
        pMapper->show3DView(false);

        T2DMap* p2dMap = pMapper->mp2dMap;
        QCOMPARE(p2dMap->mAreaID, mOtherAreaId);
        // The 2D map's y-axis points down the screen
        QCOMPARE(p2dMap->mMapCenterX, 3.0);
        QCOMPARE(p2dMap->mMapCenterY, -7.0);
        QVERIFY2(p2dMap->mShiftMode, "the 2D map went back to following the player");
        QCOMPARE(p2dMap->mRoomID, kPlayerRoomId);
    }

    // The 3D view centers on whole rooms, so taking its center back would move the 2D map
    void test_aFractional2DCenterSurvivesARoundTripThrough3D()
    {
        dlgMapper* pMapper = mapperShowing2DAtThePlayer();
        QVERIFY(pMapper);
        T2DMap* p2dMap = pMapper->mp2dMap;
        p2dMap->mShiftMode = true;
        p2dMap->mMapCenterX = 3.4;
        p2dMap->mMapCenterY = -7.6;
        pMapper->show3DView(true);
        pMapper->show3DView(false);

        QCOMPARE(p2dMap->mAreaID, mPlayerAreaId);
        QCOMPARE(p2dMap->mMapCenterX, 3.4);
        QCOMPARE(p2dMap->mMapCenterY, -7.6);
        QVERIFY(p2dMap->mShiftMode);
    }

    void test_aLevelChangeIn3DCarriesOverTo2D()
    {
        dlgMapper* pMapper = mapperShowing2DAtThePlayer();
        QVERIFY(pMapper);
        pMapper->show3DView(true);
        auto* pModern = qobject_cast<ModernGLWidget*>(pMapper->glWidget);
        QVERIFY(pModern);
        pMapper->toolButton_shiftZup->click();
        QCOMPARE(pModern->viewCenter().z(), 1.0f);
        pMapper->show3DView(false);

        QCOMPARE(pMapper->mp2dMap->mMapCenterZ, 1);
        QVERIFY(pMapper->mp2dMap->mShiftMode);
    }

    void test_aPlayerMoveSeenIn3DLeaves2DFollowing()
    {
        dlgMapper* pMapper = mapperShowing2DAtThePlayer();
        QVERIFY(pMapper);
        pMapper->show3DView(true);
        auto* pModern = qobject_cast<ModernGLWidget*>(pMapper->glWidget);
        QVERIFY(pModern);
        pModern->setViewCenter(mPlayerAreaId, 3, 7, 0);
        map()->mNewMove = true;
        pMapper->show3DView(false);

        QVERIFY2(!pMapper->mp2dMap->mShiftMode, "the 2D map kept a view the player has since moved away from");
    }
};

#include "Map3DViewSyncTest.moc"
MUDLET_GROUPED_TEST_MAIN(Map3DViewSyncTest)
