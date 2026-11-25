/***************************************************************************
 *   Copyright (C) 2026 by Vadim Peretokin - vadim.peretokin@mudlet.org    *
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

#include "EditorPlaceholderButton.h"

#include <QPainter>
#include <QPen>

namespace uiDesign {

namespace {
// A QPen dash pattern is in pen widths, so at a 1px pen these are pixels
constexpr qreal scmFramePenWidth = 1.0;
constexpr qreal scmFrameDashOn = 3.0;
constexpr qreal scmFrameDashOff = 3.0;
constexpr qreal scmFrameRadius = 6.0;
} // namespace

PlaceholderButton::PlaceholderButton(QWidget* pParent)
: QToolButton(pParent)
, mFrameRadius(scmFrameRadius)
{
}

void PlaceholderButton::setFrameColors(const QColor& resting, const QColor& active, const QColor& disabled)
{
    mRestingColor = resting;
    mActiveColor = active;
    mDisabledColor = disabled;
    update();
}

void PlaceholderButton::setFrameMargins(const QMargins& margins)
{
    mFrameMargins = margins;
    update();
}

void PlaceholderButton::setFrameRadius(const qreal radius)
{
    mFrameRadius = radius;
    update();
}

void PlaceholderButton::paintEvent(QPaintEvent* event)
{
    // Before QToolButton::paintEvent(), so the frame lies under the label and icon
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    QColor frameColor = mRestingColor;
    if (!isEnabled()) {
        frameColor = mDisabledColor;
    } else if (underMouse() || hasFocus()) {
        // The frame is the only focus indicator this button has
        frameColor = mActiveColor;
    }

    QPen pen(frameColor);
    pen.setWidthF(scmFramePenWidth);
    pen.setDashPattern({scmFrameDashOn, scmFrameDashOff});
    painter.setPen(pen);
    painter.setBrush(Qt::NoBrush);
    // A pen is centred on the path, so inset by half of it to keep the stroke on
    // the widget
    const qreal inset = scmFramePenWidth / 2.0;
    const QRectF frame = QRectF(rect().marginsRemoved(mFrameMargins)).adjusted(inset, inset, -inset, -inset);
    if (frame.isValid()) {
        painter.drawRoundedRect(frame, mFrameRadius, mFrameRadius);
    }
    painter.end();

    QToolButton::paintEvent(event);
}

} // namespace uiDesign
