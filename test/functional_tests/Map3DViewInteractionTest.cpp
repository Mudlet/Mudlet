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

#include <functional>
#include <limits>

#include <QAction>
#include <QDockWidget>
#include <QMenu>
#include <QScopeGuard>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QLineEdit>
#include <QToolTip>
#include <QtMath>
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
#include "TMapView.h"
#include "TMapViewManager.h"
#include "TRoom.h"
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

namespace {
class PaintCounter : public QObject
{
public:
    int mCount = 0;

protected:
    bool eventFilter(QObject*, QEvent* event) override
    {
        if (event->type() == QEvent::Paint) {
            ++mCount;
        }
        return false;
    }
};

// What a sysMapWindowMousePressEvent handler calls to take the press's release from the view: in a dialog it opens,
// which blocks the view's window, or in a popup
QWindow* gpPressedWindow = nullptr;
QPoint gReleaseAt;
QMenu* gpPopup = nullptr;
QPoint gPopupAt;

int releaseInADialog(lua_State*)
{
    QDialog dialog;
    QTimer::singleShot(0, &dialog, [&dialog]() {
        QTest::mouseRelease(gpPressedWindow, Qt::LeftButton, Qt::NoModifier, gReleaseAt);
        dialog.reject();
    });
    dialog.exec();
    return 0;
}

int openAPopup(lua_State*)
{
    gpPopup->popup(gPopupAt);
    return 0;
}

// Counts the repaints asked of a window, which come even where the 3D view has no GL context to paint with
class UpdateRequestCounter : public QObject
{
public:
    int mRequests = 0;

protected:
    bool eventFilter(QObject*, QEvent* event) override
    {
        if (event->type() == QEvent::UpdateRequest) {
            ++mRequests;
        }
        return false;
    }
};
} // namespace

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

    // The 3D view on the player's area in viewing mode, looking straight down with north up, nothing selected
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
        // Edit in 2D switches to editing mode, which a test using it can leave on
        p2dMap->mMapViewOnly = true;
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

    // Sent rather than QTest::mouseMove(), which the offscreen platform does not deliver without a button down
    static void hoverAt(ModernGLWidget* pModern, const QPoint& position)
    {
        QMouseEvent move(QEvent::MouseMove, position, pModern->mapToGlobal(position), Qt::NoButton, Qt::NoButton, Qt::NoModifier);
        QCoreApplication::sendEvent(pModern, &move);
    }

    // The room a double-click there speedwalks to
    static int walkTo(ModernGLWidget* pModern, const QPoint& pixel)
    {
        pModern->mpMap->mTargetID = 0;
        QMouseEvent doubleClick(QEvent::MouseButtonDblClick, pixel, pModern->mapToGlobal(pixel), Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
        QCoreApplication::sendEvent(pModern, &doubleClick);
        return pModern->mpMap->mTargetID;
    }

    static QString tipAt(ModernGLWidget* pModern, const QPoint& pixel)
    {
        QToolTip::hideText();
        QHelpEvent over(QEvent::ToolTip, pixel, pModern->mapToGlobal(pixel));
        QCoreApplication::sendEvent(pModern, &over);
        for (int wait = 0; wait < 50 && !QToolTip::isVisible(); ++wait) {
            QTest::qWait(10);
        }
        const QString tip = QToolTip::text();
        QToolTip::hideText();
        return tip;
    }

    // Looks straight down with north up from wherever puts the east room under the compass
    bool showTheEastRoomUnderTheCompass(ModernGLWidget* pModern) const
    {
        const QPoint compass = pModern->compassRect().center();
        // Centers are whole rooms, so the zoom varies too until one lands under the compass
        for (int step = 0; step <= 20; ++step) {
            pModern->setCameraPosition(1.0f + 0.1f * step, CameraController::scmMinTilt, CameraController::scmNorthUpAzimuth);
            for (int x = -15; x <= 15; ++x) {
                for (int y = -15; y <= 15; ++y) {
                    pModern->syncView(mPlayerAreaId, x, y, 0, kPlayerRoomId);
                    if (pModern->roomAt(compass) == kEastRoomId) {
                        return true;
                    }
                }
            }
        }
        return false;
    }

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
        QVERIFY2(texts.value(1) == qsl("Fit area"), qPrintable(texts.join(qsl(", "))));
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
        // Edit in 2D also switches to editing mode, which the later tests here, all in viewing mode, must not inherit
        QVERIFY(!p2dMap->mMapViewOnly);
        p2dMap->slot_toggleMapViewOnly();
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
        // Announced once only, though the hide queues it
        QTest::qWait(50);
        QCOMPARE(pressesSeenByScript(), 1);

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

    void test_tabReachesTheViewWhoseKeysThenPan_data()
    {
        QTest::addColumn<int>("onwardKey");
        QTest::addColumn<int>("modifiers");
        QTest::newRow("Tab") << static_cast<int>(Qt::Key_Tab) << static_cast<int>(Qt::NoModifier);
        QTest::newRow("Backtab") << static_cast<int>(Qt::Key_Backtab) << static_cast<int>(Qt::ShiftModifier);
    }

    // The keys pan and zoom only with focus from the keyboard, so the view must be in its window's Tab order; from
    // there Tab goes on through the window, not to the command line as it does after a click
    void test_tabReachesTheViewWhoseKeysThenPan()
    {
        QFETCH(int, onwardKey);
        QFETCH(int, modifiers);
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

        const QString typed = mpHost->mainConsoleView()->mpCommandLine->toPlainText();
        QTest::keyClick(static_cast<QWidget*>(nullptr), static_cast<Qt::Key>(onwardKey), static_cast<Qt::KeyboardModifiers>(modifiers));
        QVERIFY2(!pModern->hasFocus(), "Tab did not leave the view");
        QVERIFY2(pDock->isAncestorOf(QApplication::focusWidget()), qPrintable(qsl("focus went to %1").arg(QDebug::toString(QApplication::focusWidget()))));
        QCOMPARE(mpHost->mainConsoleView()->mpCommandLine->toPlainText(), typed);
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
        const QVector3D beforeW = pModern->viewCenter();
        QTest::keyClick(pModern, Qt::Key_W);
        QVERIFY2(pModern->viewCenter().y() > beforeW.y(), "W did not pan with focus from the keyboard");
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

        // Activating the main window for it, mudlet::changeEvent() refocuses the command line with OtherFocusReason;
        // that focus-out is the one that ends the keys panning
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
        // The first press and the later one; the press the hidden view dropped is not announced
        QCOMPARE(pressesSeenByScript(), 2);
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

    void test_hoveringRaisesTheRoomUnderThePointer()
    {
        ModernGLWidget* pModern = viewAtThePlayer();
        QVERIFY(pModern);
        hoverAt(pModern, pixelOfRoom(pModern, kEastRoomId));
        QCOMPARE(pModern->hoveredRoomId(), kEastRoomId);
        hoverAt(pModern, QPoint(pModern->width() / 2, 2));
        QCOMPARE(pModern->hoveredRoomId(), 0);
    }

    // Seen at a slant, the raised room's top stands out past where the room is when not raised
    void test_theRaisedRoomIsPickedWhereItIsDrawn()
    {
        ModernGLWidget* pModern = viewAtThePlayer();
        QVERIFY(pModern);
        pModern->setCameraPosition(0.3f, 70.0f, CameraController::scmNorthUpAzimuth);
        hoverAt(pModern, pixelOfRoom(pModern, kPlayerRoomId));
        QCOMPARE(pModern->hoveredRoomId(), kPlayerRoomId);
        // Just inside the far edge of the raised top: a room is half a unit wide, raised by its height of a sixteenth
        const std::optional<QPointF> farEdge = pModern->mCameraController.pixelForPoint(QVector3D(0.0f, 0.24f, 0.09f));
        QVERIFY(farEdge);
        hoverAt(pModern, farEdge->toPoint());
        QCOMPARE(pModern->hoveredRoomId(), kPlayerRoomId);
    }

    // F frames the whole area, however the camera is tilted and turned
    void test_fitAreaShowsEveryRoom()
    {
        ModernGLWidget* pModern = viewAtThePlayer();
        QVERIFY(pModern);
        pModern->setCameraPosition(CameraController::scmMinDistance, 60.0f, CameraController::scmNorthUpAzimuth + 30.0f);
        QVERIFY2(pixelOfRoom(pModern, kEastRoomId).x() < 0, "the east room is on screen before fitting");
        QVERIFY(focusFromTheKeyboard(pModern));

        QTest::keyClick(pModern, Qt::Key_F);
        QCOMPARE(pModern->viewCenter(), QVector3D(1.0f, 0.0f, 0.0f));
        QVERIFY(!pModern->followingPlayer());
        QTRY_VERIFY2(pixelOfRoom(pModern, kEastRoomId).x() >= 0 && pixelOfRoom(pModern, kPlayerRoomId).x() >= 0, "a room is off screen after fitting");

        // Not merely zoomed out: the rooms fill much of the view
        pModern->setCameraPosition(1.0f, CameraController::scmMinTilt, CameraController::scmNorthUpAzimuth);
        pModern->fitArea();
        QTRY_VERIFY(pixelOfRoom(pModern, kEastRoomId).x() - pixelOfRoom(pModern, kPlayerRoomId).x() > pModern->width() / 3);
    }

    void test_fitAreaShowsTheLevelsAboveAndBelow_data()
    {
        QTest::addColumn<int>("level");
        QTest::addColumn<float>("tilt");
        QTest::addColumn<float>("turn");
        QTest::newRow("a room six levels up, tilted") << 6 << 60.0f << 30.0f;
        QTest::newRow("a room six levels down, tilted") << -6 << 60.0f << 30.0f;
        // Taller than the area is wide, so the top floors come near the camera
        QTest::newRow("a tower thirty levels up, from above") << 30 << CameraController::scmMinTilt << 0.0f;
        QTest::newRow("a tower thirty levels down, from above") << -30 << CameraController::scmMinTilt << 0.0f;
    }

    void test_fitAreaShowsTheLevelsAboveAndBelow()
    {
        QFETCH(int, level);
        QFETCH(float, tilt);
        QFETCH(float, turn);
        ModernGLWidget* pModern = viewAtThePlayer();
        QVERIFY(pModern);
        constexpr int otherLevelRoomId = 3;
        QVERIFY(addRoomAt(otherLevelRoomId, mPlayerAreaId, 1, 0) && map()->setRoomCoordinates(otherLevelRoomId, 1, 0, level));
        auto removeOtherLevelRoom = qScopeGuard([this]() {
            map()->mpRoomDB->removeRoom(otherLevelRoomId);
        });
        pModern->slot_showAllLevels();
        pModern->setCameraPosition(CameraController::scmMinDistance, tilt, CameraController::scmNorthUpAzimuth + turn);

        pModern->fitArea();
        QTRY_VERIFY(!pModern->mCameraSmoothAnimating);
        pModern->updateMatrices();
        const float halfWidth = pModern->width() / 2.0f;
        const float halfHeight = pModern->height() / 2.0f;
        float reach = 0.0f;
        for (const int roomId : {kPlayerRoomId, kEastRoomId, otherLevelRoomId}) {
            const TRoom* pRoom = map()->mpRoomDB->getRoom(roomId);
            // The room's cell on the grid, which the fit leaves a margin around
            for (const QPointF& corner : {QPointF(-0.5, -0.5), QPointF(0.5, -0.5), QPointF(-0.5, 0.5), QPointF(0.5, 0.5)}) {
                const QVector3D point(pRoom->x() + corner.x(), pRoom->y() + corner.y(), pRoom->z());
                const std::optional<QPointF> pixel = pModern->mCameraController.pixelForPoint(point);
                QVERIFY2(pixel, qPrintable(qsl("room %1 is behind the camera after fitting").arg(roomId)));
                const float across = std::max(std::abs(static_cast<float>(pixel->x()) - halfWidth) / halfWidth, std::abs(static_cast<float>(pixel->y()) - halfHeight) / halfHeight);
                QVERIFY2(across <= 1.0f, qPrintable(qsl("room %1 is off screen after fitting, %2 of the way to the edge").arg(roomId).arg(across)));
                reach = std::max(reach, across);
            }
        }
        // Not merely zoomed out: what is fitted fills much of the view
        QVERIFY2(reach > 0.8f, qPrintable(qsl("the rooms reach only %1 of the way to the edge").arg(reach)));
    }

    // The 3D view can be docked smaller than the compass with its margin
    void test_aViewTooSmallForTheCompassHasNone()
    {
        ModernGLWidget* pModern = viewAtThePlayer();
        QVERIFY(pModern);
        const QSize size = pModern->size();
        pModern->resize(pModern->minimumSizeHint());
        const QRect compass = pModern->compassRect();
        pModern->resize(size);
        QVERIFY2(compass.isEmpty(),
                 qPrintable(qsl("a compass of %1x%2 in a view of %3x%4").arg(compass.width()).arg(compass.height()).arg(pModern->minimumSizeHint().width()).arg(pModern->minimumSizeHint().height())));
    }

    // With mouse tracking on, moves keep coming after a release that went to another window or a menu
    void test_aPanWhoseReleaseWentElsewhereStops()
    {
        ModernGLWidget* pModern = viewAtThePlayer();
        QVERIFY(pModern);
        QVERIFY(!pModern->editingMap());
        const QPoint start(pModern->width() / 4, pModern->height() / 4);
        QCOMPARE(pModern->roomAt(start), 0);
        QMouseEvent press(QEvent::MouseButtonPress, start, pModern->mapToGlobal(start), Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
        QCoreApplication::sendEvent(pModern, &press);
        const QVector3D before = pModern->mCameraController.getTarget();
        hoverAt(pModern, start + QPoint(120, 80));
        const QVector3D after = pModern->mCameraController.getTarget();
        const bool panning = pModern->mPanMode;
        QMouseEvent release(QEvent::MouseButtonRelease, start, pModern->mapToGlobal(start), Qt::LeftButton, Qt::NoButton, Qt::NoModifier);
        QCoreApplication::sendEvent(pModern, &release);
        QCOMPARE(after, before);
        QVERIFY(!panning);
    }

    // The pointer then often comes back by way of the level buttons, which a pan left on would undo
    void test_aPanThatLostItsReleaseEndsWhenThePointerLeaves()
    {
        ModernGLWidget* pModern = viewAtThePlayer();
        QVERIFY(pModern);
        const QPoint start(pModern->width() / 4, pModern->height() / 4);
        QCOMPARE(pModern->roomAt(start), 0);
        QCOMPARE(runLua(qsl("selectedOnPresses = {}\n"
                            "pressHandler = registerAnonymousEventHandler('sysMapWindowMousePressEvent', function()\n"
                            "  selectedOnPresses[#selectedOnPresses + 1] = 0\n"
                            "end)")),
                 QString());
        const auto restore = qScopeGuard([this]() {
            runLua(qsl("killAnonymousEventHandler(pressHandler)"));
        });
        // Sent to the view alone, so the application sees no button down, as once a popup has taken the release
        QMouseEvent press(QEvent::MouseButtonPress, start, pModern->mapToGlobal(start), Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
        QCoreApplication::sendEvent(pModern, &press);
        QCOMPARE(QGuiApplication::mouseButtons(), Qt::NoButton);
        QEvent leave(QEvent::Leave);
        QCoreApplication::sendEvent(pModern, &leave);
        // Announced, as the press would have been on its release
        QTRY_COMPARE(pressesSeenByScript(), 1);
        const int level = static_cast<int>(pModern->viewCenter().z());
        pModern->slot_shiftZup();
        hoverAt(pModern, start);
        QCOMPARE(static_cast<int>(pModern->viewCenter().z()), level + 1);
    }

    // A dialog a press handler opens takes the release, and the view hears of neither it nor a Leave
    void test_aPressWhoseHandlerTakesTheReleaseIsOver()
    {
        ModernGLWidget* pModern = viewAtThePlayer();
        QVERIFY(pModern);
        gpPressedWindow = pModern->window()->windowHandle();
        QVERIFY(gpPressedWindow);
        gReleaseAt = pModern->mapTo(pModern->window(), pixelOfRoom(pModern, kPlayerRoomId));
        lua_register(mpHost->getLuaInterpreter()->getLuaGlobalState(), "releaseInADialog", releaseInADialog);
        QCOMPARE(runLua(qsl("pressHandler = registerAnonymousEventHandler('sysMapWindowMousePressEvent', releaseInADialog)")), QString());
        const auto restore = qScopeGuard([this]() {
            runLua(qsl("killAnonymousEventHandler(pressHandler)"));
        });
        // Through the window, so the application knows the button is down until the release
        QTest::mousePress(gpPressedWindow, Qt::LeftButton, Qt::NoModifier, gReleaseAt);
        QCOMPARE(QGuiApplication::mouseButtons(), Qt::NoButton);
        QCOMPARE(selection(), QSet<int>{kPlayerRoomId});
        QVERIFY2(!pModern->mPanMode, "the press goes on panning after its release went to the dialog");
    }

    // A popup a press handler opens takes the rest of the press, and no Leave reaches the view while it is open
    void test_aPressWhoseHandlerOpensAPopupIsOver()
    {
        ModernGLWidget* pModern = viewAtThePlayer();
        QVERIFY(pModern);
        QMenu menu;
        menu.addAction(qsl("one"));
        gpPopup = &menu;
        const QPoint room = pixelOfRoom(pModern, kPlayerRoomId);
        gPopupAt = pModern->mapToGlobal(room + QPoint(5, 5));
        lua_register(mpHost->getLuaInterpreter()->getLuaGlobalState(), "openAPopup", openAPopup);
        QCOMPARE(runLua(qsl("pressHandler = registerAnonymousEventHandler('sysMapWindowMousePressEvent', openAPopup)")), QString());
        const auto restore = qScopeGuard([this]() {
            runLua(qsl("killAnonymousEventHandler(pressHandler)"));
        });
        QWindow* pWindow = pModern->window()->windowHandle();
        QVERIFY(pWindow);
        const QPoint inWindow = pModern->mapTo(pModern->window(), room);
        QTest::mousePress(pWindow, Qt::LeftButton, Qt::NoModifier, inWindow);
        const auto release = qScopeGuard([pWindow, inWindow, &menu]() {
            QTest::mouseRelease(pWindow, Qt::LeftButton, Qt::NoModifier, inWindow);
            menu.close();
        });
        if (QApplication::activePopupWidget() != &menu) {
            QSKIP("popups do not open on this platform");
        }
        QVERIFY2(!pModern->mPanMode, "the press goes on panning under the popup");
    }

    // As on platforms that send Leave while a drag holds the mouse
    void test_aLeaveWhileTheButtonIsHeldKeepsThePanGoing()
    {
        ModernGLWidget* pModern = viewAtThePlayer();
        QVERIFY(pModern);
        QWindow* pWindow = pModern->window()->windowHandle();
        QVERIFY(pWindow);
        const QPoint start(pModern->width() / 4, pModern->height() / 4);
        QCOMPARE(pModern->roomAt(start), 0);
        const QPoint inWindow = pModern->mapTo(pModern->window(), start);
        // Through the window, so the application knows the button is down
        QTest::mousePress(pWindow, Qt::LeftButton, Qt::NoModifier, inWindow);
        const auto release = qScopeGuard([pWindow, inWindow]() {
            QTest::mouseRelease(pWindow, Qt::LeftButton, Qt::NoModifier, inWindow);
        });
        QCOMPARE(QGuiApplication::mouseButtons(), Qt::LeftButton);
        QVERIFY(pModern->mPanMode);
        QEvent leave(QEvent::Leave);
        QCoreApplication::sendEvent(pModern, &leave);
        QVERIFY2(pModern->mPanMode, "the pan ended while the button was still held");
    }

    void test_clickingTheCompassTurnsTheViewNorthUp()
    {
        ModernGLWidget* pModern = viewAtThePlayer();
        QVERIFY(pModern);
        QSignalSpy cameraControls(pModern, &ModernGLWidget::cameraControlsChanged);
        pModern->setCameraPosition(1.0f, 50.0f, CameraController::scmNorthUpAzimuth + 40.0f);
        pModern->shiftCamera(0.0f, 0.0f, 25.0f);
        const QVariantList turned = cameraControls.last();
        QVERIFY(turned.at(3).toInt() != 0 && turned.at(2).toInt() != 0);
        const QRect compass = pModern->compassRect();
        QVERIFY(pModern->rect().contains(compass));

        QTest::mouseClick(pModern, Qt::LeftButton, Qt::NoModifier, compass.center());
        QCOMPARE(cameraControls.last().at(3).toInt(), 0);
        QCOMPARE(cameraControls.last().at(2).toInt(), 0);
        QCOMPARE(cameraControls.last().at(1), turned.at(1));

        // Nor does it pick the room drawn under it
        QVERIFY2(showTheEastRoomUnderTheCompass(pModern), "no view puts the east room under the compass");
        QTest::mouseClick(pModern, Qt::LeftButton, Qt::NoModifier, pModern->compassRect().center());
        QVERIFY(selection().isEmpty());
    }

    // A double click on the compass is two clicks on it, not a speedwalk to the room drawn under it
    void test_doubleClickingTheCompassDoesNotSpeedwalk()
    {
        ModernGLWidget* pModern = viewAtThePlayer();
        QVERIFY(pModern);
        QVERIFY2(showTheEastRoomUnderTheCompass(pModern), "no view puts the east room under the compass");
        const QPoint compass = pModern->compassRect().center();
        map()->mTargetID = 0;
        QMouseEvent doubleClick(QEvent::MouseButtonDblClick, compass, pModern->mapToGlobal(compass), Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
        QCoreApplication::sendEvent(pModern, &doubleClick);
        QCOMPARE(map()->mTargetID, 0);
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

    // A double click on an area exit's arrow walks into the area it leads to, as on the 2D map; hovering names the room it leads to
    void test_anAreaExitLeadsIntoTheNextArea()
    {
        ModernGLWidget* pModern = viewAtThePlayer();
        QVERIFY(pModern);
        TRoom* pEastRoom = map()->mpRoomDB->getRoom(kEastRoomId);
        QVERIFY(pEastRoom->setExit(kOtherAreaRoomId, DIR_EAST));
        const QPoint playerRoom = pixelOfRoom(pModern, kPlayerRoomId);
        const QPoint eastRoom = pixelOfRoom(pModern, kEastRoomId);
        // The rooms are two apart, and the arrow reaches a room out from the east one's middle
        const int pixelsPerRoom = (eastRoom.x() - playerRoom.x()) / 2;
        const QPoint onTheArrow = eastRoom + QPoint(pixelsPerRoom * 3 / 4, 0);
        const QPoint pastTheArrow = eastRoom + QPoint(pixelsPerRoom * 2, 0);
        const int arrowLeadsTo = pModern->areaExitAt(onTheArrow);
        const int pastLeadsTo = pModern->areaExitAt(pastTheArrow);
        const int roomOnTheArrow = pModern->roomAt(onTheArrow);

        map()->mTargetID = 0;
        QMouseEvent doubleClick(QEvent::MouseButtonDblClick, onTheArrow, pModern->mapToGlobal(onTheArrow), Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
        QCoreApplication::sendEvent(pModern, &doubleClick);
        const int walkedTo = map()->mTargetID;
        QHelpEvent overArrow(QEvent::ToolTip, onTheArrow, pModern->mapToGlobal(onTheArrow));
        QCoreApplication::sendEvent(pModern, &overArrow);
        QTRY_VERIFY(QToolTip::isVisible());
        const QString tip = QToolTip::text();
        QToolTip::hideText();
        pEastRoom->setExit(-1, DIR_EAST);

        QCOMPARE(roomOnTheArrow, 0);
        QCOMPARE(arrowLeadsTo, kOtherAreaRoomId);
        QCOMPARE(pastLeadsTo, 0);
        QCOMPARE(walkedTo, kOtherAreaRoomId);
        QVERIFY2(tip.contains(qsl("Room ID: %1").arg(kOtherAreaRoomId)) && tip.contains(qsl("Other Area")), qPrintable(tip));
    }

    // Over an area exit's arrow: the viewed level's room the arrow starts in wins, but a room on another level
    // under the arrow, or a room merely near the pointer, loses to it
    void test_anAreaExitArrowGivesWayOnlyToItsOwnRoom()
    {
        ModernGLWidget* pModern = viewAtThePlayer();
        QVERIFY(pModern);
        constexpr int lowerRoomId = 30;
        TRoom* pEastRoom = map()->mpRoomDB->getRoom(kEastRoomId);
        QVERIFY(pEastRoom->setExit(kOtherAreaRoomId, DIR_EAST));
        QVERIFY(addRoomAt(lowerRoomId, mPlayerAreaId, 3, 0) && map()->setRoomCoordinates(lowerRoomId, 3, 0, -1));

        const QPoint eastRoom = pixelOfRoom(pModern, kEastRoomId);
        const int walkedFromTheRoom = walkTo(pModern, eastRoom);
        const QString tipOverTheRoom = tipAt(pModern, eastRoom);
        // Where the arrow is drawn over the room on the level below
        QPoint overTheLowerRoom(-1, -1);
        for (int x = eastRoom.x(); x < pModern->width(); ++x) {
            const QPoint pixel(x, eastRoom.y());
            if (pModern->roomAt(pixel, false) == lowerRoomId && pModern->areaExitAt(pixel) == kOtherAreaRoomId) {
                overTheLowerRoom = pixel;
                break;
            }
        }
        const int walkedOverTheLowerRoom = overTheLowerRoom.x() < 0 ? -1 : walkTo(pModern, overTheLowerRoom);
        const QString tipOverTheLowerRoom = overTheLowerRoom.x() < 0 ? QString() : tipAt(pModern, overTheLowerRoom);
        // Another level's arrow is offered nowhere, as on the 2D map
        QVERIFY(map()->mpRoomDB->getRoom(lowerRoomId)->setExit(kOtherAreaRoomId, DIR_SOUTH));
        const std::optional<QPointF> alongTheLowerArrow = pModern->mCameraController.pixelForPoint(QVector3D(3.0f, -0.6f, -1.0f));
        const int pickedOnTheLowerArrow = alongTheLowerArrow ? pModern->areaExitAt(alongTheLowerArrow->toPoint()) : -1;
        map()->mpRoomDB->removeRoom(lowerRoomId);

        // Zoomed out until the arrow is within the slop that picks a room near the pointer
        qreal spacing = 0.0;
        for (float distance = 1.0f; distance <= CameraController::scmMaxDistance; distance *= 1.1f) {
            pModern->setCameraDistance(distance);
            pModern->updateMatrices();
            const std::optional<QPointF> from = pModern->mCameraController.pixelForPoint(QVector3D(2.0f, 0.0f, 0.0f));
            const std::optional<QPointF> to = pModern->mCameraController.pixelForPoint(QVector3D(3.0f, 0.0f, 0.0f));
            spacing = from && to ? QLineF(*from, *to).length() : 0.0;
            if (spacing <= 7.0) {
                break;
            }
        }
        const std::optional<QPointF> alongTheArrow = pModern->mCameraController.pixelForPoint(QVector3D(2.75f, 0.0f, 0.0f));
        const QPoint zoomedOutArrow = alongTheArrow ? alongTheArrow->toPoint() : QPoint(-1, -1);
        const int nearRoom = pModern->roomAt(zoomedOutArrow);
        const int walkedZoomedOut = walkTo(pModern, zoomedOutArrow);
        pEastRoom->setExit(-1, DIR_EAST);

        QCOMPARE(walkedFromTheRoom, kEastRoomId);
        QVERIFY2(tipOverTheRoom.contains(qsl("Room ID: %1").arg(kEastRoomId)) && !tipOverTheRoom.contains(qsl("Other Area")), qPrintable(tipOverTheRoom));
        QVERIFY2(overTheLowerRoom.x() >= 0, "the arrow is drawn over the lower room nowhere");
        QCOMPARE(walkedOverTheLowerRoom, kOtherAreaRoomId);
        QVERIFY2(tipOverTheLowerRoom.contains(qsl("Other Area")), qPrintable(tipOverTheLowerRoom));
        QCOMPARE(pickedOnTheLowerArrow, 0);
        QVERIFY2(spacing > 4.0 && spacing <= 7.0, qPrintable(qsl("a room spacing is %1 pixels").arg(spacing)));
        QCOMPARE(nearRoom, kEastRoomId);
        QCOMPARE(walkedZoomedOut, kOtherAreaRoomId);
    }

    // As on the 2D map, the viewed level's room wins even where the see-through room above it is nearer the camera
    void test_theViewedLevelsRoomWinsUnderARoomAbove()
    {
        ModernGLWidget* pModern = viewAtThePlayer();
        QVERIFY(pModern);
        constexpr int upperRoomId = 31;
        const QPoint eastRoom = pixelOfRoom(pModern, kEastRoomId);
        QVERIFY(eastRoom.x() >= 0);
        QVERIFY(addRoomAt(upperRoomId, mPlayerAreaId, 2, 0) && map()->setRoomCoordinates(upperRoomId, 2, 0, 1));
        const int nearestHit = pModern->roomAt(eastRoom, false);
        const int walkedUnderTheRoomAbove = walkTo(pModern, eastRoom);
        const QString tipUnderTheRoomAbove = tipAt(pModern, eastRoom);
        TRoom* pEastRoom = map()->mpRoomDB->getRoom(kEastRoomId);
        QVERIFY(pEastRoom->setExit(kOtherAreaRoomId, DIR_EAST));
        const int arrowLeadsTo = pModern->areaExitAt(eastRoom);
        const int walkedWithTheArrow = walkTo(pModern, eastRoom);
        const QString tipWithTheArrow = tipAt(pModern, eastRoom);
        pEastRoom->setExit(-1, DIR_EAST);
        map()->mpRoomDB->removeRoom(upperRoomId);

        QCOMPARE(nearestHit, upperRoomId);
        QCOMPARE(arrowLeadsTo, kOtherAreaRoomId);
        QCOMPARE(walkedUnderTheRoomAbove, kEastRoomId);
        QVERIFY2(tipUnderTheRoomAbove.contains(qsl("Room ID: %1").arg(kEastRoomId)), qPrintable(tipUnderTheRoomAbove));
        QCOMPARE(walkedWithTheArrow, kEastRoomId);
        QVERIFY2(tipWithTheArrow.contains(qsl("Room ID: %1").arg(kEastRoomId)) && !tipWithTheArrow.contains(qsl("Other Area")), qPrintable(tipWithTheArrow));
    }

    // Hovering, clicking with or without Ctrl or Shift, the room menu and picking a room up to move it all act on the room the tooltip names
    // and a double click walks to: the viewed level's, not the see-through room above it that is nearer
    void test_everyPointerActionPicksTheViewedLevelsRoomUnderARoomAbove()
    {
        ModernGLWidget* pModern = viewAtThePlayer();
        QVERIFY(pModern);
        T2DMap* p2dMap = map()->mapper()->mp2dMap;
        constexpr int upperRoomId = 31;
        const QPoint eastRoom = pixelOfRoom(pModern, kEastRoomId);
        QVERIFY(eastRoom.x() >= 0);
        QVERIFY(addRoomAt(upperRoomId, mPlayerAreaId, 2, 0) && map()->setRoomCoordinates(upperRoomId, 2, 0, 1));
        const int nearestHit = pModern->roomAt(eastRoom, false);

        hoverAt(pModern, eastRoom);
        const int hovered = pModern->mHoveredRoomId;
        QEvent leave(QEvent::Leave);
        QCoreApplication::sendEvent(pModern, &leave);
        p2dMap->mMultiSelectionSet.clear();
        QTest::mouseClick(pModern, Qt::LeftButton, Qt::NoModifier, eastRoom);
        const QSet<int> clicked = selection();
        // With Ctrl or Shift the room is picked on the release, as the press may begin an orbit
        p2dMap->mMultiSelectionSet.clear();
        QTest::mouseClick(pModern, Qt::LeftButton, Qt::ControlModifier, eastRoom);
        const QSet<int> ctrlClicked = selection();
        p2dMap->mMultiSelectionSet.clear();
        QTest::mouseClick(pModern, Qt::LeftButton, Qt::ShiftModifier, eastRoom);
        const QSet<int> shiftClicked = selection();
        p2dMap->mMultiSelectionSet.clear();
        p2dMap->mMapViewOnly = false;
        QTest::mouseClick(pModern, Qt::RightButton, Qt::NoModifier, eastRoom);
        QMenu* menu = openMenu(pModern);
        const QSet<int> menuFor = selection();
        if (menu) {
            menu->close();
        }
        p2dMap->mMultiSelectionSet.clear();
        QTest::mousePress(pModern, Qt::LeftButton, Qt::NoModifier, eastRoom);
        const bool moving = pModern->mMovingRooms;
        const QSet<int> pickedUp = selection();
        QTest::mouseRelease(pModern, Qt::LeftButton, Qt::NoModifier, eastRoom);
        p2dMap->mMapViewOnly = true;
        p2dMap->mMultiSelectionSet.clear();
        map()->mpRoomDB->removeRoom(upperRoomId);

        QCOMPARE(nearestHit, upperRoomId);
        QCOMPARE(hovered, kEastRoomId);
        QCOMPARE(clicked, QSet<int>{kEastRoomId});
        QCOMPARE(ctrlClicked, QSet<int>{kEastRoomId});
        QCOMPARE(shiftClicked, QSet<int>{kEastRoomId});
        QVERIFY2(menu, "right-clicking the room opened no menu");
        QCOMPARE(menuFor, QSet<int>{kEastRoomId});
        QVERIFY(moving);
        QCOMPARE(pickedUp, QSet<int>{kEastRoomId});
    }

    void test_keysTurnTheViewAndChangeLevel()
    {
        ModernGLWidget* pModern = viewAtThePlayer();
        QVERIFY(pModern);
        QVERIFY(focusFromTheKeyboard(pModern));
        QSignalSpy cameraControls(pModern, &ModernGLWidget::cameraControlsChanged);
        const QVector3D start = pModern->viewCenter();

        QTest::keyClick(pModern, Qt::Key_Q);
        QVERIFY(!cameraControls.isEmpty());
        const int turnedByQ = cameraControls.last().at(3).toInt();
        QTest::keyClick(pModern, Qt::Key_E);
        const int turnedBack = cameraControls.last().at(3).toInt();
        QTest::keyClick(pModern, Qt::Key_E);
        const int turnedByE = cameraControls.last().at(3).toInt();
        QVERIFY2(turnedByQ != 0 && turnedByE != 0 && turnedByQ != turnedByE, qPrintable(qsl("Q turned to %1, E to %2").arg(turnedByQ).arg(turnedByE)));
        QCOMPARE(turnedBack, 0);

        QTest::keyClick(pModern, Qt::Key_PageUp);
        QCOMPARE(pModern->viewCenter(), start + QVector3D(0.0f, 0.0f, 1.0f));
        QTest::keyClick(pModern, Qt::Key_PageDown);
        QTest::keyClick(pModern, Qt::Key_PageDown);
        QCOMPARE(pModern->viewCenter(), start - QVector3D(0.0f, 0.0f, 1.0f));
    }

    // Round rooms are picked within their circle, not their square
    void test_roundRoomsArePickedWithinTheirCircle()
    {
        ModernGLWidget* pModern = viewAtThePlayer();
        QVERIFY(pModern);
        const QPoint playerRoom = pixelOfRoom(pModern, kPlayerRoomId);
        QPoint eastRoom = pixelOfRoom(pModern, kEastRoomId);
        const int pixelsPerRoom = (eastRoom.x() - playerRoom.x()) / 2;
        // Just under a quarter of a room spacing up and across is inside a room's square (half a spacing wide) but outside its circle
        const QPoint corner = eastRoom + QPoint(pixelsPerRoom * 23 / 100, -pixelsPerRoom * 23 / 100);
        const int squarePick = pModern->roomAt(corner);
        mpHost->mBubbleMode = true;
        const int roundPick = pModern->roomAt(corner);
        const int roundMiddlePick = pModern->roomAt(eastRoom);
        mpHost->mBubbleMode = false;

        // Turned an eighth, a box around the circle leaves out the square's corners
        pModern->setCameraPosition(1.0f, CameraController::scmMinTilt, CameraController::scmNorthUpAzimuth + 45.0f);
        eastRoom = pixelOfRoom(pModern, kEastRoomId);
        const int aroundTheCircle = pixelsPerRoom * 3 / 10;
        const QRect box(eastRoom - QPoint(aroundTheCircle, aroundTheCircle), eastRoom + QPoint(aroundTheCircle, aroundTheCircle));
        const QSet<int> squareBoxed = pModern->roomsInRectangle(box, false);
        mpHost->mBubbleMode = true;
        const QSet<int> roundBoxed = pModern->roomsInRectangle(box, false);
        mpHost->mBubbleMode = false;

        QCOMPARE(squarePick, kEastRoomId);
        QCOMPARE(roundPick, 0);
        QCOMPARE(roundMiddlePick, kEastRoomId);
        QVERIFY(squareBoxed.isEmpty());
        QCOMPARE(roundBoxed, QSet<int>{kEastRoomId});
    }

    // A box takes a round room only around the whole of the rim drawn, which a turned view can push out past points
    // sampled around a truer circle
    void test_aBoxTakesARoundRoomOnlyAroundItsWholeDrawnRim()
    {
        ModernGLWidget* pModern = viewAtThePlayer();
        QVERIFY(pModern);
        mpHost->mBubbleMode = true;
        auto squareAgain = qScopeGuard([this] {
            mpHost->mBubbleMode = false;
        });
        const GeometryData mesh = GeometryManager::cylinderMesh(GeometryManager::scmCylinderSegments);
        const QVector3D center(0.0f, 0.0f, 0.0f);
        const QVector3D halfSize(1.0f / pModern->scale, 1.0f / pModern->scale, 1.0f / pModern->scale / pModern->zFlattening);
        const auto drawnBounds = [&]() {
            pModern->updateMatrices();
            qreal left = std::numeric_limits<qreal>::max();
            qreal right = std::numeric_limits<qreal>::lowest();
            qreal top = left;
            qreal bottom = right;
            for (qsizetype index = 0; index + 2 < mesh.vertices.size(); index += 3) {
                const QVector3D vertex(mesh.vertices.at(index), mesh.vertices.at(index + 1), mesh.vertices.at(index + 2));
                const std::optional<QPointF> pixel = pModern->mCameraController.pixelForPoint(center + vertex * halfSize);
                if (!pixel) {
                    return QRectF();
                }
                left = qMin(left, pixel->x());
                right = qMax(right, pixel->x());
                top = qMin(top, pixel->y());
                bottom = qMax(bottom, pixel->y());
            }
            return QRectF(QPointF(left, top), QPointF(right, bottom));
        };
        // Close enough that the rim's corners stand out by whole pixels from points between them
        const qreal radius = qMin(240.0, (qMin(pModern->width(), pModern->height()) - 40) / 2.0);
        float distance = 1.0f;
        for (; distance > CameraController::scmMinDistance; distance /= 1.05f) {
            pModern->setCameraPosition(distance, CameraController::scmMinTilt, CameraController::scmNorthUpAzimuth);
            if (drawnBounds().width() / 2.0 >= radius) {
                break;
            }
        }
        const QRectF view(pModern->rect());
        QStringList wronglyTaken;
        QStringList wronglyLeft;
        for (int step = 0; step < 96; ++step) {
            const float turn = 0.5f * static_cast<float>(step);
            pModern->setCameraPosition(distance, CameraController::scmMinTilt, CameraController::scmNorthUpAzimuth + turn);
            const QRectF drawn = drawnBounds();
            QVERIFY2(!drawn.isEmpty() && view.contains(drawn), qPrintable(qsl("the room is drawn off the view turned %1").arg(turn)));
            const int left = qFloor(drawn.left()) - 2;
            const int top = qFloor(drawn.top()) - 2;
            const int right = qCeil(drawn.right()) + 2;
            const int bottom = qCeil(drawn.bottom()) + 2;
            const auto taken = [pModern](const int boxLeft, const int boxTop, const int boxRight, const int boxBottom) {
                return pModern->roomsInRectangle(QRect(boxLeft, boxTop, boxRight - boxLeft, boxBottom - boxTop), false).contains(kPlayerRoomId);
            };
            if (!taken(left, top, right, bottom)) {
                wronglyLeft << QString::number(turn);
            }
            // Each side in turn just inside the rim drawn
            const bool cutRight = taken(left, top, qFloor(drawn.right() - 0.25), bottom);
            const bool cutLeft = taken(qCeil(drawn.left() + 0.25), top, right, bottom);
            const bool cutBottom = taken(left, top, right, qFloor(drawn.bottom() - 0.25));
            const bool cutTop = taken(left, qCeil(drawn.top() + 0.25), right, bottom);
            if (cutRight || cutLeft || cutBottom || cutTop) {
                wronglyTaken << QString::number(turn);
            }
        }

        QVERIFY2(wronglyLeft.isEmpty(), qPrintable(qsl("a box around the whole room left it out, turned %1").arg(wronglyLeft.join(qsl(", ")))));
        QVERIFY2(wronglyTaken.isEmpty(), qPrintable(qsl("a box cutting the rim drawn took the room, turned %1").arg(wronglyTaken.join(qsl(", ")))));
    }

    // The map menu's toggles for the grid, round rooms and the upper and lower levels, and Profile Preferences, which
    // sets the upper and lower levels through the same call, show at once in the mapper's 3D view and another window's
    void test_mapperSettingsRepaintEvery3DView()
    {
        ModernGLWidget* pModern = viewAtThePlayer();
        QVERIFY(pModern);
        dlgMapper* pMapper = map()->mapper();
        const bool grid = mpHost->mMapperShowGrid;
        const bool round = mpHost->mBubbleMode;
        const bool levels = mudlet::self()->drawUpperLowerLevels();
        // Every map window draws again on this signal, as the 2D map does
        QSignalSpy repaintRequests(map(), &TMap::signal_mapRepaintRequested);
        const auto requestsRepaint = [&repaintRequests](const std::function<void()>& change) {
            repaintRequests.clear();
            change();
            return !repaintRequests.isEmpty();
        };
        const bool gridRequests = requestsRepaint([pMapper, grid] {
            pMapper->slot_setShowGrid(!grid);
        });
        const bool roundRequests = requestsRepaint([pMapper, round] {
            pMapper->slot_toggleRoundRooms(!round);
        });
        const bool levelsRequest = requestsRepaint([levels] {
            mudlet::self()->setDrawUpperLowerLevels(!levels);
        });
        pMapper->slot_setShowGrid(grid);
        pMapper->slot_toggleRoundRooms(round);
        mudlet::self()->setDrawUpperLowerLevels(levels);
        QVERIFY2(gridRequests, "showing the grid did not ask the map's windows to repaint");
        QVERIFY2(roundRequests, "round rooms did not ask the map's windows to repaint");
        QVERIFY2(levelsRequest, "showing the upper and lower levels did not ask the map's windows to repaint");

        TMapViewManager* pManager = map()->getViewManager();
        QVERIFY(pManager);
        const auto [viewId, error] = pManager->createView(mPlayerAreaId);
        QVERIFY2(viewId, qPrintable(error));
        const auto closeWindow = qScopeGuard([pManager, viewId = viewId]() {
            pManager->closeView(viewId);
        });
        TMapView* pView = pManager->getView(viewId);
        QVERIFY(pView);
        auto* pWindowDock = qobject_cast<QDockWidget*>(pView->parentWidget());
        QVERIFY2(pWindowDock, "the map window is not in a dock");
        // A window of its own, so that only the 3D view in it asks that window to repaint
        setDockFloating(pWindowDock, true);
        pView->show3DView(true);
        ModernGLWidget* pWindow3D = pView->get3DView();
        QVERIFY(pWindow3D);
        QTRY_VERIFY(pWindow3D->isVisible());
        QVERIFY(pWindow3D->window() != pModern->window());
        // Asked of the window even where the view has no context to paint with, so seen on any platform
        UpdateRequestCounter windowRequests;
        pWindow3D->window()->installEventFilter(&windowRequests);
        const auto windowAsked = [&windowRequests]() {
            return windowRequests.mRequests > 0;
        };
        const auto asksTheWindow = [&windowRequests, &windowAsked](const std::function<void()>& change) {
            QCoreApplication::processEvents();
            windowRequests.mRequests = 0;
            change();
            return QTest::qWaitFor(windowAsked, 500);
        };
        const bool askedUnprompted = asksTheWindow([] {});
        const bool gridAsks = asksTheWindow([pMapper, grid] {
            pMapper->slot_setShowGrid(!grid);
        });
        const bool roundAsks = asksTheWindow([pMapper, round] {
            pMapper->slot_toggleRoundRooms(!round);
        });
        const bool levelsAsk = asksTheWindow([levels] {
            mudlet::self()->setDrawUpperLowerLevels(!levels);
        });
        pWindow3D->window()->removeEventFilter(&windowRequests);
        pMapper->slot_setShowGrid(grid);
        pMapper->slot_toggleRoundRooms(round);
        mudlet::self()->setDrawUpperLowerLevels(levels);
        QVERIFY2(!askedUnprompted, "the map window asks to repaint unprompted, so its requests show nothing");
        QVERIFY2(gridAsks, "showing the grid did not ask the map window's 3D view to repaint");
        QVERIFY2(roundAsks, "round rooms did not ask the map window's 3D view to repaint");
        QVERIFY2(levelsAsk, "showing the upper and lower levels did not ask the map window's 3D view to repaint");

        // Paints are only watched where the views have a context
        const auto contextMade = [pModern]() {
            return pModern->isValid();
        };
        if (!QTest::qWaitFor(contextMade, 1000)) {
            QSKIP("no OpenGL context on this platform, so the 3D views do not paint");
        }
        QTRY_VERIFY(pWindow3D->isVisible() && pWindow3D->isValid());
        PaintCounter mapperPaints;
        PaintCounter windowPaints;
        pModern->installEventFilter(&mapperPaints);
        pWindow3D->installEventFilter(&windowPaints);
        const auto bothPainted = [&mapperPaints, &windowPaints]() {
            return mapperPaints.mCount && windowPaints.mCount;
        };
        const auto repaintsBoth = [&mapperPaints, &windowPaints, &bothPainted](const std::function<void()>& change) {
            QCoreApplication::processEvents();
            mapperPaints.mCount = 0;
            windowPaints.mCount = 0;
            change();
            return QTest::qWaitFor(bothPainted, 500);
        };
        const bool gridRepaints = repaintsBoth([pMapper, grid] {
            pMapper->slot_setShowGrid(!grid);
        });
        const bool roundRepaints = repaintsBoth([pMapper, round] {
            pMapper->slot_toggleRoundRooms(!round);
        });
        const bool menuLevelsRepaint = repaintsBoth([pMapper, levels] {
            pMapper->slot_toggleUpperLowerLevels(!levels);
        });
        const bool preferencesLevelsRepaint = repaintsBoth([levels] {
            mudlet::self()->setDrawUpperLowerLevels(levels);
        });
        pModern->removeEventFilter(&mapperPaints);
        pWindow3D->removeEventFilter(&windowPaints);
        pMapper->slot_setShowGrid(grid);
        pMapper->slot_toggleRoundRooms(round);
        pMapper->slot_toggleUpperLowerLevels(levels);

        QVERIFY2(gridRepaints, "showing the grid did not repaint both 3D views");
        QVERIFY2(roundRepaints, "round rooms did not repaint both 3D views");
        QVERIFY2(menuLevelsRepaint, "the menu's upper and lower levels toggle did not repaint both 3D views");
        QVERIFY2(preferencesLevelsRepaint, "Profile Preferences' upper and lower levels setting did not repaint both 3D views");
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
