/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtGui module of the Qt Toolkit.
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

#ifndef QBLENDFUNCTIONS_P_H
#define QBLENDFUNCTIONS_P_H

#include <QtGui/private/qtguiglobal_p.h>
#include <qmath.h>
#include "qdrawhelper_p.h"

QT_BEGIN_NAMESPACE

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

template <typename SRC, typename T>
void qt_scale_image_16bit(uchar *destPixels, int dbpl,
                          const uchar *srcPixels, int sbpl, int srch,
                          const QRectF &targetRect,
                          const QRectF &srcRect,
                          const QRect &clip,
                          T blender)
{
    qreal sx = srcRect.width() / (qreal) targetRect.width();
    qreal sy = srcRect.height() / (qreal) targetRect.height();

    const int ix = 0x00010000 * sx;
    const int iy = 0x00010000 * sy;

//     qDebug() << "scale:" << Qt::endl
//              << " - target" << targetRect << Qt::endl
//              << " - source" << srcRect << Qt::endl
//              << " - clip" << clip << Qt::endl
//              << " - sx=" << sx << " sy=" << sy << " ix=" << ix << " iy=" << iy;

    QRect tr = targetRect.normalized().toRect();
    tr = tr.intersected(clip);
    if (tr.isEmpty())
        return;
    const int tx1 = tr.left();
    const int ty1 = tr.top();
    int h = tr.height();
    int w = tr.width();

    quint32 basex;
    quint32 srcy;

    if (sx < 0) {
        int dstx = qFloor((tx1 + qreal(0.5) - targetRect.right()) * sx * 65536) + 1;
        basex = quint32(srcRect.right() * 65536) + dstx;
    } else {
        int dstx = qCeil((tx1 + qreal(0.5) - targetRect.left()) * sx * 65536) - 1;
        basex = quint32(srcRect.left() * 65536) + dstx;
    }
    if (sy < 0) {
        int dsty = qFloor((ty1 + qreal(0.5) - targetRect.bottom()) * sy * 65536) + 1;
        srcy = quint32(srcRect.bottom() * 65536) + dsty;
    } else {
        int dsty = qCeil((ty1 + qreal(0.5) - targetRect.top()) * sy * 65536) - 1;
        srcy = quint32(srcRect.top() * 65536) + dsty;
    }

    quint16 *dst = ((quint16 *) (destPixels + ty1 * dbpl)) + tx1;

    // this bounds check here is required as floating point rounding above might in some cases lead to
    // w/h values that are one pixel too large, falling outside of the valid image area.
    const int ystart = srcy >> 16;
    if (ystart >= srch && iy < 0) {
        srcy += iy;
        --h;
    }
    const int xstart = basex >> 16;
    if (xstart >=  (int)(sbpl/sizeof(SRC)) && ix < 0) {
        basex += ix;
        --w;
    }
    int yend = (srcy + iy * (h - 1)) >> 16;
    if (yend < 0 || yend >= srch)
        --h;
    int xend = (basex + ix * (w - 1)) >> 16;
    if (xend < 0 || xend >= (int)(sbpl/sizeof(SRC)))
        --w;

    while (h--) {
        const SRC *src = (const SRC *) (srcPixels + (srcy >> 16) * sbpl);
        quint32 srcx = basex;
        int x = 0;
        for (; x<w-7; x+=8) {
            blender.write(&dst[x], src[srcx >> 16]); srcx += ix;
            blender.write(&dst[x+1], src[srcx >> 16]); srcx += ix;
            blender.write(&dst[x+2], src[srcx >> 16]); srcx += ix;
            blender.write(&dst[x+3], src[srcx >> 16]); srcx += ix;
            blender.write(&dst[x+4], src[srcx >> 16]); srcx += ix;
            blender.write(&dst[x+5], src[srcx >> 16]); srcx += ix;
            blender.write(&dst[x+6], src[srcx >> 16]); srcx += ix;
            blender.write(&dst[x+7], src[srcx >> 16]); srcx += ix;
        }
        for (; x<w; ++x) {
            blender.write(&dst[x], src[srcx >> 16]);
            srcx += ix;
        }
        blender.flush(&dst[x]);
        dst = (quint16 *)(((uchar *) dst) + dbpl);
        srcy += iy;
    }
}

