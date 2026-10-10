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

#include <QDockWidget>
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
    static constexpr int kMovedToRoomId = 3;

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
        map()->mRoomIdHash[map()->mProfileName] = kPlayerRoomId;
        T2DMap* p2dMap = pMapper->mp2dMap;
        p2dMap->switchArea(mPlayerAreaId);
        p2dMap->mRoomID = kPlayerRoomId;
        p2dMap->mShiftMode = false;
        p2dMap->mMapCenterX = 0.0;
        p2dMap->mMapCenterY = 0.0;
        p2dMap->mMapCenterZ = 0;
        {
            const QSignalBlocker blocker(pMapper->comboBox_showArea);
            pMapper->comboBox_showArea->setCurrentText(qsl("Player Area"));
        }
        map()->mNewMove = false;
        return pMapper;
    }

    // The 3D view keeps the landscape shape it had when hidden, while its floating dock turns portrait
    static void hide3DViewInAPortraitDock(dlgMapper* pMapper, QDockWidget*& pDock, ModernGLWidget*& pModern)
    {
        pDock = qobject_cast<QDockWidget*>(pMapper->parentWidget());
        QVERIFY2(pDock, "the mapper is not in a dock, so its shape cannot be changed while the 3D view is hidden");
        pDock->setFloating(true);
        pDock->resize(900, 500);
        pMapper->show3DView(true);
        pModern = qobject_cast<ModernGLWidget*>(pMapper->glWidget);
        QVERIFY(pModern);
        QTRY_VERIFY(pModern->isVisible() && pModern->width() > pModern->height());
        pMapper->show3DView(false);
        pDock->resize(300, 900);
        QTRY_VERIFY(pMapper->height() > pMapper->width());
        // As when a player switches back later: with the controls gone, showing the view lays it out
        // once without them, then again, smaller, once they reappear
        QTRY_VERIFY(pMapper->widget_3DControls->isHidden());
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
        QVERIFY(addRoomAt(kMovedToRoomId, mPlayerAreaId, 5, 5));
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
        pModern->setViewCenter(mOtherAreaId, 3, 7, 1);
        pMapper->show3DView(false);

        T2DMap* p2dMap = pMapper->mp2dMap;
        QCOMPARE(p2dMap->mAreaID, mOtherAreaId);
        QCOMPARE(pMapper->comboBox_showArea->currentText(), qsl("Other Area"));
        // The 2D map's y-axis points down the screen
        QCOMPARE(p2dMap->mMapCenterX, 3.0);
        QCOMPARE(p2dMap->mMapCenterY, -7.0);
        QCOMPARE(p2dMap->mMapCenterZ, 1);
        QVERIFY2(p2dMap->mShiftMode, "the 2D map went back to following the player");
    }

    void test_a3DViewPannedWithinTheAreaCarriesOverTo2D()
    {
        dlgMapper* pMapper = mapperShowing2DAtThePlayer();
        QVERIFY(pMapper);
        pMapper->show3DView(true);
        auto* pModern = qobject_cast<ModernGLWidget*>(pMapper->glWidget);
        QVERIFY(pModern);
        pModern->setViewCenter(mPlayerAreaId, 3, 7, 0);
        pMapper->show3DView(false);

        T2DMap* p2dMap = pMapper->mp2dMap;
        QCOMPARE(p2dMap->mAreaID, mPlayerAreaId);
        QCOMPARE(p2dMap->mMapCenterX, 3.0);
        QCOMPARE(p2dMap->mMapCenterY, -7.0);
        QVERIFY2(p2dMap->mShiftMode, "the 2D map went back to following the player");
    }

    // The hidden 2D map has not painted since the player moved, so its own idea of the player's room is stale,
    // and its next paint would drop a view kept for a room the player has left
    void test_a3DViewPannedAfterThePlayerMovedStaysPannedIn2D()
    {
        dlgMapper* pMapper = mapperShowing2DAtThePlayer();
        QVERIFY(pMapper);
        pMapper->show3DView(true);
        auto* pModern = qobject_cast<ModernGLWidget*>(pMapper->glWidget);
        QVERIFY(pModern);
        map()->mRoomIdHash[map()->mProfileName] = kMovedToRoomId;
        // As if the 3D view had painted the move
        map()->mNewMove = false;
        pModern->setViewCenter(mPlayerAreaId, 3, 7, 0);
        pMapper->show3DView(false);

        T2DMap* p2dMap = pMapper->mp2dMap;
        QCOMPARE(p2dMap->mRoomID, kMovedToRoomId);
        QVERIFY(p2dMap->mShiftMode);
        QCOMPARE(p2dMap->mMapCenterX, 3.0);
        QCOMPARE(p2dMap->mMapCenterY, -7.0);
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
        // The same button moves the hidden 2D map too, so put it elsewhere to see the 3D level come back
        pMapper->mp2dMap->mMapCenterZ = 5;
        pMapper->show3DView(false);

        QCOMPARE(pMapper->mp2dMap->mMapCenterZ, 1);
        QVERIFY(pMapper->mp2dMap->mShiftMode);
    }

    void test_aPlayerMoveSeenIn3DLeaves2DFollowing()
    {
        dlgMapper* pMapper = mapperShowing2DAtThePlayer();
        QVERIFY(pMapper);
        // Every paint of the 2D map leaves it in shift mode
        pMapper->mp2dMap->mShiftMode = true;
        pMapper->show3DView(true);
        auto* pModern = qobject_cast<ModernGLWidget*>(pMapper->glWidget);
        QVERIFY(pModern);
        pModern->setViewCenter(mPlayerAreaId, 3, 7, 0);
        map()->mNewMove = true;
        pMapper->show3DView(false);

        QVERIFY2(!pMapper->mp2dMap->mShiftMode, "the 2D map kept a view the player has since moved away from");
    }

    void test_showingThePlayersAreaMakesThe3DViewFollowAgain()
    {
        dlgMapper* pMapper = mapperShowing2DAtThePlayer();
        QVERIFY(pMapper);
        pMapper->show3DView(true);
        auto* pModern = qobject_cast<ModernGLWidget*>(pMapper->glWidget);
        QVERIFY(pModern);
        pModern->setViewCenter(mOtherAreaId, 3, 7, 0);
        QVERIFY(!pModern->followingPlayer());
        pMapper->resetAreaComboBoxToPlayerRoomArea();

        QVERIFY2(pModern->followingPlayer(), "the 3D view stayed on the area it was panned to");
    }

    // The sliders set the camera from the angles the others read back, and looking straight down
    // the roll must not read back as a turn
    void test_theRollAndTurnSlidersEachSetTheirOwnAngleOfTheTopView()
    {
        dlgMapper* pMapper = mapperShowing2DAtThePlayer();
        QVERIFY(pMapper);
        pMapper->show3DView(true);
        auto* pModern = qobject_cast<ModernGLWidget*>(pMapper->glWidget);
        QVERIFY(pModern);
        pMapper->pushButton_topView->click();
        QCOMPARE(pMapper->slider_xRot->value(), 100);
        for (int roll = 1; roll <= 20; ++roll) {
            pMapper->slider_yRot->setValue(roll);
        }
        // Has the view report its camera back to the sliders
        pModern->shiftCamera(0.0f, 0.0f, 0.0f);
        QCOMPARE(pMapper->slider_yRot->value(), 20);
        QCOMPARE(pMapper->slider_zRot->value(), 0);

        pMapper->slider_zRot->setValue(30);
        pModern->shiftCamera(0.0f, 0.0f, 0.0f);
        QCOMPARE(pMapper->slider_xRot->value(), 100);
        QCOMPARE(pMapper->slider_yRot->value(), 20);
        QCOMPARE(pMapper->slider_zRot->value(), 30);
    }

    void test_setMapZoomFramesThatManyRoomsIn3D()
    {
        dlgMapper* pMapper = mapperShowing2DAtThePlayer();
        QVERIFY(pMapper);
        pMapper->show3DView(true);
        auto* pModern = qobject_cast<ModernGLWidget*>(pMapper->glWidget);
        QVERIFY(pModern);
        QSignalSpy cameraControls(pModern, &ModernGLWidget::cameraControlsChanged);
        const auto lastScale = [&cameraControls]() {
            return cameraControls.isEmpty() ? -1 : cameraControls.last().at(0).toInt();
        };

        QVERIFY(pMapper->setMapZoom(5.0, 0).first);
        const int fewRooms = lastScale();
        QVERIFY(pMapper->setMapZoom(20.0, 0).first);
        const int manyRooms = lastScale();
        QVERIFY2(fewRooms != -1 && manyRooms != fewRooms, "the 3D view did not zoom with setMapZoom");
        QVERIFY2(manyRooms < fewRooms, "showing more rooms should move the 3D camera away, which lowers the scale");

        cameraControls.clear();
        QVERIFY(!pMapper->setMapZoom(2.9, 0).first);
        QVERIFY(!pMapper->setMapZoom(qQNaN(), 0).first);
        QVERIFY2(cameraControls.isEmpty(), "a zoom the 2D map refused still moved the 3D camera");

        // Asking again for the zoom the area already has must still undo a zoom the player made with the scale slider
        pModern->slot_setScale(fewRooms);
        cameraControls.clear();
        QVERIFY(pMapper->setMapZoom(20.0, 0).first);
        QCOMPARE(lastScale(), manyRooms);

        const qsizetype emitted = cameraControls.size();
        QVERIFY(pMapper->setMapZoom(9.0, mOtherAreaId).first);
        QCOMPARE(cameraControls.size(), emitted);
    }

    void test_aZoomMadeWhileThe3DViewIsHiddenFitsItsShapeOnceShown()
    {
        dlgMapper* pMapper = mapperShowing2DAtThePlayer();
        QVERIFY(pMapper);
        QDockWidget* pDock = nullptr;
        ModernGLWidget* pModern = nullptr;
        hide3DViewInAPortraitDock(pMapper, pDock, pModern);
        if (QTest::currentTestFailed()) {
            return;
        }
        QSignalSpy cameraControls(pModern, &ModernGLWidget::cameraControlsChanged);
        const auto lastScale = [&cameraControls]() {
            return cameraControls.isEmpty() ? -1 : cameraControls.last().at(0).toInt();
        };
        QVERIFY(pMapper->setMapZoom(20.0, 0).first);
        const int framedWhileHidden = lastScale();

        pMapper->show3DView(true);
        QTRY_VERIFY(pModern->isVisible() && pModern->height() > pModern->width());
        const int framedOnceShown = lastScale();

        // Once framed for its new shape, a zoom of the player's own outlasts later resizes
        pModern->slot_setScale(framedOnceShown + 7);
        cameraControls.clear();
        pDock->resize(320, 950);
        QTRY_COMPARE(pDock->height(), 950);
        QCoreApplication::processEvents();
        QVERIFY2(cameraControls.isEmpty(), "a resize after the hidden zoom had been framed undid the player's zoom");
        pDock->resize(300, 900);
        QTRY_COMPARE(pDock->height(), 900);

        QVERIFY(pMapper->setMapZoom(20.0, 0).first);
        const int framedWhileShown = lastScale();
        QVERIFY2(framedWhileShown != framedWhileHidden, "the 3D view's shape did not change, so this cannot tell a stale framing apart");
        QCOMPARE(framedOnceShown, framedWhileShown);

        pMapper->show3DView(false);
        pDock->setFloating(false);
    }

    void test_aDistanceAskedForAsThe3DViewIsShownWinsOverAHiddenZoom()
    {
        dlgMapper* pMapper = mapperShowing2DAtThePlayer();
        QVERIFY(pMapper);
        QDockWidget* pDock = nullptr;
        ModernGLWidget* pModern = nullptr;
        hide3DViewInAPortraitDock(pMapper, pDock, pModern);
        if (QTest::currentTestFailed()) {
            return;
        }
        QSignalSpy cameraControls(pModern, &ModernGLWidget::cameraControlsChanged);
        const auto lastScale = [&cameraControls]() {
            return cameraControls.isEmpty() ? -1 : cameraControls.last().at(0).toInt();
        };
        QVERIFY(pMapper->setMapZoom(20.0, 0).first);
        const int framedWhileHidden = lastScale();

        // As a script would in one go: show3DView lays the view out at once, so the camera is set before any paint
        pMapper->show3DView(true);
        QVERIFY(pModern->isVisible() && pModern->height() > pModern->width());
        QVERIFY(pMapper->set3DViewCameraPosition(2.0f, 45.0f, 270.0f).first);
        const int requested = lastScale();
        QVERIFY(requested != framedWhileHidden);
        // Neither a later resize nor a paint may reframe it
        pDock->resize(320, 950);
        QTRY_COMPARE(pDock->height(), 950);
        QCoreApplication::processEvents();
        QCOMPARE(lastScale(), requested);

        pMapper->show3DView(false);
        pDock->setFloating(false);
    }

    void test_aResizeLeavesTheCameraAloneWithNoZoomPending()
    {
        dlgMapper* pMapper = mapperShowing2DAtThePlayer();
        QVERIFY(pMapper);
        auto* pDock = qobject_cast<QDockWidget*>(pMapper->parentWidget());
        QVERIFY(pDock);
        pDock->setFloating(true);
        pDock->resize(900, 500);
        pMapper->show3DView(true);
        auto* pModern = qobject_cast<ModernGLWidget*>(pMapper->glWidget);
        QVERIFY(pModern);
        QTRY_VERIFY(pModern->isVisible() && pModern->width() > pModern->height());
        QVERIFY(pMapper->setMapZoom(20.0, 0).first);
        QSignalSpy cameraControls(pModern, &ModernGLWidget::cameraControlsChanged);

        pDock->resize(300, 900);
        QTRY_VERIFY(pModern->height() > pModern->width());
        QCoreApplication::processEvents();
        QVERIFY2(cameraControls.isEmpty(), "resizing the 3D view moved its camera");

        pMapper->show3DView(false);
        pDock->setFloating(false);
    }
};

#include "Map3DViewSyncTest.moc"
MUDLET_GROUPED_TEST_MAIN(Map3DViewSyncTest)
