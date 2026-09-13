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
 * The mapper's context menus delete themselves when they close, but their
 * items used to be made with the map widget as their parent, so every menu
 * left its items behind on the widget for as long as the mapper lived. Each
 * case here puts up one of those menus, closes it, and checks that its items
 * went with it.
 *
 * The items are QObject children of the widget, which nothing in the Lua API
 * can count, so this cannot be a busted spec.
 *
 * Run with: ctest -R MapContextMenuLifetimeTest -V
 */

#include <QAction>
#include <QMenu>
#include <QMouseEvent>
#include <QPixmap>
#include <QPointer>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QtTest/QtTest>

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

class MapContextMenuLifetimeTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;
    TelnetServerStub* mpServer = nullptr;
    Host* mpHost = nullptr;
    T2DMap* mp2dMap = nullptr;
    int mAreaId = 0;
    const QString mProfileName = qsl("MapContextMenuLifetime-Test");
    const QString mLocalhost = qsl("localhost");
    QString mPort;

    static constexpr int kWidgetWidth = 600;
    static constexpr int kWidgetHeight = 400;
    // Map units across the shorter dimension, which puts one map unit at 20
    // pixels in both directions.
    static constexpr double kZoom = 20.0;
    static constexpr double kPixelsPerMapUnit = 20.0;
    static constexpr double kRoomSize = 0.5;

    // The 3x3 block buildMap() lays out, a unit apart, with the player in the
    // middle of it.
    static constexpr int kPlayerRoomId = 5;
    static constexpr int kEastRoomId = 6;
    inline static const QString kLineExit = qsl("n");

    void deleteProfileDirectory() const
    {
        QDir dir(mudlet::getMudletPath(enums::profileHomePath, mProfileName));
        if (dir.exists()) {
            dir.removeRecursively();
        }
    }

    TMap* map() const { return mpHost->mpMap.data(); }

    TArea* area() const { return map()->mpRoomDB->getArea(mAreaId); }

    bool addRoomAt(const int id, const int x, const int y) const { return map()->addRoom(id) && map()->setRoomArea(id, mAreaId) && map()->setRoomCoordinates(id, x, y, 0); }

    void buildMap()
    {
        TMap* pMap = map();
        pMap->mapClear();
        mAreaId = pMap->mpRoomDB->addArea(qsl("Menu Area"));
        QVERIFY(mAreaId > 0);

        int nextId = 1;
        for (int y = 1; y >= -1; --y) {
            for (int x = -1; x <= 1; ++x) {
                QVERIFY(addRoomAt(nextId++, x, y));
            }
        }
        QVERIFY(area());

        pMap->mRoomIdHash[pMap->mProfileName] = kPlayerRoomId;
        pMap->mNewMove = false;
        pMap->setDefaultAreaShown(false);
    }

    // Points the mapper at the map buildMap() made and draws it once, which is
    // what fills in the pixels-per-map-unit the hit test works from.
    void showMapper()
    {
        // The mapper opens view-only, which hides most of the room menu and
        // refuses to select a custom line.
        mpHost->mMapViewOnly = false;
        mpHost->mRoomSize = kRoomSize;
        if (!map()->mpMapper) {
            mpHost->showHideOrCreateMapper(false);
        }
        QVERIFY(map()->mpMapper);
        mp2dMap = map()->mpMapper->mp2dMap;
        QVERIFY(mp2dMap);
        mp2dMap->init();
        mp2dMap->resize(kWidgetWidth, kWidgetHeight);
        mp2dMap->mRoomID = kPlayerRoomId;
        mp2dMap->mAreaID = mAreaId;
        mp2dMap->mPick = false;
        mp2dMap->mMapCenterX = 0;
        mp2dMap->mMapCenterY = 0;
        mp2dMap->mMapCenterZ = 0;
        mp2dMap->mMultiSelectionSet.clear();
        area()->set2DMapZoom(kZoom);

        QPixmap target(kWidgetWidth, kWidgetHeight);
        target.fill(Qt::black);
        mp2dMap->render(&target, QPoint(), QRegion(), QWidget::DrawWindowBackground);
        QCOMPARE(static_cast<double>(mp2dMap->mRoomWidth), kPixelsPerMapUnit);
    }

    QPoint viewCentre() const { return QPoint(kWidgetWidth / 2, kWidgetHeight / 2); }

    QPoint pointUnitsFromCentre(const double east, const double north) const { return viewCentre() + QPoint(qRound(east * kPixelsPerMapUnit), qRound(-north * kPixelsPerMapUnit)); }

    void sendMouse(const QEvent::Type type, const QPoint& position, const Qt::MouseButton button, const Qt::MouseButtons buttons) const
    {
        QMouseEvent event(type, QPointF(position), mp2dMap->mapToGlobal(QPointF(position)), button, buttons, Qt::NoModifier);
        QApplication::sendEvent(mp2dMap, &event);
    }

    void clickAt(const QPoint& position) const
    {
        sendMouse(QEvent::MouseButtonPress, position, Qt::LeftButton, Qt::LeftButton);
        sendMouse(QEvent::MouseButtonRelease, position, Qt::LeftButton, Qt::NoButton);
    }

    void rightClickAt(const QPoint& position) const
    {
        sendMouse(QEvent::MouseButtonPress, position, Qt::RightButton, Qt::RightButton);
        sendMouse(QEvent::MouseButtonRelease, position, Qt::RightButton, Qt::NoButton);
    }

    static QAction* menuItem(const QMenu* menu, const QString& text)
    {
        for (QAction* pAction : menu->actions()) {
            if (pAction->text() == text) {
                return pAction;
            }
        }
        return nullptr;
    }

    // A menu that deletes itself on closing only queues the delete, so the
    // queue is run here to see what the mapper is really left holding.
    static void closeTheMenu(QMenu* menu)
    {
        menu->close();
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    }

    int actionsUnderTheMap() const { return mp2dMap->findChildren<QAction*>().size(); }

    // A custom line running north out of the east room, so its first segment
    // can be clicked at (1, 2), clear of every room.
    TRoom* addLineToTheEastRoom() const
    {
        TRoom* pRoom = map()->mpRoomDB->getRoom(kEastRoomId);
        if (!pRoom) {
            return nullptr;
        }
        pRoom->customLines[kLineExit] = {QPointF(1.0, 3.0), QPointF(1.0, 4.0)};
        pRoom->customLinesColor[kLineExit] = QColor(0, 255, 128);
        pRoom->customLinesStyle[kLineExit] = Qt::SolidLine;
        pRoom->customLinesArrow[kLineExit] = false;
        pRoom->calcRoomDimensions();
        return pRoom;
    }

    // What the custom line dialog leaves behind once an exit has been picked
    // for a new line: an empty line to click points into.
    TRoom* startDrawingALine() const
    {
        TRoom* pRoom = map()->mpRoomDB->getRoom(kEastRoomId);
        if (!pRoom) {
            return nullptr;
        }
        pRoom->customLines[kLineExit] = QList<QPointF>();
        pRoom->customLinesColor[kLineExit] = QColor(255, 0, 0);
        pRoom->customLinesStyle[kLineExit] = Qt::SolidLine;
        pRoom->customLinesArrow[kLineExit] = true;
        pRoom->indexCustomLines();
        mp2dMap->mCustomLinesRoomFrom = kEastRoomId;
        mp2dMap->mCustomLinesRoomExit = kLineExit;
        return pRoom;
    }

