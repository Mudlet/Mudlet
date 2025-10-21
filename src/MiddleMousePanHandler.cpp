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

#include "MiddleMousePanHandler.h"

#include <QMouseEvent>

MiddleMousePanHandler::MiddleMousePanHandler(T2DMap& mapWidget)
: mMapWidget(mapWidget)
{
}

bool MiddleMousePanHandler::matches(const T2DMap::MapInteractionContext& context) const
{
    if (!context.event || !mMapWidget.mpMap) {
        return false;
    }

    switch (context.event->type()) {
    case QEvent::MouseButtonPress:
        return context.button == Qt::MiddleButton;
    case QEvent::MouseMove:
        return mMapWidget.isMiddlePanActive();
    case QEvent::MouseButtonRelease:
        return context.button == Qt::MiddleButton && mMapWidget.isMiddlePanPressActive();
    default:
        return false;
    }
}

bool MiddleMousePanHandler::handle(T2DMap::MapInteractionContext& context)
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

bool MiddleMousePanHandler::handleMousePress(T2DMap::MapInteractionContext& context) const
{
    if (!mMapWidget.mpMap) {
        return false;
    }

    if (mMapWidget.isMiddlePanActive()) {
        mMapWidget.cancelMiddlePan();
        return true;
    }

    mMapWidget.beginMiddlePan(context.widgetPositionF, true);

    return true;
}

bool MiddleMousePanHandler::handleMouseMove(T2DMap::MapInteractionContext& context) const
{
    if (!mMapWidget.isMiddlePanActive()) {
        return false;
    }

    mMapWidget.updateMiddlePanPointer(context.widgetPositionF);

    return true;
}

bool MiddleMousePanHandler::handleMouseRelease(T2DMap::MapInteractionContext& context) const
{
    if (!mMapWidget.isMiddlePanActive()) {
        return false;
    }

    mMapWidget.finishMiddlePanPress();

    return true;
}
