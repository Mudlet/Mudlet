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
 * is over, and the handlers a press, a drag, a release, a wheel notch and a
 * double-click are dispatched to.
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

#include <QAction>
#include <QCursor>
#include <QFileInfo>
#include <QInputDialog>
#include <QLineEdit>
#include <QMenu>
#include <QMouseEvent>
#include <QPixmap>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTimer>
#include <QTreeWidgetItem>
#include <QWheelEvent>
#include <QtTest/QtTest>

#include <cmath>
#include <functional>

#include "Host.h"
#include "MudletInstanceCoordinator.h"
#include "PortableModeTestHelper.h"
#include "ProfileTestHelper.h"
#include "MiddleMousePanHandler.h"
#include "T2DMap.h"
#include "TArea.h"
#include "TLuaInterpreter.h"
#include "TMap.h"
#include "TMapLabel.h"
#include "TRoom.h"
#include "TRoomDB.h"
#include "TelnetServerStub.h"
#include "ctelnet.h"
#include "dlgMapLabel.h"
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
    QTimer* mpModalAnswerTimer = nullptr;
    bool mModalDialogAnswered = false;
    int mModalAnswerAttemptsLeft = 0;
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
    static constexpr int kNorthWestRoomId = 1;
    static constexpr int kNorthEastRoomId = 3;
    static constexpr int kWestRoomId = 4;
    static constexpr int kPlayerRoomId = 5;
    static constexpr int kEastRoomId = 6;

    static inline const QVector3D kLabelPosition{2, 0, 0};
    static inline const QSizeF kLabelClickSize{40, 20};
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
        mp2dMap->mSizeLabel = false;
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

    // A label clear of the rooms on the level being shown.
    int addTestLabel() const { return addLabel(kLabelPosition, kLabelClickSize); }
    QPoint pointOnTestLabel() const { return pointOnLabel(kLabelPosition, kLabelClickSize); }

    // Picks Create label from the menu and drags out a box three units wide
    // and two high with its top left corner at (-5, 5).
    void dragOutALabelBox() const
    {
        rightClickAt(pointUnitsFromCentre(-2, -2));
        QVERIFY(pickContextMenuItem(qsl("Create label...")));
        QVERIFY(mp2dMap->mSizeLabel);
        dragFromTo(pointUnitsFromCentre(-2, 5), pointUnitsFromCentre(-5, 3));
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

    // A custom line north out of the east room, to the two points the tests
    // click on and drag. Its first segment runs from the room itself up
    // through the north-east room to the first point, and the second between
    // the two points, clear of every room.
    static constexpr int kLineRoomId = kEastRoomId;
    inline static const QString kLineExit = qsl("n");
    static QList<QPointF> lineAsDrawn() { return {QPointF(1.0, 3.0), QPointF(1.0, 4.0)}; }

    TRoom* addLineToTheEastRoom() const
    {
        TRoom* pRoom = map()->mpRoomDB->getRoom(kLineRoomId);
        if (!pRoom) {
            return nullptr;
        }
        pRoom->customLines[kLineExit] = lineAsDrawn();
        pRoom->customLinesColor[kLineExit] = QColor(0, 255, 128);
        pRoom->customLinesStyle[kLineExit] = Qt::SolidLine;
        pRoom->customLinesArrow[kLineExit] = false;
        pRoom->calcRoomDimensions();
        return pRoom;
    }

    QList<QPointF> linePoints() const { return map()->mpRoomDB->getRoom(kLineRoomId)->customLines.value(kLineExit); }

    // What the custom line dialog leaves behind once an exit has been picked
    // for a new line: an empty line to click points into.
    TRoom* startDrawingALine(const int fromRoomId, const QString& exit) const
    {
        TRoom* pRoom = map()->mpRoomDB->getRoom(fromRoomId);
        if (!pRoom) {
            return nullptr;
        }
        pRoom->customLines[exit] = QList<QPointF>();
        pRoom->customLinesColor[exit] = QColor(255, 0, 0);
        pRoom->customLinesStyle[exit] = Qt::SolidLine;
        pRoom->customLinesArrow[exit] = true;
        pRoom->indexCustomLines();
        mp2dMap->mCustomLinesRoomFrom = fromRoomId;
        mp2dMap->mCustomLinesRoomExit = exit;
        return pRoom;
    }

    void rightClickAt(const QPoint& position) const
    {
        sendMouse(QEvent::MouseButtonPress, position, Qt::RightButton, Qt::RightButton, Qt::NoModifier);
        sendMouse(QEvent::MouseButtonRelease, position, Qt::RightButton, Qt::NoButton, Qt::NoModifier);
    }

    // The context menu item with this text, from the menu the last right click
    // put up - or nothing, if there is no menu or no such item.
    QAction* contextMenuItem(const QString& text) const
    {
        if (!mp2dMap->mActiveContextMenu) {
            return nullptr;
        }
        for (QAction* pAction : mp2dMap->mActiveContextMenu->actions()) {
            if (pAction->text() == text) {
                return pAction;
            }
        }
        return nullptr;
    }

    void closeContextMenu() const
    {
        if (mp2dMap->mActiveContextMenu) {
            mp2dMap->mActiveContextMenu->close();
        }
    }

    // Picks the item the way a user does: the action fires and the menu goes
    // away. While a menu is up the map closes it over any click and re-posts
    // the click, so a case that clicks again after triggering an action on
    // its own would never see that click land.
    bool pickContextMenuItem(const QString& text) const
    {
        QAction* pAction = contextMenuItem(text);
        if (!pAction || !pAction->isEnabled()) {
            return false;
        }
        pAction->trigger();
        closeContextMenu();
        return true;
    }

    // Spread, Shrink and Move to position block in a modal dialog until it is
    // answered, so the answer has to come from a timer armed before the item
    // is picked. The timer keeps trying until answer() reports that it was
    // given the dialog it wanted. A dialog it keeps turning down is closed
    // instead, as is anything the item puts up after that, so the case fails
    // rather than sitting in a dialog's event loop until ctest gives up.
    void answerNextModalDialog(const std::function<bool(QWidget*)>& answer)
    {
        stopAnsweringModalDialogs();
        mModalDialogAnswered = false;
        mModalAnswerAttemptsLeft = 100;
        mpModalAnswerTimer = new QTimer(this);
        mpModalAnswerTimer->setInterval(20);
        connect(mpModalAnswerTimer, &QTimer::timeout, this, [this, answer]() {
            QWidget* pDialog = QApplication::activeModalWidget();
            if (!pDialog) {
                return;
            }
            if (mModalAnswerAttemptsLeft <= 0) {
                pDialog->close();
                return;
            }
            if (answer(pDialog)) {
                mModalDialogAnswered = true;
                stopAnsweringModalDialogs();
                return;
            }
            --mModalAnswerAttemptsLeft;
        });
        mpModalAnswerTimer->start();
    }

    void stopAnsweringModalDialogs()
    {
        if (!mpModalAnswerTimer) {
            return;
        }
        mpModalAnswerTimer->stop();
        mpModalAnswerTimer->deleteLater();
        mpModalAnswerTimer = nullptr;
    }

    // Answers the factor dialog that Spread and Shrink put up; a factor of 0
    // cancels it instead.
    bool pickFactorItem(const QString& text, const int factor)
    {
        answerNextModalDialog([factor](QWidget* pDialog) {
            auto* pInput = qobject_cast<QInputDialog*>(pDialog);
            if (!pInput) {
                return false;
            }
            if (factor) {
                pInput->setIntValue(factor);
                pInput->accept();
            } else {
                pInput->reject();
            }
            return true;
        });
        return pickContextMenuItem(text) && mModalDialogAnswered;
    }

    // Answers the coordinates dialog Move to position puts up. Its three
    // fields are the only line edits on it and are made in x, y, z order.
    bool pickMoveToPosition(const int x, const int y, const int z)
    {
        answerNextModalDialog([x, y, z](QWidget* pDialog) {
            auto* pMoveDialog = qobject_cast<QDialog*>(pDialog);
            const QList<QLineEdit*> fields = pDialog->findChildren<QLineEdit*>();
            if (!pMoveDialog || fields.size() != 3) {
                return false;
            }
            fields[0]->setText(QString::number(x));
            fields[1]->setText(QString::number(y));
            fields[2]->setText(QString::number(z));
            pMoveDialog->accept();
            return true;
        });
        return pickContextMenuItem(qsl("Move to position...")) && mModalDialogAnswered;
    }

    // The dialog Move to area puts up; it is not modal, so the case drives it
    // directly.
    QDialog* moveToAreaDialog() const { return mp2dMap->arealist_combobox ? qobject_cast<QDialog*>(mp2dMap->arealist_combobox->window()) : nullptr; }

    QSet<int> roomsInArea(const int areaId) const
    {
        const TArea* pArea = map()->mpRoomDB->getArea(areaId);
        return pArea ? pArea->rooms : QSet<int>{};
    }

    QVector3D roomPosition(const int roomId) const
    {
        const TRoom* pRoom = map()->mpRoomDB->getRoom(roomId);
        return pRoom ? QVector3D(pRoom->x(), pRoom->y(), pRoom->z()) : QVector3D(-999, -999, -999);
    }

    // An item on one of the menus a script added to the map's context menu.
    QAction* userMenuItem(const QString& menuText, const QString& itemText) const
    {
        QAction* pMenu = contextMenuItem(menuText);
        if (!pMenu || !pMenu->menu()) {
            return nullptr;
        }
        for (QAction* pAction : pMenu->menu()->actions()) {
            if (pAction->text() == itemText) {
                return pAction;
            }
        }
        return nullptr;
    }

    bool runLua(const QString& code) const { return mpHost->getLuaInterpreter()->compileAndExecuteScript(code); }

    // The pointer moving over the map with no button down, which is all the
    // map sees of a move started from the context menu.
    void hoverTo(const QPoint& position) const { sendMouse(QEvent::MouseMove, position, Qt::NoButton, Qt::NoButton, Qt::NoModifier); }

    void doubleClickAt(const QPoint& position) const { sendMouse(QEvent::MouseButtonDblClick, position, Qt::LeftButton, Qt::LeftButton, Qt::NoModifier); }

    // Rolls the wheel that many notches forward (away from the user), or back
    // for a negative count.
    void rollWheelAt(const QPoint& position, const int notches, const Qt::KeyboardModifiers modifiers = Qt::NoModifier) const
    {
        QWheelEvent event(QPointF(position), mp2dMap->mapToGlobal(QPointF(position)), QPoint(), QPoint(0, notches * 120), Qt::NoButton, modifiers, Qt::NoScrollPhase, false);
        QApplication::sendEvent(mp2dMap, &event);
    }

    // How many pixels across a map unit is drawn at, for a zoom of that many
    // map units across the shorter side of the view. The map keeps the size
    // as a float, so it is compared at that precision.
    float pixelsPerMapUnitAtZoom(const double zoom) const { return static_cast<float>(kPixelsPerMapUnit * kZoom / zoom); }

    // The wheel stores the zoom on the area, where the next frame reads it from.
    double zoom() const { return area()->get2DMapZoom(); }

    QString consoleText() const { return mpHost->mpConsole->buffer.lineBuffer.join(QChar::LineFeed); }

    QList<QTreeWidgetItem*> listedRooms() const
    {
        QList<QTreeWidgetItem*> items;
        for (int row = 0; row < mp2dMap->mMultiSelectionListWidget.topLevelItemCount(); ++row) {
            items.append(mp2dMap->mMultiSelectionListWidget.topLevelItem(row));
        }
        return items;
    }

    QSet<int> listedRoomIds() const
    {
        QSet<int> ids;
        for (const QTreeWidgetItem* item : listedRooms()) {
            ids.insert(item->text(0).toInt());
        }
        return ids;
    }

    QString listedRoomName(const int roomId) const
    {
        for (const QTreeWidgetItem* item : listedRooms()) {
            if (item->text(0).toInt() == roomId) {
                return item->text(1);
            }
        }
        return QString();
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

    // A case that fails partway through a middle-button pan leaves it running
    // on its timer, and it would carry on into the next case.
    void cleanup()
    {
        stopAnsweringModalDialogs();
        if (!mp2dMap) {
            return;
        }
        mp2dMap->mMiddleMousePanHandler->cancel();
        if (QDialog* pMoveToArea = moveToAreaDialog()) {
            pMoveToArea->close();
            mp2dMap->arealist_combobox.clear();
        }
        closeContextMenu();
        // A click sent while a menu was up is re-posted by the map, and would
        // land in whichever later case first spins the event loop.
        QCoreApplication::removePostedEvents(mp2dMap, QEvent::MouseButtonPress);
        QCoreApplication::removePostedEvents(mp2dMap, QEvent::MouseButtonRelease);
        mp2dMap->slot_setSnapCustomLinePointsToGrid(false);
        map()->mUserMenus.clear();
        map()->mUserActions.clear();
        map()->setMmpMapLocation(QString());
        mudlet::self()->setInvertMapZoom(false);
        mp2dMap->mRoomBeingMoved = false;
        mp2dMap->mRoomMoveViaContextMenu = false;
        mp2dMap->mNewMoveAction = false;
        mp2dMap->mHasRoomMoveLastMapPoint = false;
        mp2dMap->mHelpMsg.clear();
        mp2dMap->setMouseTracking(false);
        mp2dMap->mPick = false;
        mp2dMap->mStartSpeedWalk = false;
        // A label dialog left open by one case would still be up for the next,
        // which may be checking that none came up.
        if (mp2dMap->mpDlgMapLabel) {
            mp2dMap->mpDlgMapLabel->close();
        }
        runLua(qsl("mudlet.custom_speedwalk = nil\ndoSpeedWalk = nil"));
        mpHost->mpConsole->discardAll();
        mp2dMap->mCustomLinesRoomFrom = 0;
        mp2dMap->mCustomLinesRoomTo = 0;
        mp2dMap->mCustomLinesRoomExit.clear();
        mp2dMap->mCustomLineSelectedRoom = 0;
        mp2dMap->mCustomLineSelectedExit.clear();
        mp2dMap->mCustomLineSelectedPoint = -1;
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
        const int labelId = addTestLabel();

        clickAt(pointOnTestLabel());
        QVERIFY2(mp2dMap->mLabelHighlighted, "clicking a label did not pick it up");
        QVERIFY(area()->mMapLabels.value(labelId).highlight);

        clickAt(pointOnTestLabel());
        QVERIFY2(!mp2dMap->mLabelHighlighted, "clicking a picked up label did not put it down");
        QVERIFY(!area()->mMapLabels.value(labelId).highlight);
    }

    // Clicking anywhere else drops whatever label was held.
    void test_clickingAwayFromALabelPutsItDown()
    {
        buildMap();
        showMapper(false);
        addTestLabel();

        clickAt(pointOnTestLabel());
        QVERIFY(mp2dMap->mLabelHighlighted);

        clickAt(pointUnitsFromCentre(-3, -3));
        QVERIFY2(!mp2dMap->mLabelHighlighted, "a click away from the label left it picked up");
    }

    // A label on a level other than the one being shown is not there to click.
    void test_aLabelOnAnotherLevelCannotBePickedUp()
    {
        buildMap();
        showMapper(false);
        const int labelId = addLabel(QVector3D(kLabelPosition.x(), kLabelPosition.y(), 1), kLabelClickSize);

        clickAt(pointOnTestLabel());

        QVERIFY2(!mp2dMap->mLabelHighlighted, "a label a level up was picked up");
        QVERIFY(!area()->mMapLabels.value(labelId).highlight);
    }

    // With the map locked for viewing a label stays where it is.
    void test_aLabelCannotBePickedUpWhileViewing()
    {
        buildMap();
        showMapper(true);
        const int labelId = addTestLabel();

        clickAt(pointOnTestLabel());

        QVERIFY2(!mp2dMap->mLabelHighlighted, "a label was picked up while the map was locked for viewing");
        QVERIFY(!area()->mMapLabels.value(labelId).highlight);
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


    // ---------------------------------------------------------------------
    // Custom exit lines: clicking points into a new one, and picking up and
    // reshaping one that is already drawn.
    // ---------------------------------------------------------------------

    void test_clickingWhileDrawingACustomLineAddsAPointThere()
    {
        buildMap();
        showMapper(false);
        TRoom* pRoom = startDrawingALine(kPlayerRoomId, qsl("e"));
        QVERIFY(pRoom);

        clickAt(pointUnitsFromCentre(0.5, 0.5));
        clickAt(pointUnitsFromCentre(1.5, 0.5));

        const QList<QPointF> expected{QPointF(0.5, 0.5), QPointF(1.5, 0.5)};
        QCOMPARE(pRoom->customLines.value(qsl("e")), expected);
    }

    // The line is being drawn, so a click that lands on a room is a point on
    // the line and not a room being picked.
    void test_aClickOnARoomWhileDrawingAddsAPointRatherThanSelectingIt()
    {
        buildMap();
        showMapper(false);
        TRoom* pRoom = startDrawingALine(kPlayerRoomId, qsl("e"));
        QVERIFY(pRoom);

        clickAt(pointUnitsFromCentre(1, 0));

        QCOMPARE(pRoom->customLines.value(qsl("e")), QList<QPointF>{QPointF(1.0, 0.0)});
        QVERIFY2(mp2dMap->mMultiSelectionSet.isEmpty(), "the room under the click was selected instead of taking a point");
    }

    void test_pointsLandOnTheHalfGridWhileSnappingIsOn()
    {
        buildMap();
        showMapper(false);
        TRoom* pRoom = startDrawingALine(kPlayerRoomId, qsl("e"));
        QVERIFY(pRoom);
        mp2dMap->slot_setSnapCustomLinePointsToGrid(true);

        // 7 pixels east and 3 north is 0.35 by 0.15 of a unit: nearer the half
        // line on one axis and the whole one on the other.
        clickAt(viewCentre() + QPoint(7, -3));

        QCOMPARE(pRoom->customLines.value(qsl("e")), QList<QPointF>{QPointF(0.5, 0.0)});
    }

    void test_aRightClickWhileDrawingOffersToUndoTheLastPoint()
    {
        buildMap();
        showMapper(false);
        TRoom* pRoom = startDrawingALine(kPlayerRoomId, qsl("e"));
        QVERIFY(pRoom);
        clickAt(pointUnitsFromCentre(0.5, 0.5));
        clickAt(pointUnitsFromCentre(1.5, 0.5));

        rightClickAt(pointUnitsFromCentre(2, 0.5));
        QVERIFY2(pickContextMenuItem(qsl("Undo")), "a right click while drawing did not put up a menu with Undo on it");

        QCOMPARE(pRoom->customLines.value(qsl("e")), QList<QPointF>{QPointF(0.5, 0.5)});
        QVERIFY2(mp2dMap->mCustomLinesRoomFrom == kPlayerRoomId, "undoing a point ended the drawing");
    }

    void test_finishingFromTheMenuEndsTheDrawingAndKeepsTheLine()
    {
        buildMap();
        showMapper(false);
        TRoom* pRoom = startDrawingALine(kPlayerRoomId, qsl("e"));
        QVERIFY(pRoom);
        clickAt(pointUnitsFromCentre(0.5, 0.5));

        rightClickAt(pointUnitsFromCentre(2, 0.5));
        QVERIFY2(pickContextMenuItem(qsl("Finish")), "a right click while drawing did not put up a menu with Finish on it");

        QCOMPARE(mp2dMap->mCustomLinesRoomFrom, 0);
        QCOMPARE(pRoom->customLines.value(qsl("e")), QList<QPointF>{QPointF(0.5, 0.5)});
        // and a click is a click again, not another point
        clickAt(pointUnitsFromCentre(1.5, 0.5));
        QCOMPARE(pRoom->customLines.value(qsl("e")).size(), 1);
    }

    // Turning snapping on pulls the points already drawn onto the grid, and
    // turning it off again puts them back where they were clicked.
    void test_snappingFromTheMenuMovesThePointsAlreadyDrawnAndUnsnappingRestoresThem()
    {
        buildMap();
        showMapper(false);
        TRoom* pRoom = startDrawingALine(kPlayerRoomId, qsl("e"));
        QVERIFY(pRoom);
        clickAt(viewCentre() + QPoint(7, -3));
        const QList<QPointF> asClicked = pRoom->customLines.value(qsl("e"));
        QCOMPARE(asClicked.size(), 1);
        QVERIFY2((asClicked.first() - QPointF(0.35, 0.15)).manhattanLength() < 0.001, "the point should be where the click landed, off the grid");

        rightClickAt(pointUnitsFromCentre(2, 0.5));
        QAction* pSnap = contextMenuItem(qsl("Snap points to grid"));
        QVERIFY2(pSnap, "a right click while drawing did not put up a menu with snapping on it");
        pSnap->setChecked(true);
        QCOMPARE(pRoom->customLines.value(qsl("e")), QList<QPointF>{QPointF(0.5, 0.0)});

        pSnap->setChecked(false);
        QCOMPARE(pRoom->customLines.value(qsl("e")), asClicked);
        closeContextMenu();
    }

    void test_clickingOnACustomLineSelectsIt()
    {
        buildMap();
        QVERIFY(addLineToTheEastRoom());
        showMapper(false);

        clickAt(pointUnitsFromCentre(1, 2));

        QCOMPARE(mp2dMap->mCustomLineSelectedRoom, kLineRoomId);
        QCOMPARE(mp2dMap->mCustomLineSelectedExit, kLineExit);
        QCOMPARE(mp2dMap->mCustomLineSelectedPoint, -1);
    }

    // A click that misses every line by more than a whisker drops whatever
    // line was selected.
    void test_clickingClearOfEveryCustomLineDropsTheSelectedOne()
    {
        buildMap();
        QVERIFY(addLineToTheEastRoom());
        showMapper(false);
        clickAt(pointUnitsFromCentre(1, 2));
        QCOMPARE(mp2dMap->mCustomLineSelectedRoom, kLineRoomId);

        clickAt(pointUnitsFromCentre(1.5, 2));

        QCOMPARE(mp2dMap->mCustomLineSelectedRoom, 0);
        QVERIFY(mp2dMap->mCustomLineSelectedExit.isEmpty());
    }

    void test_aCustomLineCannotBeSelectedWhileViewing()
    {
        buildMap();
        QVERIFY(addLineToTheEastRoom());
        showMapper(true);

        clickAt(pointUnitsFromCentre(1, 2));

        QCOMPARE(mp2dMap->mCustomLineSelectedRoom, 0);
    }

    // While any rooms are selected the clicks are theirs, so a line cannot be
    // picked up until they are dropped.
    void test_aCustomLineIsLeftAloneWhileRoomsAreSelected()
    {
        buildMap();
        QVERIFY(addLineToTheEastRoom());
        showMapper(false);
        clickAt(pointUnitsFromCentre(-1, 0));
        QCOMPARE(mp2dMap->mMultiSelectionSet, QSet<int>{kWestRoomId});

        clickAt(pointUnitsFromCentre(1, 2));

        QCOMPARE(mp2dMap->mCustomLineSelectedRoom, 0);
    }

    // Once the line is selected its points can be picked up and dragged, which
    // is how a line is reshaped after it was drawn.
    void test_draggingAPointOfTheSelectedLineMovesIt()
    {
        buildMap();
        QVERIFY(addLineToTheEastRoom());
        showMapper(false);
        clickAt(pointUnitsFromCentre(1, 2));
        map()->resetUnsaved();

        dragFromTo(pointUnitsFromCentre(1, 3), pointUnitsFromCentre(2, 3));

        const QList<QPointF> expected{QPointF(2.0, 3.0), QPointF(1.0, 4.0)};
        QCOMPARE(linePoints(), expected);
        QVERIFY2(map()->isUnsaved(), "moving a point did not mark the map as needing a save");
    }

    void test_draggingAPointWithSnappingOnLandsItOnTheHalfGrid()
    {
        buildMap();
        QVERIFY(addLineToTheEastRoom());
        showMapper(false);
        clickAt(pointUnitsFromCentre(1, 2));
        mp2dMap->slot_setSnapCustomLinePointsToGrid(true);

        // 27 pixels east and 63 north is 1.35 by 3.15 of a unit
        dragFromTo(pointUnitsFromCentre(1, 3), viewCentre() + QPoint(27, -63));

        const QList<QPointF> expected{QPointF(1.5, 3.0), QPointF(1.0, 4.0)};
        QCOMPARE(linePoints(), expected);
    }

    // The first press on a line only selects it, so a drag that starts on a
    // point of a line that was not selected yet leaves the point alone.
    void test_aPointCannotBeDraggedUntilItsLineIsSelected()
    {
        buildMap();
        QVERIFY(addLineToTheEastRoom());
        showMapper(false);

        dragFromTo(pointUnitsFromCentre(1, 4), pointUnitsFromCentre(2, 4));

        QCOMPARE(mp2dMap->mCustomLineSelectedRoom, kLineRoomId);
        QCOMPARE(linePoints(), lineAsDrawn());
    }

    void test_theMenuOnASelectedLineCanDeleteIt()
    {
        buildMap();
        TRoom* pRoom = addLineToTheEastRoom();
        QVERIFY(pRoom);
        showMapper(false);
        clickAt(pointUnitsFromCentre(1, 2));

        rightClickAt(pointUnitsFromCentre(1, 2));
        QVERIFY2(pickContextMenuItem(qsl("Delete line")), "a right click on the selected line did not put up a menu with Delete line on it");

        QVERIFY2(!pRoom->customLines.contains(kLineExit), "the line is still there");
        QVERIFY2(!pRoom->customLinesColor.contains(kLineExit), "the line's colour outlived it");
        QCOMPARE(mp2dMap->mCustomLineSelectedRoom, 0);
    }

    void test_theMenuOnASelectedPointCanRemoveIt()
    {
        buildMap();
        QVERIFY(addLineToTheEastRoom());
        showMapper(false);
        clickAt(pointUnitsFromCentre(1, 2));
        clickAt(pointUnitsFromCentre(1, 4));
        QCOMPARE(mp2dMap->mCustomLineSelectedPoint, 1);

        rightClickAt(pointUnitsFromCentre(1, 4));
        QVERIFY2(pickContextMenuItem(qsl("Remove point")), "a right click on the selected point did not put up a menu with Remove point on it");

        QCOMPARE(linePoints(), QList<QPointF>{QPointF(1.0, 3.0)});
        QCOMPARE(mp2dMap->mCustomLineSelectedPoint, 0);
    }

    // Adding a point splits the segment that leads to the selected point, so
    // that stretch of the line can then be bent.
    void test_theMenuOnASelectedPointCanAddOneHalfwayBackAlongTheLine()
    {
        buildMap();
        QVERIFY(addLineToTheEastRoom());
        showMapper(false);
        clickAt(pointUnitsFromCentre(1, 2));
        clickAt(pointUnitsFromCentre(1, 4));
        QCOMPARE(mp2dMap->mCustomLineSelectedPoint, 1);

        rightClickAt(pointUnitsFromCentre(1, 4));
        QVERIFY2(pickContextMenuItem(qsl("Add point")), "a right click on the selected point did not put up a menu with Add point on it");

        const QList<QPointF> expected{QPointF(1.0, 3.0), QPointF(1.0, 3.5), QPointF(1.0, 4.0)};
        QCOMPARE(linePoints(), expected);
        QCOMPARE(mp2dMap->mCustomLineSelectedPoint, 2);
    }

    // The line starts from the edge of the room on the side its exit leaves
    // by, so a point added before the first one goes halfway back to there.
    void test_addingAPointBeforeTheFirstOneGoesHalfwayBackToTheRoomsEdge()
    {
        buildMap();
        QVERIFY(addLineToTheEastRoom());
        showMapper(false);
        clickAt(pointUnitsFromCentre(1, 2));
        clickAt(pointUnitsFromCentre(1, 3));
        QCOMPARE(mp2dMap->mCustomLineSelectedPoint, 0);

        rightClickAt(pointUnitsFromCentre(1, 3));
        QVERIFY(pickContextMenuItem(qsl("Add point")));

        const QList<QPointF> expected{QPointF(1.0, 1.75), QPointF(1.0, 3.0), QPointF(1.0, 4.0)};
        QCOMPARE(linePoints(), expected);
        QCOMPARE(mp2dMap->mCustomLineSelectedPoint, 1);
    }

    // A line's last point is meant to end on the room its exit leads to, and
    // the menu can put it there without the user having to aim.
    void test_theMenuCanMoveTheLastPointOntoTheRoomTheExitLeadsTo()
    {
        buildMap();
        TRoom* pRoom = addLineToTheEastRoom();
        QVERIFY(pRoom);
        QVERIFY(pRoom->setExit(kNorthEastRoomId, DIR_NORTH));
        showMapper(false);
        clickAt(pointUnitsFromCentre(1, 2));

        rightClickAt(pointUnitsFromCentre(1, 2));
        QAction* pMove = contextMenuItem(qsl("Move last point to target room"));
        QVERIFY2(pMove, "a right click on the selected line did not put up a menu with the move on it");
        QVERIFY2(pMove->isEnabled(), "the move is greyed out for a line whose exit leads somewhere");
        QVERIFY(pickContextMenuItem(qsl("Move last point to target room")));

        const QList<QPointF> expected{QPointF(1.0, 3.0), QPointF(1.0, 1.0)};
        QCOMPARE(linePoints(), expected);
    }

    // The same item is on the menu while a line is still being drawn.
    void test_theMenuWhileDrawingCanMoveTheLastPointOntoTheRoomTheExitLeadsTo()
    {
        buildMap();
        showMapper(false);
        TRoom* pRoom = startDrawingALine(kPlayerRoomId, qsl("e"));
        QVERIFY(pRoom);
        QVERIFY(pRoom->setExit(kEastRoomId, DIR_EAST));
        clickAt(pointUnitsFromCentre(0.5, 0.5));

        rightClickAt(pointUnitsFromCentre(2, 0.5));
        QVERIFY2(pickContextMenuItem(qsl("Move last point to target room")), "the move is missing or greyed out while drawing a line whose exit leads somewhere");

        QCOMPARE(pRoom->customLines.value(qsl("e")), QList<QPointF>{QPointF(1.0, 0.0)});
        QCOMPARE(mp2dMap->mCustomLinesRoomFrom, kPlayerRoomId);
    }

    void test_theLastPointCannotBeMovedOntoARoomTheExitDoesNotLeadTo()
    {
        buildMap();
        QVERIFY(addLineToTheEastRoom());
        showMapper(false);
        clickAt(pointUnitsFromCentre(1, 2));

        rightClickAt(pointUnitsFromCentre(1, 2));
        QAction* pMove = contextMenuItem(qsl("Move last point to target room"));
        QVERIFY(pMove);
        QVERIFY2(!pMove->isEnabled(), "the move is offered for a line whose exit leads nowhere");
    }

    void test_rightClickingARoomWhileEditingSelectsItAndOffersTheRoomItems()
    {
        buildMap();
        showMapper(false);
        rightClickAt(pointUnitsFromCentre(1, 0));

        QCOMPARE(mp2dMap->mMultiSelectionSet, QSet<int>{kEastRoomId});
        QCOMPARE(mp2dMap->mMultiSelectionHighlightRoomId, kEastRoomId);
        QVERIFY(contextMenuItem(qsl("Set exits...")));
        QAction* pExitLine = contextMenuItem(qsl("Create exit line..."));
        QVERIFY(pExitLine);
        QVERIFY(pExitLine->isEnabled());
        QVERIFY(contextMenuItem(qsl("Delete")));
        QVERIFY(contextMenuItem(qsl("Set player location")));
        QVERIFY(contextMenuItem(qsl("Switch to viewing mode")));
        QVERIFY(!contextMenuItem(qsl("Create new room here")));
        QVERIFY(!contextMenuItem(qsl("Spread...")));
    }

    // Mappers select a room and then right click on empty space to work on
    // it, since hitting the room itself is hard when zoomed out (#9915).
    void test_rightClickingEmptySpaceKeepsTheSelectedRoomAndItsItems()
    {
        buildMap();
        showMapper(false);
        clickAt(pointUnitsFromCentre(1, 0));
        QCOMPARE(mp2dMap->mMultiSelectionSet, QSet<int>{kEastRoomId});

        rightClickAt(pointUnitsFromCentre(2, 0.5));
        QCOMPARE(mp2dMap->mMultiSelectionSet, QSet<int>{kEastRoomId});
        QVERIFY(contextMenuItem(qsl("Set exits...")));
        QVERIFY(!contextMenuItem(qsl("Create new room here")));
    }

    void test_rightClickingEmptySpaceWithNothingSelectedOffersToCreateARoomThere()
    {
        buildMap();
        showMapper(false);
        const QPoint here = pointUnitsFromCentre(3, 2);
        // The new room goes where the pointer is rather than where the click
        // was, and the two are only the same when the pointer can be parked.
        QCursor::setPos(mp2dMap->mapToGlobal(here));
        if (QCursor::pos() != mp2dMap->mapToGlobal(here)) {
            QSKIP("this platform does not let a test park the pointer");
        }
        rightClickAt(here);
        QVERIFY(mp2dMap->mMultiSelectionSet.isEmpty());
        QVERIFY(!contextMenuItem(qsl("Delete")));
        QVERIFY(pickContextMenuItem(qsl("Create new room here")));

        QCOMPARE(map()->mpRoomDB->size(), 10);
        TRoom* pRoom = map()->mpRoomDB->getRoom(10);
        QVERIFY(pRoom);
        QCOMPARE(pRoom->x(), 3);
        QCOMPARE(pRoom->y(), 2);
        QCOMPARE(pRoom->getArea(), mAreaId);
    }

    void test_rightClickingARoomWhileViewingOffersOnlyTheViewingItems()
    {
        buildMap();
        showMapper(true);
        rightClickAt(pointUnitsFromCentre(1, 0));

        QCOMPARE(mp2dMap->mMultiSelectionSet, QSet<int>{kEastRoomId});
        QVERIFY(contextMenuItem(qsl("Set player location")));
        QVERIFY(contextMenuItem(qsl("Switch to editing mode")));
        QVERIFY(!contextMenuItem(qsl("Delete")));
        QVERIFY(!contextMenuItem(qsl("Set exits...")));
        QVERIFY(!contextMenuItem(qsl("Create label...")));
    }

    void test_settingThePlayerLocationFromTheMenuMovesThePlayerThere()
    {
        buildMap();
        showMapper(true);
        QVERIFY(runLua(qsl("manualLocationSetTo = nil\n"
                           "registerAnonymousEventHandler('sysManualLocationSetEvent', function(_, roomId) manualLocationSetTo = roomId end)")));
        rightClickAt(pointUnitsFromCentre(1, 0));
        QVERIFY(pickContextMenuItem(qsl("Set player location")));

        QCOMPARE(map()->mRoomIdHash.value(map()->mProfileName), kEastRoomId);
        QVERIFY(runLua(qsl("assert(manualLocationSetTo == %1)").arg(kEastRoomId)));
    }

    void test_theMenuSwitchesBetweenViewingAndEditing()
    {
        buildMap();
        showMapper(true);
        QVERIFY(runLua(qsl("mapModeSeen = nil\n"
                           "registerAnonymousEventHandler('mapModeChangeEvent', function(_, mode) mapModeSeen = mode end)")));
        rightClickAt(pointUnitsFromCentre(1, 0));
        QVERIFY(pickContextMenuItem(qsl("Switch to editing mode")));
        QVERIFY(!mp2dMap->mMapViewOnly);
        QVERIFY(!mpHost->mMapViewOnly);
        QVERIFY(runLua(qsl("assert(mapModeSeen == 'editing')")));

        rightClickAt(pointUnitsFromCentre(1, 0));
        QVERIFY(pickContextMenuItem(qsl("Switch to viewing mode")));
        QVERIFY(mp2dMap->mMapViewOnly);
        QVERIFY(mpHost->mMapViewOnly);
        QVERIFY(runLua(qsl("assert(mapModeSeen == 'viewing')")));
    }

    void test_rightClickingOneOfSeveralSelectedRoomsOffersTheGroupItems()
    {
        buildMap();
        showMapper(false);
        clickAt(pointUnitsFromCentre(1, 0));
        clickAt(pointUnitsFromCentre(-1, 0), Qt::ShiftModifier);
        const QSet<int> both{kWestRoomId, kEastRoomId};
        QCOMPARE(mp2dMap->mMultiSelectionSet, both);

        rightClickAt(pointUnitsFromCentre(1, 0));
        QCOMPARE(mp2dMap->mMultiSelectionSet, both);
        QVERIFY(contextMenuItem(qsl("Spread...")));
        QVERIFY(contextMenuItem(qsl("Shrink...")));
        QVERIFY(contextMenuItem(qsl("Delete")));
        QVERIFY(!contextMenuItem(qsl("Set exits...")));
        QVERIFY(!contextMenuItem(qsl("Set player location")));
    }

    void test_rightClickingARoomOutsideTheSelectionSelectsThatRoomInstead()
    {
        buildMap();
        showMapper(false);
        clickAt(pointUnitsFromCentre(1, 0));
        clickAt(pointUnitsFromCentre(-1, 0), Qt::ShiftModifier);

        rightClickAt(pointUnitsFromCentre(0, 1));
        QCOMPARE(mp2dMap->mMultiSelectionSet, QSet<int>{kNorthRoomId});
        QCOMPARE(mp2dMap->mMultiSelectionHighlightRoomId, kNorthRoomId);
        QVERIFY(contextMenuItem(qsl("Set exits...")));
    }

    void test_theExitLineItemIsGreyedOutInGridMode()
    {
        buildMap(true);
        showMapper(false);
        rightClickAt(pointUnitsFromCentre(1, 0));

        QAction* pItem = contextMenuItem(qsl("Create exit line..."));
        QVERIFY(pItem);
        QVERIFY(!pItem->isEnabled());
    }

    void test_deletingFromTheMenuRemovesEverySelectedRoom()
    {
        buildMap();
        showMapper(false);
        clickAt(pointUnitsFromCentre(1, 0));
        clickAt(pointUnitsFromCentre(-1, 0), Qt::ShiftModifier);
        map()->resetUnsaved();
        rightClickAt(pointUnitsFromCentre(1, 0));
        QVERIFY(pickContextMenuItem(qsl("Delete")));

        QVERIFY(!map()->mpRoomDB->getRoom(kEastRoomId));
        QVERIFY(!map()->mpRoomDB->getRoom(kWestRoomId));
        QCOMPARE(map()->mpRoomDB->size(), 7);
        QVERIFY(mp2dMap->mMultiSelectionSet.isEmpty());
        QVERIFY(map()->isUnsaved());
    }

    void test_aMenuAndEventAddedFromLuaFireWithTheSelectedRooms()
    {
        buildMap();
        showMapper(false);
        QVERIFY(runLua(qsl("addMapMenu('mouseTestMenu', '', 'Mouse tests')\n"
                           "addMapEvent('mouseTestAction', 'mouseTestEvent', 'mouseTestMenu', 'Do the thing')\n"
                           "mouseTestRooms = nil\n"
                           "registerAnonymousEventHandler('mouseTestEvent', function(_, uniqueName, ...) mouseTestName = uniqueName mouseTestRooms = {...} end)")));
        clickAt(pointUnitsFromCentre(1, 0));
        clickAt(pointUnitsFromCentre(-1, 0), Qt::ShiftModifier);
        rightClickAt(pointUnitsFromCentre(1, 0));

        QAction* pAction = userMenuItem(qsl("Mouse tests"), qsl("Do the thing"));
        QVERIFY2(pAction, "the script's menu and item are not on the context menu");
        pAction->trigger();
        closeContextMenu();
        QVERIFY(runLua(qsl("assert(mouseTestRooms, 'the event never fired')\n"
                           "assert(mouseTestName == 'mouseTestAction', mouseTestName)\n"
                           "table.sort(mouseTestRooms)\n"
                           "assert(#mouseTestRooms == 2 and mouseTestRooms[1] == %1 and mouseTestRooms[2] == %2, table.concat(mouseTestRooms, ','))")
                               .arg(kWestRoomId)
                               .arg(kEastRoomId)));
    }

    void test_theMenuOnAnEmptyMapOffersToLoadOrCreateOne()
    {
        buildMap();
        showMapper(true);
        map()->mapClear();
        rightClickAt(viewCentre());

        QVERIFY(contextMenuItem(qsl("Load map...")));
        QVERIFY(contextMenuItem(qsl("Create new map")));
        QVERIFY(!contextMenuItem(qsl("Download from game")));
        QVERIFY(!contextMenuItem(qsl("Set player location")));
    }

    void test_theMenuOnAnEmptyMapOffersTheGamesMapWhenItHasOne()
    {
        buildMap();
        showMapper(true);
        map()->mapClear();
        map()->setMmpMapLocation(qsl("https://example.com/map.xml"));
        rightClickAt(viewCentre());

        QVERIFY(contextMenuItem(qsl("Download from game")));
    }

    void test_creatingAMapFromTheMenuMakesARoomToStandInAndOpensItForEditing()
    {
        buildMap();
        showMapper(true);
        map()->mapClear();
        rightClickAt(viewCentre());
        QVERIFY(pickContextMenuItem(qsl("Create new map")));

        QCOMPARE(map()->mpRoomDB->size(), 1);
        const int roomId = map()->mRoomIdHash.value(map()->mProfileName);
        TRoom* pRoom = map()->mpRoomDB->getRoom(roomId);
        QVERIFY(pRoom);
        QCOMPARE(pRoom->x(), 0);
        QCOMPARE(pRoom->y(), 0);
        QVERIFY2(!mp2dMap->mMapViewOnly, "a brand new map should open ready to edit");
    }

    // Rolling the wheel forward zooms in, 7% a notch: fewer map units fit
    // across the view, so each room is drawn bigger.
    void test_rollingTheWheelForwardZoomsIn()
    {
        buildMap();
        showMapper(true);

        rollWheelAt(viewCentre(), 1);

        const double expectedZoom = kZoom / 1.07;
        QCOMPARE(zoom(), expectedZoom);
        renderFrame();
        QCOMPARE(mp2dMap->mRoomWidth, pixelsPerMapUnitAtZoom(expectedZoom));
        QCOMPARE(mp2dMap->mRoomHeight, pixelsPerMapUnitAtZoom(expectedZoom));
    }

    void test_rollingTheWheelBackZoomsOut()
    {
        buildMap();
        showMapper(true);

        rollWheelAt(viewCentre(), -1);

        const double expectedZoom = kZoom * 1.07;
        QCOMPARE(zoom(), expectedZoom);
        renderFrame();
        QCOMPARE(mp2dMap->mRoomWidth, pixelsPerMapUnitAtZoom(expectedZoom));
    }

    // The preference for the older way round: forward zooms out.
    void test_invertedZoomRollsTheOtherWay()
    {
        buildMap();
        showMapper(true);
        mudlet::self()->setInvertMapZoom(true);

        rollWheelAt(viewCentre(), 1);

        QCOMPARE(zoom(), kZoom * 1.07);
    }

    // Holding Control makes each notch count for five, for getting across a
    // big area's worth of zoom without wearing the wheel out.
    void test_holdingControlZoomsFiveNotchesAtATime()
    {
        buildMap();
        showMapper(true);

        rollWheelAt(viewCentre(), 1, Qt::ControlModifier);

        QCOMPARE(zoom(), kZoom / std::pow(1.07, 5));
    }

    // Zooming with the pointer off-centre shifts the view so that whatever
    // was under the pointer stays under it, which is what lets a user zoom in
    // on a corner of an area without then having to pan back to it.
    void test_theRoomUnderThePointerStaysThereWhileZooming()
    {
        buildMap();
        showMapper(true);
        const QPoint northEast = pointUnitsFromCentre(1, 1);
        QCOMPARE(mp2dMap->roomIdAtWidgetPosition(northEast, area()), std::optional<int>(kNorthEastRoomId));

        // Far enough out that a room drawn from a view that had not shifted
        // would be well clear of where it was.
        rollWheelAt(northEast, -10);
        renderFrame();

        QCOMPARE(mp2dMap->roomIdAtWidgetPosition(northEast, area()), std::optional<int>(kNorthEastRoomId));
        QVERIFY2(mp2dMap->roomIdAtWidgetPosition(viewCentre(), area()) != std::optional<int>(kPlayerRoomId), "the view did not shift, so the room under the pointer only stayed put by luck");
    }

    // There is a limit to how far in the map goes, so a runaway wheel cannot
    // leave a single room filling the view many times over.
    void test_zoomingInStopsAtTheClosestTheMapGoes()
    {
        buildMap();
        showMapper(true);

        rollWheelAt(viewCentre(), 100);

        QCOMPARE(zoom(), T2DMap::csmMinXYZoom);
        rollWheelAt(viewCentre(), 1);
        QCOMPARE(zoom(), T2DMap::csmMinXYZoom);
    }

    // "Move" from the context menu picks the selected rooms up without a
    // button held: they follow how far the pointer moves from there, in whole
    // units, taking their custom lines along, until a click puts them down.
    void test_moveFromTheMenuCarriesTheRoomsWithThePointerUntilAClick()
    {
        buildMap();
        TRoom* pEastRoom = addLineToTheEastRoom();
        QVERIFY(pEastRoom);
        showMapper(false);
        // Just short of the middle, where the custom line's first segment would
        // take the click instead.
        rightClickAt(pointUnitsFromCentre(1, -0.2));
        QVERIFY(pickContextMenuItem(qsl("Move")));
        QVERIFY2(!mp2dMap->mHelpMsg.isEmpty(), "the map does not say how to finish the move");

        // The first hover only says where the pointer is starting from.
        hoverTo(pointUnitsFromCentre(3, -1));
        QCOMPARE(pEastRoom->x(), 1);
        QCOMPARE(pEastRoom->y(), 0);

        hoverTo(pointUnitsFromCentre(5, -2));
        QCOMPARE(pEastRoom->x(), 3);
        QCOMPARE(pEastRoom->y(), -1);
        QCOMPARE(linePoints(), (QList<QPointF>{QPointF(3.0, 2.0), QPointF(3.0, 3.0)}));

        clickAt(pointUnitsFromCentre(3, -1));
        QVERIFY2(!mp2dMap->mRoomBeingMoved, "a click did not put the rooms down");
        QVERIFY(mp2dMap->mHelpMsg.isEmpty());
        hoverTo(pointUnitsFromCentre(7, -2));
        QCOMPARE(pEastRoom->x(), 3);
        QCOMPARE(pEastRoom->y(), -1);
    }

    // Double-clicking a room walks the player to it along the exits the map
    // knows about, by handing the directions to the profile's doSpeedWalk.
    void test_doubleClickingARoomWalksThereAlongTheKnownExits()
    {
        buildMap();
        QVERIFY(map()->setExit(kPlayerRoomId, kEastRoomId, DIR_EAST));
        showMapper(true);
        QVERIFY(runLua(qsl("walkedDirs = nil\n"
                           "function doSpeedWalk() walkedDirs = table.concat(speedWalkDir, ',') walkedPath = table.concat(speedWalkPath, ',') end")));

        doubleClickAt(pointUnitsFromCentre(1, 0));
        // The room under a double-click is looked up as the next frame is drawn.
        renderFrame();

        QVERIFY(runLua(qsl("assert(walkedDirs, 'doSpeedWalk was never called')\n"
                           "assert(walkedDirs == 'e', walkedDirs)\n"
                           "assert(walkedPath == '%1', walkedPath)")
                               .arg(kEastRoomId)));
        QCOMPARE(mp2dMap->mTargetRoomId, kEastRoomId);
    }

    void test_doubleClickingARoomWithNoWayToItSaysSo()
    {
        buildMap();
        showMapper(true);
        QVERIFY(runLua(qsl("walked = false\nfunction doSpeedWalk() walked = true end")));

        doubleClickAt(pointUnitsFromCentre(1, 0));
        renderFrame();

        QVERIFY(consoleText().contains(qsl("Mapper: Cannot find a path from %1 to %2 using known exits.").arg(QString::number(kPlayerRoomId), QString::number(kEastRoomId))));
        QVERIFY(runLua(qsl("assert(not walked, 'doSpeedWalk was called with no path to walk')")));
    }

    // A profile that finds its own way, by setting mudlet.custom_speedwalk,
    // gets the two ends of the walk instead of a path the map worked out.
    void test_aProfileThatWalksItsOwnWayGetsTheEndsOfTheWalk()
    {
        buildMap();
        showMapper(true);
        QVERIFY(runLua(qsl("mudlet.custom_speedwalk = true\n"
                           "walkFrom, walkTo = nil, nil\n"
                           "function doSpeedWalk() walkFrom, walkTo = speedWalkFrom, speedWalkTo end")));

        doubleClickAt(pointUnitsFromCentre(1, 0));
        renderFrame();

        QVERIFY(runLua(qsl("assert(walkFrom == %1 and walkTo == %2, tostring(walkFrom) .. ' to ' .. tostring(walkTo))").arg(kPlayerRoomId).arg(kEastRoomId)));
        QVERIFY(!consoleText().contains(qsl("Cannot find a path")));
    }

    void test_doubleClickingEmptySpaceStartsNoWalk()
    {
        buildMap();
        QVERIFY(map()->setExit(kPlayerRoomId, kEastRoomId, DIR_EAST));
        showMapper(true);
        QVERIFY(runLua(qsl("walked = false\nfunction doSpeedWalk() walked = true end")));

        doubleClickAt(pointUnitsFromCentre(2, 0.5));
        renderFrame();

        QVERIFY(runLua(qsl("assert(not walked, 'doSpeedWalk was called for a double-click on nothing')")));
        QVERIFY(!consoleText().contains(qsl("Cannot find a path")));
    }

    // A right click on a label is about the label, not the rooms: it drops
    // whatever rooms were selected and offers the label's own items.
    void test_rightClickingALabelDropsTheRoomSelectionAndOffersTheLabelItems()
    {
        buildMap();
        showMapper(false);
        const int labelId = addTestLabel();
        clickAt(pointUnitsFromCentre(-1, 0));
        QCOMPARE(mp2dMap->mMultiSelectionSet, QSet<int>{kWestRoomId});

        rightClickAt(pointOnTestLabel());

        QVERIFY(mp2dMap->mLabelHighlighted);
        QVERIFY(area()->mMapLabels.value(labelId).highlight);
        QVERIFY(mp2dMap->mMultiSelectionSet.isEmpty());
        QVERIFY(contextMenuItem(qsl("Move")));
        QVERIFY(contextMenuItem(qsl("Delete")));
        QVERIFY2(!contextMenuItem(qsl("Configure areas...")), "the room menu came up over the label");
    }

    void test_deleteFromTheLabelMenuRemovesIt()
    {
        buildMap();
        showMapper(false);
        const int labelId = addTestLabel();
        map()->resetUnsaved();
        rightClickAt(pointOnTestLabel());

        QVERIFY(pickContextMenuItem(qsl("Delete")));

        QVERIFY(!area()->mMapLabels.contains(labelId));
        QVERIFY(map()->isUnsaved());
    }

    // Move from the label's menu works like Move for rooms: the label follows
    // the pointer with no button held until a click puts it down, and since a
    // label is part of the map, moving one leaves the map needing a save.
    void test_moveFromTheLabelMenuCarriesItWithThePointerUntilAClick()
    {
        buildMap();
        showMapper(false);
        const int labelId = addTestLabel();
        rightClickAt(pointOnTestLabel());
        QVERIFY(pickContextMenuItem(qsl("Move")));
        QVERIFY(mp2dMap->mMoveLabel);
        QVERIFY2(!mp2dMap->mHelpMsg.isEmpty(), "the map does not say how to finish the move");
        map()->resetUnsaved();

        hoverTo(pointUnitsFromCentre(-2, 1));
        QCOMPARE(area()->mMapLabels.value(labelId).pos, QVector3D(-2, 1, 0));
        QVERIFY2(map()->isUnsaved(), "moving a label did not leave the map needing a save");

        clickAt(pointUnitsFromCentre(-2, 1));
        QVERIFY2(!mp2dMap->mMoveLabel, "a click did not put the label down");
        QVERIFY(!mp2dMap->mLabelHighlighted);
        QVERIFY(!area()->mMapLabels.value(labelId).highlight);
        QVERIFY(mp2dMap->mHelpMsg.isEmpty());
        hoverTo(pointUnitsFromCentre(3, 3));
        QCOMPARE(area()->mMapLabels.value(labelId).pos, QVector3D(-2, 1, 0));
    }

    // A temporary label, the kind a script puts up and never saves, moves the
    // same way but is not worth a save.
    void test_movingATemporaryLabelLeavesTheMapSaved()
    {
        buildMap();
        showMapper(false);
        const int labelId = addTestLabel();
        area()->mMapLabels[labelId].temporary = true;
        rightClickAt(pointOnTestLabel());
        QVERIFY(pickContextMenuItem(qsl("Move")));
        map()->resetUnsaved();

        hoverTo(pointUnitsFromCentre(-2, 1));

        QCOMPARE(area()->mMapLabels.value(labelId).pos, QVector3D(-2, 1, 0));
        QVERIFY(!map()->isUnsaved());
    }

    // A right click away from a picked-up label puts it down, and the click
    // then goes on to do what it does on empty space: put up the room menu.
    void test_rightClickingAwayFromALabelPutsItDownAndOffersTheRoomMenu()
    {
        buildMap();
        showMapper(false);
        const int labelId = addTestLabel();
        clickAt(pointOnTestLabel());
        QVERIFY(mp2dMap->mLabelHighlighted);

        rightClickAt(pointUnitsFromCentre(-2, -2));

        QVERIFY(!mp2dMap->mLabelHighlighted);
        QVERIFY(!area()->mMapLabels.value(labelId).highlight);
        QVERIFY(contextMenuItem(qsl("Create new room here")));
        QVERIFY2(!contextMenuItem(qsl("Delete")), "the label menu came up instead of the room menu");
    }

    // The box dragged out after picking Create label is where the label goes,
    // and the dialog that comes up for its text is showing a label in that
    // box before anything has been typed. Cancelling the dialog takes the
    // label away again.
    void test_createLabelFromTheMenuPutsTheLabelInTheBoxDraggedOut()
    {
        buildMap();
        showMapper(false);

        dragOutALabelBox();

        QVERIFY(!mp2dMap->mSizeLabel);
        QCOMPARE(area()->mMapLabels.size(), 1);
        QCOMPARE(area()->mMapLabels.first().pos, QVector3D(-5, 5, 0));
        QCOMPARE(area()->mMapLabels.first().size, QSizeF(3, 2));
        QVERIFY2(mp2dMap->mpDlgMapLabel, "no dialog came up to fill the label in");

        mp2dMap->mpDlgMapLabel->close();
        QVERIFY2(area()->mMapLabels.isEmpty(), "cancelling the dialog left the label on the map");
    }

    // Dragging the box out is what puts the label on the map and marks it
    // unsaved; saving the dialog just has to leave both as they are.
    void test_savingTheLabelDialogKeepsTheLabel()
    {
        buildMap();
        showMapper(false);
        map()->resetUnsaved();
        dragOutALabelBox();
        QVERIFY(mp2dMap->mpDlgMapLabel);

        mp2dMap->mpDlgMapLabel->accept();

        QCOMPARE(area()->mMapLabels.size(), 1);
        QVERIFY(map()->isUnsaved());
    }

    // A box is a box whichever corner it was dragged out from: the label goes
    // in it exactly, at the size it was drawn.
    void test_theLabelBoxLandsWhereItWasDrawnFromAnyCorner()
    {
        buildMap();
        showMapper(false);
        const QPoint topLeft = pointUnitsFromCentre(-5, 5);
        const QPoint bottomRight = pointUnitsFromCentre(-2, 3);
        const QPoint topRight(bottomRight.x(), topLeft.y());
        const QPoint bottomLeft(topLeft.x(), bottomRight.y());
        const struct
        {
            QString direction;
            QPoint from;
            QPoint to;
        } drags[] = {
                {qsl("down and right"), topLeft, bottomRight},
                {qsl("up and left"), bottomRight, topLeft},
                {qsl("down and left"), topRight, bottomLeft},
                {qsl("up and right"), bottomLeft, topRight},
        };

        for (const auto& drag : drags) {
            rightClickAt(pointUnitsFromCentre(-2, -2));
            QVERIFY(pickContextMenuItem(qsl("Create label...")));
            dragFromTo(drag.from, drag.to);

            QCOMPARE(area()->mMapLabels.size(), 1);
            const TMapLabel label = area()->mMapLabels.first();
            QVERIFY2(label.pos == QVector3D(-5, 5, 0), qPrintable(qsl("dragged out %1 the label landed at %2, %3").arg(drag.direction).arg(label.pos.x()).arg(label.pos.y())));
            QVERIFY2(label.size == QSizeF(3, 2), qPrintable(qsl("dragged out %1 the label came out %2 by %3").arg(drag.direction).arg(label.size.width()).arg(label.size.height())));

            QVERIFY(mp2dMap->mpDlgMapLabel);
            mp2dMap->mpDlgMapLabel->close();
            QVERIFY(area()->mMapLabels.isEmpty());
        }
    }

    // A click with no drag has no box to put a label in, so nothing is made
    // and no dialog comes up.
    void test_clickingWithoutDraggingOutABoxMakesNoLabel()
    {
        buildMap();
        showMapper(false);
        rightClickAt(pointUnitsFromCentre(-2, -2));
        QVERIFY(pickContextMenuItem(qsl("Create label...")));

        clickAt(pointUnitsFromCentre(-5, 5));

        QVERIFY(!mp2dMap->mSizeLabel);
        QVERIFY2(area()->mMapLabels.isEmpty(), "a label with no box was made");
        QVERIFY2(!mp2dMap->mpDlgMapLabel, "a dialog came up for a label with no box");
    }

    // A script can clear the map, or load another one, while the dialog is
    // open. Cancelling it then has no area to take the label back out of and
    // must not reach for the one that was deleted, which only AddressSanitizer
    // can see: the assertion is just that the dialog still closes.
    void test_cancellingTheLabelDialogAfterTheMapWasClearedClosesIt()
    {
        buildMap();
        showMapper(false);
        dragOutALabelBox();
        QVERIFY(mp2dMap->mpDlgMapLabel);
        map()->mapClear();
        QVERIFY(!area());

        mp2dMap->mpDlgMapLabel->close();

        QTRY_VERIFY2(!mp2dMap->mpDlgMapLabel, "cancelling the dialog did not close it");
    }

    // Typing into the dialog redraws the label in its area, which is gone once
    // the map has been cleared, so nothing left on the cleared map may get the
    // label.
    void test_editingTheLabelDialogAfterTheMapWasClearedMakesNoLabel()
    {
        buildMap();
        showMapper(false);
        dragOutALabelBox();
        QVERIFY(mp2dMap->mpDlgMapLabel);
        map()->mapClear();
        QVERIFY(!area());
        map()->resetUnsaved();

        mp2dMap->mpDlgMapLabel->plainTextEdit_labelText->setPlainText(qsl("typed after the map went"));

        QVERIFY2(!map()->isUnsaved(), "the label was written back into the cleared map");
    }

    // The dialog is not modal, so the mapper can be switched to another area
    // while it is open. The label belongs to the area its box was dragged out
    // in: typing keeps it there, and cancelling takes it back out of there.
    void test_theLabelDialogStaysWithTheAreaItsBoxWasDraggedOutIn()
    {
        buildMap();
        showMapper(false);
        dragOutALabelBox();
        QVERIFY(mp2dMap->mpDlgMapLabel);
        const int otherAreaId = map()->mpRoomDB->addArea(qsl("Next Door"));
        QVERIFY(otherAreaId > 0);
        const int nextDoorRoomId = 20;
        QVERIFY(map()->addRoom(nextDoorRoomId) && map()->setRoomArea(nextDoorRoomId, otherAreaId) && map()->setRoomCoordinates(nextDoorRoomId, 0, 0, 0));
        mp2dMap->switchArea(otherAreaId);
        QCOMPARE(mp2dMap->mAreaID, otherAreaId);

        mp2dMap->mpDlgMapLabel->plainTextEdit_labelText->setPlainText(qsl("typed in another area"));

        QCOMPARE(area()->mMapLabels.size(), 1);
        QVERIFY2(map()->mpRoomDB->getArea(otherAreaId)->mMapLabels.isEmpty(), "the label landed in the area the mapper was switched to");

        mp2dMap->mpDlgMapLabel->close();

        QTRY_VERIFY(!mp2dMap->mpDlgMapLabel);
        QVERIFY2(area()->mMapLabels.isEmpty(), "cancelling the dialog left the label in the area it was made in");
    }

    // Boxing several rooms puts a list of them up in the corner of the map, so
    // rooms that are drawn on top of one another can still be told apart.
    void test_boxingSeveralRoomsListsThemInTheCornerOfTheMap()
    {
        buildMap();
        showMapper(false);

        dragFromTo(pointUnitsFromCentre(-1.5, 0.5), pointUnitsFromCentre(1.5, -0.5));

        QVERIFY2(!mp2dMap->mMultiSelectionListWidget.isHidden(), "no list came up for the boxed rooms");
        QCOMPARE(listedRoomIds(), (QSet<int>{kWestRoomId, kPlayerRoomId, kEastRoomId}));
        QVERIFY2(mp2dMap->mMultiSelectionListWidget.isColumnHidden(1), "a names column came up for rooms with no names");

        clickAt(pointUnitsFromCentre(0, 1));
        QCOMPARE(mp2dMap->mMultiSelectionSet, QSet<int>{kNorthRoomId});
        QVERIFY2(mp2dMap->mMultiSelectionListWidget.isHidden(), "the list stayed up for a single room");
    }

    void test_roomsWithNamesGetANamesColumnInTheList()
    {
        buildMap();
        map()->mpRoomDB->getRoom(kWestRoomId)->name = qsl("West");
        map()->mpRoomDB->getRoom(kEastRoomId)->name = qsl("East");
        showMapper(false);

        dragFromTo(pointUnitsFromCentre(-1.5, 0.5), pointUnitsFromCentre(1.5, -0.5));

        QVERIFY2(!mp2dMap->mMultiSelectionListWidget.isColumnHidden(1), "the names column is hidden");
        QCOMPARE(listedRoomIds(), (QSet<int>{kWestRoomId, kPlayerRoomId, kEastRoomId}));
        QCOMPARE(listedRoomName(kWestRoomId), qsl("West"));
        QCOMPARE(listedRoomName(kEastRoomId), qsl("East"));
        QVERIFY(listedRoomName(kPlayerRoomId).isEmpty());
    }

    // Picking rooms in the list is what narrows a selection down to the ones
    // the box could not separate.
    void test_pickingRoomsInTheListNarrowsTheSelectionToThem()
    {
        buildMap();
        showMapper(false);
        dragFromTo(pointUnitsFromCentre(-1.5, 0.5), pointUnitsFromCentre(1.5, -0.5));

        for (QTreeWidgetItem* item : listedRooms()) {
            item->setSelected(item->text(0).toInt() == kEastRoomId);
        }

        QCOMPARE(mp2dMap->mMultiSelectionSet, QSet<int>{kEastRoomId});
        QCOMPARE(mp2dMap->mMultiSelectionHighlightRoomId, kEastRoomId);

        mp2dMap->mMultiSelectionListWidget.clearSelection();
        QVERIFY2(mp2dMap->mMultiSelectionSet.isEmpty(), "clearing the list left rooms selected");
        QCOMPARE(mp2dMap->mMultiSelectionHighlightRoomId, 0);
    }

    // The list scrolls with the wheel, so a wheel over it must not zoom the map
    // behind it as well.
    void test_rollingTheWheelOverTheListDoesNotZoomTheMap()
    {
        buildMap();
        showMapper(false);
        dragFromTo(pointUnitsFromCentre(-1.5, 0.5), pointUnitsFromCentre(1.5, -0.5));
        QVERIFY(!mp2dMap->mMultiSelectionListWidget.isHidden());

        rollWheelAt(mp2dMap->mMultiSelectionListWidget.geometry().center(), 1);

        QCOMPARE(zoom(), kZoom);
    }

    void test_holdingControlTogglesARoomInAndOutOfTheSelection()
    {
        buildMap();
        showMapper(false);
        clickAt(pointUnitsFromCentre(-1, 0));

        clickAt(pointUnitsFromCentre(1, 0), Qt::ControlModifier);
        QCOMPARE(mp2dMap->mMultiSelectionSet, (QSet<int>{kWestRoomId, kEastRoomId}));

        clickAt(pointUnitsFromCentre(1, 0), Qt::ControlModifier);
        QCOMPARE(mp2dMap->mMultiSelectionSet, QSet<int>{kWestRoomId});
    }

    // An exit into another area is drawn as a stub with an arrow head one unit
    // out from its room. Double-clicking that stub walks to the room it leads
    // to, the same as double-clicking a room in this area.
    void test_doubleClickingTheStubOfAnExitToAnotherAreaWalksThere()
    {
        buildMap();
        const int otherAreaId = map()->mpRoomDB->addArea(qsl("Next Door"));
        QVERIFY(otherAreaId > 0);
        const int nextDoorRoomId = 20;
        QVERIFY(map()->addRoom(nextDoorRoomId) && map()->setRoomArea(nextDoorRoomId, otherAreaId) && map()->setRoomCoordinates(nextDoorRoomId, 0, 0, 0));
        QVERIFY(map()->setExit(kPlayerRoomId, kEastRoomId, DIR_EAST));
        QVERIFY(map()->setExit(kEastRoomId, nextDoorRoomId, DIR_EAST));
        showMapper(true);
        QVERIFY(runLua(qsl("walkedDirs = nil\n"
                           "function doSpeedWalk() walkedDirs = table.concat(speedWalkDir, ',') walkedPath = table.concat(speedWalkPath, ',') end")));

        doubleClickAt(pointUnitsFromCentre(2, 0));
        renderFrame();

        QVERIFY(runLua(qsl("assert(walkedDirs, 'doSpeedWalk was never called')\n"
                           "assert(walkedDirs == 'e,e', walkedDirs)\n"
                           "assert(walkedPath == '%1,%2', walkedPath)")
                               .arg(kEastRoomId)
                               .arg(nextDoorRoomId)));
        QCOMPARE(mp2dMap->mTargetRoomId, nextDoorRoomId);
    }

    void test_spreadFromTheMenuMovesTheSelectedRoomsOutFromTheHighlightedOne()
    {
        buildMap();
        showMapper(false);
        dragFromTo(pointUnitsFromCentre(-1.5, 0.5), pointUnitsFromCentre(1.5, -0.5));
        QCOMPARE(mp2dMap->mMultiSelectionHighlightRoomId, kPlayerRoomId);
        map()->resetUnsaved();
        rightClickAt(pointUnitsFromCentre(1, 0));

        QVERIFY(pickFactorItem(qsl("Spread..."), 3));

        QCOMPARE(roomPosition(kWestRoomId), QVector3D(-3, 0, 0));
        QCOMPARE(roomPosition(kPlayerRoomId), QVector3D(0, 0, 0));
        QCOMPARE(roomPosition(kEastRoomId), QVector3D(3, 0, 0));
        QCOMPARE(roomPosition(kNorthRoomId), QVector3D(0, 1, 0));
        QVERIFY(map()->isUnsaved());
    }

    // A custom exit line is scaled about the highlighted room the same way
    // its rooms are, so it still runs between them afterwards. The rooms sit
    // off the diagonal so that scaling y about the wrong axis shows up.
    void test_spreadingRoomsCarriesTheirCustomLinesWithThem()
    {
        buildMap();
        for (int roomId = 1; roomId <= 9; ++roomId) {
            const TRoom* pRoom = map()->mpRoomDB->getRoom(roomId);
            QVERIFY(map()->setRoomCoordinates(roomId, pRoom->x() + 3, pRoom->y(), 0));
        }
        QVERIFY(map()->setExit(kPlayerRoomId, kEastRoomId, DIR_EAST));
        map()->mpRoomDB->getRoom(kPlayerRoomId)->customLines[qsl("e")] = QList<QPointF>{QPointF(3.5, 1.0), QPointF(4.0, 0.5)};
        showMapper(false);
        dragFromTo(pointUnitsFromCentre(1.5, 0.5), pointUnitsFromCentre(4.5, -0.5));
        QCOMPARE(mp2dMap->mMultiSelectionHighlightRoomId, kPlayerRoomId);
        rightClickAt(pointUnitsFromCentre(4, 0));

        QVERIFY(pickFactorItem(qsl("Spread..."), 2));

        QCOMPARE(roomPosition(kPlayerRoomId), QVector3D(3, 0, 0));
        QCOMPARE(roomPosition(kEastRoomId), QVector3D(5, 0, 0));
        QCOMPARE(map()->mpRoomDB->getRoom(kPlayerRoomId)->customLines.value(qsl("e")), (QList<QPointF>{QPointF(4.0, 2.0), QPointF(5.0, 1.0)}));
    }

    void test_shrinkingRoomsCarriesTheirCustomLinesWithThem()
    {
        buildMap();
        for (int roomId = 1; roomId <= 9; ++roomId) {
            const TRoom* pRoom = map()->mpRoomDB->getRoom(roomId);
            QVERIFY(map()->setRoomCoordinates(roomId, pRoom->x() * 2 + 3, pRoom->y() * 2, 0));
        }
        QVERIFY(map()->setExit(kPlayerRoomId, kEastRoomId, DIR_EAST));
        map()->mpRoomDB->getRoom(kPlayerRoomId)->customLines[qsl("e")] = QList<QPointF>{QPointF(4.0, 2.0), QPointF(5.0, 1.0)};
        showMapper(false);
        dragFromTo(pointUnitsFromCentre(0.5, 0.5), pointUnitsFromCentre(5.5, -0.5));
        QCOMPARE(mp2dMap->mMultiSelectionHighlightRoomId, kPlayerRoomId);
        rightClickAt(pointUnitsFromCentre(5, 0));

        QVERIFY(pickFactorItem(qsl("Shrink..."), 2));

        QCOMPARE(roomPosition(kPlayerRoomId), QVector3D(3, 0, 0));
        QCOMPARE(roomPosition(kEastRoomId), QVector3D(4, 0, 0));
        QCOMPARE(map()->mpRoomDB->getRoom(kPlayerRoomId)->customLines.value(qsl("e")), (QList<QPointF>{QPointF(3.5, 1.0), QPointF(4.0, 0.5)}));
    }

    void test_shrinkFromTheMenuPullsTheSelectedRoomsInTowardsTheHighlightedOne()
    {
        buildMap();
        QVERIFY(map()->setRoomCoordinates(kWestRoomId, -4, 0, 0));
        QVERIFY(map()->setRoomCoordinates(kEastRoomId, 4, 0, 0));
        showMapper(false);
        dragFromTo(pointUnitsFromCentre(-4.5, 0.5), pointUnitsFromCentre(4.5, -0.5));
        QCOMPARE(mp2dMap->mMultiSelectionHighlightRoomId, kPlayerRoomId);
        map()->resetUnsaved();
        rightClickAt(pointUnitsFromCentre(4, 0));

        QVERIFY(pickFactorItem(qsl("Shrink..."), 2));

        QCOMPARE(roomPosition(kWestRoomId), QVector3D(-2, 0, 0));
        QCOMPARE(roomPosition(kPlayerRoomId), QVector3D(0, 0, 0));
        QCOMPARE(roomPosition(kEastRoomId), QVector3D(2, 0, 0));
        QCOMPARE(roomPosition(kNorthRoomId), QVector3D(0, 1, 0));
        QVERIFY(map()->isUnsaved());
    }

    void test_cancellingTheSpreadDialogLeavesTheRoomsWhereTheyAre()
    {
        buildMap();
        showMapper(false);
        dragFromTo(pointUnitsFromCentre(-1.5, 0.5), pointUnitsFromCentre(1.5, -0.5));
        map()->resetUnsaved();
        rightClickAt(pointUnitsFromCentre(1, 0));

        QVERIFY(pickFactorItem(qsl("Spread..."), 0));

        QCOMPARE(roomPosition(kWestRoomId), QVector3D(-1, 0, 0));
        QCOMPARE(roomPosition(kEastRoomId), QVector3D(1, 0, 0));
        QVERIFY2(!map()->isUnsaved(), "cancelling the dialog left the map needing a save");
    }

    // The coordinates typed in are for the highlighted room and the rest of the
    // selection moves by the same amount, so a block of rooms keeps its shape.
    void test_moveToPositionFromTheMenuShiftsTheWholeSelectionTogether()
    {
        buildMap();
        showMapper(false);
        dragFromTo(pointUnitsFromCentre(-1.5, 0.5), pointUnitsFromCentre(1.5, -0.5));
        QCOMPARE(mp2dMap->mMultiSelectionHighlightRoomId, kPlayerRoomId);
        map()->resetUnsaved();
        rightClickAt(pointUnitsFromCentre(1, 0));

        QVERIFY(pickMoveToPosition(3, 2, 1));

        QCOMPARE(roomPosition(kWestRoomId), QVector3D(2, 2, 1));
        QCOMPARE(roomPosition(kPlayerRoomId), QVector3D(3, 2, 1));
        QCOMPARE(roomPosition(kEastRoomId), QVector3D(4, 2, 1));
        QCOMPARE(roomPosition(kNorthRoomId), QVector3D(0, 1, 0));
        QVERIFY(map()->isUnsaved());
    }

    void test_moveToPositionCarriesTheCustomLinesAlong()
    {
        buildMap();
        QVERIFY(map()->setExit(kPlayerRoomId, kEastRoomId, DIR_EAST));
        TRoom* pPlayerRoom = map()->mpRoomDB->getRoom(kPlayerRoomId);
        QVERIFY(pPlayerRoom);
        pPlayerRoom->customLines[qsl("e")] = QList<QPointF>{QPointF(0.5, 0.3), QPointF(0.8, -0.2)};
        pPlayerRoom->calcRoomDimensions();
        showMapper(false);
        clickAt(pointUnitsFromCentre(0, 0));
        rightClickAt(pointUnitsFromCentre(0, 0));

        QVERIFY(pickMoveToPosition(3, 2, 0));

        QCOMPARE(roomPosition(kPlayerRoomId), QVector3D(3, 2, 0));
        QCOMPARE(pPlayerRoom->customLines.value(qsl("e")), (QList<QPointF>{QPointF(3.5, 2.3), QPointF(3.8, 1.8)}));
        QCOMPARE(pPlayerRoom->max_x, 3.8);
        QCOMPARE(pPlayerRoom->min_y, 1.8);
    }

    // The view lands on the rooms that were moved rather than in the middle
    // of the area they went to, which may be a long way from them.
    void test_moveToAreaFromTheMenuMovesTheSelectionIntoThatAreaAndShowsIt()
    {
        buildMap();
        const int otherAreaId = map()->mpRoomDB->addArea(qsl("Next Door"));
        QVERIFY(otherAreaId > 0);
        const QSet<int> farRooms{20, 21};
        for (const int roomId : farRooms) {
            QVERIFY(map()->addRoom(roomId) && map()->setRoomArea(roomId, otherAreaId) && map()->setRoomCoordinates(roomId, roomId - 10, 10, 0));
        }
        showMapper(false);
        // The mapper's list of areas was filled before this map's areas existed.
        map()->mpMapper->updateAreaComboBox();
        dragFromTo(pointUnitsFromCentre(-1.5, 1.5), pointUnitsFromCentre(1.5, 0.5));
        const QSet<int> northRow{kNorthWestRoomId, kNorthRoomId, kNorthEastRoomId};
        QCOMPARE(mp2dMap->mMultiSelectionSet, northRow);
        map()->resetUnsaved();
        rightClickAt(pointUnitsFromCentre(1, 1));
        QVERIFY(pickContextMenuItem(qsl("Move to area...")));
        QDialog* pDialog = moveToAreaDialog();
        QVERIFY(pDialog);
        const int nextDoorIndex = mp2dMap->arealist_combobox->findData(QString::number(otherAreaId));
        QVERIFY2(nextDoorIndex >= 0, "Next Door is not offered in the list of areas");

        mp2dMap->arealist_combobox->setCurrentIndex(nextDoorIndex);
        pDialog->accept();

        QCOMPARE(roomsInArea(otherAreaId), northRow + farRooms);
        QVERIFY2(!roomsInArea(mAreaId).intersects(northRow), "the rooms are still in the area they came from");
        QCOMPARE(mp2dMap->mAreaID, otherAreaId);
        QCOMPARE(map()->mpMapper->comboBox_showArea->currentText(), qsl("Next Door"));
        QCOMPARE(mp2dMap->mMapCenterX, 0.0);
        QCOMPARE(mp2dMap->mMapCenterY, -1.0);
        QVERIFY(map()->isUnsaved());
    }

    void test_typingANewAreaNameIntoMoveToAreaMakesThatArea()
    {
        buildMap();
        showMapper(false);
        clickAt(pointUnitsFromCentre(1, 0));
        map()->resetUnsaved();
        rightClickAt(pointUnitsFromCentre(1, 0));
        QVERIFY(pickContextMenuItem(qsl("Move to area...")));
        QDialog* pDialog = moveToAreaDialog();
        QVERIFY(pDialog);

        mp2dMap->arealist_combobox->setCurrentText(qsl("Brand New"));
        pDialog->accept();

        const int newAreaId = map()->mpRoomDB->getAreaNamesMap().key(qsl("Brand New"), 0);
        QVERIFY2(newAreaId > 0, "no area called Brand New was made");
        QCOMPARE(roomsInArea(newAreaId), QSet<int>{kEastRoomId});
        QCOMPARE(mp2dMap->mAreaID, newAreaId);
        QCOMPARE(map()->mpMapper->comboBox_showArea->currentText(), qsl("Brand New"));
        QVERIFY(map()->isUnsaved());
    }
};

#include "MapMouseInteractionTest.moc"
MUDLET_GROUPED_TEST_MAIN(MapMouseInteractionTest)
