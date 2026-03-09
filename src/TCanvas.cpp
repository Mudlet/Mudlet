/***************************************************************************
 *   Copyright (C) 2024 by Mudlet contributors                            *
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

#include "TCanvas.h"

#include <QPainter>
#include <QPaintEvent>

TCanvas::TCanvas(QWidget* parent)
    : QWidget(parent)
{
    setAttribute(Qt::WA_TranslucentBackground, true);
    setAttribute(Qt::WA_TransparentForMouseEvents, true);
    setAutoFillBackground(false);
}

void TCanvas::paintEvent(QPaintEvent*)
{
    if (mDrawCmds.isEmpty()) {
        return;
    }

    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);

    for (const auto& cmd : mDrawCmds) {
        QPen pen(cmd.color, cmd.lineWidth, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
        p.setPen(pen);

        switch (cmd.type) {
        case CanvasDrawCmd::Line:
            p.drawLine(cmd.points[0], cmd.points[1]);
            break;

        case CanvasDrawCmd::Rect:
            p.setBrush(Qt::NoBrush);
            p.drawRect(QRectF(cmd.points[0], cmd.points[1]));
            break;

        case CanvasDrawCmd::FilledRect:
            p.setBrush(cmd.fillColor);
            p.drawRect(QRectF(cmd.points[0], cmd.points[1]));
            break;

        case CanvasDrawCmd::Ellipse:
            p.setBrush(Qt::NoBrush);
            p.drawEllipse(QRectF(cmd.points[0], cmd.points[1]));
            break;

        case CanvasDrawCmd::FilledEllipse:
            p.setBrush(cmd.fillColor);
            p.drawEllipse(QRectF(cmd.points[0], cmd.points[1]));
            break;
        }
    }
}

void TCanvas::addLine(qreal x1, qreal y1, qreal x2, qreal y2, const QColor& color, qreal width)
{
    CanvasDrawCmd cmd;
    cmd.type = CanvasDrawCmd::Line;
    cmd.points = {QPointF(x1, y1), QPointF(x2, y2)};
    cmd.color = color;
    cmd.lineWidth = width;
    mDrawCmds.append(cmd);
    update();
}

void TCanvas::addRect(qreal x, qreal y, qreal w, qreal h, const QColor& strokeColor, qreal lineWidth, const QColor& fillColor)
{
    CanvasDrawCmd cmd;
    cmd.type = fillColor.alpha() > 0 ? CanvasDrawCmd::FilledRect : CanvasDrawCmd::Rect;
    cmd.points = {QPointF(x, y), QPointF(x + w, y + h)};
    cmd.color = strokeColor;
    cmd.lineWidth = lineWidth;
    cmd.fillColor = fillColor;
    mDrawCmds.append(cmd);
    update();
}

void TCanvas::addEllipse(qreal cx, qreal cy, qreal rx, qreal ry, const QColor& strokeColor, qreal lineWidth, const QColor& fillColor)
{
    CanvasDrawCmd cmd;
    cmd.type = fillColor.alpha() > 0 ? CanvasDrawCmd::FilledEllipse : CanvasDrawCmd::Ellipse;
    cmd.points = {QPointF(cx - rx, cy - ry), QPointF(cx + rx, cy + ry)};
    cmd.color = strokeColor;
    cmd.lineWidth = lineWidth;
    cmd.fillColor = fillColor;
    mDrawCmds.append(cmd);
    update();
}

void TCanvas::clearDrawings()
{
    mDrawCmds.clear();
    update();
}

void TCanvas::setClickThrough(bool enabled)
{
    setAttribute(Qt::WA_TransparentForMouseEvents, enabled);
}
