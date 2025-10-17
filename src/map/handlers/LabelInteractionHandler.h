#ifndef MUDLET_LABELINTERACTIONHANDLER_H
#define MUDLET_LABELINTERACTIONHANDLER_H

#include "T2DMap.h"

class LabelInteractionHandler : public T2DMap::IInteractionHandler
{
public:
    explicit LabelInteractionHandler(T2DMap& mapWidget);

    bool matches(const T2DMap::MapInteractionContext& context) const override;
    bool handle(T2DMap::MapInteractionContext& context) override;

private:
    bool handleMousePress(T2DMap::MapInteractionContext& context) const;
    bool handleMouseMove(T2DMap::MapInteractionContext& context) const;
    bool handleMouseRelease(T2DMap::MapInteractionContext& context) const;

    T2DMap& mMapWidget;
};

#endif // MUDLET_LABELINTERACTIONHANDLER_H
