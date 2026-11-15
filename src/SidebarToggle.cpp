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

#include "SidebarToggle.h"

#include "uiDesign.h"

#include <QEvent>
#include <QPainter>
#include <QPolygonF>

namespace uiDesign {

namespace {
// A tall pill rather than a circle: a circle this size reads as a bullet, not
// a handle on the line
constexpr int scmToggleWidth = 20;
constexpr int scmToggleHeight = 28;
// The weight the trees' own chevrons are drawn at
constexpr qreal scmChevronArm = 3.5;
constexpr qreal scmChevronPenWidth = 1.5;
// The window's hover wash, mixed into the fill rather than laid over it since
// this is painted rather than styled
constexpr qreal scmHoverWeight = 0.07;
} // namespace

SidebarToggle::SidebarToggle(QWidget* pSeam, QWidget* pParent)
: QAbstractButton(pParent)
, mpSeam(pSeam)
{
    setFixedSize(scmToggleWidth, scmToggleHeight);
    setCursor(Qt::PointingHandCursor);

    if (mpSeam) {
        mpSeam->installEventFilter(this);
    }
    if (pParent) {
        pParent->installEventFilter(this);
    }
    reposition();
}

void SidebarToggle::setPointingLeft(const bool pointingLeft)
{
    if (mPointingLeft == pointingLeft) {
        return;
    }
    mPointingLeft = pointingLeft;
    update();
}

void SidebarToggle::reposition()
{
    QWidget* pParent = parentWidget();
    if (!mpSeam || !pParent) {
        return;
    }
    // mapTo() rather than geometry(): the pane may sit a layout or two deeper
    // than the parent's own children
    const QPoint paneTopLeft = mpSeam->mapTo(pParent, QPoint(0, 0));
    move(paneTopLeft.x() + mpSeam->width() - width() / 2, paneTopLeft.y() + (mpSeam->height() - height()) / 2);
    // A neighbour stacked above the pill would hide the half that overlaps it
    raise();
}

bool SidebarToggle::eventFilter(QObject* pWatched, QEvent* pEvent)
{
    switch (pEvent->type()) {
    case QEvent::Resize:
    case QEvent::Move:
    case QEvent::Show:
        reposition();
        break;
    default:
        break;
    }
    return QAbstractButton::eventFilter(pWatched, pEvent);
}

void SidebarToggle::enterEvent(TEnterEvent* event)
{
    QAbstractButton::enterEvent(event);
    update();
}

void SidebarToggle::leaveEvent(QEvent* event)
{
    QAbstractButton::leaveEvent(event);
    update();
}

void SidebarToggle::paintEvent(QPaintEvent*)
{
    const ThemeTokens tokens = themeTokens();
    const bool lit = isEnabled() && (underMouse() || isDown());

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // Half a pixel in: a pen is centred on the path, so half the hairline would
    // fall outside the widget
    const QRectF pill = QRectF(rect()).adjusted(0.5, 0.5, -0.5, -0.5);
    const qreal radius = pill.width() / 2.0;
    const QColor frame = hasFocus() ? tokens.accent : tokens.border;
    painter.setPen(QPen(frame, 1.0));
    painter.setBrush(lit ? blend(tokens.card, tokens.text, scmHoverWeight) : tokens.card);
    painter.drawRoundedRect(pill, radius, radius);

    QColor ink = tokens.mutedText;
    if (!isEnabled()) {
        ink = tokens.disabledText;
    } else if (lit) {
        ink = tokens.accentText;
    }
    QPen pen(ink);
    pen.setWidthF(scmChevronPenWidth);
    pen.setCapStyle(Qt::RoundCap);
    pen.setJoinStyle(Qt::RoundJoin);
    painter.setPen(pen);
    painter.setBrush(Qt::NoBrush);

    const QPointF centre = pill.center();
    // Shortened like the trees' chevrons, so the mark stays inside its square
    // whichever way round it is drawn
    const qreal reach = scmChevronArm / 1.6;
    QPolygonF stroke;
    if (mPointingLeft) {
        stroke << QPointF(centre.x() + reach, centre.y() - scmChevronArm) << QPointF(centre.x() - reach, centre.y()) << QPointF(centre.x() + reach, centre.y() + scmChevronArm);
    } else {
        stroke << QPointF(centre.x() - reach, centre.y() - scmChevronArm) << QPointF(centre.x() + reach, centre.y()) << QPointF(centre.x() - reach, centre.y() + scmChevronArm);
    }
    painter.drawPolyline(stroke);
}

} // namespace uiDesign
