#pragma once

#include "T2DMap.h"

class QMenu;
class TArea;

class RoomContextMenuHandler : public T2DMap::IInteractionHandler
{
public:
    explicit RoomContextMenuHandler(T2DMap& mapWidget);

    bool matches(const T2DMap::MapInteractionContext& context) const override;
    bool handle(T2DMap::MapInteractionContext& context) override;

private:
    void populateEditModeActions(QMenu* menu, int selectionSize, TArea* area) const;
    void populateViewModeActions(QMenu* menu, int selectionSize) const;
    bool hasCustomLineSelection(const T2DMap::MapInteractionContext& context) const;

    T2DMap& mMapWidget;
};
