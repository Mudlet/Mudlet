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

#ifndef MUDLET_MIDDLEMOUSEPANHANDLER_H
#define MUDLET_MIDDLEMOUSEPANHANDLER_H

#include "T2DMap.h"

#include <QElapsedTimer>
#include <QPointF>
#include <QTimer>

class QPainter;

class MiddleMousePanHandler : public T2DMap::IInteractionHandler
{
public:
    explicit MiddleMousePanHandler(T2DMap& mapWidget);

    bool matches(const T2DMap::MapInteractionContext& context) const override;
    bool handle(T2DMap::MapInteractionContext& context) override;

    void renderIndicator(QPainter& painter);
    void handleTick();
    void cancel();

    bool isActive() const { return mActive; }
    bool isPressActive() const { return mPressActive; }

private:
    void beginPan(const QPointF& widgetPosition, bool fromPress);
    void updatePointer(const QPointF& widgetPosition);
    void finishPress();

    bool handleMousePress(T2DMap::MapInteractionContext& context);
    bool handleMouseMove(T2DMap::MapInteractionContext& context);
    bool handleMouseRelease(T2DMap::MapInteractionContext& context);

    T2DMap& mMapWidget;

    bool mActive = false;
    bool mPressActive = false;
    QPointF mAnchor;
    QPointF mCurrentPosition;
    QTimer mTimer;
    QElapsedTimer mPressTimer;
};

#endif // MUDLET_MIDDLEMOUSEPANHANDLER_H
