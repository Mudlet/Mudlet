/***************************************************************************
 *   Copyright (C) 2025 by Piotr Wilczynski - delwing@gmail.com            *
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

#include "CustomLineSession.h"

#include "T2DMap.h"
#include "TMap.h"
#include "TRoom.h"
#include "TRoomDB.h"

#include <cmath>

CustomLineSession::CustomLineSession(T2DMap& mapWidget)
: mMapWidget(mapWidget)
{
}

bool CustomLineSession::isSnapToGridEnabled() const
{
    return mSnapToGridEnabled;
}

void CustomLineSession::setSnapToGridEnabled(bool enabled)
{
    if (mSnapToGridEnabled == enabled) {
        return;
    }

    if (enabled) {
        restoreOriginalLineIfNeeded();
        const auto key = currentLineKey();
        if (!key) {
            mSnapToGridEnabled = enabled;
            return;
        }

        TRoom* room = nullptr;
        QList<QPointF>* points = resolveLinePoints(*key, room);
        if (!points || !room) {
            mSnapToGridEnabled = enabled;
            return;
        }

        rememberOriginalLine(*key, *points);
        snapLineToGrid(*key, *points, *room);
    } else {
        restoreOriginalLineIfNeeded();
        mSnapToGridEnabled = enabled;
        if (mMapWidget.mpMap) {
            mMapWidget.repaint();
        }
        mOriginalLine.reset();
        return;
    }

    mSnapToGridEnabled = enabled;
}

QPointF CustomLineSession::snapPointToGrid(const QPointF& point) const
{
    constexpr qreal snapIncrement = 0.5;
    const qreal snappedX = std::round(point.x() / snapIncrement) * snapIncrement;
    const qreal snappedY = std::round(point.y() / snapIncrement) * snapIncrement;
    return QPointF(snappedX, snappedY);
}

bool CustomLineSession::canMoveSelectedCustomLineLastPointToTargetRoom() const
{
    if (!mMapWidget.mpMap || !mMapWidget.mpMap->mpRoomDB) {
        return false;
    }

    if (mMapWidget.mCustomLineSelectedRoom <= 0 || mMapWidget.mCustomLineSelectedExit.isEmpty()) {
        return false;
    }

    TRoom* room = mMapWidget.mpMap->mpRoomDB->getRoom(mMapWidget.mCustomLineSelectedRoom);
    if (!room) {
        return false;
    }

    return canMoveCustomLineLastPointToTargetRoom(*room, mMapWidget.mCustomLineSelectedExit);
}

bool CustomLineSession::canMoveCustomLineLastPointToTargetRoom(const TRoom& room, const QString& exitKey) const
{
    if (!mMapWidget.mpMap || !mMapWidget.mpMap->mpRoomDB) {
        return false;
    }

    const auto itLine = room.customLines.constFind(exitKey);
    if (itLine == room.customLines.constEnd()) {
        return false;
    }

    const QList<QPointF>& points = itLine.value();
    if (points.isEmpty()) {
        return false;
    }

    const auto targetRoomId = resolveCustomLineTargetRoomId(room, exitKey);
    if (!targetRoomId || *targetRoomId <= 0) {
        return false;
    }

    return mMapWidget.mpMap->mpRoomDB->getRoom(*targetRoomId) != nullptr;
}

void CustomLineSession::moveCustomLineLastPointToTargetRoom()
{
    if (!mMapWidget.mpMap || !mMapWidget.mpMap->mpRoomDB) {
        return;
    }

    const auto key = currentLineKey();
    if (!key) {
        return;
    }

    TRoom* room = nullptr;
    QList<QPointF>* points = resolveLinePoints(*key, room);
    if (!points || !room || points->isEmpty()) {
        return;
    }

    const auto targetRoomId = resolveCustomLineTargetRoomId(*room, key->exitKey);
    if (!targetRoomId || *targetRoomId <= 0) {
        return;
    }

    TRoom* targetRoom = mMapWidget.mpMap->mpRoomDB->getRoom(*targetRoomId);
    if (!targetRoom) {
        return;
    }

    QPointF newPoint(targetRoom->x(), targetRoom->y());
    if (mSnapToGridEnabled) {
        newPoint = snapPointToGrid(newPoint);
    }

    QPointF& lastPoint = points->last();
    if (qFuzzyIsNull(lastPoint.x() - newPoint.x()) && qFuzzyIsNull(lastPoint.y() - newPoint.y())) {
        return;
    }

    lastPoint = newPoint;
    room->calcRoomDimensions();
    mMapWidget.repaint();
    if (mMapWidget.mpMap) {
        mMapWidget.mpMap->setUnsaved(__func__);
    }
}

void CustomLineSession::clearOriginalPoints()
{
    mOriginalLine.reset();
}

std::optional<CustomLineSession::LineKey> CustomLineSession::currentLineKey() const
{
    if (mMapWidget.mCustomLineSelectedRoom > 0 && !mMapWidget.mCustomLineSelectedExit.isEmpty()) {
        return LineKey { mMapWidget.mCustomLineSelectedRoom, mMapWidget.mCustomLineSelectedExit };
    }

    if (mMapWidget.mCustomLinesRoomFrom > 0 && !mMapWidget.mCustomLinesRoomExit.isEmpty()) {
        return LineKey { mMapWidget.mCustomLinesRoomFrom, mMapWidget.mCustomLinesRoomExit };
    }

    return std::nullopt;
}

QList<QPointF>* CustomLineSession::resolveLinePoints(const LineKey& key, TRoom*& room) const
{
    if (!key.isValid() || !mMapWidget.mpMap || !mMapWidget.mpMap->mpRoomDB) {
        return nullptr;
    }

    room = mMapWidget.mpMap->mpRoomDB->getRoom(key.roomId);
    if (!room) {
        return nullptr;
    }

    auto itLine = room->customLines.find(key.exitKey);
    if (itLine == room->customLines.end()) {
        return nullptr;
    }

    return &itLine.value();
}

void CustomLineSession::rememberOriginalLine(const LineKey& key, const QList<QPointF>& points)
{
    if (!key.isValid()) {
        mOriginalLine.reset();
        return;
    }

    mOriginalLine = OriginalLine { key, points };
}

void CustomLineSession::snapLineToGrid(LineKey key, QList<QPointF>& points, TRoom& room)
{
    if (!key.isValid()) {
        return;
    }

    bool changed = false;
    for (QPointF& point : points) {
        const QPointF snapped = snapPointToGrid(point);
        if (!qFuzzyIsNull(point.x() - snapped.x()) || !qFuzzyIsNull(point.y() - snapped.y())) {
            point = snapped;
            changed = true;
        }
    }

    if (changed) {
        room.calcRoomDimensions();
        mMapWidget.repaint();
        if (mMapWidget.mpMap) {
            mMapWidget.mpMap->setUnsaved(__func__);
        }
    }
}

void CustomLineSession::restoreOriginalLineIfNeeded()
{
    if (!mOriginalLine) {
        return;
    }

    const auto key = currentLineKey();
    if (!key || !(mOriginalLine->key == *key)) {
        mOriginalLine.reset();
        return;
    }

    TRoom* room = nullptr;
    QList<QPointF>* points = resolveLinePoints(mOriginalLine->key, room);
    if (!points || !room) {
        mOriginalLine.reset();
        return;
    }

    if (*points == mOriginalLine->points) {
        mOriginalLine.reset();
        return;
    }

    *points = mOriginalLine->points;
    room->calcRoomDimensions();
    mMapWidget.repaint();
    if (mMapWidget.mpMap) {
        mMapWidget.mpMap->setUnsaved(__func__);
    }

    mOriginalLine.reset();
}

std::optional<int> CustomLineSession::resolveCustomLineTargetRoomId(const TRoom& room, const QString& exitKey) const
{
    if (exitKey == QLatin1String("nw")) {
        return room.getNorthwest();
    }
    if (exitKey == QLatin1String("n")) {
        return room.getNorth();
    }
    if (exitKey == QLatin1String("ne")) {
        return room.getNortheast();
    }
    if (exitKey == QLatin1String("up")) {
        return room.getUp();
    }
    if (exitKey == QLatin1String("w")) {
        return room.getWest();
    }
    if (exitKey == QLatin1String("e")) {
        return room.getEast();
    }
    if (exitKey == QLatin1String("down")) {
        return room.getDown();
    }
    if (exitKey == QLatin1String("sw")) {
        return room.getSouthwest();
    }
    if (exitKey == QLatin1String("s")) {
        return room.getSouth();
    }
    if (exitKey == QLatin1String("se")) {
        return room.getSoutheast();
    }
    if (exitKey == QLatin1String("in")) {
        return room.getIn();
    }
    if (exitKey == QLatin1String("out")) {
        return room.getOut();
    }

    const auto& specialExits = room.getSpecialExits();
    const auto itSpecial = specialExits.constFind(exitKey);
    if (itSpecial != specialExits.constEnd()) {
        return itSpecial.value();
    }

    return std::nullopt;
}

