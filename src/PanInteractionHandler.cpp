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

#include "PanInteractionHandler.h"

#include "TMap.h"

#include <QMouseEvent>
#include <QtGlobal>

PanInteractionHandler::PanInteractionHandler(T2DMap& mapWidget)
: mMapWidget(mapWidget)
{
}

bool PanInteractionHandler::matches(const T2DMap::MapInteractionContext& context) const
{
    if (!context.event || !mMapWidget.mpMap) {
        return false;
    }

    switch (context.event->type()) {
    case QEvent::MouseButtonPress:
        return context.button == Qt::LeftButton
            && (context.modifiers.testFlag(Qt::AltModifier) || context.isMapViewOnly);
    case QEvent::MouseMove:
        return mMapWidget.mpMap->mLeftDown || mMapWidget.mpMap->m2DPanMode;
    case QEvent::MouseButtonRelease:
        return context.button == Qt::LeftButton
            && (mMapWidget.mpMap->mLeftDown || mMapWidget.mpMap->m2DPanMode);
    default:
        return false;
    }
}

bool PanInteractionHandler::handle(T2DMap::MapInteractionContext& context)
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

bool PanInteractionHandler::handleMousePress(T2DMap::MapInteractionContext& context) const
{
    if (!mMapWidget.mpMap) {
        return false;
    }

    if (context.button != Qt::LeftButton) {
        return false;
    }

    if (!context.modifiers.testFlag(Qt::AltModifier) && !context.isMapViewOnly) {
        return false;
    }

    mMapWidget.setCursor(Qt::ClosedHandCursor);
    mMapWidget.mpMap->mLeftDown = true;

    return false;
}

bool PanInteractionHandler::handleMouseMove(T2DMap::MapInteractionContext& context) const
{
    if (!mMapWidget.mpMap) {
        return false;
    }

    auto* map = mMapWidget.mpMap;

    if (!map->mLeftDown && !map->m2DPanMode) {
        return false;
    }

    const bool hasPanModifier = context.modifiers.testFlag(Qt::AltModifier) || context.isMapViewOnly;

    if (!map->m2DPanMode) {
        if (!hasPanModifier) {
            return false;
        }

        map->m2DPanStart = context.widgetPositionF;
        map->m2DPanMode = true;
    }

    if (!hasPanModifier) {
        map->m2DPanMode = false;
        map->mLeftDown = false;
        return true;
    }

    const QPointF panNewPosition = context.widgetPositionF;
    const QPointF movement = map->m2DPanStart - panNewPosition;

    const qreal roomWidth = static_cast<qreal>(mMapWidget.mRoomWidth);
    const qreal roomHeight = static_cast<qreal>(mMapWidget.mRoomHeight);

    if (!qFuzzyIsNull(roomWidth)) {
        mMapWidget.mMapCenterX += movement.x() / roomWidth;
    }

    if (!qFuzzyIsNull(roomHeight)) {
        mMapWidget.mMapCenterY += movement.y() / roomHeight;
    }

    map->m2DPanStart = panNewPosition;
    mMapWidget.mShiftMode = true;
    mMapWidget.update();

    return true;
}

bool PanInteractionHandler::handleMouseRelease(T2DMap::MapInteractionContext& context) const
{
    if (!mMapWidget.mpMap) {
        return false;
    }

    auto* map = mMapWidget.mpMap;

    if (context.button != Qt::LeftButton) {
        return false;
    }

    if (!map->mLeftDown && !map->m2DPanMode) {
        return false;
    }

    map->mLeftDown = false;
    map->m2DPanMode = false;
    mMapWidget.unsetCursor();

    return false;
}

