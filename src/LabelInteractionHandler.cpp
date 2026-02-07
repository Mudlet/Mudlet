/***************************************************************************
 *   Copyright (C) 2025 by Piotr Wilczynski - delwing@gmail.com            *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#include "LabelInteractionHandler.h"

#include "TArea.h"
#include "TMap.h"
#include "TMapLabel.h"
#include "TRoomDB.h"

#include <QMap>
#include <QAction>
#include <QMenu>
#include <QMouseEvent>
#include <QMutableMapIterator>
#include <QRectF>
#include <QtGlobal>

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
        if (!context.area || context.isCustomLineDrawing) {
            return false;
        }

        if (context.button == Qt::LeftButton) {
            return !context.isMapViewOnly;
        }

        if (context.button == Qt::RightButton) {
            return true;
        }

        return false;
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
    auto* area = context.area;
    if (!area) {
        if (context.button == Qt::RightButton) {
            mMapWidget.mLabelHighlighted = false;
            mMapWidget.update();
        }
        context.isLabelHighlighted = false;
        return false;
    }

    if (context.button == Qt::LeftButton) {
        if (area->mMapLabels.isEmpty()) {
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
                context.isLabelHighlighted = mMapWidget.mLabelHighlighted;
                iterator.setValue(mapLabel);
                mMapWidget.update();
                return true;
            }
        }

        mMapWidget.mLabelHighlighted = false;
        context.isLabelHighlighted = false;
        mMapWidget.update();

        return false;
    }

    if (context.button == Qt::RightButton) {
        bool highlightChanged = false;
        bool labelFound = false;

        QMutableMapIterator<int, TMapLabel> iterator(area->mMapLabels);
        while (iterator.hasNext()) {
            iterator.next();
            auto mapLabel = iterator.value();

            const bool isSameZLevel = qRound(mapLabel.pos.z()) == mMapWidget.mMapCenterZ;
            bool shouldHighlight = false;
            if (isSameZLevel) {
                const float labelX = mapLabel.pos.x() * mMapWidget.mRoomWidth + mMapWidget.mRX;
                const float labelY = mapLabel.pos.y() * mMapWidget.mRoomHeight * -1 + mMapWidget.mRY;
                const QRectF boundingRect(labelX, labelY, mapLabel.clickSize.width(), mapLabel.clickSize.height());
                shouldHighlight = boundingRect.contains(context.widgetPosition);
            }

            if (mapLabel.highlight != shouldHighlight) {
                mapLabel.highlight = shouldHighlight;
                iterator.setValue(mapLabel);
                highlightChanged = true;
            }

            if (shouldHighlight) {
                labelFound = true;
            }
        }

        mMapWidget.mLabelHighlighted = labelFound;
        context.isLabelHighlighted = labelFound;

        if (highlightChanged || !labelFound) {
            mMapWidget.update();
        }

        if (!labelFound) {
            return false;
        }

        bool clearedRoomSelection = false;
        if (!mMapWidget.mMultiSelectionSet.isEmpty()) {
            mMapWidget.clearSelection();
            mMapWidget.mMultiSelectionSet.clear();
            clearedRoomSelection = true;
        }

        if (mMapWidget.mMultiSelectionHighlightRoomId != 0) {
            mMapWidget.mMultiSelectionHighlightRoomId = 0;
            clearedRoomSelection = true;
        }

        if (clearedRoomSelection) {
            mMapWidget.updateSelectionWidget();
        }

        auto* popup = new QMenu(&mMapWidget);
        popup->setToolTipsVisible(true);
        popup->setAttribute(Qt::WA_DeleteOnClose);
        mMapWidget.registerContextMenu(popup);

        //: 2D Mapper context menu (label) item
        auto* moveLabel = new QAction(mMapWidget.tr("Move"), popup);
        //: 2D Mapper context menu item (label) tooltip
        moveLabel->setToolTip(mMapWidget.tr("Move label"));
        QObject::connect(moveLabel, &QAction::triggered, &mMapWidget, &T2DMap::slot_moveLabel);

        //: 2D Mapper context menu (label) item
        auto* deleteLabel = new QAction(mMapWidget.tr("Delete"), popup);
        //: 2D Mapper context menu (label) item tooltip
        deleteLabel->setToolTip(mMapWidget.tr("Delete label"));
        QObject::connect(deleteLabel, &QAction::triggered, &mMapWidget, &T2DMap::slot_deleteLabel);

        popup->addAction(moveLabel);
        popup->addAction(deleteLabel);

        mMapWidget.mPopupMenu = true;
        popup->popup(mMapWidget.mapToGlobal(context.widgetPosition));
        mMapWidget.update();

        return true;
    }

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
    if (context.button == Qt::LeftButton) {
        if (mMapWidget.mMoveLabel) {
            mMapWidget.mMoveLabel = false;
        }
    }

    return false;
}
