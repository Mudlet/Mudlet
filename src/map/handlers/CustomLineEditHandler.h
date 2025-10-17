#ifndef MUDLET_CUSTOMLINEEDITHANDLER_H
#define MUDLET_CUSTOMLINEEDITHANDLER_H

#include "T2DMap.h"

class CustomLineEditHandler : public T2DMap::IInteractionHandler
{
public:
    explicit CustomLineEditHandler(T2DMap& mapWidget);

    bool matches(const T2DMap::MapInteractionContext& context) const override;
    bool handle(T2DMap::MapInteractionContext& context) override;

private:
    bool handleMousePress(T2DMap::MapInteractionContext& context);
    bool handleMouseMove(T2DMap::MapInteractionContext& context);
    bool handleMouseRelease();
    void resetSelection();

    T2DMap& mMapWidget;
};

#endif // MUDLET_CUSTOMLINEEDITHANDLER_H
