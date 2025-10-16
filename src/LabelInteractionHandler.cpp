#include "LabelInteractionHandler.h"

#include "TArea.h"
#include "TMap.h"
#include "TMapLabel.h"
#include "TRoomDB.h"

#include "pre_guard.h"
#include <QMap>
#include <QMouseEvent>
#include <QMutableMapIterator>
#include <QRectF>
#include <QtGlobal>
#include "post_guard.h"

LabelInteractionHandler::LabelInteractionHandler(T2DMap& mapWidget)
: mMapWidget(mapWidget)
{
}

bool LabelInteractionHandler::matches(const T2DMap::MapInteractionContext& context) const
{
    if (!context.event || !mMapWidget.mpMap) {
        return false;
    }

    switch (context.event->type()) {
    case QEvent::MouseButtonPress:
        return context.button == Qt::LeftButton
            && !context.isMapViewOnly
            && context.area
            && !context.isCustomLineDrawing;
    case QEvent::MouseMove:
        return context.isLabelHighlighted || context.isMoveLabelActive;
    case QEvent::MouseButtonRelease:
        return context.button == Qt::LeftButton && context.isMoveLabelActive;
    default:
        return false;
    }
}

bool LabelInteractionHandler::handle(T2DMap::MapInteractionContext& context)
{
    if (!context.event) {
        return false;
    }

    switch (context.event->type()) {
    case QEvent::MouseButtonPress:
        return handleMousePress(context);
    case QEvent::MouseMove:
        return handleMouseMove(context);
    case QEvent::MouseButtonRelease:
        return handleMouseRelease(context);
    default:
        return false;
    }
}

bool LabelInteractionHandler::handleMousePress(T2DMap::MapInteractionContext& context) const
{
    if (context.button != Qt::LeftButton) {
        return false;
    }

    auto* area = context.area;
    if (!area || area->mMapLabels.isEmpty()) {
        return false;
    }

    QMutableMapIterator<int, TMapLabel> iterator(area->mMapLabels);
    while (iterator.hasNext()) {
        iterator.next();
        auto mapLabel = iterator.value();
        if (mapLabel.pos.z() != mMapWidget.mMapCenterZ) {
            continue;
        }

        const float labelX = mapLabel.pos.x() * mMapWidget.mRoomWidth + mMapWidget.mRX;
        const float labelY = mapLabel.pos.y() * mMapWidget.mRoomHeight * -1 + mMapWidget.mRY;
        const QPoint click = context.widgetPosition;
        const QRectF boundingRect(labelX, labelY, mapLabel.clickSize.width(), mapLabel.clickSize.height());
        if (boundingRect.contains(click)) {
            mapLabel.highlight = !mapLabel.highlight;
            mMapWidget.mLabelHighlighted = mapLabel.highlight;
            iterator.setValue(mapLabel);
            mMapWidget.update();
            return true;
        }
    }

    mMapWidget.mLabelHighlighted = false;
    mMapWidget.update();

    return false;
}

bool LabelInteractionHandler::handleMouseMove(T2DMap::MapInteractionContext& context) const
{
    if (!mMapWidget.mpMap || !mMapWidget.mpMap->mpRoomDB) {
        return false;
    }

    if (mMapWidget.mLabelHighlighted) {
        auto* area = mMapWidget.mpMap->mpRoomDB->getArea(mMapWidget.mAreaID);
        if (!area || area->mMapLabels.isEmpty()) {
            return false;
        }

        bool needUpdate = false;
        bool needToSave = false;
        QMapIterator<int, TMapLabel> iterator(area->mMapLabels);
        while (iterator.hasNext()) {
            iterator.next();
            auto mapLabel = iterator.value();
            if (qRound(mapLabel.pos.z()) != mMapWidget.mMapCenterZ) {
                continue;
            }
            if (!mapLabel.highlight) {
                continue;
            }

            mapLabel.pos = QVector3D(static_cast<float>(context.mapX), static_cast<float>(context.mapY), static_cast<float>(mMapWidget.mMapCenterZ));
            area->mMapLabels[iterator.key()] = mapLabel;
            needUpdate = true;
            if (!mapLabel.temporary) {
                needToSave = true;
            }
        }
        if (needUpdate) {
            mMapWidget.update();
            if (needToSave) {
                mMapWidget.mpMap->setUnsaved(__func__);
            }
        }
        return true;
    }

    if (mMapWidget.mMoveLabel) {
        mMapWidget.mMoveLabel = false;
        return false;
    }

    return false;
}

bool LabelInteractionHandler::handleMouseRelease(T2DMap::MapInteractionContext& context) const
{
    if (context.button != Qt::LeftButton) {
        return false;
    }

    if (mMapWidget.mMoveLabel) {
        mMapWidget.mMoveLabel = false;
    }

    return false;
}
