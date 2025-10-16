#ifndef MUDLET_PANINTERACTIONHANDLER_H
#define MUDLET_PANINTERACTIONHANDLER_H

#include "T2DMap.h"

class PanInteractionHandler : public T2DMap::IInteractionHandler
{
public:
    explicit PanInteractionHandler(T2DMap& mapWidget);

    bool matches(const T2DMap::MapInteractionContext& context) const override;
    bool handle(T2DMap::MapInteractionContext& context) override;

private:
    bool handleMousePress(T2DMap::MapInteractionContext& context) const;
    bool handleMouseMove(T2DMap::MapInteractionContext& context) const;
    bool handleMouseRelease(T2DMap::MapInteractionContext& context) const;

    T2DMap& mMapWidget;
};

#endif // MUDLET_PANINTERACTIONHANDLER_H