template <typename T> void qt_scale_image_32bit(uchar *destPixels, int dbpl,
                                                const uchar *srcPixels, int sbpl, int srch,
                                                const QRectF &targetRect,
                                                const QRectF &srcRect,
                                                const QRect &clip,
                                                T blender)
{
    qreal sx = srcRect.width() / (qreal) targetRect.width();
    qreal sy = srcRect.height() / (qreal) targetRect.height();

    const int ix = 0x00010000 * sx;
    const int iy = 0x00010000 * sy;

//     qDebug() << "scale:" << Qt::endl
//              << " - target" << targetRect << Qt::endl
//              << " - source" << srcRect << Qt::endl
//              << " - clip" << clip << Qt::endl
//              << " - sx=" << sx << " sy=" << sy << " ix=" << ix << " iy=" << iy;

    QRect tr = targetRect.normalized().toRect();
    tr = tr.intersected(clip);
    if (tr.isEmpty())
        return;
    const int tx1 = tr.left();
    const int ty1 = tr.top();
    int h = tr.height();
    int w = tr.width();

    quint32 basex;
    quint32 srcy;

    if (sx < 0) {
        int dstx = qFloor((tx1 + qreal(0.5) - targetRect.right()) * sx * 65536) + 1;
        basex = quint32(srcRect.right() * 65536) + dstx;
    } else {
        int dstx = qCeil((tx1 + qreal(0.5) - targetRect.left()) * sx * 65536) - 1;
        basex = quint32(srcRect.left() * 65536) + dstx;
    }
    if (sy < 0) {
        int dsty = qFloor((ty1 + qreal(0.5) - targetRect.bottom()) * sy * 65536) + 1;
        srcy = quint32(srcRect.bottom() * 65536) + dsty;
    } else {
        int dsty = qCeil((ty1 + qreal(0.5) - targetRect.top()) * sy * 65536) - 1;
        srcy = quint32(srcRect.top() * 65536) + dsty;
    }

    quint32 *dst = ((quint32 *) (destPixels + ty1 * dbpl)) + tx1;

    // this bounds check here is required as floating point rounding above might in some cases lead to
    // w/h values that are one pixel too large, falling outside of the valid image area.
    const int ystart = srcy >> 16;
    if (ystart >= srch && iy < 0) {
        srcy += iy;
        --h;
    }
    const int xstart = basex >> 16;
    if (xstart >=  (int)(sbpl/sizeof(quint32)) && ix < 0) {
        basex += ix;
        --w;
    }
    int yend = (srcy + iy * (h - 1)) >> 16;
    if (yend < 0 || yend >= srch)
        --h;
    int xend = (basex + ix * (w - 1)) >> 16;
    if (xend < 0 || xend >= (int)(sbpl/sizeof(quint32)))
        --w;

    while (h--) {
        const uint *src = (const quint32 *) (srcPixels + (srcy >> 16) * sbpl);
        quint32 srcx = basex;
        int x = 0;
        for (; x<w; ++x) {
            blender.write(&dst[x], src[srcx >> 16]);
            srcx += ix;
        }
        blender.flush(&dst[x]);
        dst = (quint32 *)(((uchar *) dst) + dbpl);
        srcy += iy;
    }
}

struct QTransformImageVertex
{
    qreal x, y, u, v; // destination coordinates (x, y) and source coordinates (u, v)
};

