#ifndef MUDLET_ROOMMOVEACTIVATIONHANDLER_H
#define MUDLET_ROOMMOVEACTIVATIONHANDLER_H

#include "T2DMap.h"

class RoomMoveActivationHandler : public T2DMap::IInteractionHandler
{
public:
    explicit RoomMoveActivationHandler(T2DMap& mapWidget);

    bool matches(const T2DMap::MapInteractionContext& context) const override;
    bool handle(T2DMap::MapInteractionContext& context) override;

private:
    T2DMap& mMapWidget;
};

#endif // MUDLET_ROOMMOVEACTIVATIONHANDLER_H
