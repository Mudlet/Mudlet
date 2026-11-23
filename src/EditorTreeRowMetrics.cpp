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

#include "EditorTreeRowMetrics.h"

#include <QColor>
#include <QModelIndex>
#include <QPainter>
#include <QPixmap>
#include <QPolygonF>

namespace uiDesign {

int treeRowLevelOf(const QModelIndex& index)
{
    int level = 0;
    for (QModelIndex ancestor = index.parent(); ancestor.isValid(); ancestor = ancestor.parent()) {
        ++level;
    }
    return level;
}

QPixmap treeRowChevronGlyph(const bool open, const QColor& ink, const qreal ratio)
{
    QPixmap glyph(qRound(scmTreeChevronBox * ratio), qRound(scmTreeChevronBox * ratio));
    glyph.setDevicePixelRatio(ratio);
    glyph.fill(Qt::transparent);

    QPainter painter(&glyph);
    painter.setRenderHint(QPainter::Antialiasing, true);
    QPen pen(ink);
    pen.setWidthF(scmTreeChevronPenWidth);
    pen.setCapStyle(Qt::RoundCap);
    pen.setJoinStyle(Qt::RoundJoin);
    painter.setPen(pen);
    painter.setBrush(Qt::NoBrush);
    const qreal centre = scmTreeChevronBox / 2.0;
    // Shortened so the chevron stays inside its square whichever way round it is drawn
    const qreal reach = scmTreeChevronArm / 1.6;
    QPolygonF stroke;
    if (open) {
        stroke << QPointF(centre - scmTreeChevronArm, centre - reach) << QPointF(centre, centre + reach) << QPointF(centre + scmTreeChevronArm, centre - reach);
    } else {
        stroke << QPointF(centre - reach, centre - scmTreeChevronArm) << QPointF(centre + reach, centre) << QPointF(centre - reach, centre + scmTreeChevronArm);
    }
    painter.drawPolyline(stroke);
    // Before returning: a copy taken while a painter is still active copies a pixmap mid-write
    painter.end();
    return glyph;
}

QPixmap treeRowDotGlyph(const TreeDotForm form, const QColor& ink, const qreal ratio)
{
    QPixmap glyph(qRound(scmTreeDotDiameter * ratio), qRound(scmTreeDotDiameter * ratio));
    glyph.setDevicePixelRatio(ratio);
    glyph.fill(Qt::transparent);

    QPainter painter(&glyph);
    painter.setRenderHint(QPainter::Antialiasing, true);
    const bool filled = form == TreeDotForm::Filled;
    // A stroke is centred on its path, so a hollow dot is inset by half a pen to match a filled one
    const qreal inset = filled ? 0.0 : scmTreeHollowPenWidth / 2.0;
    const QRectF circle(inset, inset, scmTreeDotDiameter - 2.0 * inset, scmTreeDotDiameter - 2.0 * inset);
    if (filled) {
        painter.setPen(Qt::NoPen);
        painter.setBrush(ink);
    } else {
        QPen pen(ink);
        pen.setWidthF(scmTreeHollowPenWidth);
        painter.setPen(pen);
        painter.setBrush(Qt::NoBrush);
    }
    painter.drawEllipse(circle);
    if (form == TreeDotForm::RingWithCentre) {
        painter.setPen(Qt::NoPen);
        painter.setBrush(ink);
        const qreal centreInset = (scmTreeDotDiameter - scmTreeDotCentreDiameter) / 2.0;
        painter.drawEllipse(QRectF(centreInset, centreInset, scmTreeDotCentreDiameter, scmTreeDotCentreDiameter));
    }
    painter.end();
    return glyph;
}

} // namespace uiDesign
