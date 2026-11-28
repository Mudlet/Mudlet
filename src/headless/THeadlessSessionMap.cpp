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

#include "THeadlessSession.h"

#include "Host.h"
#include "TArea.h"
#include "TMap.h"
#include "TMapViewsFrontend.h"
#include "TNullConsoleFrontend.h"
#include "TNullMapView.h"
#include "TRoom.h"
#include "TRoomDB.h"

#include <QPainter>
#include <QQueue>
#include <QStringMatcher>

#include <algorithm>

namespace {

// The room the map takes as the player's, as paint() and TMapPainter take it; 0 for none
int playerRoomOf(const TMap* pMap)
{
    const int roomId = pMap->mRoomIdHash.value(pMap->mProfileName);
    return pMap->mpRoomDB->getRoom(roomId) ? roomId : 0;
}

// The fewest exits from the room to each it reaches, as TMap::findPath() goes: by no locked exit and into no
// locked room
QHash<int, int> stepsFrom(TRoomDB* pRoomDB, const int fromId)
{
    QHash<int, int> steps;
    if (!pRoomDB->getRoom(fromId)) {
        return steps;
    }
    steps.insert(fromId, 0);
    QQueue<int> queue;
    queue.enqueue(fromId);
    while (!queue.isEmpty()) {
        const int roomId = queue.dequeue();
        const TRoom* pRoom = pRoomDB->getRoom(roomId);
        const int next = steps.value(roomId) + 1;
        const auto reach = [&](const int toId) {
            const TRoom* pTo = pRoomDB->getRoom(toId);
            if (pTo && !pTo->isLocked && !steps.contains(toId)) {
                steps.insert(toId, next);
                queue.enqueue(toId);
            }
        };
        for (int direction = DIR_NORTH; direction <= DIR_OUT; ++direction) {
            if (!pRoom->hasExitLock(direction)) {
                reach(pRoom->getExit(direction));
            }
        }
        const QMap<QString, int>& specialExits = pRoom->getSpecialExits();
        for (auto it = specialExits.cbegin(); it != specialExits.cend(); ++it) {
            if (!pRoom->hasSpecialExitLock(it.key())) {
                reach(it.value());
            }
        }
    }
    return steps;
}

} // namespace

bool THeadlessSession::renderMap(QPainter& painter, const QSize& size)
{
    if (!usable()) {
        return false;
    }
    auto* pView = dynamic_cast<TNullMapView*>(mpHost->mapViewFrontend());
    if (!pView) {
        //: Error from a frontend with no window that was asked to draw the 2D map before a script opened one
        mErrorString = tr("No map is open; a script opens one, as with openMapWidget().");
        return false;
    }
    if (!painter.isActive()) {
        //: Error from a frontend with no window that was asked to draw the 2D map with a painter that has nothing to paint on
        mErrorString = tr("The painter is not active.");
        return false;
    }
    // T2DMap draws nothing smaller
    if (size.width() < 10 || size.height() < 10) {
        //: Error from a frontend with no window that was asked to draw the 2D map smaller than it can be drawn; %1 and %2 are the width and height asked for
        mErrorString = tr("The map is drawn at 10 by 10 pixels or more, not %1 by %2.").arg(size.width()).arg(size.height());
        return false;
    }
    pView->render(painter, size);
    return true;
}

bool THeadlessSession::mapPointer(const MapPointerEvent& event)
{
    if (!usable()) {
        return false;
    }
    return pointMap(event, event.button, std::nullopt);
}

bool THeadlessSession::mapViewOnly() const
{
    return !mpHost || mpHost->mMapViewOnly;
}

