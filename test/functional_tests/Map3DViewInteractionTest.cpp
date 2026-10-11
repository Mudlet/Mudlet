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
 * The modern 3D view answers the mouse and keyboard as the 2D map does: it
 * selects, speedwalks, offers the room menu and raises the press event, and
 * each area keeps its own camera.
 *
 * Why not a spec: no Lua call can click, hover over or type into the map, and
 * the 3D view's center and camera are not reported to Lua either.
 *
 * Run with: ctest -R Map3DViewInteractionTest -V
 */

#include <limits>

#include <QAction>
#include <QDockWidget>
#include <QMenu>
#include <QScopeGuard>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QLineEdit>
#include <QToolTip>
#include <QtTest/QtTest>

#include "Host.h"
#include "MudletApp.h"
#include "MudletInstanceCoordinator.h"
#include "PortableModeTestHelper.h"
#include "ProfileTestHelper.h"
#include "T2DMap.h"
#include "TCommandLine.h"
#include "TLuaInterpreter.h"
#include "TMainConsole.h"
#include "TMap.h"
#include "TRoomDB.h"
#include "TUiTour.h"
#include "TelnetServerStub.h"
#include "ctelnet.h"
#include "dlgMapper.h"
#include "modern_glwidget.h"
#include "mudlet.h"

extern "C" {
#if defined(INCLUDE_VERSIONED_LUA_HEADERS)
#include <lua5.1/lauxlib.h>
#include <lua5.1/lua.h>
#else
#include <lauxlib.h>
#include <lua.h>
#endif
}

#include "GroupedTest.h"

using namespace std::chrono_literals;

