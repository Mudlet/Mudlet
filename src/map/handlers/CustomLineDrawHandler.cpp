#include "map/handlers/CustomLineDrawHandler.h"

#include "TMap.h"
#include "TRoom.h"
#include "TRoomDB.h"

#include "pre_guard.h"
#include <QEvent>
#include <QMouseEvent>
#include <QPointF>
#include "post_guard.h"

CustomLineDrawHandler::CustomLineDrawHandler(T2DMap& mapWidget)
: mMapWidget(mapWidget)
{
}

bool CustomLineDrawHandler::matches(const T2DMap::MapInteractionContext& context) const
{
    if (!context.event || !mMapWidget.mpMap) {
        return false;
    }

    if (context.event->type() != QEvent::MouseButtonPress) {
        return false;
    }

    if (context.button != Qt::LeftButton) {
        return false;
    }

    if (!context.isCustomLineDrawing) {
        return false;
    }

    if (context.customLinesRoomFrom <= 0) {
        return false;
    }

    return true;
}

bool CustomLineDrawHandler::handle(T2DMap::MapInteractionContext& context)
{
    if (!mMapWidget.mpMap || !mMapWidget.mpMap->mpRoomDB) {
        return false;
    }

    if (context.isDialogLocked) {
        return true;
    }

    TRoom* room = mMapWidget.mpMap->mpRoomDB->getRoom(context.customLinesRoomFrom);

    if (!room) {
        return false;
    }

    const float mapX = static_cast<float>(context.mapX);
    const float mapY = static_cast<float>(context.mapY);

    room->customLines[context.customLinesRoomExit].push_back(QPointF(mapX, mapY));
    room->calcRoomDimensions();
    mMapWidget.repaint();

    return true;
}