bool THeadlessSession::setMapViewOnly(const bool viewOnly)
{
    if (!usable()) {
        return false;
    }
    auto* pView = dynamic_cast<TNullMapView*>(mpHost->mapViewFrontend());
    const bool mainShown = pView && pView->shown();
    // The app's map has the item too, as the GUI's main window always has its mapper
    const bool appShown = mpAppMap && mpAppMap->parent() == mpHost;
    // The GUI's item is on a secondary map view as well
    if (!mainShown && !appShown && (!mpHost->mpMap || mpHost->mpMap->mapViewsFrontend()->getViewIds().isEmpty())) {
        //: Error from a frontend with no window that was asked to switch the 2D map between viewing and editing before a script opened one, or after it closed or hid it
        mErrorString = tr("No map is open; a script opens one, as with openMapWidget().");
        return false;
    }
    if (mpHost->mMapViewOnly == viewOnly) {
        return true;
    }
    const QPointer<THeadlessSession> self(this);
    const QPointer<Host> pHost(mpHost);
    const QString profile = mpHost->getName();
    if (mainShown) {
        pView->setViewOnly(viewOnly);
    } else if (appShown) {
        mpAppMap->setViewOnly(viewOnly);
    } else {
        TNullMapView::switchMode(mpHost, viewOnly);
    }
    if (self && pHost && mpHost == pHost && !pHost->isClosingDown()) {
        emit mapViewOnlyChanged(profile, viewOnly);
    }
    return true;
}

bool THeadlessSession::pointMapper(const ScriptWindow& mapper, const WindowPointerEvent& event)
{
    MapPointerEvent passed{.position = event.position, .button = event.button, .modifiers = event.modifiers, .angleDelta = event.angleDelta};
    switch (event.type) {
    case WindowPointerEvent::Type::press:
        passed.type = MapPointerEvent::Type::press;
        break;
    case WindowPointerEvent::Type::doubleClick:
        passed.type = MapPointerEvent::Type::doubleClick;
        break;
    case WindowPointerEvent::Type::wheel:
        passed.type = MapPointerEvent::Type::wheel;
        break;
    case WindowPointerEvent::Type::move:
        // T2DMap pans, sizes a selection or moves rooms with the left button and pans with the middle one; with
        // another it moves the label its press took, when the map is being edited
        if (const bool drags = event.buttons.testAnyFlags(Qt::LeftButton | Qt::MiddleButton) || (event.buttons != Qt::NoButton && !mpHost->mMapViewOnly); !drags) {
            return true;
        }
        //: Error from a frontend with no window that was given a drag of the mouse, a move with a button held, on the 2D map
        mErrorString = tr("The map's drags are not available with no main window.");
        return false;
    default:
        return true;
    }
    return pointMap(passed, event.buttons | event.button, mapper.geometry.size());
}

bool THeadlessSession::pointMap(const MapPointerEvent& event, const Qt::MouseButtons buttons, const std::optional<QSize>& windowSize)
{
    auto* pView = dynamic_cast<TNullMapView*>(mpHost->mapViewFrontend());
    if (!pView) {
        //: Error from a frontend with no window that was given a mouse action on the 2D map before a script opened one
        mErrorString = tr("No map is open; a script opens one, as with openMapWidget().");
        return false;
    }
    if (!pView->rendered()) {
        //: Error from a frontend with no window that was given a mouse action on the 2D map before it had drawn the map, which the action's position is in
        mErrorString = tr("The map has not been drawn yet; the position is in what renderMap() drew.");
        return false;
    }
    return pointView(*pView, event, buttons, windowSize, true);
}