template <class SrcT, class DestT, class Blender>
void qt_transform_image_rasterize(DestT *destPixels, int dbpl,
                                  const SrcT *srcPixels, int sbpl,
                                  const QTransformImageVertex &topLeft, const QTransformImageVertex &bottomLeft,
                                  const QTransformImageVertex &topRight, const QTransformImageVertex &bottomRight,
                                  const QRect &sourceRect,
                                  const QRect &clip,
                                  qreal topY, qreal bottomY,
                                  int dudx, int dvdx, int dudy, int dvdy, int u0, int v0,
                                  Blender blender)
{
    int fromY = qMax(qRound(topY), clip.top());
    int toY = qMin(qRound(bottomY), clip.top() + clip.height());
    if (fromY >= toY)
        return;

    qreal leftSlope = (bottomLeft.x - topLeft.x) / (bottomLeft.y - topLeft.y);
    qreal rightSlope = (bottomRight.x - topRight.x) / (bottomRight.y - topRight.y);
    int dx_l = int(leftSlope * 0x10000);
    int dx_r = int(rightSlope * 0x10000);
    int x_l = int((topLeft.x + (qreal(0.5) + fromY - topLeft.y) * leftSlope + qreal(0.5)) * 0x10000);
    int x_r = int((topRight.x + (qreal(0.5) + fromY - topRight.y) * rightSlope + qreal(0.5)) * 0x10000);

    int fromX, toX, x1, x2, u, v, i, ii;
    DestT *line;
    for (int y = fromY; y < toY; ++y) {
        line = reinterpret_cast<DestT *>(reinterpret_cast<uchar *>(destPixels) + y * dbpl);

        fromX = qMax(x_l >> 16, clip.left());
        toX = qMin(x_r >> 16, clip.left() + clip.width());
        if (fromX < toX) {
            // Because of rounding, we can get source coordinates outside the source image.
            // Clamp these coordinates to the source rect to avoid segmentation fault and
            // garbage on the screen.

            // Find the first pixel on the current scan line where the source coordinates are within the source rect.
            x1 = fromX;
            u = x1 * dudx + y * dudy + u0;
            v = x1 * dvdx + y * dvdy + v0;
            for (; x1 < toX; ++x1) {
                int uu = u >> 16;
                int vv = v >> 16;
                if (uu >= sourceRect.left() && uu < sourceRect.left() + sourceRect.width()
                    && vv >= sourceRect.top() && vv < sourceRect.top() + sourceRect.height()) {
                    break;
                }
                u += dudx;
                v += dvdx;
            }

            // Find the last pixel on the current scan line where the source coordinates are within the source rect.
            x2 = toX;
            u = (x2 - 1) * dudx + y * dudy + u0;
            v = (x2 - 1) * dvdx + y * dvdy + v0;
            for (; x2 > x1; --x2) {
                int uu = u >> 16;
                int vv = v >> 16;
                if (uu >= sourceRect.left() && uu < sourceRect.left() + sourceRect.width()
                    && vv >= sourceRect.top() && vv < sourceRect.top() + sourceRect.height()) {
                    break;
                }
                u -= dudx;
                v -= dvdx;
            }

            // Set up values at the beginning of the scan line.
            u = fromX * dudx + y * dudy + u0;
            v = fromX * dvdx + y * dvdy + v0;
            line += fromX;

            // Beginning of the scan line, with per-pixel checks.
            i = x1 - fromX;
            while (i) {
                int uu = qBound(sourceRect.left(), u >> 16, sourceRect.left() + sourceRect.width() - 1);
                int vv = qBound(sourceRect.top(), v >> 16, sourceRect.top() + sourceRect.height() - 1);
                blender.write(line, reinterpret_cast<const SrcT *>(reinterpret_cast<const uchar *>(srcPixels) + vv * sbpl)[uu]);
                u += dudx;
                v += dvdx;
                ++line;
                --i;
            }

            // Middle of the scan line, without checks.
            // Manual loop unrolling.
            i = x2 - x1;
            ii = i >> 3;
            while (ii) {
                blender.write(&line[0], reinterpret_cast<const SrcT *>(reinterpret_cast<const uchar *>(srcPixels) + (v >> 16) * sbpl)[u >> 16]); u += dudx; v += dvdx;
                blender.write(&line[1], reinterpret_cast<const SrcT *>(reinterpret_cast<const uchar *>(srcPixels) + (v >> 16) * sbpl)[u >> 16]); u += dudx; v += dvdx;
                blender.write(&line[2], reinterpret_cast<const SrcT *>(reinterpret_cast<const uchar *>(srcPixels) + (v >> 16) * sbpl)[u >> 16]); u += dudx; v += dvdx;
                blender.write(&line[3], reinterpret_cast<const SrcT *>(reinterpret_cast<const uchar *>(srcPixels) + (v >> 16) * sbpl)[u >> 16]); u += dudx; v += dvdx;
                blender.write(&line[4], reinterpret_cast<const SrcT *>(reinterpret_cast<const uchar *>(srcPixels) + (v >> 16) * sbpl)[u >> 16]); u += dudx; v += dvdx;
                blender.write(&line[5], reinterpret_cast<const SrcT *>(reinterpret_cast<const uchar *>(srcPixels) + (v >> 16) * sbpl)[u >> 16]); u += dudx; v += dvdx;
                blender.write(&line[6], reinterpret_cast<const SrcT *>(reinterpret_cast<const uchar *>(srcPixels) + (v >> 16) * sbpl)[u >> 16]); u += dudx; v += dvdx;
                blender.write(&line[7], reinterpret_cast<const SrcT *>(reinterpret_cast<const uchar *>(srcPixels) + (v >> 16) * sbpl)[u >> 16]); u += dudx; v += dvdx;

                line += 8;

                --ii;
            }
            switch (i & 7) {
                case 7: blender.write(line, reinterpret_cast<const SrcT *>(reinterpret_cast<const uchar *>(srcPixels) + (v >> 16) * sbpl)[u >> 16]); u += dudx; v += dvdx; ++line; Q_FALLTHROUGH();
                case 6: blender.write(line, reinterpret_cast<const SrcT *>(reinterpret_cast<const uchar *>(srcPixels) + (v >> 16) * sbpl)[u >> 16]); u += dudx; v += dvdx; ++line; Q_FALLTHROUGH();
                case 5: blender.write(line, reinterpret_cast<const SrcT *>(reinterpret_cast<const uchar *>(srcPixels) + (v >> 16) * sbpl)[u >> 16]); u += dudx; v += dvdx; ++line; Q_FALLTHROUGH();
                case 4: blender.write(line, reinterpret_cast<const SrcT *>(reinterpret_cast<const uchar *>(srcPixels) + (v >> 16) * sbpl)[u >> 16]); u += dudx; v += dvdx; ++line; Q_FALLTHROUGH();
                case 3: blender.write(line, reinterpret_cast<const SrcT *>(reinterpret_cast<const uchar *>(srcPixels) + (v >> 16) * sbpl)[u >> 16]); u += dudx; v += dvdx; ++line; Q_FALLTHROUGH();
                case 2: blender.write(line, reinterpret_cast<const SrcT *>(reinterpret_cast<const uchar *>(srcPixels) + (v >> 16) * sbpl)[u >> 16]); u += dudx; v += dvdx; ++line; Q_FALLTHROUGH();
                case 1: blender.write(line, reinterpret_cast<const SrcT *>(reinterpret_cast<const uchar *>(srcPixels) + (v >> 16) * sbpl)[u >> 16]); u += dudx; v += dvdx; ++line;
            }

            // End of the scan line, with per-pixel checks.
            i = toX - x2;
            while (i) {
                int uu = qBound(sourceRect.left(), u >> 16, sourceRect.left() + sourceRect.width() - 1);
                int vv = qBound(sourceRect.top(), v >> 16, sourceRect.top() + sourceRect.height() - 1);
                blender.write(line, reinterpret_cast<const SrcT *>(reinterpret_cast<const uchar *>(srcPixels) + vv * sbpl)[uu]);
                u += dudx;
                v += dvdx;
                ++line;
                --i;
            }

            blender.flush(line);
        }
        x_l += dx_l;
        x_r += dx_r;
    }
}

