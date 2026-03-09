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

#ifndef MUDLET_TCANVAS_H
#define MUDLET_TCANVAS_H

#include <QColor>
#include <QPointF>
#include <QVector>
#include <QWidget>

struct CanvasDrawCmd {
    enum Type { Line, Rect, FilledRect, Ellipse, FilledEllipse };
    Type type;
    QVector<QPointF> points; // 2 points: for Line=(p1,p2), Rect/Ellipse=(topLeft,bottomRight)
    QColor color;
    qreal lineWidth;
    QColor fillColor;
};

class TCanvas : public QWidget
{
    Q_OBJECT

public:
    explicit TCanvas(QWidget* parent = nullptr);

    void addLine(qreal x1, qreal y1, qreal x2, qreal y2, const QColor& color, qreal width = 1.0);
    void addRect(qreal x, qreal y, qreal w, qreal h, const QColor& strokeColor, qreal lineWidth = 1.0, const QColor& fillColor = Qt::transparent);
    void addEllipse(qreal cx, qreal cy, qreal rx, qreal ry, const QColor& strokeColor, qreal lineWidth = 1.0, const QColor& fillColor = Qt::transparent);
    void clearDrawings();
    void setClickThrough(bool enabled);

    QString mName;

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    QVector<CanvasDrawCmd> mDrawCmds;
};

#endif // MUDLET_TCANVAS_H