bool THeadlessSession::pointView(TNullMapView& view, const MapPointerEvent& event, const Qt::MouseButtons buttons, const std::optional<QSize>& windowSize, const bool scriptMap)
{
    auto* pView = &view;
    if (const QSize drawn = pView->renderedSize(); windowSize && drawn != *windowSize) {
        //: Error from a frontend with no window that was given a mouse action on the map's window after a script resized that window and before the map was drawn again; %1 and %2 are the width and height the map was last drawn at, %3 and %4 those of its window
        mErrorString = tr("The map was last drawn at %1 by %2 pixels and its window is %3 by %4; the position is in what renderMap() drew.")
                               .arg(drawn.width())
                               .arg(drawn.height())
                               .arg(windowSize->width())
                               .arg(windowSize->height());
        return false;
    }
    // Before the press, as what it raises may ask getMousePosition()
    if (scriptMap && !windowSize) {
        noteMapPointer(event.position);
    }
    switch (event.type) {
    case MapPointerEvent::Type::press:
        pView->press(event.position.toPoint(), event.button, event.modifiers);
        return true;
    case MapPointerEvent::Type::doubleClick:
        // T2DMap::mouseDoubleClickEvent() asks for the left button and no other held
        if (buttons != Qt::LeftButton) {
            if (windowSize) {
                return true;
            }
            //: Error from a frontend with no window that was given a double-click on the 2D map with a mouse button other than the left one
            mErrorString = tr("Only a double-click of the left button acts on the map.");
            return false;
        }
        pView->doubleClick(event.position.toPoint());
        return true;
    case MapPointerEvent::Type::wheel:
        if (!pView->showsArea()) {
            if (windowSize) {
                return true;
            }
            //: Error from a frontend with no window that was given a turn of the mouse wheel on the 2D map while the map it last drew showed no area, as for a map with no rooms
            mErrorString = tr("The map drawn last shows no area to zoom.");
            return false;
        }
        if (!pView->wheel(event.position, event.angleDelta, event.modifiers) && !windowSize) {
            //: Error from a frontend with no window that was given a turn of the mouse wheel on the 2D map too small to zoom it
            mErrorString = tr("Less than half a notch of the wheel does not zoom the map.");
            return false;
        }
        return true;
    }
    return false;
}

TNullMapView* THeadlessSession::appMap()
{
    if (!mpHost) {
        return nullptr;
    }
    // One a closed profile left is gone with it; a profile opened since has none yet
    if (!mpAppMap || mpAppMap->parent() != mpHost) {
        mpAppMap = new TNullMapView(mpHost, mpHost, false);
        mpAppMap->setShown(true);
        if (auto* pFrontend = dynamic_cast<TNullConsoleFrontend*>(mpHost->consoleFrontend())) {
            pFrontend->mAppMapOpened = true;
        }
    }
    return mpAppMap;
}

bool THeadlessSession::renderAppMap(QPainter& painter, const QSize& size)
{
    if (!usable()) {
        return false;
    }
    if (!painter.isActive()) {
        //: Error from a frontend with no window that was asked to draw the 2D map with a painter that has nothing to paint on
        mErrorString = tr("The painter is not active.");
        return false;
    }
    if (size.width() < 10 || size.height() < 10) {
        //: Error from a frontend with no window that was asked to draw the 2D map smaller than it can be drawn; %1 and %2 are the width and height asked for
        mErrorString = tr("The map is drawn at 10 by 10 pixels or more, not %1 by %2.").arg(size.width()).arg(size.height());
        return false;
    }
    // Made first, so the map's first rooms ask for a repaint as for the script's map
    TNullMapView* pView = appMap();
    if (!mpHost->mpMap->mpRoomDB || mpHost->mpMap->mpRoomDB->isEmpty()) {
        //: Error from a frontend with no window that was asked to draw the app's own 2D map for a profile whose map has no rooms
        mErrorString = tr("The map has no rooms.");
        return false;
    }
    pView->render(painter, size);
    return true;
}

bool THeadlessSession::appMapPointer(const MapPointerEvent& event)
{
    if (!usable()) {
        return false;
    }
    if (!mpAppMap || mpAppMap->parent() != mpHost || !mpAppMap->rendered()) {
        //: Error from a frontend with no window that was given a mouse action on its own 2D map before it had drawn that map, which the action's position is in
        mErrorString = tr("The app's map has not been drawn yet; the position is in what renderAppMap() drew.");
        return false;
    }
    return pointView(*mpAppMap, event, event.button, std::nullopt, false);
}

bool THeadlessSession::showRoomOnAppMap(const int roomId)
{
    if (!usable()) {
        return false;
    }
    if (!appMap()->showRoom(roomId)) {
        //: Error from a frontend with no window that was asked to show a room on its own 2D map that is not in the map; %1 is the room's id
        mErrorString = tr("There is no room %1 in the map.").arg(roomId);
        return false;
    }
    return true;
}

