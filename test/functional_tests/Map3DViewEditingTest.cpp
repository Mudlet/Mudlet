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
 * Editing the map from the modern 3D view as on the 2D map: box-selecting and
 * dragging rooms, the 3D view in a new map window, and handing over to the 2D
 * map to edit there.
 *
 * Why not a spec: no Lua call can drag on the map or open a map window's 3D
 * view, and the 3D view's camera is not reported to Lua.
 *
 * Run with: ctest -R Map3DViewEditingTest -V
 */

#include <QAction>
#include <QDockWidget>
#include <QMenu>
#include <QSignalSpy>
#include <QSlider>
#include <QTemporaryDir>
#include <QToolButton>
#include <QtTest/QtTest>

#include "Host.h"
#include "MudletApp.h"
#include "MudletInstanceCoordinator.h"
#include "PortableModeTestHelper.h"
#include "ProfileTestHelper.h"
#include "T2DMap.h"
#include "TArea.h"
#include "TLuaInterpreter.h"
#include "TMap.h"
#include "TMapView.h"
#include "TMapViewManager.h"
#include "TRoom.h"
#include "TRoomDB.h"
#include "TelnetServerStub.h"
#include "ctelnet.h"
#include "dlgMapper.h"
#include "modern_glwidget.h"
#include "mudlet.h"

#include "GroupedTest.h"

using namespace std::chrono_literals;