template <class SrcT, class DestT, class Blender>
void qt_transform_image(DestT *destPixels, int dbpl,
                        const SrcT *srcPixels, int sbpl,
                        const QRectF &targetRect,
                        const QRectF &sourceRect,
                        const QRect &clip,
                        const QTransform &targetRectTransform,
                        Blender blender)
{
    enum Corner
    {
        TopLeft,
        TopRight,
        BottomRight,
        BottomLeft
    };

    // map source rectangle to destination.
    QTransformImageVertex v[4];
    v[TopLeft].u = v[BottomLeft].u = sourceRect.left();
    v[TopLeft].v = v[TopRight].v = sourceRect.top();
    v[TopRight].u = v[BottomRight].u = sourceRect.right();
    v[BottomLeft].v = v[BottomRight].v = sourceRect.bottom();
    targetRectTransform.map(targetRect.left(), targetRect.top(), &v[TopLeft].x, &v[TopLeft].y);
    targetRectTransform.map(targetRect.right(), targetRect.top(), &v[TopRight].x, &v[TopRight].y);
    targetRectTransform.map(targetRect.left(), targetRect.bottom(), &v[BottomLeft].x, &v[BottomLeft].y);
    targetRectTransform.map(targetRect.right(), targetRect.bottom(), &v[BottomRight].x, &v[BottomRight].y);

    // find topmost vertex.
    int topmost = 0;
    for (int i = 1; i < 4; ++i) {
        if (v[i].y < v[topmost].y)
            topmost = i;
    }
    // rearrange array such that topmost vertex is at index 0.
    switch (topmost) {
    case 1:
        {
            QTransformImageVertex t = v[0];
            for (int i = 0; i < 3; ++i)
                v[i] = v[i+1];
            v[3] = t;
        }
        break;
    case 2:
        qSwap(v[0], v[2]);
        qSwap(v[1], v[3]);
        break;
    case 3:
        {
            QTransformImageVertex t = v[3];
            for (int i = 3; i > 0; --i)
                v[i] = v[i-1];
            v[0] = t;
        }
        break;
    }

    // if necessary, swap vertex 1 and 3 such that 1 is to the left of 3.
    qreal dx1 = v[1].x - v[0].x;
    qreal dy1 = v[1].y - v[0].y;
    qreal dx2 = v[3].x - v[0].x;
    qreal dy2 = v[3].y - v[0].y;
    if (dx1 * dy2 - dx2 * dy1 > 0)
        qSwap(v[1], v[3]);

    QTransformImageVertex u = {v[1].x - v[0].x, v[1].y - v[0].y, v[1].u - v[0].u, v[1].v - v[0].v};
    QTransformImageVertex w = {v[2].x - v[0].x, v[2].y - v[0].y, v[2].u - v[0].u, v[2].v - v[0].v};

    qreal det = u.x * w.y - u.y * w.x;
    if (det == 0)
        return;

    qreal invDet = 1.0 / det;
    qreal m11, m12, m21, m22, mdx, mdy;

    m11 = (u.u * w.y - u.y * w.u) * invDet;
    m12 = (u.x * w.u - u.u * w.x) * invDet;
    m21 = (u.v * w.y - u.y * w.v) * invDet;
    m22 = (u.x * w.v - u.v * w.x) * invDet;
    mdx = v[0].u - m11 * v[0].x - m12 * v[0].y;
    mdy = v[0].v - m21 * v[0].x - m22 * v[0].y;

    int dudx = int(m11 * 0x10000);
    int dvdx = int(m21 * 0x10000);
    int dudy = int(m12 * 0x10000);
    int dvdy = int(m22 * 0x10000);
    int u0 = qCeil((qreal(0.5) * m11 + qreal(0.5) * m12 + mdx) * 0x10000) - 1;
    int v0 = qCeil((qreal(0.5) * m21 + qreal(0.5) * m22 + mdy) * 0x10000) - 1;

    int x1 = qFloor(sourceRect.left());
    int y1 = qFloor(sourceRect.top());
    int x2 = qCeil(sourceRect.right());
    int y2 = qCeil(sourceRect.bottom());
    QRect sourceRectI(x1, y1, x2 - x1, y2 - y1);

    // rasterize trapezoids.
    if (v[1].y < v[3].y) {
        qt_transform_image_rasterize(destPixels, dbpl, srcPixels, sbpl, v[0], v[1], v[0], v[3], sourceRectI, clip, v[0].y, v[1].y, dudx, dvdx, dudy, dvdy, u0, v0, blender);
        qt_transform_image_rasterize(destPixels, dbpl, srcPixels, sbpl, v[1], v[2], v[0], v[3], sourceRectI, clip, v[1].y, v[3].y, dudx, dvdx, dudy, dvdy, u0, v0, blender);
        qt_transform_image_rasterize(destPixels, dbpl, srcPixels, sbpl, v[1], v[2], v[3], v[2], sourceRectI, clip, v[3].y, v[2].y, dudx, dvdx, dudy, dvdy, u0, v0, blender);
    } else {
        qt_transform_image_rasterize(destPixels, dbpl, srcPixels, sbpl, v[0], v[1], v[0], v[3], sourceRectI, clip, v[0].y, v[3].y, dudx, dvdx, dudy, dvdy, u0, v0, blender);
        qt_transform_image_rasterize(destPixels, dbpl, srcPixels, sbpl, v[0], v[1], v[3], v[2], sourceRectI, clip, v[3].y, v[1].y, dudx, dvdx, dudy, dvdy, u0, v0, blender);
        qt_transform_image_rasterize(destPixels, dbpl, srcPixels, sbpl, v[1], v[2], v[3], v[2], sourceRectI, clip, v[1].y, v[2].y, dudx, dvdx, dudy, dvdy, u0, v0, blender);
    }
}

QT_END_NAMESPACE

#endif // QBLENDFUNCTIONS_P_H