void THeadlessSession::closeAppMap()
{
    delete mpAppMap.data();
}

TNullMapView* THeadlessSession::drawnMap(const MapTarget map)
{
    if (map == MapTarget::app) {
        if (!mpAppMap || mpAppMap->parent() != mpHost || !mpAppMap->rendered()) {
            //: Error from a frontend with no window that was asked to move its own 2D map before it had drawn that map
            mErrorString = tr("The app's map has not been drawn yet; it moves what renderAppMap() drew.");
            return nullptr;
        }
        return mpAppMap;
    }
    auto* pView = dynamic_cast<TNullMapView*>(mpHost->mapViewFrontend());
    if (!pView) {
        //: Error from a frontend with no window that was asked to move the 2D map before a script opened one
        mErrorString = tr("No map is open; a script opens one, as with openMapWidget().");
        return nullptr;
    }
    if (!pView->rendered()) {
        //: Error from a frontend with no window that was asked to move the 2D map before it had drawn the map
        mErrorString = tr("The map has not been drawn yet; it moves what renderMap() drew.");
        return nullptr;
    }
    return pView;
}

bool THeadlessSession::moveMapView(const MapTarget map, const QPointF& pan, const qreal zoom, const QPointF& about)
{
    if (!usable()) {
        return false;
    }
    TNullMapView* pView = drawnMap(map);
    if (!pView) {
        return false;
    }
    if (!(zoom > 0.0) || !qIsFinite(zoom) || !qIsFinite(pan.x()) || !qIsFinite(pan.y()) || !qIsFinite(about.x()) || !qIsFinite(about.y())) {
        //: Error from a frontend with no window that was asked to zoom the 2D map by a factor that is not a number above 0, or to move it by a distance that is not a number
        mErrorString = tr("The map moves by a distance and zooms by a factor above 0, both numbers.");
        return false;
    }
    if (!pView->showsArea() || !pView->move(pan, zoom, about)) {
        //: Error from a frontend with no window that was asked to move the 2D map while the map it last drew showed no area, as for a map with no rooms
        mErrorString = tr("The map drawn last shows no area to move.");
        return false;
    }
    return true;
}

bool THeadlessSession::followPlayerOnMap(const MapTarget map)
{
    if (!usable()) {
        return false;
    }
    TNullMapView* pView = drawnMap(map);
    if (!pView) {
        return false;
    }
    if (!pView->follow()) {
        //: Error from a frontend with no window that was asked to centre the 2D map on the player while the map has no room for the player
        mErrorString = tr("The map has no room the player is in.");
        return false;
    }
    return true;
}

bool THeadlessSession::showMapLevel(const MapTarget map, const int level)
{
    if (!usable()) {
        return false;
    }
    TNullMapView* pView = drawnMap(map);
    if (!pView) {
        return false;
    }
    if (!pView->showsArea()) {
        //: Error from a frontend with no window that was asked to show another level of the 2D map while the map it last drew showed no area, as for a map with no rooms
        mErrorString = tr("The map drawn last shows no area to move.");
        return false;
    }
    if (!pView->showLevel(level)) {
        //: Error from a frontend with no window that was asked to show a level of the 2D map's area that has no rooms; %1 is the level
        mErrorString = tr("The area shown has no rooms on level %1.").arg(level);
        return false;
    }
    return true;
}

bool THeadlessSession::showMapArea(const MapTarget map, const int areaId)
{
    if (!usable()) {
        return false;
    }
    TNullMapView* pView = drawnMap(map);
    if (!pView) {
        return false;
    }
    if (!pView->showArea(areaId)) {
        //: Error from a frontend with no window that was asked to show an area on the 2D map that is not in the map or has no rooms; %1 is the area's id
        mErrorString = tr("There is no area %1 with rooms in the map.").arg(areaId);
        return false;
    }
    return true;
}

