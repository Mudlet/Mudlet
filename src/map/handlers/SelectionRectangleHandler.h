#ifndef MUDLET_SELECTIONRECTANGLEHANDLER_H
#define MUDLET_SELECTIONRECTANGLEHANDLER_H

#include "T2DMap.h"

class SelectionRectangleHandler : public T2DMap::IInteractionHandler
{
public:
    explicit SelectionRectangleHandler(T2DMap& mapWidget);

    bool matches(const T2DMap::MapInteractionContext& context) const override;
    bool handle(T2DMap::MapInteractionContext& context) override;

private:
    bool handleMousePress(T2DMap::MapInteractionContext& context) const;
    bool handleMouseMove(T2DMap::MapInteractionContext& context) const;
    bool handleMouseRelease(T2DMap::MapInteractionContext& context) const;

    void populateMultiSelectionWidget() const;

    T2DMap& mMapWidget;
};

#endif // MUDLET_SELECTIONRECTANGLEHANDLER_H
