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
 * Everything the mapper does with the mouse: which room a point on the widget
 * is over, and the handlers a press, a drag and a release are dispatched to.
 *
 * None of it has a Lua route - the hit test and the selection are private to
 * the widget, and no scripting call makes a selection or pans the view - so
 * this cannot be a busted spec.
 *
 * The point of a room is worked out from mRoomWidth, the pixels one map unit
 * is drawn at, which paintEvent() sets. So every test renders a frame first,
 * and mShiftMode keeps that paint from re-centring the view on the player.
 *
 * Run with: ctest -R MapMouseInteractionTest -V
 */

#include <QCursor>
#include <QFileInfo>
#include <QMouseEvent>
#include <QPixmap>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QtTest/QtTest>

#include "Host.h"
#include "MudletInstanceCoordinator.h"
#include "PortableModeTestHelper.h"
#include "ProfileTestHelper.h"
#include "MiddleMousePanHandler.h"
#include "T2DMap.h"
#include "TArea.h"
#include "TMap.h"
#include "TMapLabel.h"
#include "TRoom.h"
#include "TRoomDB.h"
#include "TelnetServerStub.h"
#include "ctelnet.h"
#include "dlgMapper.h"
#include "mudlet.h"

#include "GroupedTest.h"

class MapMouseInteractionTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;
    TelnetServerStub* mpServer = nullptr;
    Host* mpHost = nullptr;
    T2DMap* mp2dMap = nullptr;
    int mAreaId = 0;
    const QString mProfileName = qsl("MapMouseInteraction-Test");
    const QString mLocalhost = qsl("localhost");
    QString mPort;

    // Wider than it is tall, so the two axes cannot be confused for each other.
    static constexpr int kWidgetWidth = 600;
    static constexpr int kWidgetHeight = 400;
    // Map units across the shorter dimension. With this widget that puts one
    // map unit at 20 pixels in both directions, so a room and its neighbour are
    // 20 pixels apart.
    static constexpr double kZoom = 20.0;
    static constexpr double kPixelsPerMapUnit = 20.0;
    // A room is drawn at half the size of its cell, so it reaches 5 pixels
    // either side of its centre and the 10 pixels to its neighbour are empty.
    static constexpr double kRoomSize = 0.5;

    // The 3x3 block buildMap() lays out, a unit apart, with the player in the
    // middle of it.
    static constexpr int kNorthRoomId = 2;
    static constexpr int kWestRoomId = 4;
    static constexpr int kPlayerRoomId = 5;
    static constexpr int kEastRoomId = 6;
    static constexpr int kSouthRoomId = 8;

    void deleteProfileDirectory() const
    {
        QDir dir(mudlet::getMudletPath(enums::profileHomePath, mProfileName));
        if (dir.exists()) {
            dir.removeRecursively();
        }
    }

    TMap* map() const { return mpHost->mpMap.data(); }

    TArea* area() const { return map()->mpRoomDB->getArea(mAreaId); }

    bool addRoomAt(const int id, const int x, const int y, const int z) const { return map()->addRoom(id) && map()->setRoomArea(id, mAreaId) && map()->setRoomCoordinates(id, x, y, z); }

    // A 3x3 block of rooms one unit apart, numbered from the north-west corner
    // along each row, which puts the player in the middle at the origin.
    void buildMap(const bool gridMode = false)
    {
        TMap* pMap = map();
        pMap->mapClear();
        mAreaId = pMap->mpRoomDB->addArea(qsl("Mouse Area"));
        QVERIFY(mAreaId > 0);

        int nextId = 1;
        for (int y = 1; y >= -1; --y) {
            for (int x = -1; x <= 1; ++x) {
                QVERIFY(addRoomAt(nextId++, x, y, 0));
            }
        }
        QVERIFY(area());
        area()->gridMode = gridMode;

        pMap->mRoomIdHash[pMap->mProfileName] = kPlayerRoomId;
        pMap->mNewMove = false;
        pMap->setDefaultAreaShown(false);
    }

    // Points the mapper at the map buildMap() made and draws it once, which is
    // what fills in the pixels-per-map-unit the hit test works from.
    void showMapper(const bool viewOnly)
    {
        mpHost->mMapViewOnly = viewOnly;
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
        // paintEvent() re-centres on the player and draws somewhere else unless
        // the player's room, mRoomID and mShiftMode all agree.
        mp2dMap->mShiftMode = true;
        mp2dMap->mPick = false;
        mp2dMap->mMapCenterX = 0;
        mp2dMap->mMapCenterY = 0;
        mp2dMap->mMapCenterZ = 0;
        mp2dMap->mMultiSelectionSet.clear();
        // The mapper is made once and reused, so anything a test picked up has
        // to be put back down before the next one runs.
        mp2dMap->mLabelHighlighted = false;
        mp2dMap->mMoveLabel = false;
        mp2dMap->mMultiSelection = false;
        area()->set2DMapZoom(kZoom);
        renderFrame();
        QCOMPARE(mp2dMap->mMapViewOnly, viewOnly);
        QCOMPARE(static_cast<double>(mp2dMap->mRoomWidth), kPixelsPerMapUnit);
        QCOMPARE(static_cast<double>(mp2dMap->mRoomHeight), kPixelsPerMapUnit);
    }

    void renderFrame() const
    {
        QPixmap target(kWidgetWidth, kWidgetHeight);
        target.fill(Qt::black);
        mp2dMap->render(&target, QPoint(), QRegion(), QWidget::DrawWindowBackground);
    }

    QPoint viewCentre() const { return QPoint(kWidgetWidth / 2, kWidgetHeight / 2); }

    // Where a room that many units east and north of the middle of the view is
    // drawn, if the mapper puts north up and east right.
    QPoint pointUnitsFromCentre(const double east, const double north) const { return viewCentre() + QPoint(qRound(east * kPixelsPerMapUnit), qRound(-north * kPixelsPerMapUnit)); }

    // Labels are drawn from their map position down and to the right, so this
    // is the point that lands in the middle of one.
    QPoint pointOnLabel(const QVector3D& position, const QSizeF& clickSize) const
    {
        return QPoint(qRound(position.x() * kPixelsPerMapUnit + mp2dMap->mRX + clickSize.width() / 2), qRound(position.y() * -1 * kPixelsPerMapUnit + mp2dMap->mRY + clickSize.height() / 2));
    }

    int addLabel(const QVector3D& position, const QSizeF& clickSize) const
    {
        TMapLabel label;
        label.pos = position;
        label.size = clickSize;
        label.clickSize = clickSize;
        label.text = qsl("a label");
        const int id = area()->mMapLabels.isEmpty() ? 0 : area()->mMapLabels.lastKey() + 1;
        area()->mMapLabels.insert(id, label);
        return id;
    }

    void sendMouse(const QEvent::Type type, const QPoint& position, const Qt::MouseButton button, const Qt::MouseButtons buttons, const Qt::KeyboardModifiers modifiers) const
    {
        QMouseEvent event(type, QPointF(position), mp2dMap->mapToGlobal(QPointF(position)), button, buttons, modifiers);
        QApplication::sendEvent(mp2dMap, &event);
    }

    void pressAt(const QPoint& position, const Qt::KeyboardModifiers modifiers = Qt::NoModifier) const { sendMouse(QEvent::MouseButtonPress, position, Qt::LeftButton, Qt::LeftButton, modifiers); }

    void moveTo(const QPoint& position, const Qt::KeyboardModifiers modifiers = Qt::NoModifier) const { sendMouse(QEvent::MouseMove, position, Qt::NoButton, Qt::LeftButton, modifiers); }

    // A press does not itself start a drag: the handlers take the point the
    // drag runs from off the first move that follows it, so a drag is a press,
    // a move that does not go anywhere, and then the move that does.
    void dragFromTo(const QPoint& from, const QPoint& to, const Qt::KeyboardModifiers modifiers = Qt::NoModifier) const
    {
        pressAt(from, modifiers);
        moveTo(from, modifiers);
        moveTo(to, modifiers);
        releaseAt(to, modifiers);
    }

    void releaseAt(const QPoint& position, const Qt::KeyboardModifiers modifiers = Qt::NoModifier) const { sendMouse(QEvent::MouseButtonRelease, position, Qt::LeftButton, Qt::NoButton, modifiers); }

    void clickAt(const QPoint& position, const Qt::KeyboardModifiers modifiers = Qt::NoModifier) const
    {
        pressAt(position, modifiers);
        releaseAt(position, modifiers);
    }

    // The middle button pans on its own timer: the map keeps drifting away from
    // where the button went down for as long as the mouse is held off it.
    void middlePressAt(const QPoint& position) const { sendMouse(QEvent::MouseButtonPress, position, Qt::MiddleButton, Qt::MiddleButton, Qt::NoModifier); }

    void middleMoveTo(const QPoint& position) const { sendMouse(QEvent::MouseMove, position, Qt::NoButton, Qt::MiddleButton, Qt::NoModifier); }

    void middleReleaseAt(const QPoint& position) const { sendMouse(QEvent::MouseButtonRelease, position, Qt::MiddleButton, Qt::NoButton, Qt::NoModifier); }

    // The pan hides the pointer while it runs and puts it back when it stops,
    // which is the one thing about it a user can see without the map moving.
    bool panning() const { return mp2dMap->testAttribute(Qt::WA_SetCursor) && mp2dMap->cursor().shape() == Qt::BlankCursor; }

    // Long enough for the pan's 16ms timer to tick a good few times.
    void letThePanRun() const { QTest::qWait(150); }

    // How far east the view moves per tick of the pan's timer. The event loop
    // does not get round to the same number of ticks in every window, so the
    // distance over a window is only comparable between two of them per tick.
    qreal eastwardPanPerTick() const
    {
        QSignalSpy ticks(&mp2dMap->mMiddleMousePanHandler->mTimer, &QTimer::timeout);
        mp2dMap->mMapCenterX = 0.0;
        letThePanRun();
        return ticks.isEmpty() ? 0.0 : mp2dMap->mMapCenterX / ticks.count();
    }

    // A pan that is still running would follow the pointer parked east of the
    // middle, so a view that stays put is one whose pan has really stopped -
    // the pointer coming back on its own only says the cursor was reset.
    void verifyNothingPansAnyMore() const
    {
        QVERIFY2(!mp2dMap->mMiddleMousePanHandler->isActive(), "the pan is still running");
        mp2dMap->mMapCenterX = 0.0;
        const QPoint east = mp2dMap->mapToGlobal(viewCentre() + QPoint(60, 0));
        QCursor::setPos(east);
        // Where the pointer cannot be parked there is nothing for a leftover
        // pan to follow, and the handler's word above has to do.
        if (QCursor::pos() != east) {
            return;
        }
        letThePanRun();
        QVERIFY2(qFuzzyIsNull(mp2dMap->mMapCenterX), "the map is still panning");
    }

    // Longer than the hold that makes a release end the pan rather than leave it running.
    void holdTheButton() const { QTest::qWait(400); }

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

    // A case that fails partway through a middle-button pan leaves it running
    // on its timer, and it would carry on into the next case.
    void cleanup()
    {
        if (mp2dMap) {
            mp2dMap->mMiddleMousePanHandler->cancel();
        }
    }

    // The view is centred on where the player is, so the middle of the widget
    // is over the room they are standing in.
    void test_theRoomYouAreInIsUnderTheMiddleOfTheView()
    {
        buildMap();
        showMapper(true);

        QCOMPARE(mp2dMap->roomIdAtWidgetPosition(viewCentre(), area()), std::optional<int>(kPlayerRoomId));
    }

    // North is up and east is right: get either axis or its sign wrong and a
    // click lands on the room opposite the one under the cursor.
    void test_theRoomsAroundYouAreDrawnInTheDirectionsTheyLieIn()
    {
        buildMap();
        showMapper(true);

        QCOMPARE(mp2dMap->roomIdAtWidgetPosition(pointUnitsFromCentre(0, 1), area()), std::optional<int>(kNorthRoomId));
        QCOMPARE(mp2dMap->roomIdAtWidgetPosition(pointUnitsFromCentre(0, -1), area()), std::optional<int>(kSouthRoomId));
        QCOMPARE(mp2dMap->roomIdAtWidgetPosition(pointUnitsFromCentre(1, 0), area()), std::optional<int>(kEastRoomId));
        QCOMPARE(mp2dMap->roomIdAtWidgetPosition(pointUnitsFromCentre(-1, 0), area()), std::optional<int>(kWestRoomId));
    }

    // A room is drawn smaller than the cell it sits in, so there is a gap
    // between it and the next one that belongs to neither.
    void test_thePointBetweenTwoRoomsIsOverNeitherOfThem()
    {
        buildMap();
        showMapper(true);

        QVERIFY2(!mp2dMap->roomIdAtWidgetPosition(pointUnitsFromCentre(0.5, 0), area()).has_value(), "the gap between two rooms was reported as a room");
        QVERIFY2(!mp2dMap->roomIdAtWidgetPosition(pointUnitsFromCentre(0, 0.5), area()).has_value(), "the gap above a room was reported as a room");
        // A room is drawn half a unit across, so three tenths of a unit out
        // from its middle is already past its edge.
        QVERIFY2(!mp2dMap->roomIdAtWidgetPosition(pointUnitsFromCentre(0.3, 0), area()).has_value(), "a point past the edge of a room was reported as being on it");
    }

    // In grid mode a room fills its cell, so there is no gap to fall into.
    void test_inGridModeThereIsNoGapBetweenTwoRooms()
    {
        buildMap(true);
        showMapper(true);

        const auto roomId = mp2dMap->roomIdAtWidgetPosition(pointUnitsFromCentre(0.4, 0), area());
        QVERIFY2(roomId.has_value(), "no room was found in what is a filled cell in grid mode");
        QCOMPARE(roomId, std::optional<int>(kPlayerRoomId));
    }

    // Only the level being shown is clickable, or a room drawn faintly on the
    // level above would take clicks meant for the one under the cursor.
    void test_aRoomOnAnotherLevelIsNotUnderTheCursor()
    {
        buildMap();
        const int roomAboveId = 20;
        QVERIFY(addRoomAt(roomAboveId, 3, 0, 1));
        showMapper(true);

        const QPoint pointOverTheRoomAbove = pointUnitsFromCentre(3, 0);
        QVERIFY2(!mp2dMap->roomIdAtWidgetPosition(pointOverTheRoomAbove, area()).has_value(), "a room a level up was picked out of the level being shown");

        mp2dMap->mMapCenterZ = 1;
        QCOMPARE(mp2dMap->roomIdAtWidgetPosition(pointOverTheRoomAbove, area()), std::optional<int>(roomAboveId));
    }

    // Two rooms can share a coordinate, and the mapper offers a choice between
    // them rather than picking one - so the hit test has to report both.
    void test_everyRoomStackedOnTheSameSpotIsReported()
    {
        buildMap();
        const int stackedRoomId = 21;
        QVERIFY(addRoomAt(stackedRoomId, 0, 0, 0));
        showMapper(true);

        const QSet<int> expected{kPlayerRoomId, stackedRoomId};
        QCOMPARE(mp2dMap->roomIdsAtWidgetPosition(viewCentre(), area()), expected);
        QVERIFY2(expected.contains(mp2dMap->roomIdAtWidgetPosition(viewCentre(), area()).value_or(0)), "the single-room hit test named a room that is not on the spot");
    }

    // Viewing mode is for reading the map, so a plain drag moves the map under
    // the cursor instead of selecting anything.
    void test_draggingWhileViewingPansTheMap()
    {
        buildMap();
        showMapper(true);

        dragFromTo(viewCentre(), viewCentre() + QPoint(qRound(kPixelsPerMapUnit), 0));

        // Dragging the map a unit to the right brings the room a unit west of
        // the middle into it.
        QCOMPARE(mp2dMap->mMapCenterX, -1.0);
        QCOMPARE(mp2dMap->mMapCenterY, 0.0);
        renderFrame();
        QCOMPARE(mp2dMap->roomIdAtWidgetPosition(viewCentre(), area()), std::optional<int>(kWestRoomId));
    }

    // Editing mode gives the drag to the selection rectangle, so panning there
    // is what Alt is for - as the message the mapper puts up says.
    void test_holdingAltAndDraggingPansTheMapWhileEditing()
    {
        buildMap();
        showMapper(false);

        dragFromTo(viewCentre(), viewCentre() + QPoint(0, qRound(kPixelsPerMapUnit)), Qt::AltModifier);

        QCOMPARE(mp2dMap->mMapCenterY, -1.0);
        renderFrame();
        QCOMPARE(mp2dMap->roomIdAtWidgetPosition(viewCentre(), area()), std::optional<int>(kNorthRoomId));
    }

    void test_clickingARoomWhileEditingSelectsIt()
    {
        buildMap();
        showMapper(false);

        clickAt(pointUnitsFromCentre(1, 0));

        QCOMPARE(mp2dMap->mMultiSelectionSet, QSet<int>{kEastRoomId});
    }

    void test_draggingABoxOverSeveralRoomsSelectsThemAll()
    {
        buildMap();
        showMapper(false);

        // From above and left of the north-west room down to below and right of
        // the middle one, which takes in the four rooms of that quarter.
        dragFromTo(pointUnitsFromCentre(-1.5, 1.5), pointUnitsFromCentre(0.5, -0.5));

        const QSet<int> expected{1, kNorthRoomId, kWestRoomId, kPlayerRoomId};
        QCOMPARE(mp2dMap->mMultiSelectionSet, expected);
    }

    // The same drag while viewing pans instead of drawing a box, which is what
    // the map being locked for viewing means.
    void test_draggingWhileViewingDrawsNoSelectionBox()
    {
        buildMap();
        showMapper(true);

        dragFromTo(pointUnitsFromCentre(-1.5, 1.5), pointUnitsFromCentre(0.5, -0.5));

        QVERIFY2(mp2dMap->mMultiSelectionSet.isEmpty(), "a drag while the map was locked for viewing selected rooms");
        QVERIFY2(mp2dMap->mMapCenterX != 0.0, "the drag did not pan the map either");
    }

    // Without shift a new selection replaces the old one, with it the two add
    // up - which is the only way to select rooms that are not next to each other.
    void test_holdingShiftAddsToTheSelectionInsteadOfReplacingIt()
    {
        buildMap();
        showMapper(false);

        clickAt(pointUnitsFromCentre(-1, 0));
        QCOMPARE(mp2dMap->mMultiSelectionSet, QSet<int>{kWestRoomId});

        clickAt(pointUnitsFromCentre(1, 0), Qt::ShiftModifier);

        const QSet<int> expected{kWestRoomId, kEastRoomId};
        QCOMPARE(mp2dMap->mMultiSelectionSet, expected);
    }

    // Dragging a room moves it on the map, in whole units, and takes the points
    // of its custom lines along with it.
    void test_draggingARoomMovesItAndItsCustomLines()
    {
        buildMap();
        TRoom* pEastRoom = map()->mpRoomDB->getRoom(kEastRoomId);
        QVERIFY(pEastRoom);
        pEastRoom->customLines[qsl("n")] = QList<QPointF>{QPointF(1.0, 3.0), QPointF(1.0, 4.0)};
        pEastRoom->customLinesColor[qsl("n")] = QColor(0, 255, 128);
        pEastRoom->customLinesStyle[qsl("n")] = Qt::SolidLine;
        pEastRoom->customLinesArrow[qsl("n")] = false;
        pEastRoom->calcRoomDimensions();
        showMapper(false);

        // A custom line's first segment runs out of the middle of the room it
        // belongs to, so press just short of the middle: on it, the handler that
        // edits a line's points would take the press instead.
        dragFromTo(pointUnitsFromCentre(1, -0.2), pointUnitsFromCentre(1, 1.8));

        QCOMPARE(pEastRoom->x(), 1);
        QCOMPARE(pEastRoom->y(), 2);
        const QList<QPointF> movedLine = pEastRoom->customLines.value(qsl("n"));
        QCOMPARE(movedLine.size(), 2);
        QCOMPARE(movedLine.constFirst(), QPointF(1.0, 5.0));
        QCOMPARE(movedLine.constLast(), QPointF(1.0, 6.0));
    }

    // Clicking a map label picks it up, and clicking it again puts it down.
    void test_clickingALabelPicksItUpAndClickingItAgainPutsItDown()
    {
        buildMap();
        showMapper(false);
        const QVector3D where(2, 0, 0);
        const QSizeF clickSize(40, 20);
        const int labelId = addLabel(where, clickSize);

        clickAt(pointOnLabel(where, clickSize));
        QVERIFY2(mp2dMap->mLabelHighlighted, "clicking a label did not pick it up");
        QVERIFY(area()->mMapLabels.value(labelId).highlight);

        clickAt(pointOnLabel(where, clickSize));
        QVERIFY2(!mp2dMap->mLabelHighlighted, "clicking a picked up label did not put it down");
        QVERIFY(!area()->mMapLabels.value(labelId).highlight);
    }

    // Clicking anywhere else drops whatever label was held.
    void test_clickingAwayFromALabelPutsItDown()
    {
        buildMap();
        showMapper(false);
        const QVector3D where(2, 0, 0);
        const QSizeF clickSize(40, 20);
        addLabel(where, clickSize);

        clickAt(pointOnLabel(where, clickSize));
        QVERIFY(mp2dMap->mLabelHighlighted);

        clickAt(pointUnitsFromCentre(-3, -3));
        QVERIFY2(!mp2dMap->mLabelHighlighted, "a click away from the label left it picked up");
    }

    // A label on a level other than the one being shown is not there to click.
    void test_aLabelOnAnotherLevelCannotBePickedUp()
    {
        buildMap();
        showMapper(false);
        const QVector3D where(2, 0, 1);
        const QSizeF clickSize(40, 20);
        const int labelId = addLabel(where, clickSize);

        clickAt(pointOnLabel(QVector3D(where.x(), where.y(), 0), clickSize));

        QVERIFY2(!mp2dMap->mLabelHighlighted, "a label a level up was picked up");
        QVERIFY(!area()->mMapLabels.value(labelId).highlight);
    }

    // With the map locked for viewing a label stays where it is.
    void test_aLabelCannotBePickedUpWhileViewing()
    {
        buildMap();
        showMapper(true);
        const QVector3D where(2, 0, 0);
        const QSizeF clickSize(40, 20);
        const int labelId = addLabel(where, clickSize);

        clickAt(pointOnLabel(where, clickSize));

        QVERIFY2(!mp2dMap->mLabelHighlighted, "a label was picked up while the map was locked for viewing");
        QVERIFY(!area()->mMapLabels.value(labelId).highlight);
    }

    // A label that has been picked up follows the mouse until it is put down.
    void test_aPickedUpLabelFollowsTheMouse()
    {
        buildMap();
        showMapper(false);
        const QVector3D where(2, 0, 0);
        const QSizeF clickSize(40, 20);
        const int labelId = addLabel(where, clickSize);

        pressAt(pointOnLabel(where, clickSize));
        QVERIFY(mp2dMap->mLabelHighlighted);
        moveTo(pointUnitsFromCentre(3, 1));

        const QVector3D moved = area()->mMapLabels.value(labelId).pos;
        QCOMPARE(static_cast<double>(moved.x()), 3.0);
        QCOMPARE(static_cast<double>(moved.y()), 1.0);
    }

    // A room the map is locked for viewing cannot be dragged out of place by a
    // stray click.
    void test_aRoomCannotBeDraggedWhileViewing()
    {
        buildMap();
        showMapper(true);

        dragFromTo(pointUnitsFromCentre(1, 0), pointUnitsFromCentre(1, 2));

        TRoom* pEastRoom = map()->mpRoomDB->getRoom(kEastRoomId);
        QVERIFY(pEastRoom);
        QCOMPARE(pEastRoom->x(), 1);
        QCOMPARE(pEastRoom->y(), 0);
    }

    // ---------------------------------------------------------------------
    // The middle button pans the map on a timer, away from where it went down.
    // ---------------------------------------------------------------------

    void test_holdingTheMiddleButtonOffToOneSidePansTheMapThatWay()
    {
        buildMap();
        showMapper(true);

        middlePressAt(viewCentre());
        QVERIFY2(panning(), "the pointer should be hidden while the pan runs");
        middleMoveTo(viewCentre() + QPoint(60, 0));
        letThePanRun();

        QVERIFY2(mp2dMap->mMapCenterX > 0.0, "holding the mouse east of the press should carry the view east");
        QCOMPARE(mp2dMap->mMapCenterY, 0.0);

        const qreal afterOneWindow = mp2dMap->mMapCenterX;
        letThePanRun();
        QVERIFY2(mp2dMap->mMapCenterX > afterOneWindow, "the view should keep moving for as long as the mouse is held off the spot");

        holdTheButton();
        middleReleaseAt(viewCentre() + QPoint(60, 0));
        QVERIFY2(!panning(), "releasing after a hold should end the pan and put the pointer back");
        verifyNothingPansAnyMore();
    }

    void test_theMiddleButtonPansVerticallyToo()
    {
        buildMap();
        showMapper(true);

        middlePressAt(viewCentre());
        middleMoveTo(viewCentre() + QPoint(0, 60));
        letThePanRun();

        QCOMPARE(mp2dMap->mMapCenterX, 0.0);
        // The Alt-drag case above pins down that a negative centre Y is the view
        // over the north room, so south of the press is a positive one.
        QVERIFY2(mp2dMap->mMapCenterY > 0.0, "holding the mouse south of the press should carry the view south");

        holdTheButton();
        middleReleaseAt(viewCentre() + QPoint(0, 60));
    }

    // A hand is never perfectly still, so the pan does not start until the
    // mouse is clearly off the spot it went down on.
    void test_theMiddleButtonDoesNotPanForAWobble()
    {
        buildMap();
        showMapper(true);

        middlePressAt(viewCentre());
        QVERIFY2(panning(), "the pan should be armed from the press, even before the mouse moves");
        middleMoveTo(viewCentre() + QPoint(5, 5));
        letThePanRun();

        QCOMPARE(mp2dMap->mMapCenterX, 0.0);
        QCOMPARE(mp2dMap->mMapCenterY, 0.0);

        holdTheButton();
        middleReleaseAt(viewCentre() + QPoint(5, 5));
    }

    void test_theFurtherTheMouseIsHeldTheFasterTheMapPans()
    {
        buildMap();
        showMapper(true);

        middlePressAt(viewCentre());
        middleMoveTo(viewCentre() + QPoint(30, 0));
        const qreal nearby = eastwardPanPerTick();

        middleMoveTo(viewCentre() + QPoint(150, 0));
        const qreal farAway = eastwardPanPerTick();

        QVERIFY(nearby > 0.0);
        QVERIFY2(farAway > nearby * 2, qPrintable(qsl("30px off moved the view %1 a tick, 150px off moved it %2").arg(nearby).arg(farAway)));

        holdTheButton();
        middleReleaseAt(viewCentre() + QPoint(150, 0));
    }

    // The speed tops out well inside the widget, so flinging the mouse to the
    // far edge does not send the map flying.
    void test_thePanStopsGettingFasterOnceTheMouseIsFarEnoughAway()
    {
        buildMap();
        showMapper(true);

        middlePressAt(viewCentre());
        middleMoveTo(viewCentre() + QPoint(500, 0));
        const qreal atFiveHundred = eastwardPanPerTick();

        middleMoveTo(viewCentre() + QPoint(1500, 0));
        const qreal atFifteenHundred = eastwardPanPerTick();

        QVERIFY(atFiveHundred > 0.0);
        QCOMPARE(atFifteenHundred, atFiveHundred);

        holdTheButton();
        middleReleaseAt(viewCentre() + QPoint(1500, 0));
    }

    // A click, as opposed to a hold, leaves the pan running hands-free until
    // another button press stops it.
    void test_aQuickMiddleClickLeavesThePanRunningUntilAnyButtonIsPressed()
    {
        buildMap();
        showMapper(true);

        middlePressAt(viewCentre());
        middleReleaseAt(viewCentre());
        QVERIFY2(panning(), "a quick click should leave the pan running");

        // With no button down there are no move events to follow, so the pan
        // reads the pointer's position for itself.
        const QPoint east = mp2dMap->mapToGlobal(viewCentre() + QPoint(60, 0));
        QCursor::setPos(east);
        if (QCursor::pos() != east) {
            QSKIP("this platform does not let a test move the pointer, so there is nothing for a hands-free pan to follow");
        }
        letThePanRun();
        QVERIFY2(mp2dMap->mMapCenterX > 0.0, "a hands-free pan should follow the pointer east of where the click was");

        clickAt(viewCentre());
        verifyNothingPansAnyMore();
    }

    void test_aSecondMiddlePressEndsAHandsFreePan()
    {
        buildMap();
        showMapper(true);

        middlePressAt(viewCentre());
        middleReleaseAt(viewCentre());
        QVERIFY(panning());

        middlePressAt(viewCentre());
        QVERIFY2(!panning(), "the middle button should stop the pan it started");
        middleReleaseAt(viewCentre());
        QVERIFY2(!panning(), "letting go of the button that stopped the pan should not start it again");
        verifyNothingPansAnyMore();
    }

    // The press that ends a hands-free pan is not swallowed: it still does
    // whatever it would have done on a map that was standing still.
    void test_aDragThatEndsAHandsFreePanStillPansByTheDrag()
    {
        buildMap();
        showMapper(true);

        middlePressAt(viewCentre());
        middleReleaseAt(viewCentre());
        QVERIFY(panning());

        dragFromTo(viewCentre(), viewCentre() + QPoint(qRound(kPixelsPerMapUnit), 0));
        QCOMPARE(mp2dMap->mMapCenterX, -1.0);
        verifyNothingPansAnyMore();
    }

    // Clicking a room while editing selects it, and that press has to end a
    // hands-free pan like any other rather than being used up by the selection.
    void test_clickingARoomWhileEditingEndsAHandsFreePan()
    {
        buildMap();
        showMapper(false);

        middlePressAt(viewCentre());
        middleReleaseAt(viewCentre());
        QVERIFY(panning());

        clickAt(pointUnitsFromCentre(1, 0));
        QVERIFY2(mp2dMap->mMultiSelectionSet.contains(kEastRoomId), "the click that ends the pan should still select the room");
        verifyNothingPansAnyMore();
    }

    // Editing does not need the middle button for anything else, so it pans
    // there too.
    void test_theMiddleButtonPansWhileEditingToo()
    {
        buildMap();
        showMapper(false);

        middlePressAt(viewCentre());
        middleMoveTo(viewCentre() + QPoint(60, 0));
        letThePanRun();

        QVERIFY(mp2dMap->mMapCenterX > 0.0);

        holdTheButton();
        middleReleaseAt(viewCentre() + QPoint(60, 0));
        QVERIFY(!panning());
    }
};

#include "MapMouseInteractionTest.moc"
MUDLET_GROUPED_TEST_MAIN(MapMouseInteractionTest)
