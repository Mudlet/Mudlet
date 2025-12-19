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

#include <QCursor>
#include <QMouseEvent>
#include <QPainter>
#include <algorithm>
#include <cmath>

namespace
{
constexpr int csmTimerIntervalMs = 16;
constexpr int csmHoldThresholdMs = 300;
constexpr qreal csmDeadZone = 9.0;
constexpr qreal csmMaxDistance = 400.0;
constexpr qreal csmSpeedMultiplier = 4.0;
constexpr qreal csmMovementScale = 0.3;
}

MiddleMousePanHandler::MiddleMousePanHandler(T2DMap& mapWidget)
: mMapWidget(mapWidget)
{
    QObject::connect(&mTimer, &QTimer::timeout, [this]() { handleTick(); });
}

bool MiddleMousePanHandler::matches(const T2DMap::MapInteractionContext& context) const
{
    if (!context.event || !mMapWidget.mpMap) {
        return false;
    }

    switch (context.event->type()) {
    case QEvent::MouseButtonPress:
        if (context.button == Qt::MiddleButton) {
            return true;
        }
        // Cancel pan if another button is pressed while panning
        if (mActive && context.button != Qt::MiddleButton) {
            return true;
        }
        return false;
    case QEvent::MouseMove:
        return mActive;
    case QEvent::MouseButtonRelease:
        return context.button == Qt::MiddleButton && mPressActive;
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

bool MiddleMousePanHandler::handleMousePress(T2DMap::MapInteractionContext& context)
{
    if (!mMapWidget.mpMap) {
        return false;
    }

    // Cancel if another button is pressed while panning
    if (mActive && context.button != Qt::MiddleButton) {
        cancel();
        return false;
    }

    if (mActive) {
        cancel();
        return true;
    }

    beginPan(context.widgetPositionF, true);
    return true;
}

bool MiddleMousePanHandler::handleMouseMove(T2DMap::MapInteractionContext& context)
{
    updatePointer(context.widgetPositionF);
    return true;
}

bool MiddleMousePanHandler::handleMouseRelease(T2DMap::MapInteractionContext& context)
{
    Q_UNUSED(context);

    if (!mActive) {
        return false;
    }

    finishPress();
    return true;
}

void MiddleMousePanHandler::beginPan(const QPointF& widgetPosition, bool fromPress)
{
    if (!mMapWidget.mpMap) {
        return;
    }

    mActive = true;
    mPressActive = fromPress;
    mAnchor = widgetPosition;
    mCurrentPosition = widgetPosition;

    if (fromPress) {
        mPressTimer.restart();
    } else {
        mPressTimer.invalidate();
    }

    if (!mTimer.isActive()) {
        mTimer.start(csmTimerIntervalMs);
    }

    mMapWidget.setCursor(Qt::BlankCursor);
    mMapWidget.update();
}

void MiddleMousePanHandler::updatePointer(const QPointF& widgetPosition)
{
    mCurrentPosition = widgetPosition;
    mMapWidget.update();
}

void MiddleMousePanHandler::finishPress()
{
    if (!mActive || !mPressActive) {
        return;
    }

    mPressActive = false;

    const bool shouldStop = mPressTimer.isValid() && mPressTimer.elapsed() >= csmHoldThresholdMs;
    mPressTimer.invalidate();

    if (shouldStop) {
        cancel();
        return;
    }

    mMapWidget.update();
}

void MiddleMousePanHandler::cancel()
{
    if (!mActive && !mPressActive) {
        return;
    }

    mActive = false;
    mPressActive = false;

    if (mTimer.isActive()) {
        mTimer.stop();
    }

    mPressTimer.invalidate();
    mAnchor = QPointF();
    mCurrentPosition = QPointF();

    mMapWidget.unsetCursor();
    mMapWidget.update();
}

void MiddleMousePanHandler::handleTick()
{
    if (!mActive) {
        if (mTimer.isActive()) {
            mTimer.stop();
        }
        return;
    }

    if (!mMapWidget.mpMap) {
        cancel();
        return;
    }

    if (!mPressActive) {
        mCurrentPosition = mMapWidget.mapFromGlobal(QCursor::pos());
    }

    const qreal roomWidth = static_cast<qreal>(mMapWidget.mRoomWidth);
    const qreal roomHeight = static_cast<qreal>(mMapWidget.mRoomHeight);

    QPointF delta = mAnchor - mCurrentPosition;

    auto applyDeadZone = [](qreal value) {
        if (std::abs(value) <= csmDeadZone) {
            return 0.0;
        }
        return value > 0 ? value - csmDeadZone : value + csmDeadZone;
    };

    delta.setX(applyDeadZone(delta.x()));
    delta.setY(applyDeadZone(delta.y()));

    delta.setX(std::clamp(delta.x(), -csmMaxDistance, csmMaxDistance));
    delta.setY(std::clamp(delta.y(), -csmMaxDistance, csmMaxDistance));

    const bool hasHorizontalMovement = !qFuzzyIsNull(delta.x());
    const bool hasVerticalMovement = !qFuzzyIsNull(delta.y());

    const qreal intervalSeconds = static_cast<qreal>(csmTimerIntervalMs) / 1000.0;
    const qreal movementFactor = csmSpeedMultiplier * intervalSeconds;

    if (hasHorizontalMovement && !qFuzzyIsNull(roomWidth)) {
        mMapWidget.mMapCenterX -= (delta.x() / roomWidth) * movementFactor;
        mMapWidget.mShiftMode = true;
    }

    if (hasVerticalMovement && !qFuzzyIsNull(roomHeight)) {
        mMapWidget.mMapCenterY -= (delta.y() / roomHeight) * movementFactor;
        mMapWidget.mShiftMode = true;
    }

    if (hasHorizontalMovement || hasVerticalMovement) {
        mMapWidget.update();
    }
}

void MiddleMousePanHandler::renderIndicator(QPainter& painter)
{
    if (!mActive) {
        return;
    }

    painter.save();
    painter.setRenderHint(QPainter::Antialiasing, true);

    const QPointF direction = mCurrentPosition - mAnchor;
    const qreal directionLength = std::hypot(direction.x(), direction.y());
    const bool inDeadZone = directionLength <= csmDeadZone;

    if (inDeadZone) {
        constexpr qreal deadZoneRadius = 10.0;
        QPen deadZonePen(QColor(0, 0, 0, 120));
        deadZonePen.setWidthF(1.5);
        deadZonePen.setCosmetic(true);
        painter.setPen(deadZonePen);
        painter.setBrush(QColor(255, 255, 255, 170));
        painter.drawEllipse(mCurrentPosition, deadZoneRadius, deadZoneRadius);
    }

    constexpr qreal circleRadius = 3.28125;
    QPen circlePen(QColor(255, 255, 255, 180));
    circlePen.setWidthF(1.5);
    circlePen.setCosmetic(true);
    painter.setPen(circlePen);
    painter.setBrush(QColor(0, 0, 0, 200));
    painter.drawEllipse(mCurrentPosition, circleRadius, circleRadius);

    const QPointF scaledDirection = direction * csmMovementScale;
    const qreal scaledLength = std::hypot(scaledDirection.x(), scaledDirection.y());
    QPointF unitDirection = (scaledLength > 0.1) ? scaledDirection / scaledLength : QPointF(0, -1);

    constexpr qreal triangleDistance = 6.075;
    constexpr qreal triangleHeight = 6.328125;
    constexpr qreal triangleHalfWidth = 5.5773;

    const QPointF triangleBase = mCurrentPosition + unitDirection * triangleDistance;
    const QPointF triangleTip = triangleBase + unitDirection * triangleHeight;
    const QPointF perpendicular(-unitDirection.y(), unitDirection.x());

    QPolygonF triangle;
    triangle << triangleTip
             << triangleBase + perpendicular * triangleHalfWidth
             << triangleBase - perpendicular * triangleHalfWidth;

    QPen trianglePen(QColor(255, 255, 255, 180));
    trianglePen.setWidthF(1.5);
    trianglePen.setCosmetic(true);
    painter.setPen(trianglePen);
    painter.setBrush(QColor(0, 0, 0, 200));
    painter.drawPolygon(triangle);

    painter.restore();
}
