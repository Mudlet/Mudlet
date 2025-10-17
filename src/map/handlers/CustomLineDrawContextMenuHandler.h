#pragma once

#include "T2DMap.h"

class CustomLineDrawContextMenuHandler : public T2DMap::IInteractionHandler
{
public:
    explicit CustomLineDrawContextMenuHandler(T2DMap& mapWidget);

    bool matches(const T2DMap::MapInteractionContext& context) const override;
    bool handle(T2DMap::MapInteractionContext& context) override;

private:
    T2DMap& mMapWidget;
};
