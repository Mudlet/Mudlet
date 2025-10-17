#ifndef MUDLET_CUSTOMLINEDRAWHANDLER_H
#define MUDLET_CUSTOMLINEDRAWHANDLER_H

#include "T2DMap.h"

class CustomLineDrawHandler : public T2DMap::IInteractionHandler
{
public:
    explicit CustomLineDrawHandler(T2DMap& mapWidget);

    bool matches(const T2DMap::MapInteractionContext& context) const override;
    bool handle(T2DMap::MapInteractionContext& context) override;

private:
    T2DMap& mMapWidget;
};

#endif // MUDLET_CUSTOMLINEDRAWHANDLER_H
