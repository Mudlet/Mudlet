#include "map/handlers/RoomMoveDragHandler.h"

#include "TArea.h"
#include "TRoom.h"
#include "TRoomDB.h"

#include "pre_guard.h"
#include <QMouseEvent>
#include <QPointF>
#include <QRect>
#include <QSet>
#include <QtGlobal>
#include "post_guard.h"

RoomMoveDragHandler::RoomMoveDragHandler(T2DMap& mapWidget)
: mMapWidget(mapWidget)
{
}

bool RoomMoveDragHandler::matches(const T2DMap::MapInteractionContext& context) const
{
    if (!context.event) {
        return false;
    }

    if (!context.isRoomBeingMoved || context.isSizingLabel || !context.hasMultiSelection) {
        return false;
    }

    if (!context.multiSelectionSet || context.multiSelectionSet->isEmpty()) {
        return false;
    }

    return context.event->type() == QEvent::MouseMove;
}

bool RoomMoveDragHandler::handle(T2DMap::MapInteractionContext& context)
{
    if (!context.event || context.event->type() != QEvent::MouseMove) {
        return false;
    }

    if (!context.isRoomBeingMoved || context.isSizingLabel || !context.multiSelectionSet || context.multiSelectionSet->isEmpty()) {
        return false;
    }

    if (!mMapWidget.mpMap || !mMapWidget.mpMap->mpRoomDB) {
        return false;
    }

    mMapWidget.mMultiRect = QRect(0, 0, 0, 0);

    auto* roomDb = mMapWidget.mpMap->mpRoomDB;
    if (!roomDb->getRoom(mMapWidget.mRoomID)) {
        return false;
    }

    if (!roomDb->getArea(mMapWidget.mAreaID)) {
        return false;
    }

    if (!mMapWidget.getCenterSelection()) {
        return false;
    }

    TRoom* referenceRoom = roomDb->getRoom(mMapWidget.mMultiSelectionHighlightRoomId);
    if (!referenceRoom) {
        return false;
    }

    const int dx = qRound(context.mapX) - referenceRoom->x();
    const int dy = qRound(context.mapY) - referenceRoom->y();

    QSetIterator<int> roomIterator = mMapWidget.mMultiSelectionSet;
    while (roomIterator.hasNext()) {
        TRoom* room = roomDb->getRoom(roomIterator.next());
        if (!room) {
            continue;
        }

        room->offset(dx, dy, 0);

        QMapIterator<QString, QList<QPointF>> customLineIterator(room->customLines);
        QMap<QString, QList<QPointF>> updatedLines;
        while (customLineIterator.hasNext()) {
            customLineIterator.next();
            QList<QPointF> points = customLineIterator.value();
            for (auto& point : points) {
                const QPointF originalPoint = point;
                point.setX(static_cast<float>(originalPoint.x() + dx));
                point.setY(static_cast<float>(originalPoint.y() + dy));
            }
            updatedLines.insert(customLineIterator.key(), points);
        }
        room->customLines = updatedLines;
        room->calcRoomDimensions();
    }

    mMapWidget.repaint();
    mMapWidget.mpMap->setUnsaved(__func__);

    return true;
}
