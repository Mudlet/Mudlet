/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtQuick module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QQUICKCONTEXT2DCOMMANDBUFFER_P_H
#define QQUICKCONTEXT2DCOMMANDBUFFER_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <private/qtquickglobal_p.h>

QT_REQUIRE_CONFIG(quick_canvas);

#include <QtCore/qmutex.h>
#include "qquickcontext2d_p.h"

QT_BEGIN_NAMESPACE

class QQuickCanvasItem;
class QMutex;

class QQuickContext2DCommandBuffer
{
public:
    QQuickContext2DCommandBuffer();
    ~QQuickContext2DCommandBuffer();
    void reset();
    void clear();

    inline int size() const { return commands.size(); }
    inline bool isEmpty() const {return commands.isEmpty(); }
    inline bool hasNext() const {return cmdIdx < commands.size(); }
    inline QQuickContext2D::PaintCommand takeNextCommand() { return commands.at(cmdIdx++); }

    inline qreal takeGlobalAlpha() { return takeReal(); }
    inline QPainter::CompositionMode takeGlobalCompositeOperation(){ return static_cast<QPainter::CompositionMode>(takeInt()); }
    inline QBrush takeStrokeStyle() { return takeBrush(); }
    inline QBrush takeFillStyle() { return takeBrush(); }

    inline qreal takeLineWidth() { return takeReal(); }
    inline Qt::PenCapStyle takeLineCap() { return static_cast<Qt::PenCapStyle>(takeInt());}
    inline Qt::PenJoinStyle takeLineJoin(){ return static_cast<Qt::PenJoinStyle>(takeInt());}
    inline qreal takeMiterLimit() { return takeReal(); }

    inline void setGlobalAlpha( qreal alpha)
    {
        commands << QQuickContext2D::GlobalAlpha;
        reals << alpha;
    }

    inline void setGlobalCompositeOperation(QPainter::CompositionMode cm)
    {
        commands << QQuickContext2D::GlobalCompositeOperation;
        ints << cm;
    }

    inline void setStrokeStyle(const QBrush &style, bool repeatX = false, bool repeatY = false)
    {
        commands << QQuickContext2D::StrokeStyle;
        brushes << style;
        bools << repeatX << repeatY;
    }

    inline void drawImage(const QImage& image,  const QRectF& sr, const QRectF& dr)
    {
        commands << QQuickContext2D::DrawImage;
        images << image;
        rects << sr << dr;
    }

    inline void drawPixmap(QQmlRefPointer<QQuickCanvasPixmap> pixmap, const QRectF& sr, const QRectF& dr)
    {
        commands << QQuickContext2D::DrawPixmap;
        pixmaps << pixmap;
        rects << sr << dr;
    }

    inline qreal takeShadowOffsetX() { return takeReal(); }
    inline qreal takeShadowOffsetY() { return takeReal(); }
    inline qreal takeShadowBlur() { return takeReal(); }
    inline QColor takeShadowColor() { return takeColor(); }


    inline void updateMatrix(const QTransform& matrix)
    {
        commands << QQuickContext2D::UpdateMatrix;
        matrixes << matrix;
    }

    inline void clearRect(const QRectF& r)
    {
        commands << QQuickContext2D::ClearRect;
        rects << r;
    }

    inline void fillRect(const QRectF& r)
    {
        commands << QQuickContext2D::FillRect;
        rects << r;
    }

    inline void strokeRect(const QRectF& r)
    {
        QPainterPath p;
        p.addRect(r);

        commands << QQuickContext2D::Stroke;
        pathes << p;
    }


    inline void fill(const QPainterPath& path)
    {
        commands << QQuickContext2D::Fill;
        pathes << path;

    }

    inline void stroke(const QPainterPath& path)
    {
        commands << QQuickContext2D::Stroke;
        pathes << path;
    }

    inline void clip(bool enabled, const QPainterPath& path)
    {
        commands << QQuickContext2D::Clip;
        bools << enabled;
        pathes << path;
    }



    inline void setFillStyle(const QBrush &style, bool repeatX = false, bool repeatY = false)
    {
        commands << QQuickContext2D::FillStyle;
        brushes << style;
        bools << repeatX << repeatY;
    }


    inline void setLineWidth( qreal w)
    {
        commands << QQuickContext2D::LineWidth;
        reals << w;
    }

    inline void setLineCap(Qt::PenCapStyle  cap)
    {
        commands << QQuickContext2D::LineCap;
        ints << cap;
    }

    inline void setLineJoin(Qt::PenJoinStyle join)
    {
        commands << QQuickContext2D::LineJoin;
        ints << join;
    }

    inline void setLineDash(const QVector<qreal> &pattern)
    {
        commands << QQuickContext2D::LineDash;
        reals << pattern.length();
        for (qreal r : pattern)
            reals << r;
    }

    inline void setLineDashOffset( qreal offset)
    {
        commands << QQuickContext2D::LineDashOffset;
        reals << offset;
    }

    inline void setMiterLimit( qreal limit)
    {
        commands << QQuickContext2D::MiterLimit;
        reals << limit;
    }

    inline void setShadowOffsetX( qreal x)
    {
        commands << QQuickContext2D::ShadowOffsetX;
        reals << x;
    }

    inline void setShadowOffsetY( qreal y)
    {
        commands << QQuickContext2D::ShadowOffsetY;
        reals << y;
    }

    inline void setShadowBlur( qreal b)
    {
        commands << QQuickContext2D::ShadowBlur;
        reals << b;
    }

    inline void setShadowColor(const QColor &color)
    {
        commands << QQuickContext2D::ShadowColor;
        colors << color;
    }

    inline QTransform takeMatrix() { return matrixes.at(matrixIdx++); }

    inline QRectF takeRect() { return rects.at(rectIdx++); }

    inline QPainterPath takePath() { return pathes.at(pathIdx++); }

    inline const QImage& takeImage() { return images.at(imageIdx++); }
    inline QQmlRefPointer<QQuickCanvasPixmap> takePixmap() { return pixmaps.at(pixmapIdx++); }

    inline int takeInt() { return ints.at(intIdx++); }
    inline bool takeBool() {return bools.at(boolIdx++); }
    inline qreal takeReal() { return reals.at(realIdx++); }
    inline QColor takeColor() { return colors.at(colorIdx++); }
    inline QBrush takeBrush() { return brushes.at(brushIdx++); }

    void replay(QPainter* painter, QQuickContext2D::State& state, const QVector2D &scaleFactor);

private:
    static QPen makePen(const QQuickContext2D::State& state);
    void setPainterState(QPainter* painter, const QQuickContext2D::State& state, const QPen& pen);
    int cmdIdx;
    int intIdx;
    int boolIdx;
    int realIdx;
    int rectIdx;
    int colorIdx;
    int matrixIdx;
    int brushIdx;
    int pathIdx;
    int imageIdx;
    int pixmapIdx;
    QVector<QQuickContext2D::PaintCommand> commands;

    QVector<int> ints;
    QVector<bool> bools;
    QVector<qreal> reals;
    QVector<QRectF> rects;
    QVector<QColor> colors;
    QVector<QTransform> matrixes;
    QVector<QBrush> brushes;
    QVector<QPainterPath> pathes;
    QVector<QImage> images;
    QVector<QQmlRefPointer<QQuickCanvasPixmap> > pixmaps;
    QMutex queueLock;
};

QT_END_NAMESPACE

#endif // QQUICKCONTEXT2DCOMMANDBUFFER_P_H