class Map3DViewEditingTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;
    TelnetServerStub* mpServer = nullptr;
    Host* mpHost = nullptr;
    const QString mProfileName = qsl("Map3DViewEditing-Test");
    const QString mLocalhost = qsl("localhost");
    QString mPort;
    int mPlayerAreaId = 0;
    int mOtherAreaId = 0;
    static constexpr int kPlayerRoomId = 1;
    static constexpr int kEastRoomId = 2;
    static constexpr int kFarEastRoomId = 3;
    static constexpr int kUpperRoomId = 4;
    static constexpr int kOtherAreaRoomId = 10;

    TMap* map() const { return mpHost->mpMap.data(); }

    void deleteProfileDirectory() const
    {
        QDir dir(MudletApp::getMudletPath(enums::profileHomePath, mProfileName));
        if (dir.exists()) {
            dir.removeRecursively();
        }
    }

    bool addRoomAt(const int id, const int areaId, const int x, const int y, const int z = 0) const
    {
        return map()->addRoom(id) && map()->setRoomArea(id, areaId) && map()->setRoomCoordinates(id, x, y, z);
    }

    // Every room back where initTestCase() put it
    void resetRooms() const
    {
        const QList<std::tuple<int, int, int, int>> places{{kPlayerRoomId, 0, 0, 0}, {kEastRoomId, 2, 0, 0}, {kFarEastRoomId, 6, 0, 0}, {kUpperRoomId, 2, 2, 1}};
        for (const auto& [roomId, x, y, z] : places) {
            map()->setRoomCoordinates(roomId, x, y, z);
        }
    }

    // The 3D view on the player's area, looking straight down with north up, nothing selected
    ModernGLWidget* viewAtThePlayer(const bool editing) const
    {
        dlgMapper* pMapper = map()->mapper();
        if (!pMapper || !pMapper->mp2dMap) {
            return nullptr;
        }
        pMapper->show3DView(false);
        resetRooms();
        map()->mRoomIdHash[map()->mProfileName] = kPlayerRoomId;
        map()->mNewMove = false;
        T2DMap* p2dMap = pMapper->mp2dMap;
        if (p2dMap->mMapViewOnly == editing) {
            p2dMap->slot_toggleMapViewOnly();
        }
        p2dMap->switchArea(mPlayerAreaId);
        p2dMap->mRoomID = kPlayerRoomId;
        p2dMap->mShiftMode = false;
        p2dMap->mMapCenterX = 0.0;
        p2dMap->mMapCenterY = 0.0;
        p2dMap->mMapCenterZ = 0;
        p2dMap->mMultiSelectionSet.clear();
        pMapper->show3DView(true);
        auto* pModern = qobject_cast<ModernGLWidget*>(pMapper->glWidget);
        if (!pModern) {
            return nullptr;
        }
        pModern->setCameraPosition(1.0f, CameraController::scmMinTilt, CameraController::scmNorthUpAzimuth);
        return pModern;
    }

    // The middle of where the view draws the room, or (-1, -1) when it is not on screen
    static QPoint pixelOfRoom(ModernGLWidget* pModern, const int roomId)
    {
        QPoint sum;
        int hits = 0;
        for (int y = 0; y < pModern->height(); y += 3) {
            for (int x = 0; x < pModern->width(); x += 3) {
                if (pModern->roomAt(QPoint(x, y)) == roomId) {
                    sum += QPoint(x, y);
                    ++hits;
                }
            }
        }
        return hits ? sum / hits : QPoint(-1, -1);
    }

    QSet<int> selection() const { return map()->mapper()->mp2dMap->mMultiSelectionSet; }

    QVector3D roomPlace(const int roomId) const
    {
        const TRoom* pRoom = map()->mpRoomDB->getRoom(roomId);
        return pRoom ? QVector3D(pRoom->x(), pRoom->y(), pRoom->z()) : QVector3D();
    }

    // A box around these points, reaching a third of the way to the next room on the grid, where no room is
    static QRect boxAround(const QList<QPoint>& points, const int roomSpacing)
    {
        QRect box(points.first(), QSize(1, 1));
        for (const QPoint& point : points) {
            box = box.united(QRect(point, QSize(1, 1)));
        }
        const int margin = roomSpacing / 3;
        return box.adjusted(-margin, -margin, margin, margin);
    }

    static QToolButton* buttonNamed(QWidget* pParent, const QString& text)
    {
        for (QToolButton* button : pParent->findChildren<QToolButton*>()) {
            if (button->text() == text) {
                return button;
            }
        }
        return nullptr;
    }

    static QMenu* openMenu(QWidget* pView)
    {
        for (QMenu* menu : pView->findChildren<QMenu*>()) {
            if (menu->isVisible()) {
                return menu;
            }
        }
        return nullptr;
    }

    static QAction* actionNamed(const QMenu* menu, const QString& text)
    {
        for (QAction* action : menu->actions()) {
            if (action->text() == text) {
                return action;
            }
        }
        return nullptr;
    }

    // A left-button drag as a hand makes it, in several moves
    static void drag(QWidget* pView, const QPoint& from, const QPoint& to, const Qt::KeyboardModifiers modifiers)
    {
        QTest::mousePress(pView, Qt::LeftButton, modifiers, from);
        constexpr int steps = 8;
        for (int step = 1; step <= steps; ++step) {
            const QPointF point = QPointF(from) + QPointF(to - from) * step / steps;
            QMouseEvent move(QEvent::MouseMove, point, pView->mapToGlobal(point), Qt::NoButton, Qt::LeftButton, modifiers);
            QCoreApplication::sendEvent(pView, &move);
        }
        QTest::mouseRelease(pView, Qt::LeftButton, modifiers, to);
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
        QVERIFY(addRoomAt(kEastRoomId, mPlayerAreaId, 2, 0));
        QVERIFY(addRoomAt(kFarEastRoomId, mPlayerAreaId, 6, 0));
        QVERIFY(addRoomAt(kUpperRoomId, mPlayerAreaId, 2, 2, 1));
        QVERIFY(addRoomAt(kOtherAreaRoomId, mOtherAreaId, 20, 20));
        map()->mRoomIdHash[map()->mProfileName] = kPlayerRoomId;

        mpHost->showHideOrCreateMapper(false);
        QVERIFY2(map()->mapper(), "the profile's mapper could not be created");
        auto* pDock = qobject_cast<QDockWidget*>(map()->mapper()->parentWidget());
        QVERIFY(pDock);
        pDock->setFloating(true);
        pDock->resize(800, 600);
        ModernGLWidget* pModern = viewAtThePlayer(false);
        QVERIFY2(pModern, "the 3D view is not the modern one");
        QTRY_VERIFY(pModern->isVisible() && pModern->height() > 300);
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

    // In editing mode a drag from empty space picks the rooms it covers, with Ctrl and Shift as on the 2D map
    void test_dragRectangleSelectsRoomsInEditingMode()
    {
        ModernGLWidget* pModern = viewAtThePlayer(true);
        QVERIFY(pModern);
        const QPoint player = pixelOfRoom(pModern, kPlayerRoomId);
        const QPoint east = pixelOfRoom(pModern, kEastRoomId);
        const QPoint farEast = pixelOfRoom(pModern, kFarEastRoomId);
        const QPoint upper = pixelOfRoom(pModern, kUpperRoomId);
        for (const QPoint& pixel : {player, east, farEast, upper}) {
            QVERIFY2(pixel.x() >= 0, "a room is not on screen");
        }
        // Two grid steps apart
        const int spacing = east.x() - player.x();
        QVERIFY(spacing > 20);

        QRect box = boxAround({player, east}, spacing);
        drag(pModern, box.topLeft(), box.bottomRight(), Qt::NoModifier);
        QCOMPARE(selection(), (QSet<int>{kPlayerRoomId, kEastRoomId}));

        // Ctrl toggles the rooms covered
        box = boxAround({east}, spacing);
        drag(pModern, box.topLeft(), box.bottomRight(), Qt::ControlModifier);
        QCOMPARE(selection(), (QSet<int>{kPlayerRoomId}));

        // Shift adds them, and reaches rooms on other levels on show
        box = boxAround({farEast}, spacing);
        drag(pModern, box.topLeft(), box.bottomRight(), Qt::ShiftModifier);
        QCOMPARE(selection(), (QSet<int>{kPlayerRoomId, kFarEastRoomId}));
        box = boxAround({upper}, spacing);
        drag(pModern, box.topLeft(), box.bottomRight(), Qt::ShiftModifier);
        QCOMPARE(selection(), (QSet<int>{kPlayerRoomId, kFarEastRoomId, kUpperRoomId}));

        // Without Shift only the level in the middle of the view counts
        box = boxAround({east, upper}, spacing);
        drag(pModern, box.topLeft(), box.bottomRight(), Qt::NoModifier);
        QCOMPARE(selection(), (QSet<int>{kEastRoomId}));

        // A box around nothing selects nothing, and none of this moved a room or the view
        box = boxAround({QPoint((player.x() + east.x()) / 2, player.y() + spacing)}, spacing);
        drag(pModern, box.topLeft(), box.bottomRight(), Qt::NoModifier);
        QCOMPARE(selection(), QSet<int>());
        QCOMPARE(roomPlace(kPlayerRoomId), QVector3D(0, 0, 0));
        QCOMPARE(roomPlace(kEastRoomId), QVector3D(2, 0, 0));
        QCOMPARE(pModern->viewCenter(), QVector3D(0, 0, 0));
    }

    // Viewing mode keeps its drag to pan
    void test_dragInViewingModePansRatherThanSelecting()
    {
        ModernGLWidget* pModern = viewAtThePlayer(false);
        QVERIFY(pModern);
        const QPoint player = pixelOfRoom(pModern, kPlayerRoomId);
        const QPoint east = pixelOfRoom(pModern, kEastRoomId);
        const int spacing = east.x() - player.x();
        const QRect box = boxAround({player, east}, spacing);
        drag(pModern, box.topLeft(), box.bottomRight(), Qt::NoModifier);
        QCOMPARE(selection(), QSet<int>());
        QVERIFY(pModern->viewCenter() != QVector3D(0, 0, 0));
    }

    // In editing mode a drag from a selected room moves the whole selection, in whole grid steps, as on the 2D map
    void test_draggingSelectedRoomsMovesThemOnTheGrid()
    {
        ModernGLWidget* pModern = viewAtThePlayer(true);
        QVERIFY(pModern);
        const QPoint player = pixelOfRoom(pModern, kPlayerRoomId);
        const QPoint east = pixelOfRoom(pModern, kEastRoomId);
        const int spacing = east.x() - player.x();
        QVERIFY(spacing > 20);
        QTest::mouseClick(pModern, Qt::LeftButton, Qt::NoModifier, player);
        QTest::mouseClick(pModern, Qt::LeftButton, Qt::ControlModifier, east);
        QCOMPARE(selection(), (QSet<int>{kPlayerRoomId, kEastRoomId}));
        map()->resetUnsaved();
        pModern->followPlayer();
        QVERIFY(pModern->followingPlayer());

        // Three steps east and one north, from a little off the room's middle
        const QPoint grab = east + QPoint(spacing / 12, spacing / 12);
        drag(pModern, grab, grab + QPoint(spacing * 3 / 2, -spacing / 2), Qt::NoModifier);
        QCOMPARE(roomPlace(kPlayerRoomId), QVector3D(3, 1, 0));
        QCOMPARE(roomPlace(kEastRoomId), QVector3D(5, 1, 0));
        QCOMPARE(roomPlace(kFarEastRoomId), QVector3D(6, 0, 0));
        QCOMPARE(roomPlace(kUpperRoomId), QVector3D(2, 2, 1));
        QVERIFY2(map()->isUnsaved(), "moving rooms did not mark the map as changed");
        // Moving the player's room does not drag the view after it
        QVERIFY(!pModern->followingPlayer());
        QCOMPARE(pModern->viewCenter(), QVector3D(0, 0, 0));
        QCOMPARE(selection(), (QSet<int>{kPlayerRoomId, kEastRoomId}));
        // The rooms' area finds them at their new places, as after a move on the 2D map
        QVERIFY(map()->mpRoomDB->getArea(mPlayerAreaId)->getGridIndex().roomsAt(0, 5, 1).contains(kEastRoomId));

        // Dragging an unselected room moves just that one
        const QPoint farEast = pixelOfRoom(pModern, kFarEastRoomId);
        drag(pModern, farEast, farEast + QPoint(0, spacing), Qt::NoModifier);
        QCOMPARE(roomPlace(kFarEastRoomId), QVector3D(6, -2, 0));
        QCOMPARE(roomPlace(kEastRoomId), QVector3D(5, 1, 0));
        QCOMPARE(selection(), (QSet<int>{kFarEastRoomId}));
    }

    void test_draggingARoomInViewingModeLeavesItInPlace()
    {
        ModernGLWidget* pModern = viewAtThePlayer(false);
        QVERIFY(pModern);
        const QPoint east = pixelOfRoom(pModern, kEastRoomId);
        drag(pModern, east, east + QPoint(100, 0), Qt::NoModifier);
        QCOMPARE(roomPlace(kEastRoomId), QVector3D(2, 0, 0));
    }

    // A new map window offers the 3D view too, which shows that window's area and shares its selection
    void test_newMapWindowShowsItsOwnAreaIn3D()
    {
        ModernGLWidget* pMain = viewAtThePlayer(false);
        QVERIFY(pMain);
        TMapViewManager* pManager = map()->getViewManager();
        QVERIFY(pManager);
        const auto [viewId, error] = pManager->createView(mOtherAreaId);
        QVERIFY2(viewId, qPrintable(error));
        TMapView* pView = pManager->getView(viewId);
        QVERIFY(pView);
        auto* pDock = qobject_cast<QDockWidget*>(pView->parentWidget());
        QVERIFY(pDock);
        pDock->resize(600, 600);

        QToolButton* p3DButton = buttonNamed(pView, qsl("3D"));
        QVERIFY2(p3DButton, "the map window has no 3D button");
        QVERIFY(p3DButton->isEnabled());
        p3DButton->click();
        ModernGLWidget* pWindow3D = pView->get3DView();
        QVERIFY(pWindow3D);
        QTRY_VERIFY(pWindow3D->isVisible() && pWindow3D->height() > 200);
        QVERIFY(!pView->get2DMap()->isVisible());
        QVERIFY(pWindow3D != pMain);
        QCOMPARE(pWindow3D->shownAreaId(), mOtherAreaId);
        QCOMPARE(pView->getCurrentAreaId(), mOtherAreaId);
        QVERIFY2(!pWindow3D->followingPlayer(), "the map window's 3D view follows the player");
        // The mapper's own 3D view stays with the player
        QCOMPARE(pMain->shownAreaId(), mPlayerAreaId);

        pWindow3D->setCameraPosition(1.0f, CameraController::scmMinTilt, CameraController::scmNorthUpAzimuth);
        QTest::mouseClick(pWindow3D, Qt::LeftButton, Qt::NoModifier, pixelOfRoom(pWindow3D, kOtherAreaRoomId));
        QCOMPARE(pView->get2DMap()->mMultiSelectionSet, QSet<int>{kOtherAreaRoomId});
        QCOMPARE(selection(), QSet<int>());

        // Moving the window's view to another area leaves the mapper's alone
        pView->setArea(mPlayerAreaId);
        QCOMPARE(pWindow3D->shownAreaId(), mPlayerAreaId);
        pView->setArea(mOtherAreaId);
        QCOMPARE(pWindow3D->shownAreaId(), mOtherAreaId);
        QCOMPARE(pMain->shownAreaId(), mPlayerAreaId);

        // Edit in 2D works there as in the mapper
        QTest::mouseClick(pWindow3D, Qt::RightButton, Qt::NoModifier, pixelOfRoom(pWindow3D, kOtherAreaRoomId));
        QMenu* menu = openMenu(pWindow3D);
        QVERIFY2(menu, "right-clicking a room in the map window's 3D view opened no menu");
        QAction* editIn2D = actionNamed(menu, qsl("Edit in 2D"));
        QVERIFY(editIn2D);
        editIn2D->trigger();
        menu->close();
        T2DMap* pWindow2D = pView->get2DMap();
        QVERIFY(pWindow2D->isVisible() && !pWindow3D->isVisible());
        QVERIFY(!p3DButton->isChecked());
        QVERIFY(!pWindow2D->mMapViewOnly);
        QCOMPARE(pWindow2D->getAreaId(), mOtherAreaId);
        QCOMPARE(pWindow2D->mMapCenterX, 20.0);
        QCOMPARE(pWindow2D->mMapCenterY, -20.0);
        QCOMPARE(pWindow2D->mMultiSelectionSet, QSet<int>{kOtherAreaRoomId});
        pWindow2D->slot_toggleMapViewOnly();

        // Switching to the classic 3D renderer takes the window's 3D view away, and back brings the button back
        p3DButton->click();
        QVERIFY(pWindow3D->isVisible());
        QVERIFY(mpHost->getLuaInterpreter()->compileAndExecuteScript(qsl("setConfig('experiment.3dmap.modernmapper', false)")));
        QVERIFY(!pView->get3DView());
        QVERIFY(pView->get2DMap()->isVisible());
        QVERIFY(!p3DButton->isEnabled() && !p3DButton->isChecked());
        QVERIFY(mpHost->getLuaInterpreter()->compileAndExecuteScript(qsl("setConfig('experiment.3dmap.modernmapper', true)")));
        QVERIFY(p3DButton->isEnabled());

        // A script switching renderer from the view's own mouse event leaves it to finish handling that event
        p3DButton->click();
        QPointer<ModernGLWidget> pReplaced = pView->get3DView();
        QVERIFY(pReplaced && pReplaced->isVisible());
        QVERIFY(mpHost->getLuaInterpreter()->compileAndExecuteScript(
                qsl("mapEditingTestHandler = registerAnonymousEventHandler('sysMapWindowMousePressEvent', function() setConfig('experiment.3dmap.modernmapper', false) end)")));
        QTest::mouseClick(pReplaced, Qt::LeftButton, Qt::NoModifier, QPoint(5, 5));
        QVERIFY(mpHost->getLuaInterpreter()->compileAndExecuteScript(qsl("killAnonymousEventHandler(mapEditingTestHandler)")));
        QVERIFY(!pView->get3DView());
        QVERIFY(pView->get2DMap()->isVisible());
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        QVERIFY(!pReplaced);
        QVERIFY(mpHost->getLuaInterpreter()->compileAndExecuteScript(qsl("setConfig('experiment.3dmap.modernmapper', true)")));

        QVERIFY(pManager->closeView(viewId).first);
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        QVERIFY(qobject_cast<ModernGLWidget*>(map()->mapper()->glWidget));
    }

    // Selected rooms survive the handover, the 2D map is ready to edit at once, and the 3D view keeps its camera
    void test_editIn2DHandsOverAtOnceAndKeepsThe3DCamera()
    {
        ModernGLWidget* pModern = viewAtThePlayer(false);
        QVERIFY(pModern);
        dlgMapper* pMapper = map()->mapper();
        T2DMap* p2dMap = pMapper->mp2dMap;
        pModern->setCameraPosition(1.5f, 35.0f, 250.0f);
        const QList<int> camera{pMapper->slider_scale->value(), pMapper->slider_xRot->value(), pMapper->slider_yRot->value(), pMapper->slider_zRot->value()};

        QTest::mouseClick(pModern, Qt::LeftButton, Qt::NoModifier, pixelOfRoom(pModern, kPlayerRoomId));
        QTest::mouseClick(pModern, Qt::LeftButton, Qt::ControlModifier, pixelOfRoom(pModern, kEastRoomId));
        QCOMPARE(selection(), (QSet<int>{kPlayerRoomId, kEastRoomId}));

        QTest::mouseClick(pModern, Qt::RightButton, Qt::NoModifier, pixelOfRoom(pModern, kEastRoomId));
        QMenu* menu = openMenu(pModern);
        QVERIFY2(menu, "right-clicking a room in the 3D view opened no menu");
        QAction* editIn2D = actionNamed(menu, qsl("Edit in 2D"));
        QVERIFY(editIn2D);
        editIn2D->trigger();
        menu->close();

        // All in the one step: no 3D controls left over a moment later, and no switching mode by hand
        QVERIFY(p2dMap->isVisible() && !pModern->isVisible());
        QVERIFY2(pMapper->widget_3DControls->isHidden(), "the 3D controls stay up after switching to 2D");
        QVERIFY2(!p2dMap->mMapViewOnly, "the 2D map is not ready for editing");
        QCOMPARE(p2dMap->mAreaID, mPlayerAreaId);
        QCOMPARE(p2dMap->mMapCenterX, 2.0);
        QCOMPARE(p2dMap->mMapCenterY, 0.0);
        QCOMPARE(selection(), (QSet<int>{kPlayerRoomId, kEastRoomId}));

        // Back in 3D, the view is as it was
        pMapper->show3DView(true);
        QTRY_VERIFY(pModern->isVisible());
        const QList<int> cameraAfter{pMapper->slider_scale->value(), pMapper->slider_xRot->value(), pMapper->slider_yRot->value(), pMapper->slider_zRot->value()};
        QCOMPARE(cameraAfter, camera);
        QCoreApplication::processEvents();
        QTest::qWait(150);
        QVERIFY2(pMapper->widget_3DControls->isVisible(), "the 3D controls went away after switching back to 3D");
        QCOMPARE(selection(), (QSet<int>{kPlayerRoomId, kEastRoomId}));
        QCOMPARE(pModern->viewCenter(), QVector3D(2.0f, 0.0f, 0.0f));
    }
};

#include "Map3DViewEditingTest.moc"
MUDLET_GROUPED_TEST_MAIN(Map3DViewEditingTest)
