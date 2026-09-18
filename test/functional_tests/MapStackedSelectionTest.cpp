/***************************************************************************
 *   Copyright (C) 2026 by the Mudlet authors                              *
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
 *   Free Software Foundation, Inc.,                                         *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

/*
 * Two rooms stacked on the same coordinate both get selected by a click, and
 * the selection window lets the user deselect one. Starting a drag from the
 * stack is another left-click: RoomMoveActivationHandler used to look only at
 * the FIRST room under the cursor, and if that one happened to be the room the
 * user had just deselected it cleared the selection and re-selected the whole
 * stack - dragging back a room the user had explicitly dropped. The fix: if
 * ANY of the rooms under the cursor is already selected, leave the selection
 * alone and drag the existing set.
 *
 * The selection is private to the mapper widget and there is no Lua way to
 * make one, so this cannot be a busted spec.
 *
 * Run with: ctest -R MapStackedSelectionTest -V
 */

#include <QApplication>
#include <QFileInfo>
#include <QMouseEvent>
#include <QTemporaryDir>
#include <QtTest/QtTest>

#include "Host.h"
#include "HostManager.h"
#include "MudletInstanceCoordinator.h"
#include "T2DMap.h"
#include "TArea.h"
#include "TMap.h"
#include "TRoom.h"
#include "TRoomDB.h"
#include "mudlet.h"

#include "GroupedTest.h"

class MapStackedSelectionTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;
    Host* mpHost = nullptr;
    T2DMap* mp2dMap = nullptr;
    const QString mProfileName = qsl("MapStackedSelection-Test");
    const QString mAreaName = qsl("stack area");
    int mAreaId = 0;

    // Widget geometry and zoom chosen so one map unit is 20 px in both axes,
    // matching MapMouseInteractionTest: a room and its neighbour are 20 px
    // apart, and a room reaches 5 px either side of its centre.
    static constexpr int kWidgetWidth = 600;
    static constexpr int kWidgetHeight = 400;
    static constexpr double kZoom = 20.0;
    static constexpr double kPixelsPerMapUnit = 20.0;

    // Two rooms stacked on the same coordinate, and a third room one cell east
    // so the stack is unambiguously the only thing under the cursor at its
    // pixel.
    static constexpr int kStackRoomA = 11;
    static constexpr int kStackRoomB = 61;
    static constexpr int kNeighbourRoom = 12;
    static constexpr int kStackX = 0;
    static constexpr int kStackY = 0;

    static bool portableMarkerPresent()
    {
        return QFileInfo::exists(qsl("%1/portable.txt").arg(QCoreApplication::applicationDirPath()))
               || QFileInfo::exists(qsl("%1/.config/mudlet/portable.txt").arg(QDir::homePath()));
    }

    TMap* map() const { return mpHost->mpMap.data(); }
    TRoomDB* roomDB() const { return mpHost->mpMap->mpRoomDB.get(); }
    TArea* area() const { return roomDB()->getArea(mAreaId); }

    void deleteProfileDirectory() const
    {
        QDir dir(mudlet::getMudletPath(enums::profileHomePath, mProfileName));
        if (dir.exists()) {
            dir.removeRecursively();
        }
    }

    void buildStackedMap()
    {
        map()->mapClear();
        QVERIFY(roomDB()->getRoomIDList().isEmpty());

        mAreaId = roomDB()->addArea(mAreaName);
        QVERIFY(mAreaId > 0);

        QVERIFY(map()->addRoom(kStackRoomA));
        QVERIFY(map()->setRoomArea(kStackRoomA, mAreaId));
        QVERIFY(map()->setRoomCoordinates(kStackRoomA, kStackX, kStackY, 0));

        QVERIFY(map()->addRoom(kStackRoomB));
        QVERIFY(map()->setRoomArea(kStackRoomB, mAreaId));
        QVERIFY(map()->setRoomCoordinates(kStackRoomB, kStackX, kStackY, 0));

        QVERIFY(map()->addRoom(kNeighbourRoom));
        QVERIFY(map()->setRoomArea(kNeighbourRoom, mAreaId));
        QVERIFY(map()->setRoomCoordinates(kNeighbourRoom, kStackX + 1, kStackY, 0));

        mp2dMap->mAreaID = mAreaId;
        mp2dMap->mRoomID = kStackRoomA;
        mp2dMap->mMapViewOnly = false;
        mp2dMap->mShiftMode = true;
        mp2dMap->mPick = false;
        mp2dMap->mMapCenterX = kStackX;
        mp2dMap->mMapCenterY = kStackY;
        mp2dMap->mMapCenterZ = 0;
        mp2dMap->mMultiSelectionSet.clear();
        mp2dMap->mMultiSelection = false;
        area()->set2DMapZoom(kZoom);
        // Paint would compute these from the zoom, but paintEvent() reaches
        // mpMap->mpMapper which is null without the dialog - so set them by hand
        // to the same values paintEvent() would, and never paint.
        mp2dMap->mRoomWidth = kPixelsPerMapUnit;
        mp2dMap->mRoomHeight = kPixelsPerMapUnit;
        // xspan/yspan are the map units spanning the widget; for a 600x400 widget
        // at zoom 20 that is 30 wide x 20 tall (landscape), per paintEvent().
        mp2dMap->xspan = 30.0f;
        mp2dMap->yspan = 20.0f;
    }

    QPoint viewCentre() const { return QPoint(kWidgetWidth / 2, kWidgetHeight / 2); }

    // Pixel position of the stacked rooms' centre (they share it).
    QPoint stackPixel() const { return viewCentre(); }

    void sendMouse(const QEvent::Type type, const QPoint& position, const Qt::MouseButton button, const Qt::MouseButtons buttons) const
    {
        QMouseEvent event(type, QPointF(position), mp2dMap->mapToGlobal(QPointF(position)), button, buttons, Qt::NoModifier);
        QApplication::sendEvent(mp2dMap, &event);
    }

    void pressAt(const QPoint& position) const { sendMouse(QEvent::MouseButtonPress, position, Qt::LeftButton, Qt::LeftButton); }

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
        QCOMPARE(mudlet::getMudletPath(enums::mainPath), qsl("%1/mudlet").arg(mConfigDir.path()));
        mudlet::self()->takeOwnershipOfInstanceCoordinator(std::make_unique<MudletInstanceCoordinator>("MudletInstanceCoordinator"));
        mudlet::self()->init();
        mudlet::self()->setStorePasswordsSecurely(false);
        deleteProfileDirectory();

        auto& hostManager = mudlet::self()->getHostManager();
        QVERIFY2(hostManager.addHost(mProfileName, qsl("23"), QString(), QString()), "failed to create the Host");
        mpHost = hostManager.getHost(mProfileName);
        QVERIFY(mpHost);
        QVERIFY(map());

        // Construct the 2D map directly rather than through dlgMapper: the latter
        // needs a console, which only exists once the profile has connected.
        // paintEvent() reaches mpMap->mpMapper (e.g. comboBox_showArea) and would
        // dereference null without the dialog, so the tests below never paint -
        // they set the pixel geometry by hand instead.
        mp2dMap = new T2DMap();
        mp2dMap->mpMap = map();
        mp2dMap->mpHost = mpHost;
        // RoomMoveActivationHandler declines to act in view-only mode, and a
        // freshly-built T2DMap defaults to view-only - flip it off before init()
        // syncs the rest of the host's mapper settings.
        mpHost->mMapViewOnly = false;
        mp2dMap->init();
        mp2dMap->resize(kWidgetWidth, kWidgetHeight);
    }

    void cleanupTestCase()
    {
        delete mp2dMap;
        mp2dMap = nullptr;
        if (mudlet::self()) {
            deleteProfileDirectory();
        }
        mSavedXdg.isNull() ? qunsetenv("XDG_CONFIG_HOME") : qputenv("XDG_CONFIG_HOME", mSavedXdg);
    }

    void init()
    {
        // Each test starts from a clean selection state.
        if (mp2dMap) {
            mp2dMap->mMultiSelectionSet.clear();
            mp2dMap->mMultiSelection = false;
            mp2dMap->mRoomBeingMoved = false;
        }
    }

    // Sanity: a click on the stack selects both stacked rooms.
    void clickSelectsWholeStack()
    {
        buildStackedMap();
        pressAt(stackPixel());

        QCOMPARE(mp2dMap->mMultiSelectionSet, (QSet<int>{kStackRoomA, kStackRoomB}));
    }

    // The bug: after deselecting one room of the stack, a click to start a drag
    // must not bring it back - the deselected room stays deselected because its
    // partner is still selected.
    void dragDoesNotReselectDeselectedRoom()
    {
        buildStackedMap();
        // Pretend the user selected the stack then deselected room B in the
        // selection window, leaving only A selected.
        mp2dMap->mMultiSelectionSet = {kStackRoomA};

        pressAt(stackPixel());

        QVERIFY2(mp2dMap->mMultiSelectionSet == QSet<int>{kStackRoomA},
                 "a click on the stack re-selected the room the user had deselected");
        QVERIFY2(!mp2dMap->mMultiSelectionSet.contains(kStackRoomB),
                 "room B was pulled back into the selection by the drag-start click");
    }

    // Symmetric: deselect A, leave B - the click must keep only B.
    void dragDoesNotReselectEitherDeselectedRoom()
    {
        buildStackedMap();
        mp2dMap->mMultiSelectionSet = {kStackRoomB};

        pressAt(stackPixel());

        QCOMPARE(mp2dMap->mMultiSelectionSet, (QSet<int>{kStackRoomB}));
        QVERIFY(!mp2dMap->mMultiSelectionSet.contains(kStackRoomA));
    }

    // When none of the stacked rooms is selected, the click establishes a fresh
    // selection of them (the original "click selects the stack" behaviour).
    void dragSelectsStackWhenNoneSelected()
    {
        buildStackedMap();
        // Empty selection - a click on the stack should adopt both rooms.
        pressAt(stackPixel());

        QCOMPARE(mp2dMap->mMultiSelectionSet, (QSet<int>{kStackRoomA, kStackRoomB}));
    }
};

#include "MapStackedSelectionTest.moc"

MUDLET_GROUPED_TEST_MAIN(MapStackedSelectionTest)
