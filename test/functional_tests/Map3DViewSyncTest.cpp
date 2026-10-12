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

#include <QOpenGLContext>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QtTest/QtTest>

#include "Host.h"
#include "MudletApp.h"
#include "MudletInstanceCoordinator.h"
#include "PortableModeTestHelper.h"
#include "ProfileTestHelper.h"
#include "T2DMap.h"
#include "TArea.h"
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

    // There is no GL to start the view with here; what a camera call does to the camera needs none
    static auto standInForAViewThatStarted(ModernGLWidget* pModern)
    {
        const QString failure = pModern->mFailureMessage;
        pModern->mFailureMessage.clear();
        return qScopeGuard([pModern, failure]() {
            pModern->mFailureMessage = failure;
        });
    }

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
        // Has the view report its camera back to the sliders; without the report they keep what was set
        QSignalSpy reported(pModern, &ModernGLWidget::cameraControlsChanged);
        pModern->shiftCamera(0.0f, 0.0f, 0.0f);
        QCOMPARE(reported.count(), 1);
        QCOMPARE(pMapper->slider_yRot->value(), 20);
        QCOMPARE(pMapper->slider_zRot->value(), 0);

        pMapper->slider_zRot->setValue(30);
        pModern->shiftCamera(0.0f, 0.0f, 0.0f);
        QCOMPARE(reported.count(), 2);
        QCOMPARE(pMapper->slider_xRot->value(), 100);
        QCOMPARE(pMapper->slider_yRot->value(), 20);
        QCOMPARE(pMapper->slider_zRot->value(), 30);
    }

    // While the view follows the player, the next paint shows the player's area, wherever the view is now
    void test_theGridModeCheckLooksAtTheAreaTheNextPaintShows()
    {
        dlgMapper* pMapper = mapperShowing2DAtThePlayer();
        QVERIFY(pMapper);
        pMapper->show3DView(true);
        auto* pModern = qobject_cast<ModernGLWidget*>(pMapper->glWidget);
        QVERIFY(pModern);
        TArea* pPlayerArea = map()->mpRoomDB->getArea(mPlayerAreaId);
        TArea* pOtherArea = map()->mpRoomDB->getArea(mOtherAreaId);
        QVERIFY(pPlayerArea && pOtherArea);
        const auto noGridMode = qScopeGuard([pPlayerArea, pOtherArea]() {
            pPlayerArea->gridMode = false;
            pOtherArea->gridMode = false;
        });

        pOtherArea->gridMode = true;
        pModern->setViewCenter(mOtherAreaId, 20, 20, 0);
        QVERIFY2(pModern->cameraFixedByGridMode(), "the view is panned to a grid-mode area");
        pModern->followPlayer();
        QCOMPARE(pModern->shownAreaId(), mOtherAreaId);
        QVERIFY2(!pModern->cameraFixedByGridMode(), "the next paint shows the player's area, which is not in grid mode");

        pOtherArea->gridMode = false;
        pPlayerArea->gridMode = true;
        QVERIFY2(pModern->cameraFixedByGridMode(), "the next paint shows the player's area, which is in grid mode");
        pModern->setViewCenter(mOtherAreaId, 20, 20, 0);
        QVERIFY2(!pModern->cameraFixedByGridMode(), "the view is panned away from the player's grid-mode area");
    }

    // Grid mode fixes the angles, not the distance
    void test_aGridModeAreaStillTakesTheDistance()
    {
        dlgMapper* pMapper = mapperShowing2DAtThePlayer();
        QVERIFY(pMapper);
        TArea* pPlayerArea = map()->mpRoomDB->getArea(mPlayerAreaId);
        QVERIFY(pPlayerArea);
        const auto noGridMode = qScopeGuard([pPlayerArea]() {
            pPlayerArea->gridMode = false;
        });
        pPlayerArea->gridMode = true;
        pMapper->show3DView(true);
        auto* pModern = qobject_cast<ModernGLWidget*>(pMapper->glWidget);
        QVERIFY(pModern);
        const auto restoreFailure = standInForAViewThatStarted(pModern);
        pModern->mCameraController.setScale(CameraController::scmMaxDistance);
        pModern->mDistanceRequested = false;
        {
            const QSignalBlocker blocker(pMapper->slider_scale);
            pMapper->slider_scale->setValue(0);
        }
        const auto [moved, refusal] = pMapper->set3DViewCameraPosition(0.5f, 45.0f, 270.0f);
        QVERIFY(!moved);
        QCOMPARE(refusal, qsl("the area shown in the 3D view is in grid mode, which always looks straight down, so only the distance was applied"));
        QCOMPARE(pModern->mCameraController.getScale(), 0.5f);
        QVERIFY2(pModern->mDistanceRequested, "the next paint would frame the area over the distance asked for");
        // A stale slider would put the old distance back the next time the user touched it
        QVERIFY2(pMapper->slider_scale->value() > 0, "the zoom slider still shows the old distance");
    }

    void test_aCameraMoveThatSucceedsMovesTheCamera()
    {
        dlgMapper* pMapper = mapperShowing2DAtThePlayer();
        QVERIFY(pMapper);
        pMapper->show3DView(true);
        auto* pModern = qobject_cast<ModernGLWidget*>(pMapper->glWidget);
        QVERIFY(pModern);
        QVERIFY(!pModern->cameraFixedByGridMode());
        const auto restoreFailure = standInForAViewThatStarted(pModern);
        pModern->mCameraController.setPosition(1.0f, 10.0f, 100.0f);
        pModern->mDistanceRequested = false;
        {
            const QSignalBlocker scaleBlocker(pMapper->slider_scale);
            const QSignalBlocker turnBlocker(pMapper->slider_zRot);
            pMapper->slider_scale->setValue(0);
            pMapper->slider_zRot->setValue(90);
        }

        QCOMPARE(pMapper->set3DViewCameraPosition(2.0f, 45.0f, 270.0f), std::make_pair(true, QString()));
        QVector3D position = pModern->mCameraController.getPosition();
        QVERIFY2(qAbs(position.x() - 2.0f) < 0.01f && qAbs(position.y() - 45.0f) < 0.01f && qAbs(std::remainder(position.z() - 270.0f, 360.0f)) < 0.01f,
                 qPrintable(qsl("asked for (2, 45, 270), the camera is at (%1, %2, %3)").arg(position.x()).arg(position.y()).arg(position.z())));
        QVERIFY2(pModern->mDistanceRequested, "the next paint would frame the area over the distance asked for");
        QVERIFY2(pMapper->slider_scale->value() > 0, "the zoom slider still shows the old distance");
        QCOMPARE(pMapper->slider_zRot->value(), 0);

        QCOMPARE(pMapper->shift3DViewCamera(0.0f, 30.0f, 0.0f), std::make_pair(true, QString()));
        position = pModern->mCameraController.getPosition();
        QVERIFY2(qAbs(std::remainder(position.z() - 300.0f, 360.0f)) < 0.01f, qPrintable(qsl("turned by 30 from 270, the camera faces %1").arg(position.z())));
    }

    // The view only shows why it could not start, so moving its camera would do nothing; asked with no event loop
    // in between, as a script showing the view and setting its camera in one go does
    void test_aViewThatCouldNotStartRefusesCameraMoves()
    {
        dlgMapper* pMapper = mapperShowing2DAtThePlayer();
        QVERIFY(pMapper);
        // A fresh view, whose failure no earlier test's event loop can have reported; recreate3DView() keeps a
        // widget of the right kind, so go by way of the classic one
        const QPointer<QOpenGLWidget> pOldWidget = pMapper->glWidget;
        QVERIFY(mpHost->setExperimentEnabled(qsl("experiment.3dmap.modernmapper"), false).first);
        pMapper->recreate3DView();
        QVERIFY(mpHost->setExperimentEnabled(qsl("experiment.3dmap.modernmapper"), true).first);
        pMapper->recreate3DView();
        QVERIFY2(pMapper->glWidget != pOldWidget, "the 3D view was not made afresh");
        pMapper->show3DView(true);
        auto* pModern = qobject_cast<ModernGLWidget*>(pMapper->glWidget);
        QVERIFY(pModern);
        QVERIFY2(pModern->isVisible() && !pModern->size().isEmpty(), "the 3D view is not showing, so nothing tried to start it");
        const bool showTriedForAContext = pModern->context();
        const QString failureOnShow = pModern->failureMessage();
        if (!showTriedForAContext) {
            // Where GL works, Qt drops the context of a view shown straight after switching renderers and makes it again
            // on resize; this resize only finds out whether GL works, so the checks below go by what the show did
            QResizeEvent resize(pModern->size(), pModern->size());
            QCoreApplication::sendEvent(pModern, &resize);
        }
        if (pModern->isValid()) {
            QSKIP("this machine created a GL context for the modern 3D view");
        }
        QVERIFY2(showTriedForAContext, "showing the view did not try for a GL context, so a script moving the camera straight after is not refused");
        QVERIFY2(!failureOnShow.isEmpty(), "the modern 3D view has not said why it could not start");
        const QString refusal = qsl("the modern 3D map view could not start: %1").arg(pModern->failureMessage());
        QCOMPARE(pMapper->set3DViewCameraPosition(2.0f, 45.0f, 270.0f), std::make_pair(false, refusal));
        QCOMPARE(pMapper->shift3DViewCamera(0.0f, 30.0f, 0.0f), std::make_pair(false, refusal));
    }

    // Each retry would leak the context that failed, for either renderer
    void test_aContextThatCouldNotBeCreatedIsNotRetried()
    {
        dlgMapper* pMapper = mapperShowing2DAtThePlayer();
        QVERIFY(pMapper);
        for (const bool modern : {false, true}) {
            QVERIFY(mpHost->setExperimentEnabled(qsl("experiment.3dmap.modernmapper"), modern).first);
            pMapper->recreate3DView();
            pMapper->show3DView(true);
            QCoreApplication::processEvents();
            QOpenGLWidget* pWidget = pMapper->glWidget;
            QVERIFY(pWidget);
            QCOMPARE(qobject_cast<ModernGLWidget*>(pWidget) != nullptr, modern);
            if (pWidget->isValid()) {
                QSKIP("this machine created a GL context, so there is no failed one to retry");
            }
            QOpenGLContext* pFirstContext = pWidget->context();
            QVERIFY2(pFirstContext, "nothing tried to create a GL context");
            pWidget->hide();
            pWidget->show();
            pWidget->resize(pWidget->size() + QSize(10, 10));
            pWidget->repaint();
            QCoreApplication::processEvents();
            // A leaked context is never freed, so its address cannot come back for a new one
            QVERIFY2(pWidget->context() == pFirstContext, modern ? "the modern 3D view tried again for a context" : "the classic 3D view tried again for a context");
        }
    }
};

#include "Map3DViewSyncTest.moc"
MUDLET_GROUPED_TEST_MAIN(Map3DViewSyncTest)