bool THeadlessSession::walkToRoom(const int roomId)
{
    if (!usable()) {
        return false;
    }
    TMap* pMap = mpHost->mpMap.data();
    if (!pMap->mpRoomDB->getRoom(roomId)) {
        //: Error from a frontend with no window that was asked to walk to a room that is not in the 2D map; %1 is the room's id
        mErrorString = tr("There is no room %1 in the map.").arg(roomId);
        return false;
    }
    const int playerRoomId = playerRoomOf(pMap);
    if (!playerRoomId) {
        //: Error from a frontend with no window that was asked to walk to a room while the 2D map has no room the player is in
        mErrorString = tr("The map has no room the player is in.");
        return false;
    }
    TMapPainter::startSpeedWalk(mpHost, pMap, playerRoomId, roomId);
    return true;
}

std::optional<THeadlessSession::MapView> THeadlessSession::mapView(const MapTarget map) const
{
    if (!mpHost || !mpHost->mpMap || !mpHost->mpMap->mpRoomDB) {
        return std::nullopt;
    }
    const TNullMapView* pView = map == MapTarget::app ? (mpAppMap && mpAppMap->parent() == mpHost ? mpAppMap.data() : nullptr) : dynamic_cast<TNullMapView*>(mpHost->mapViewFrontend());
    if (!pView || !pView->showsArea()) {
        return std::nullopt;
    }
    MapView view{.areaId = pView->drawnAreaId(),
                 .areaName = mpHost->mpMap->mpRoomDB->getAreaNamesMap().value(pView->drawnAreaId()),
                 .level = pView->drawnLevel(),
                 .zoom = pView->drawnZoom(),
                 .minZoom = TMap::scmMinXYZoom,
                 .maxZoom = pView->maxZoom(),
                 .following = pView->following(),
                 .playerRoomId = pView->playerRoomId(),
                 .roomSize = pView->drawnRoomSize()};
    for (const TNullMapView::DrawnRoom& room : pView->drawnRooms()) {
        view.rooms.append(MapRoom{.id = room.id, .position = room.position, .name = room.name});
    }
    view.levels = pView->drawnLevels();
    return view;
}

QList<THeadlessSession::RoomMatch> THeadlessSession::findRooms(const QString& text) const
{
    QList<RoomMatch> found;
    if (!mpHost || !mpHost->mpMap || !mpHost->mpMap->mpRoomDB || text.isEmpty()) {
        return found;
    }
    const TRoomDB* pRoomDB = mpHost->mpMap->mpRoomDB.get();
    const QMap<int, QString>& areaNames = pRoomDB->getAreaNamesMap();
    bool isId = false;
    const int id = text.trimmed().toInt(&isId);
    // As searchRoom(): the matcher prepares the search once, but never finds what is outside the BMP ignoring case
    const bool beyondBmp = std::any_of(text.cbegin(), text.cend(), [](const QChar c) {
        return c.isSurrogate();
    });
    const QStringMatcher matcher(beyondBmp ? QString() : text, Qt::CaseInsensitive);
    for (const TRoom* pRoom : pRoomDB->getRoomMap()) {
        if (!pRoom) {
            continue;
        }
        const bool matches = (isId && pRoom->getId() == id) || (beyondBmp ? pRoom->name.contains(text, Qt::CaseInsensitive) : matcher.indexIn(pRoom->name) != -1);
        if (matches) {
            found.append(RoomMatch{.id = pRoom->getId(), .name = pRoom->name, .areaId = pRoom->getArea(), .areaName = areaNames.value(pRoom->getArea())});
        }
    }
    if (found.isEmpty()) {
        return found;
    }
    TRoomDB* pRooms = mpHost->mpMap->mpRoomDB.get();
    const int playerRoomId = playerRoomOf(mpHost->mpMap.data());
    const TRoom* pPlayerRoom = pRooms->getRoom(playerRoomId);
    const QHash<int, int> steps = stepsFrom(pRooms, playerRoomId);
    for (RoomMatch& match : found) {
        match.inPlayerArea = pPlayerRoom && match.areaId == pPlayerRoom->getArea();
        match.steps = steps.value(match.id, -1);
    }
    return found;
}

