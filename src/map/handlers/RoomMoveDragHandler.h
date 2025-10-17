#ifndef MUDLET_ROOMMOVEDRAGHANDLER_H
#define MUDLET_ROOMMOVEDRAGHANDLER_H

#include "T2DMap.h"

class RoomMoveDragHandler : public T2DMap::IInteractionHandler
{
public:
    explicit RoomMoveDragHandler(T2DMap& mapWidget);

    bool matches(const T2DMap::MapInteractionContext& context) const override;
    bool handle(T2DMap::MapInteractionContext& context) override;

private:
    T2DMap& mMapWidget;
};

#endif // MUDLET_ROOMMOVEDRAGHANDLER_H
