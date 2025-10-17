#include "map/handlers/RoomMoveActivationHandler.h"

#include "pre_guard.h"
#include <QMouseEvent>
#include <QRect>
#include "post_guard.h"

RoomMoveActivationHandler::RoomMoveActivationHandler(T2DMap& mapWidget)
: mMapWidget(mapWidget)
{
}

bool RoomMoveActivationHandler::matches(const T2DMap::MapInteractionContext& context) const
{
    if (!context.event) {
        return false;
    }

    if (!context.isRoomBeingMoved) {
        return false;
    }

    return context.event->type() == QEvent::MouseButtonPress
        && context.button == Qt::LeftButton;
}

bool RoomMoveActivationHandler::handle(T2DMap::MapInteractionContext& context)
{
    if (!context.event || !context.isRoomBeingMoved) {
        return false;
    }

    mMapWidget.mPopupMenu = false;
    mMapWidget.mPick = true;
    mMapWidget.setMouseTracking(false);
    mMapWidget.mRoomBeingMoved = false;
    mMapWidget.mMultiRect = QRect(0, 0, 0, 0);

    context.isRoomBeingMoved = false;

    return false;
}