std::optional<THeadlessSession::RoomInfo> THeadlessSession::roomInfo(const int roomId) const
{
    if (!mpHost || !mpHost->mpMap || !mpHost->mpMap->mpRoomDB) {
        return std::nullopt;
    }
    TRoomDB* pRoomDB = mpHost->mpMap->mpRoomDB.get();
    const TRoom* pRoom = pRoomDB->getRoom(roomId);
    if (!pRoom) {
        return std::nullopt;
    }
    RoomInfo info{.id = roomId,
                  .name = pRoom->name,
                  .areaId = pRoom->getArea(),
                  .areaName = pRoomDB->getAreaNamesMap().value(pRoom->getArea()),
                  .x = pRoom->x(),
                  .y = pRoom->y(),
                  .z = pRoom->z(),
                  .steps = stepsFrom(pRoomDB, playerRoomOf(mpHost->mpMap.data())).value(roomId, -1)};
    const auto exit = [pRoomDB](const QString& direction, const int toId, const bool locked) {
        const TRoom* pTo = pRoomDB->getRoom(toId);
        return RoomExit{.direction = direction, .to = toId, .toName = pTo ? pTo->name : QString(), .locked = locked};
    };
    for (int direction = DIR_NORTH; direction <= DIR_OUT; ++direction) {
        if (const int toId = pRoom->getExit(direction); toId > 0) {
            info.exits.append(exit(TRoom::dirCodeToShortString(direction), toId, pRoom->hasExitLock(direction)));
        }
    }
    const QMap<QString, int>& specialExits = pRoom->getSpecialExits();
    for (auto it = specialExits.cbegin(); it != specialExits.cend(); ++it) {
        info.exits.append(exit(it.key(), it.value(), pRoom->hasSpecialExitLock(it.key())));
    }
    return info;
}

QList<THeadlessSession::MapArea> THeadlessSession::mapAreas() const
{
    QList<MapArea> areas;
    if (!mpHost || !mpHost->mpMap || !mpHost->mpMap->mpRoomDB) {
        return areas;
    }
    TRoomDB* pRoomDB = mpHost->mpMap->mpRoomDB.get();
    const QMap<int, QString>& names = pRoomDB->getAreaNamesMap();
    for (auto it = names.cbegin(); it != names.cend(); ++it) {
        const TArea* pArea = pRoomDB->getArea(it.key());
        if (pArea && !pArea->getAreaRooms().isEmpty()) {
            areas.append(MapArea{.id = it.key(), .name = it.value()});
        }
    }
    std::sort(areas.begin(), areas.end(), [](const MapArea& a, const MapArea& b) {
        const int byName = a.name.compare(b.name, Qt::CaseInsensitive);
        return byName != 0 ? byName < 0 : a.id < b.id;
    });
    return areas;
}

// Gathered into one a turn, as the GUI's update() calls are into one repaint
void THeadlessSession::relayMapRepaints(Host* pHost, const unsigned int run)
{
    auto* pFrontend = dynamic_cast<TNullConsoleFrontend*>(pHost->consoleFrontend());
    if (!pFrontend) {
        return;
    }
    // Held by the profile, which may outlive the session
    pFrontend->mMapRepaintRequested = [self = QPointer<THeadlessSession>(this), run]() {
        if (!self || run != self->mRun || self->mMapRepaintQueuedRun == run) {
            return;
        }
        self->mMapRepaintQueuedRun = run;
        QMetaObject::invokeMethod(
                self.data(),
                [session = self.data(), run]() {
                    // A later run's cue may be queued behind this one
                    if (session->mMapRepaintQueuedRun == run) {
                        session->mMapRepaintQueuedRun = 0;
                    }
                    if (run == session->mRun) {
                        const QString profile = session->mProfileName;
                        emit session->mapRepaintRequested(profile);
                    }
                },
                Qt::QueuedConnection);
    };
}