private slots:
    void initTestCase()
    {
        if (portableMarkerPresent()) {
            QSKIP("portable.txt present - it takes precedence over XDG_CONFIG_HOME, so the config dir cannot be redirected");
        }

        // A config root of this process's own, so this never reads or writes
        // the developer's ~/.config/mudlet. Since #9712 the opt-in that makes
        // setupConfig() adopt a directory is $XDG_CONFIG_HOME/mudlet/profiles,
        // not the mudlet directory alone.
        QVERIFY(mConfigDir.isValid());
        QVERIFY(QDir().mkpath(qsl("%1/mudlet/profiles").arg(mConfigDir.path())));
        mSavedXdg = qgetenv("XDG_CONFIG_HOME");
        qputenv("XDG_CONFIG_HOME", mConfigDir.path().toUtf8());

        mpServer = new TelnetServerStub(qApp);
        mpServer->start(mLocalhost, 0);
        mPort = QString::number(mpServer->serverPort());

        mudlet::start();
        mudlet::self()->setupConfig();
        QCOMPARE(mudlet::getMudletPath(enums::mainPath), qsl("%1/mudlet").arg(mConfigDir.path()));
        mudlet::self()->takeOwnershipOfInstanceCoordinator(std::make_unique<MudletInstanceCoordinator>("MudletInstanceCoordinator"));
        mudlet::self()->init();
        // The context menu items are found by their text.
        mudlet::self()->setInterfaceLanguage(qsl("en_US"));
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
        mp2dMap = nullptr;
        mpHost = nullptr;
        delete mpServer;
        mpServer = nullptr;
        if (mudlet::self()) {
            deleteProfileDirectory();
            delete mudlet::self();
        }
        mSavedXdg.isNull() ? qunsetenv("XDG_CONFIG_HOME") : qputenv("XDG_CONFIG_HOME", mSavedXdg);
    }

    // The mapper is made once and reused, so a line left selected or half
    // drawn would change which menu the next case's right click puts up.
    void cleanup()
    {
        if (!mp2dMap) {
            return;
        }
        if (mp2dMap->mActiveContextMenu) {
            closeTheMenu(mp2dMap->mActiveContextMenu);
        }
        mp2dMap->mCustomLinesRoomFrom = 0;
        mp2dMap->mCustomLinesRoomTo = 0;
        mp2dMap->mCustomLinesRoomExit.clear();
        mp2dMap->mCustomLineSelectedRoom = 0;
        mp2dMap->mCustomLineSelectedExit.clear();
        mp2dMap->mCustomLineSelectedPoint = -1;
    }

    void test_theRoomMenusItemsGoAwayWithTheMenu()
    {
        buildMap();
        showMapper();
        const int before = actionsUnderTheMap();

        rightClickAt(pointUnitsFromCentre(1, 0));
        QPointer<QMenu> menu = mp2dMap->mActiveContextMenu;
        QVERIFY2(menu, "the right click on a room put up no menu");
        QPointer<QAction> item = menuItem(menu, qsl("Set exits..."));
        QVERIFY2(item, "the menu is not the room menu");

        closeTheMenu(menu);
        QVERIFY2(menu.isNull(), "the menu did not delete itself on closing");
        QVERIFY2(item.isNull(), "the menu's item outlived the menu");
        QCOMPARE(actionsUnderTheMap(), before);
    }

    void test_theEmptySpaceMenusItemsGoAwayWithTheMenu()
    {
        buildMap();
        showMapper();
        const int before = actionsUnderTheMap();

        rightClickAt(pointUnitsFromCentre(4, 3));
        QPointer<QMenu> menu = mp2dMap->mActiveContextMenu;
        QVERIFY2(menu, "the right click on empty space put up no menu");
        QPointer<QAction> item = menuItem(menu, qsl("Create new room here"));
        QVERIFY2(item, "the menu is not the empty space menu");

        closeTheMenu(menu);
        QVERIFY2(menu.isNull(), "the menu did not delete itself on closing");
        QVERIFY2(item.isNull(), "the menu's item outlived the menu");
        QCOMPARE(actionsUnderTheMap(), before);
    }

    void test_theEmptyMapMenusItemsGoAwayWithTheMenu()
    {
        buildMap();
        showMapper();
        map()->mapClear();
        const int before = actionsUnderTheMap();

        rightClickAt(viewCentre());
        QPointer<QMenu> menu = mp2dMap->mActiveContextMenu;
        QVERIFY2(menu, "the right click on an empty map put up no menu");
        QPointer<QAction> item = menuItem(menu, qsl("Create new map"));
        QVERIFY2(item, "the menu is not the empty map menu");

        closeTheMenu(menu);
        QVERIFY2(menu.isNull(), "the menu did not delete itself on closing");
        QVERIFY2(item.isNull(), "the menu's item outlived the menu");
        QCOMPARE(actionsUnderTheMap(), before);
    }

    void test_theDrawingMenusItemsGoAwayWithTheMenu()
    {
        buildMap();
        showMapper();
        QVERIFY(startDrawingALine());
        const int before = actionsUnderTheMap();

        rightClickAt(pointUnitsFromCentre(2, 0.5));
        QPointer<QMenu> menu = mp2dMap->mActiveContextMenu;
        QVERIFY2(menu, "the right click while drawing a line put up no menu");
        QPointer<QAction> item = menuItem(menu, qsl("Finish"));
        QVERIFY2(item, "the menu is not the line drawing menu");

        closeTheMenu(menu);
        QVERIFY2(menu.isNull(), "the menu did not delete itself on closing");
        QVERIFY2(item.isNull(), "the menu's item outlived the menu");
        QCOMPARE(actionsUnderTheMap(), before);
    }

    void test_theSelectedLineMenusItemsGoAwayWithTheMenu()
    {
        buildMap();
        QVERIFY(addLineToTheEastRoom());
        showMapper();
        clickAt(pointUnitsFromCentre(1, 2));
        QCOMPARE(mp2dMap->mCustomLineSelectedRoom, kEastRoomId);
        const int before = actionsUnderTheMap();

        rightClickAt(pointUnitsFromCentre(1, 2));
        QPointer<QMenu> menu = mp2dMap->mActiveContextMenu;
        QVERIFY2(menu, "the right click on the selected line put up no menu");
        QPointer<QAction> item = menuItem(menu, qsl("Delete line"));
        QVERIFY2(item, "the menu is not the selected line menu");

        closeTheMenu(menu);
        QVERIFY2(menu.isNull(), "the menu did not delete itself on closing");
        QVERIFY2(item.isNull(), "the menu's item outlived the menu");
        QCOMPARE(actionsUnderTheMap(), before);
    }
};

#include "MapContextMenuLifetimeTest.moc"
MUDLET_GROUPED_TEST_MAIN(MapContextMenuLifetimeTest)