class Map3DViewInteractionTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;
    TelnetServerStub* mpServer = nullptr;
    Host* mpHost = nullptr;
    const QString mProfileName = qsl("Map3DViewInteraction-Test");
    const QString mLocalhost = qsl("localhost");
    QString mPort;
    int mPlayerAreaId = 0;
    int mOtherAreaId = 0;
    static constexpr int kPlayerRoomId = 1;
    static constexpr int kEastRoomId = 2;
    static constexpr int kOtherAreaRoomId = 10;

    TMap* map() const { return mpHost->mpMap.data(); }

    void deleteProfileDirectory() const
    {
        QDir dir(MudletApp::getMudletPath(enums::profileHomePath, mProfileName));
        if (dir.exists()) {
            dir.removeRecursively();
        }
    }

    bool addRoomAt(const int id, const int areaId, const int x, const int y) const { return map()->addRoom(id) && map()->setRoomArea(id, areaId) && map()->setRoomCoordinates(id, x, y, 0); }

    QString runLua(const QString& code) const
    {
        lua_State* L = mpHost->getLuaInterpreter()->getLuaGlobalState();
        if (luaL_dostring(L, code.toUtf8().constData()) == 0) {
            return QString();
        }
        const QString error = QString::fromUtf8(lua_tostring(L, -1));
        lua_pop(L, 1);
        return error;
    }

    int luaGlobalNumber(const char* name) const
    {
        lua_State* L = mpHost->getLuaInterpreter()->getLuaGlobalState();
        lua_getglobal(L, name);
        const int value = static_cast<int>(lua_tonumber(L, -1));
        lua_pop(L, 1);
        return value;
    }

    QString luaGlobalString(const char* name) const
    {
        lua_State* L = mpHost->getLuaInterpreter()->getLuaGlobalState();
        lua_getglobal(L, name);
        const QString value = QString::fromUtf8(lua_tostring(L, -1));
        lua_pop(L, 1);
        return value;
    }

    // Focus as Tab gives it, with which the view pans and zooms with keys
    // qWaitForWindowActive() can return before QApplication moves focus into the window, and focus set before then
    // arrives later with ActiveWindowFocusReason
    static bool activate(QWidget* pWindow)
    {
        pWindow->activateWindow();
        return QTest::qWaitFor([pWindow]() {
            return QApplication::activeWindow() == pWindow;
        });
    }

    static bool focusFromTheKeyboard(ModernGLWidget* pModern)
    {
        if (!activate(pModern->window())) {
            return false;
        }
        // setFocus() does nothing to the widget that already has focus, whatever the reason it got it for
        pModern->clearFocus();
        pModern->setFocus(Qt::TabFocusReason);
        return QTest::qWaitFor([pModern]() {
            return pModern->hasFocus();
        });
    }

    static QDockWidget* dockOf(QWidget* pWidget)
    {
        for (QWidget* pParent = pWidget->parentWidget(); pParent; pParent = pParent->parentWidget()) {
            if (auto* pDock = qobject_cast<QDockWidget*>(pParent)) {
                return pDock;
            }
        }
        return nullptr;
    }

    // QDockWidget::setFloating() applies the state again even when it is unchanged, which loses the window's activation
    static void setDockFloating(QDockWidget* pDock, const bool floating)
    {
        if (pDock->isFloating() != floating) {
            pDock->setFloating(floating);
        }
    }

    int pressesSeenByScript() const { return runLua(qsl("pressesSeen = #selectedOnPresses")).isEmpty() ? luaGlobalNumber("pressesSeen") : -1; }

    // Each row of pixels in the middle column of the view at which roomId is picked
    static QList<int> rowsPickingInTheMiddleColumn(ModernGLWidget* pModern, const int roomId)
    {
        QList<int> rows;
        for (int y = 0; y < pModern->height(); ++y) {
            if (pModern->roomAt(QPoint(pModern->width() / 2, y)) == roomId) {
                rows.append(y);
            }
        }
        return rows;
    }

    // The 3D view on the player's area, looking straight down with north up, nothing selected
    ModernGLWidget* viewAtThePlayer() const
    {
        dlgMapper* pMapper = map()->mapper();
        if (!pMapper || !pMapper->mp2dMap) {
            return nullptr;
        }
        pMapper->show3DView(false);
        map()->mRoomIdHash[map()->mProfileName] = kPlayerRoomId;
        map()->mNewMove = false;
        T2DMap* p2dMap = pMapper->mp2dMap;
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

    static QMenu* openMenu(ModernGLWidget* pModern)
    {
        for (QMenu* menu : pModern->findChildren<QMenu*>()) {
            if (menu->isVisible()) {
                return menu;
            }
        }
        return nullptr;
    }

    static QStringList actionTexts(const QMenu* menu)
    {
        QStringList texts;
        for (const QAction* action : menu->actions()) {
            if (!action->isSeparator()) {
                texts << action->text();
            }
        }
        return texts;
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
        // The first-run tour opens a second after the profile loads and takes the main window's keys
        TUiTour::rememberShown();
        mudlet::self()->takeOwnershipOfInstanceCoordinator(std::make_unique<MudletInstanceCoordinator>("MudletInstanceCoordinator"));
        mudlet::self()->init();
        mudlet::self()->setStorePasswordsSecurely(false);
        QVERIFY2(mudlet::self()->experiencedMudletPlayer(), "the first-run UI would open over these tests and eat their key presses");
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
        QVERIFY(addRoomAt(kOtherAreaRoomId, mOtherAreaId, 20, 20));
        map()->mpRoomDB->getRoom(kPlayerRoomId)->name = qsl("Market Square");
        map()->mRoomIdHash[map()->mProfileName] = kPlayerRoomId;

        mpHost->showHideOrCreateMapper(false);
        QVERIFY2(map()->mapper(), "the profile's mapper could not be created");
        auto* pDock = qobject_cast<QDockWidget*>(map()->mapper()->parentWidget());
        QVERIFY(pDock);
        pDock->setFloating(true);
        pDock->resize(800, 600);
        ModernGLWidget* pModern = viewAtThePlayer();
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

    void test_aClickSelectsTheRoomAsOnThe2DMap()
    {
        ModernGLWidget* pModern = viewAtThePlayer();
        QVERIFY(pModern);
        const QPoint playerRoom = pixelOfRoom(pModern, kPlayerRoomId);
        const QPoint eastRoom = pixelOfRoom(pModern, kEastRoomId);
        QVERIFY2(playerRoom.x() >= 0 && eastRoom.x() > playerRoom.x(), "the view does not show the east room to the right of the player's");
        QCOMPARE(pModern->roomAt(QPoint(2, 2)), 0);

        QTest::mouseClick(pModern, Qt::LeftButton, Qt::NoModifier, playerRoom);
        QCOMPARE(selection(), QSet<int>{kPlayerRoomId});
        QTest::mouseClick(pModern, Qt::LeftButton, Qt::ControlModifier, eastRoom);
        QCOMPARE(selection(), (QSet<int>{kPlayerRoomId, kEastRoomId}));
        QTest::mouseClick(pModern, Qt::LeftButton, Qt::ControlModifier, eastRoom);
        QCOMPARE(selection(), QSet<int>{kPlayerRoomId});
        QTest::mouseClick(pModern, Qt::LeftButton, Qt::ShiftModifier, eastRoom);
        QCOMPARE(selection(), (QSet<int>{kPlayerRoomId, kEastRoomId}));
        QTest::mouseClick(pModern, Qt::LeftButton, Qt::NoModifier, eastRoom);
        QCOMPARE(selection(), QSet<int>{kEastRoomId});

        // Dragging from empty space pans, which must not drop the selection
        QTest::mousePress(pModern, Qt::LeftButton, Qt::NoModifier, QPoint(5, 5));
        QMouseEvent drag(QEvent::MouseMove, QPointF(60, 60), pModern->mapToGlobal(QPointF(60, 60)), Qt::NoButton, Qt::LeftButton, Qt::NoModifier);
        QCoreApplication::sendEvent(pModern, &drag);
        QTest::mouseRelease(pModern, Qt::LeftButton, Qt::NoModifier, QPoint(60, 60));
        QCOMPARE(selection(), QSet<int>{kEastRoomId});

        QTest::mouseClick(pModern, Qt::LeftButton, Qt::NoModifier, QPoint(2, 2));
        QVERIFY(selection().isEmpty());
    }

    void test_aPressRaisesTheMapWindowMousePressEvent()
    {
        ModernGLWidget* pModern = viewAtThePlayer();
        QVERIFY(pModern);
        QCOMPARE(runLua(qsl("mapPresses = 0; selectedOnPress = 0\n"
                            "pressHandler = registerAnonymousEventHandler('sysMapWindowMousePressEvent', function()\n"
                            "  mapPresses = mapPresses + 1\n"
                            "  selectedOnPress = #getMapSelection().rooms\n"
                            "end)")),
                 QString());

        QTest::mouseClick(pModern, Qt::LeftButton, Qt::NoModifier, pixelOfRoom(pModern, kPlayerRoomId));
        QTest::mouseClick(pModern, Qt::RightButton, Qt::NoModifier, QPoint(2, 2));
        if (QMenu* menu = openMenu(pModern)) {
            menu->close();
        }
        QCOMPARE(runLua(qsl("killAnonymousEventHandler(pressHandler)")), QString());
        QCOMPARE(luaGlobalNumber("mapPresses"), 2);
        // As on the 2D map, a script handling the press already sees the room clicked on selected
        QCOMPARE(luaGlobalNumber("selectedOnPress"), 1);
    }

    void test_aDoubleClickSpeedwalksToTheRoom()
    {
        ModernGLWidget* pModern = viewAtThePlayer();
        QVERIFY(pModern);
        map()->mTargetID = 0;
        const QPoint eastRoom = pixelOfRoom(pModern, kEastRoomId);
        QMouseEvent doubleClick(QEvent::MouseButtonDblClick, eastRoom, pModern->mapToGlobal(eastRoom), Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
        QCoreApplication::sendEvent(pModern, &doubleClick);
        QCOMPARE(map()->mTargetID, kEastRoomId);
    }

    void test_hoveringOverARoomNamesIt()
    {
        ModernGLWidget* pModern = viewAtThePlayer();
        QVERIFY(pModern);
        const QPoint playerRoom = pixelOfRoom(pModern, kPlayerRoomId);
        QHelpEvent overRoom(QEvent::ToolTip, playerRoom, pModern->mapToGlobal(playerRoom));
        QCoreApplication::sendEvent(pModern, &overRoom);
        QTRY_VERIFY(QToolTip::isVisible());
        QVERIFY2(QToolTip::text().contains(qsl("Market Square")), qPrintable(QToolTip::text()));
        QVERIFY2(QToolTip::text().contains(qsl("Room ID: %1").arg(kPlayerRoomId)), qPrintable(QToolTip::text()));
        QVERIFY2(QToolTip::text().contains(qsl("Player Area")), qPrintable(QToolTip::text()));

        QHelpEvent overNothing(QEvent::ToolTip, QPoint(2, 2), pModern->mapToGlobal(QPoint(2, 2)));
        QCoreApplication::sendEvent(pModern, &overNothing);
        QTRY_VERIFY(!QToolTip::isVisible());
    }

    void test_aRightClickOffersThe2DRoomMenuAndEditIn2D()
    {
        ModernGLWidget* pModern = viewAtThePlayer();
        QVERIFY(pModern);
        dlgMapper* pMapper = map()->mapper();
        T2DMap* p2dMap = pMapper->mp2dMap;

        p2dMap->mMapViewOnly = false;
        QTest::mouseClick(pModern, Qt::RightButton, Qt::NoModifier, pixelOfRoom(pModern, kEastRoomId));
        QMenu* menu = openMenu(pModern);
        QVERIFY2(menu, "right-clicking a room in the 3D view opened no menu");
        QStringList texts = actionTexts(menu);
        QVERIFY2(texts.value(0) == qsl("Edit in 2D"), qPrintable(texts.join(qsl(", "))));
        QVERIFY2(texts.contains(qsl("Configure room...")), qPrintable(texts.join(qsl(", "))));
        QVERIFY2(texts.contains(qsl("Set player location")), qPrintable(texts.join(qsl(", "))));
        // These need a pointer on the 2D map itself
        QVERIFY2(!texts.contains(qsl("Move")) && !texts.contains(qsl("Create label...")), qPrintable(texts.join(qsl(", "))));
        QCOMPARE(selection(), QSet<int>{kEastRoomId});
        menu->close();
        p2dMap->mMapViewOnly = true;

        pModern->setViewCenter(mOtherAreaId, 20, 20, 0);
        QTest::mouseClick(pModern, Qt::RightButton, Qt::NoModifier, pixelOfRoom(pModern, kOtherAreaRoomId));
        menu = openMenu(pModern);
        QVERIFY(menu);
        texts = actionTexts(menu);
        QVERIFY2(!texts.contains(qsl("Configure room...")), "the menu offered editing in viewing mode");
        menu->actions().constFirst()->trigger();
        menu->close();

        QVERIFY2(!pModern->isVisible() && p2dMap->isVisible(), "Edit in 2D did not switch to the 2D map");
        QCOMPARE(p2dMap->mAreaID, mOtherAreaId);
        QCOMPARE(p2dMap->mMapCenterX, 20.0);
        // The 2D map's y-axis points down the screen
        QCOMPARE(p2dMap->mMapCenterY, -20.0);
        QCOMPARE(selection(), QSet<int>{kOtherAreaRoomId});
    }

    // A click away from the 3D view's room menu closes it and goes on to the 3D view, as one away from the 2D
    // map's menu does there; the hidden 2D map must not take it at its own coordinates
    void test_aClickAwayFromTheRoomMenuGoesOnToThe3DView()
    {
        ModernGLWidget* pModern = viewAtThePlayer();
        QVERIFY(pModern);
        QTest::mouseClick(pModern, Qt::RightButton, Qt::NoModifier, pixelOfRoom(pModern, kPlayerRoomId));
        QMenu* menu = openMenu(pModern);
        QVERIFY(menu);
        QCOMPARE(selection(), QSet<int>{kPlayerRoomId});
        QPointer<QMenu> menuGuard(menu);

        const QPoint eastRoom = pixelOfRoom(pModern, kEastRoomId);
        // Near the screen's right edge the menu shifts left, over the east room
        const QPoint eastRoomOnScreen = pModern->mapToGlobal(eastRoom);
        if (menu->geometry().contains(eastRoomOnScreen)) {
            menu->move(eastRoomOnScreen.x() - menu->width() - 10, menu->y());
        }
        QVERIFY(!menu->geometry().contains(eastRoomOnScreen));
        // To the view, not to the menu's window, whose handling of a press outside it varies by platform
        QTest::mouseClick(pModern, Qt::LeftButton, Qt::NoModifier, eastRoom);
        QTRY_VERIFY(menuGuard.isNull() || !menuGuard->isVisible());
        QTRY_COMPARE(selection(), QSet<int>{kEastRoomId});
    }

    // Checks picking against where the camera is known to look, not against roomAt() itself as pixelOfRoom() does
    void test_pickingFindsTheRoomTheCameraLooksAt()
    {
        ModernGLWidget* pModern = viewAtThePlayer();
        QVERIFY(pModern);
        const QPoint middle(pModern->width() / 2, pModern->height() / 2);
        QCOMPARE(pModern->roomAt(middle), kPlayerRoomId);
        // Looking straight down with north up, east is to the right on the same row
        bool eastFound = false;
        for (int x = 0; x < pModern->width(); ++x) {
            if (pModern->roomAt(QPoint(x, middle.y())) == kEastRoomId) {
                eastFound = true;
                QVERIFY2(x > middle.x(), qPrintable(qsl("the east room was picked at x %1, left of the middle %2").arg(x).arg(middle.x())));
            }
        }
        QVERIFY2(eastFound, "the east room was not picked anywhere on the middle row");

        // North above the middle and south below it, looking straight down and tilted, so that a ray turned
        // upside down picks the room on the other side
        constexpr int northRoomId = 22;
        constexpr int southRoomId = 23;
        QVERIFY(addRoomAt(northRoomId, mPlayerAreaId, 0, 2) && addRoomAt(southRoomId, mPlayerAreaId, 0, -2));
        for (const float tilt : {CameraController::scmMinTilt, (CameraController::scmMinTilt + CameraController::scmMaxTilt) / 2.0f}) {
            pModern->setCameraPosition(1.0f, tilt, CameraController::scmNorthUpAzimuth);
            const QList<int> northRows = rowsPickingInTheMiddleColumn(pModern, northRoomId);
            const QList<int> southRows = rowsPickingInTheMiddleColumn(pModern, southRoomId);
            const QString pose = qsl("tilt %1: north at rows %2, south at rows %3, middle %4")
                                         .arg(tilt)
                                         .arg(northRows.isEmpty() ? -1 : northRows.constFirst())
                                         .arg(southRows.isEmpty() ? -1 : southRows.constFirst())
                                         .arg(middle.y());
            QVERIFY2(!northRows.isEmpty() && !southRows.isEmpty(), qPrintable(pose));
            QVERIFY2(northRows.constLast() < middle.y() && southRows.constFirst() > middle.y(), qPrintable(pose));
        }
        map()->mpRoomDB->removeRoom(northRoomId);
        map()->mpRoomDB->removeRoom(southRoomId);

        pModern->setCameraPosition(1.0f, (CameraController::scmMinTilt + CameraController::scmMaxTilt) / 2.0f, CameraController::scmNorthUpAzimuth + 30.0f);
        QCOMPARE(pModern->roomAt(middle), kPlayerRoomId);
    }

    // Rooms the view does not draw, on levels it hides or hidden themselves, must not be picked either
    void test_pickingSkipsRoomsTheViewDoesNotShow()
    {
        ModernGLWidget* pModern = viewAtThePlayer();
        QVERIFY(pModern);
        const QPoint middle(pModern->width() / 2, pModern->height() / 2);
        constexpr int upperRoomId = 20;
        constexpr int hiddenRoomId = 21;
        QVERIFY(addRoomAt(upperRoomId, mPlayerAreaId, 0, 0) && map()->setRoomCoordinates(upperRoomId, 0, 0, 1));
        QVERIFY(addRoomAt(hiddenRoomId, mPlayerAreaId, 0, 2));
        const QList<int> hiddenRoomRows = rowsPickingInTheMiddleColumn(pModern, hiddenRoomId);

        pModern->slot_singleLevelView();
        const int pickedOnOneLevel = pModern->roomAt(middle);
        pModern->slot_showAllLevels();
        // Looking straight down, the room above is the nearer
        const int pickedOnAllLevels = pModern->roomAt(middle);
        map()->mpRoomDB->getRoom(hiddenRoomId)->setHidden(true);
        const int pickedWhereTheHiddenRoomIs = hiddenRoomRows.isEmpty() ? -1 : pModern->roomAt(QPoint(middle.x(), hiddenRoomRows.at(hiddenRoomRows.size() / 2)));
        map()->mpRoomDB->removeRoom(upperRoomId);
        map()->mpRoomDB->removeRoom(hiddenRoomId);

        QCOMPARE(pickedOnOneLevel, kPlayerRoomId);
        QCOMPARE(pickedOnAllLevels, upperRoomId);
        QVERIFY2(!hiddenRoomRows.isEmpty(), "the room to hide was not picked anywhere while shown");
        QVERIFY(pickedWhereTheHiddenRoomIs != hiddenRoomId);
    }

    // Room actions from the 3D view's menu must never reach rooms of an area it no longer shows
    void test_changingAreaIn3DDropsTheOldAreasRoomsFromTheSelection()
    {
        ModernGLWidget* pModern = viewAtThePlayer();
        QVERIFY(pModern);
        QTest::mouseClick(pModern, Qt::LeftButton, Qt::NoModifier, pixelOfRoom(pModern, kPlayerRoomId));
        QCOMPARE(selection(), QSet<int>{kPlayerRoomId});
        pModern->setViewCenter(mOtherAreaId, 20, 20, 0);
        // The hidden 2D map stays where it was, so only the 3D view can have dropped them
        QCOMPARE(map()->mapper()->mp2dMap->mAreaID, mPlayerAreaId);
        QVERIFY(selection().isEmpty());
    }

    // As on the 2D map, which changes the selection on the press, a script handling the press reads the selection
    // the click leaves, also for the clicks that the 3D view settles only on the release
    void test_aScriptHandlingThePressSeesTheSelectionTheClickLeaves()
    {
        ModernGLWidget* pModern = viewAtThePlayer();
        QVERIFY(pModern);
        QCOMPARE(runLua(qsl("selectedOnPresses = {}\n"
                            "pressHandler = registerAnonymousEventHandler('sysMapWindowMousePressEvent', function()\n"
                            "  selectedOnPresses[#selectedOnPresses + 1] = #(getMapSelection().rooms or {})\n"
                            "end)")),
                 QString());
        const QPoint playerRoom = pixelOfRoom(pModern, kPlayerRoomId);
        QTest::mouseClick(pModern, Qt::LeftButton, Qt::NoModifier, playerRoom);
        QTest::mouseClick(pModern, Qt::LeftButton, Qt::ControlModifier, pixelOfRoom(pModern, kEastRoomId));
        QTest::mouseClick(pModern, Qt::LeftButton, Qt::NoModifier, QPoint(2, 2));
        QTest::mouseClick(pModern, Qt::LeftButton, Qt::NoModifier, playerRoom);
        // A drag from empty space pans and leaves the selection, and is announced once, as soon as it is a drag
        QTest::mousePress(pModern, Qt::LeftButton, Qt::NoModifier, QPoint(5, 5));
        QMouseEvent drag(QEvent::MouseMove, QPointF(60, 60), pModern->mapToGlobal(QPointF(60, 60)), Qt::NoButton, Qt::LeftButton, Qt::NoModifier);
        QCoreApplication::sendEvent(pModern, &drag);
        const int announcedBeforeTheRelease = pressesSeenByScript();
        QTest::mouseRelease(pModern, Qt::LeftButton, Qt::NoModifier, QPoint(60, 60));
        // The 2D map selects the room right-clicked on before it announces the press
        QTest::mouseClick(pModern, Qt::LeftButton, Qt::NoModifier, QPoint(2, 2));
        QTest::mouseClick(pModern, Qt::RightButton, Qt::NoModifier, playerRoom);
        if (QMenu* menu = openMenu(pModern)) {
            menu->close();
        }
        QCOMPARE(runLua(qsl("killAnonymousEventHandler(pressHandler)\n"
                            "selectedOnPressesText = table.concat(selectedOnPresses, ',')")),
                 QString());
        QCOMPARE(announcedBeforeTheRelease, 5);
        QCOMPARE(luaGlobalString("selectedOnPressesText"), qsl("1,2,0,1,1,0,1"));
    }

    // Each press is announced, also one made while another button is still down
    void test_aPressWhileAnotherButtonIsDownIsAnnouncedToo()
    {
        ModernGLWidget* pModern = viewAtThePlayer();
        QVERIFY(pModern);
        QCOMPARE(runLua(qsl("selectedOnPresses = {}\n"
                            "pressHandler = registerAnonymousEventHandler('sysMapWindowMousePressEvent', function()\n"
                            "  selectedOnPresses[#selectedOnPresses + 1] = 0\n"
                            "end)")),
                 QString());
        const QPointF corner(5, 5);
        const QPointF globalCorner = pModern->mapToGlobal(corner);
        QMouseEvent leftPress(QEvent::MouseButtonPress, corner, globalCorner, Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
        QCoreApplication::sendEvent(pModern, &leftPress);
        QMouseEvent rightPress(QEvent::MouseButtonPress, corner, globalCorner, Qt::RightButton, Qt::LeftButton | Qt::RightButton, Qt::NoModifier);
        QCoreApplication::sendEvent(pModern, &rightPress);
        QMouseEvent rightRelease(QEvent::MouseButtonRelease, corner, globalCorner, Qt::RightButton, Qt::LeftButton, Qt::NoModifier);
        QCoreApplication::sendEvent(pModern, &rightRelease);
        if (QMenu* menu = openMenu(pModern)) {
            menu->close();
        }
        QMouseEvent leftRelease(QEvent::MouseButtonRelease, corner, globalCorner, Qt::LeftButton, Qt::NoButton, Qt::NoModifier);
        QCoreApplication::sendEvent(pModern, &leftRelease);
        const int presses = pressesSeenByScript();
        QCOMPARE(runLua(qsl("killAnonymousEventHandler(pressHandler)")), QString());
        QCOMPARE(presses, 2);
    }

    // The release of a press can go elsewhere once the view hides, as with "Edit in 2D"; the press is still
    // announced, and a drag begun then does not pan the view when it is back
    void test_aPressIsAnnouncedWhenTheViewHidesBeforeTheRelease()
    {
        ModernGLWidget* pModern = viewAtThePlayer();
        QVERIFY(pModern);
        QCOMPARE(runLua(qsl("selectedOnPresses = {}\n"
                            "pressHandler = registerAnonymousEventHandler('sysMapWindowMousePressEvent', function()\n"
                            "  selectedOnPresses[#selectedOnPresses + 1] = 0\n"
                            "end)")),
                 QString());
        const QPointF corner(5, 5);
        QMouseEvent leftPress(QEvent::MouseButtonPress, corner, pModern->mapToGlobal(corner), Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
        QCoreApplication::sendEvent(pModern, &leftPress);
        map()->mapper()->show3DView(false);
        QTRY_COMPARE(pressesSeenByScript(), 1);

        pModern = viewAtThePlayer();
        QVERIFY(pModern);
        const QVector3D start = pModern->viewCenter();
        QMouseEvent rightPress(QEvent::MouseButtonPress, corner, pModern->mapToGlobal(corner), Qt::RightButton, Qt::RightButton, Qt::NoModifier);
        QCoreApplication::sendEvent(pModern, &rightPress);
        QMouseEvent rightDrag(QEvent::MouseMove, QPointF(60, 60), pModern->mapToGlobal(QPointF(60, 60)), Qt::NoButton, Qt::RightButton, Qt::NoModifier);
        QCoreApplication::sendEvent(pModern, &rightDrag);
        QMouseEvent rightRelease(QEvent::MouseButtonRelease, QPointF(60, 60), pModern->mapToGlobal(QPointF(60, 60)), Qt::RightButton, Qt::NoButton, Qt::NoModifier);
        QCoreApplication::sendEvent(pModern, &rightRelease);
        if (QMenu* menu = openMenu(pModern)) {
            menu->close();
        }
        QCOMPARE(runLua(qsl("killAnonymousEventHandler(pressHandler)")), QString());
        QCOMPARE(pModern->viewCenter(), start);
    }

    // Typing goes to the command line after a click on the view, as it does after one on the 2D map, so that "s",
    // "w", the arrows and keypad key bindings are not taken for panning and zooming
    void test_keysTypedAfterAClickOnTheViewReachTheCommandLine()
    {
        ModernGLWidget* pModern = viewAtThePlayer();
        QVERIFY(pModern);
        QDockWidget* pDock = dockOf(pModern);
        QVERIFY2(pDock, "the map is not in a dock");
        TCommandLine* commandLine = mpHost->mainConsoleView()->mpCommandLine;
        const auto restore = qScopeGuard([pDock, floating = pDock->isFloating(), commandLine]() {
            setDockFloating(pDock, floating);
            commandLine->clear();
        });
        setDockFloating(pDock, true);
        QVERIFY(activate(pDock));
        QSignalSpy cameraControls(pModern, &ModernGLWidget::cameraControlsChanged);
        const QVector3D start = pModern->viewCenter();
        commandLine->clear();

        QTest::mouseClick(pModern, Qt::LeftButton, Qt::NoModifier, pixelOfRoom(pModern, kPlayerRoomId));
        QTRY_VERIFY(pModern->hasFocus());
        for (const Qt::Key key : {Qt::Key_S, Qt::Key_W}) {
            QTest::keyClick(static_cast<QWidget*>(nullptr), key);
            QCoreApplication::processEvents();
        }
        QCOMPARE(commandLine->toPlainText(), qsl("sw"));
        QCOMPARE(pModern->viewCenter(), start);
        commandLine->clear();

        // Even with focus from the keyboard, with which W does pan
        QVERIFY(focusFromTheKeyboard(pModern));
        QTest::keyClick(pModern, Qt::Key_W);
        QVERIFY2(pModern->viewCenter().y() > start.y(), "W did not pan with focus from the keyboard");
        cameraControls.clear();
        QTest::keyClick(pModern, Qt::Key_Plus, Qt::KeypadModifier);
        QCoreApplication::processEvents();
        QCOMPARE(commandLine->toPlainText(), qsl("+"));
        QVERIFY2(cameraControls.isEmpty(), "keypad + zoomed the view instead of reaching the command line");
    }

    // Qt sets KeypadModifier on every arrow key on macOS, where the arrows must still pan; elsewhere it marks the
    // keypad's own arrows, which are left to key bindings
    void test_arrowsWithTheKeypadModifierPanOnlyOnMacOS()
    {
        ModernGLWidget* pModern = viewAtThePlayer();
        QVERIFY(pModern);
        QVERIFY(focusFromTheKeyboard(pModern));
        const QVector3D before = pModern->viewCenter();
        QTest::keyClick(pModern, Qt::Key_Right);
        QVERIFY2(pModern->viewCenter().x() > before.x(), "the right arrow did not pan east without KeypadModifier");
        const QVector3D start = pModern->viewCenter();
        QTest::keyClick(pModern, Qt::Key_Right, Qt::KeypadModifier);
#if defined(Q_OS_MACOS)
        QVERIFY2(pModern->viewCenter().x() > start.x(), "the right arrow did not pan east");
#else
        QCOMPARE(pModern->viewCenter(), start);
#endif
    }

    // The keys pan and zoom only with focus from the keyboard, so the view must be in its window's Tab order
    void test_tabReachesTheViewWhoseKeysThenPan()
    {
        ModernGLWidget* pModern = viewAtThePlayer();
        QVERIFY(pModern);
        QDockWidget* pDock = dockOf(pModern);
        QVERIFY2(pDock, "the map is not in a dock");
        const auto restore = qScopeGuard([pDock, floating = pDock->isFloating()]() {
            setDockFloating(pDock, floating);
        });
        setDockFloating(pDock, true);
        QVERIFY(activate(pDock));
        pModern->clearFocus();
        for (int presses = 0; presses < 50 && !pModern->hasFocus(); ++presses) {
            QTest::keyClick(static_cast<QWidget*>(nullptr), Qt::Key_Tab);
        }
        QVERIFY2(pModern->hasFocus(), "Tab never reached the view");
        const QVector3D start = pModern->viewCenter();
        QTest::keyClick(static_cast<QWidget*>(nullptr), Qt::Key_W);
        QVERIFY2(pModern->viewCenter().y() > start.y(), "W did not pan after Tab reached the view");
    }

    // A click takes the keys back for the command line also when the view already had focus from the keyboard
    void test_aClickAfterTabFocusSendsTheKeysToTheCommandLine()
    {
        ModernGLWidget* pModern = viewAtThePlayer();
        QVERIFY(pModern);
        TCommandLine* commandLine = mpHost->mainConsoleView()->mpCommandLine;
        const auto restore = qScopeGuard([commandLine]() {
            commandLine->clear();
        });
        QVERIFY(focusFromTheKeyboard(pModern));
        const QVector3D beforeTheClick = pModern->viewCenter();
        QTest::keyClick(pModern, Qt::Key_W);
        QVERIFY2(pModern->viewCenter().y() > beforeTheClick.y(), "W did not pan with focus from the keyboard");
        commandLine->clear();
        QTest::mouseClick(pModern, Qt::LeftButton, Qt::NoModifier, pixelOfRoom(pModern, kPlayerRoomId));
        QVERIFY(pModern->hasFocus());
        const QVector3D start = pModern->viewCenter();
        QTest::keyClick(static_cast<QWidget*>(nullptr), Qt::Key_S);
        QCoreApplication::processEvents();
        QCOMPARE(commandLine->toPlainText(), qsl("s"));
        QCOMPARE(pModern->viewCenter(), start);
    }

    // Switching to another application and back hands focus back with ActiveWindowFocusReason, which must leave the
    // keys panning as they did
    void test_keysStillPanAfterSwitchingWindowsAndBack()
    {
        ModernGLWidget* pModern = viewAtThePlayer();
        QVERIFY(pModern);
        QDockWidget* pDock = dockOf(pModern);
        QVERIFY2(pDock, "the map is not in a dock");
        TCommandLine* commandLine = mpHost->mainConsoleView()->mpCommandLine;
        const auto restore = qScopeGuard([pDock, floating = pDock->isFloating(), commandLine]() {
            setDockFloating(pDock, floating);
            commandLine->clear();
        });
        setDockFloating(pDock, true);
        QVERIFY(focusFromTheKeyboard(pModern));
        // Another application's window, as activating Mudlet's main window puts focus on the command line
        QWidget otherWindow;
        otherWindow.resize(200, 200);
        otherWindow.show();
        QVERIFY(activate(&otherWindow));
        QVERIFY(!pModern->hasFocus());
        QVERIFY(activate(pDock));
        QTRY_VERIFY(pModern->hasFocus());
        commandLine->clear();
        const QVector3D start = pModern->viewCenter();
        QTest::keyClick(static_cast<QWidget*>(nullptr), Qt::Key_W);
        QCoreApplication::processEvents();
        QVERIFY2(pModern->viewCenter().y() > start.y(), qPrintable(qsl("W did not pan; the command line got \"%1\"").arg(commandLine->toPlainText())));
    }

    // After a click the keys go on to the command line, so the view must also win the shortcuts that the command
    // line wins over the application's, as the 2D map, which leaves focus on the command line, does
    void test_aClickedViewClaimsTheShortcutsTheCommandLineClaims()
    {
        ModernGLWidget* pModern = viewAtThePlayer();
        QVERIFY(pModern);
        const auto restore = qScopeGuard([this]() {
            mpHost->mCaretShortcut = Host::CaretShortcut::None;
        });
        mpHost->mCaretShortcut = Host::CaretShortcut::CtrlTab;
        QTest::mouseClick(pModern, Qt::LeftButton, Qt::NoModifier, pixelOfRoom(pModern, kPlayerRoomId));

        QKeyEvent caretShortcut(QEvent::ShortcutOverride, Qt::Key_Tab, Qt::ControlModifier);
        QVERIFY(mpHost->mainConsoleView()->mpCommandLine->claimsShortcutOverride(&caretShortcut));
        caretShortcut.ignore();
        QCoreApplication::sendEvent(pModern, &caretShortcut);
        QVERIFY2(caretShortcut.isAccepted(), "the view left Ctrl+Tab to the application's profile-switching shortcut");

        QKeyEvent other(QEvent::ShortcutOverride, Qt::Key_X, Qt::ControlModifier);
        other.ignore();
        QCoreApplication::sendEvent(pModern, &other);
        QVERIFY(!other.isAccepted());
    }

    // Hiding the focused view hands focus on to the next widget in its window, which in a floating map is one of
    // its buttons; "Edit in 2D" must leave the keys typed next to the command line, as the 2D map does
    void test_editIn2DLeavesTheKeysToTheCommandLine()
    {
        ModernGLWidget* pModern = viewAtThePlayer();
        QVERIFY(pModern);
        QDockWidget* pDock = dockOf(pModern);
        QVERIFY2(pDock, "the map is not in a dock");
        TCommandLine* commandLine = mpHost->mainConsoleView()->mpCommandLine;
        const auto restore = qScopeGuard([pDock, floating = pDock->isFloating(), commandLine]() {
            setDockFloating(pDock, floating);
            commandLine->clear();
        });
        setDockFloating(pDock, true);
        QVERIFY(activate(pDock));
        QTest::mouseClick(pModern, Qt::LeftButton, Qt::NoModifier, pixelOfRoom(pModern, kPlayerRoomId));
        QTRY_VERIFY(pModern->hasFocus());
        commandLine->clear();

        emit pModern->editIn2DRequested(kPlayerRoomId);
        QVERIFY(!pModern->isVisible());
        QTRY_VERIFY2(commandLine->hasFocus(), qPrintable(QDebug::toString(QApplication::focusWidget())));
        QTest::keyClick(static_cast<QWidget*>(nullptr), Qt::Key_N);
        QCOMPARE(commandLine->toPlainText(), qsl("n"));
    }

    // After a click Tab is the command line's, as with the 2D map: QWidget would otherwise take a plain Tab to move
    // focus before the view could pass it on, so the caret key on Tab would not work
    void test_theCaretKeyWorksAfterAClick_data()
    {
        QTest::addColumn<int>("shortcut");
        QTest::addColumn<int>("modifiers");
        QTest::newRow("Tab") << static_cast<int>(Host::CaretShortcut::Tab) << static_cast<int>(Qt::NoModifier);
        QTest::newRow("Ctrl+Tab") << static_cast<int>(Host::CaretShortcut::CtrlTab) << static_cast<int>(Qt::ControlModifier);
    }

    void test_theCaretKeyWorksAfterAClick()
    {
        QFETCH(int, shortcut);
        QFETCH(int, modifiers);
        ModernGLWidget* pModern = viewAtThePlayer();
        QVERIFY(pModern);
        const auto restore = qScopeGuard([this]() {
            mpHost->mCaretShortcut = Host::CaretShortcut::None;
            // TConsole asserts on leaving a caret mode it is not in
            if (mpHost->caretEnabled()) {
                mpHost->setCaretEnabled(false);
            }
        });
        mpHost->mCaretShortcut = static_cast<Host::CaretShortcut>(shortcut);
        QVERIFY(!mpHost->caretEnabled());
        QVERIFY(activate(pModern->window()));
        QTest::mouseClick(pModern, Qt::LeftButton, Qt::NoModifier, pixelOfRoom(pModern, kPlayerRoomId));
        QTRY_VERIFY(pModern->hasFocus());
        // Sent as the window system would, through the shortcut map to the focused widget
        QTest::keyClick(static_cast<QWidget*>(nullptr), Qt::Key_Tab, static_cast<Qt::KeyboardModifiers>(modifiers));
        QCoreApplication::processEvents();
        QVERIFY2(mpHost->caretEnabled(), qPrintable(qsl("focus went to %1").arg(QDebug::toString(QApplication::focusWidget()))));
    }

    // Keypad keys go on to the command line's key bindings even while the view pans with the other keys
    void test_keypadKeyBindingsRunWithFocusFromTheKeyboard()
    {
        ModernGLWidget* pModern = viewAtThePlayer();
        QVERIFY(pModern);
        QCOMPARE(runLua(qsl("keypadBindings = {}\n"
                            "keypadEight = tempKey(mudlet.keymodifier.Keypad, mudlet.key['8'], function() keypadBindings[#keypadBindings + 1] = '8' end)\n"
                            "keypadUp = tempKey(mudlet.keymodifier.Keypad, mudlet.key.Up, function() keypadBindings[#keypadBindings + 1] = 'up' end)")),
                 QString());
        const auto restore = qScopeGuard([this]() {
            runLua(qsl("killKey(keypadEight); killKey(keypadUp)"));
        });
        QVERIFY(focusFromTheKeyboard(pModern));
        const QVector3D start = pModern->viewCenter();
        QTest::keyClick(pModern, Qt::Key_8, Qt::KeypadModifier);
        QCoreApplication::processEvents();
#if !defined(Q_OS_MACOS)
        // On macOS this is how Qt reports the plain up arrow, which pans
        QVERIFY(focusFromTheKeyboard(pModern));
        QTest::keyClick(pModern, Qt::Key_Up, Qt::KeypadModifier);
        QCoreApplication::processEvents();
#endif
        QCOMPARE(runLua(qsl("keypadBindingsText = table.concat(keypadBindings, ',')")), QString());
#if defined(Q_OS_MACOS)
        QCOMPARE(luaGlobalString("keypadBindingsText"), qsl("8"));
#else
        QCOMPARE(luaGlobalString("keypadBindingsText"), qsl("8,up"));
#endif
        QCOMPARE(pModern->viewCenter(), start);
    }

    // A menu, the menu bar or a window switch hands focus back with its own reason and leaves the keys panning; a
    // key passed on to the command line ends that, so re-activating the map's window later does not pan with W
    void test_focusComingBackKeepsTheKeysOnlyIfTheViewDidNotPassThemOn()
    {
        ModernGLWidget* pModern = viewAtThePlayer();
        QVERIFY(pModern);
        QDockWidget* pDock = dockOf(pModern);
        QVERIFY2(pDock, "the map is not in a dock");
        TCommandLine* commandLine = mpHost->mainConsoleView()->mpCommandLine;
        const auto restore = qScopeGuard([pDock, floating = pDock->isFloating(), commandLine]() {
            setDockFloating(pDock, floating);
            commandLine->clear();
        });
        setDockFloating(pDock, true);
        QVERIFY(focusFromTheKeyboard(pModern));

        QMenu menu(pModern);
        menu.addAction(qsl("item"));
        menu.popup(pModern->mapToGlobal(QPoint(10, 10)));
        QTRY_VERIFY(menu.isVisible());
        menu.close();
        QTRY_VERIFY(pModern->hasFocus());
        QVector3D start = pModern->viewCenter();
        QTest::keyClick(static_cast<QWidget*>(nullptr), Qt::Key_W);
        QVERIFY2(pModern->viewCenter().y() > start.y(), "W did not pan after a menu closed");

        // The offscreen platform leaves the closed menu as the active window
        QVERIFY(activate(pDock));
        QTRY_VERIFY(pModern->hasFocus());
        // Stands in for the menu bar, which takes focus with this reason in keyboard mode
        QLineEdit other(pDock);
        other.show();
        other.setFocus(Qt::MenuBarFocusReason);
        QTRY_VERIFY(other.hasFocus());
        pModern->setFocus(Qt::MenuBarFocusReason);
        QTRY_VERIFY(pModern->hasFocus());
        start = pModern->viewCenter();
        QTest::keyClick(static_cast<QWidget*>(nullptr), Qt::Key_W);
        QVERIFY2(pModern->viewCenter().y() > start.y(), "W did not pan after the menu bar gave focus back");

        commandLine->clear();
        QTest::keyClick(static_cast<QWidget*>(nullptr), Qt::Key_X);
        QTRY_VERIFY(commandLine->hasFocus());
        QVERIFY(activate(pDock));
        QTRY_VERIFY(pModern->hasFocus());
        start = pModern->viewCenter();
        QTest::keyClick(static_cast<QWidget*>(nullptr), Qt::Key_W);
        QCoreApplication::processEvents();
        QCOMPARE(commandLine->toPlainText(), qsl("xw"));
        QCOMPARE(pModern->viewCenter(), start);
    }

    // A script handling an earlier press announced at the next one may hide the view; that press must not start a
    // drag on the hidden view, which would pan it when it is back
    void test_aScriptHidingTheMapOnAPressLeavesNoDrag()
    {
        ModernGLWidget* pModern = viewAtThePlayer();
        QVERIFY(pModern);
        QCOMPARE(runLua(qsl("selectedOnPresses = {}\n"
                            "pressHandler = registerAnonymousEventHandler('sysMapWindowMousePressEvent', function()\n"
                            "  selectedOnPresses[#selectedOnPresses + 1] = 0\n"
                            "  if #selectedOnPresses == 1 then closeMapWidget() end\n"
                            "end)")),
                 QString());
        const auto restore = qScopeGuard([this]() {
            runLua(qsl("killAnonymousEventHandler(pressHandler); openMapWidget()"));
        });
        const QPointF corner(5, 5);
        QMouseEvent leftPress(QEvent::MouseButtonPress, corner, pModern->mapToGlobal(corner), Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
        QCoreApplication::sendEvent(pModern, &leftPress);
        QMouseEvent rightPress(QEvent::MouseButtonPress, corner, pModern->mapToGlobal(corner), Qt::RightButton, Qt::LeftButton | Qt::RightButton, Qt::NoModifier);
        QCoreApplication::sendEvent(pModern, &rightPress);
        QVERIFY2(!pModern->isVisible(), "the script did not hide the map");

        // As the toolbar's Map button does; openMapWidget() and viewAtThePlayer() re-dock or swap the view, which
        // would end the drag themselves
        QVERIFY(!mpHost->mapperShown());
        mpHost->showHideOrCreateMapper(false);
        QTRY_VERIFY(pModern->isVisible());
        // A right-drag on its own does not pan; it would carry on the left drag a press on the hidden view began
        const QVector3D start = pModern->viewCenter();
        QMouseEvent laterRightPress(QEvent::MouseButtonPress, corner, pModern->mapToGlobal(corner), Qt::RightButton, Qt::RightButton, Qt::NoModifier);
        QCoreApplication::sendEvent(pModern, &laterRightPress);
        QPointF at = corner;
        for (int step = 0; step < 20; ++step) {
            at += QPointF(5, 5);
            QMouseEvent rightDrag(QEvent::MouseMove, at, pModern->mapToGlobal(at), Qt::NoButton, Qt::RightButton, Qt::NoModifier);
            QCoreApplication::sendEvent(pModern, &rightDrag);
        }
        // A drag moves the view center only as it ends
        QMouseEvent laterRightRelease(QEvent::MouseButtonRelease, at, pModern->mapToGlobal(at), Qt::RightButton, Qt::NoButton, Qt::NoModifier);
        QCoreApplication::sendEvent(pModern, &laterRightRelease);
        if (QMenu* menu = openMenu(pModern)) {
            menu->close();
        }
        QCOMPARE(pModern->viewCenter(), start);
    }

    void test_keysPanZoomAndRecenter()
    {
        ModernGLWidget* pModern = viewAtThePlayer();
        QVERIFY(pModern);
        QVERIFY(focusFromTheKeyboard(pModern));
        QSignalSpy cameraControls(pModern, &ModernGLWidget::cameraControlsChanged);
        const QVector3D start = pModern->viewCenter();

        QTest::keyClick(pModern, Qt::Key_Right);
        QVERIFY2(pModern->viewCenter().x() > start.x(), "the right arrow did not pan east");
        QTest::keyClick(pModern, Qt::Key_W);
        QVERIFY2(pModern->viewCenter().y() > start.y(), "W did not pan north");
        QVERIFY(!pModern->followingPlayer());

        QTest::keyClick(pModern, Qt::Key_Plus, Qt::ShiftModifier);
        QVERIFY(!cameraControls.isEmpty());
        const int zoomedIn = cameraControls.last().at(0).toInt();
        QTest::keyClick(pModern, Qt::Key_Minus);
        QVERIFY2(cameraControls.last().at(0).toInt() < zoomedIn, "minus did not zoom out");

        QTest::keyClick(pModern, Qt::Key_Home);
        QVERIFY2(pModern->followingPlayer(), "Home did not go back to following the player");

        TCommandLine* commandLine = mpHost->mainConsoleView()->mpCommandLine;
        commandLine->clear();
        QTest::keyClick(pModern, Qt::Key_X, Qt::NoModifier);
        QCOMPARE(commandLine->toPlainText(), qsl("x"));
        commandLine->clear();
    }

    // A floating map is a window of its own, so focusing the command line alone leaves the keys typed next here
    void test_typingAfterClickingAFloatingViewReachesTheCommandLine()
    {
        ModernGLWidget* pModern = viewAtThePlayer();
        QVERIFY(pModern);
        QDockWidget* pDock = nullptr;
        for (QWidget* pWidget = pModern->parentWidget(); pWidget && !pDock; pWidget = pWidget->parentWidget()) {
            pDock = qobject_cast<QDockWidget*>(pWidget);
        }
        QVERIFY2(pDock, "the map is not in a dock");
        const auto restore = qScopeGuard([pDock, floating = pDock->isFloating()]() {
            setDockFloating(pDock, floating);
        });
        setDockFloating(pDock, true);
        QVERIFY(activate(pDock));
        pModern->setFocus(Qt::MouseFocusReason);
        QTRY_VERIFY(pModern->hasFocus());
        const QVector3D start = pModern->viewCenter();
        TCommandLine* commandLine = mpHost->mainConsoleView()->mpCommandLine;
        commandLine->clear();

        // Sent where the window system would send them: to whichever widget has the keyboard focus
        for (const Qt::Key key : {Qt::Key_X, Qt::Key_A, Qt::Key_D}) {
            QTest::keyClick(static_cast<QWidget*>(nullptr), key);
            QCoreApplication::processEvents();
        }
        const QString typed = commandLine->toPlainText();
        const QVector3D end = pModern->viewCenter();
        commandLine->clear();
        QCOMPARE(typed, qsl("xad"));
        QCOMPARE(end, start);
    }

    void test_keyPanningStopsAtTheEndsOfTheCoordinateRange()
    {
        ModernGLWidget* pModern = viewAtThePlayer();
        QVERIFY(pModern);
        QVERIFY(focusFromTheKeyboard(pModern));
        // viewCenter() is in floats, which cannot hold the ends exactly, so check only which side of the map it is on
        pModern->setViewCenter(mPlayerAreaId, std::numeric_limits<int>::max(), 0, 0);
        QTest::keyClick(pModern, Qt::Key_Right);
        QVERIFY(pModern->viewCenter().x() > 0.0f);
        pModern->setViewCenter(mPlayerAreaId, 0, std::numeric_limits<int>::min(), 0);
        QTest::keyClick(pModern, Qt::Key_Down);
        QVERIFY(pModern->viewCenter().y() < 0.0f);
    }

    // Rooms picked on the 3D view can be in an area the hidden 2D map is not showing; other views of the map
    // showing that area must still hear that they changed
    void test_editingRoomsPickedIn3DAnnouncesTheirOwnArea()
    {
        ModernGLWidget* pModern = viewAtThePlayer();
        QVERIFY(pModern);
        T2DMap* p2dMap = map()->mapper()->mp2dMap;
        pModern->setViewCenter(mOtherAreaId, 20, 20, 0);
        QCOMPARE(p2dMap->mAreaID, mPlayerAreaId);
        QTest::mouseClick(pModern, Qt::LeftButton, Qt::NoModifier, pixelOfRoom(pModern, kOtherAreaRoomId));
        QCOMPARE(selection(), QSet<int>{kOtherAreaRoomId});

        QSignalSpy areaChanged(map(), &TMap::signal_areaChanged);
        const QString oldName = map()->mpRoomDB->getRoom(kOtherAreaRoomId)->name;
        p2dMap->slot_setRoomProperties(true, qsl("Renamed"), false, 0, false, QString(), false, QColor(), false, 1, false, std::nullopt, false, std::nullopt, false, QColor(), false, 0, selection());
        map()->mpRoomDB->getRoom(kOtherAreaRoomId)->name = oldName;
        QTRY_COMPARE(areaChanged.size(), 1);
        QCOMPARE(areaChanged.constFirst().at(0).toInt(), mOtherAreaId);
    }

    void test_eachAreaKeepsItsOwnCamera()
    {
        ModernGLWidget* pModern = viewAtThePlayer();
        QVERIFY(pModern);
        QSignalSpy cameraControls(pModern, &ModernGLWidget::cameraControlsChanged);
        pModern->setCameraPosition(2.0f, 30.0f, 200.0f);
        const QVariantList playerAreaCamera = cameraControls.last();

        pModern->setViewCenter(mOtherAreaId, 20, 20, 0);
        pModern->setCameraPosition(0.5f, 60.0f, 300.0f);
        QVERIFY(cameraControls.last() != playerAreaCamera);

        pModern->setViewCenter(mPlayerAreaId, 0, 0, 0);
        QCOMPARE(cameraControls.last(), playerAreaCamera);

        // And it outlasts the session
        const QString saved = mpHost->readProfileData(qsl("3dmapper_cameras"));
        QVERIFY2(saved.contains(qsl("\"%1\"").arg(mPlayerAreaId)) && saved.contains(qsl("\"%1\"").arg(mOtherAreaId)), qPrintable(saved));
    }

    // Closing the profile frees the Host and its TMap before the floating dock holding the view is
    // deleted, and that dock's teardown hides the view; must run last as it closes the profile
    void test_zzClosingTheProfileWithTheViewShownLeavesTheFreedMapAlone()
    {
        QPointer<ModernGLWidget> pModern(viewAtThePlayer());
        QVERIFY(pModern);
        mudlet::self()->slot_closeProfileByName(mProfileName);
        mpHost = nullptr;
        QTRY_VERIFY_WITH_TIMEOUT(pModern.isNull(), 5000);
    }
};

#include "Map3DViewInteractionTest.moc"
MUDLET_GROUPED_TEST_MAIN(Map3DViewInteractionTest)
