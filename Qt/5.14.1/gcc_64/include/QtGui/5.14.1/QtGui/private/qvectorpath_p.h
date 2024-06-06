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

#ifndef QVECTORPATH_P_H
#define QVECTORPATH_P_H

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

#include <QtGui/private/qtguiglobal_p.h>
#include <QtGui/qpaintengine.h>

#include <private/qpaintengine_p.h>
#include <private/qstroker_p.h>
#include <private/qpainter_p.h>


QT_BEGIN_NAMESPACE


class QPaintEngineEx;

typedef void (*qvectorpath_cache_cleanup)(QPaintEngineEx *engine, void *data);

struct QRealRect {
    qreal x1, y1, x2, y2;
};

class Q_GUI_EXPORT QVectorPath
{
public:
    enum Hint {
        // Shape hints, in 0x000000ff, access using shape()
        AreaShapeMask           = 0x0001,       // shape covers an area
        NonConvexShapeMask      = 0x0002,       // shape is not convex
        CurvedShapeMask         = 0x0004,       // shape contains curves...
        LinesShapeMask          = 0x0008,
        RectangleShapeMask      = 0x0010,
        ShapeMask               = 0x001f,

        // Shape hints merged into basic shapes..
        LinesHint               = LinesShapeMask,
        RectangleHint           = AreaShapeMask | RectangleShapeMask,
        EllipseHint             = AreaShapeMask | CurvedShapeMask,
        ConvexPolygonHint       = AreaShapeMask,
        PolygonHint             = AreaShapeMask | NonConvexShapeMask,
        RoundedRectHint         = AreaShapeMask | CurvedShapeMask,
        ArbitraryShapeHint      = AreaShapeMask | NonConvexShapeMask | CurvedShapeMask,

        // Other hints
        IsCachedHint            = 0x0100, // Set if the cache hint is set
        ShouldUseCacheHint      = 0x0200, // Set if the path should be cached when possible..
        ControlPointRect        = 0x0400, // Set if the control point rect has been calculated...

        // Shape rendering specifiers...
        OddEvenFill             = 0x1000,
        WindingFill             = 0x2000,
        ImplicitClose           = 0x4000,
        ExplicitOpen            = 0x8000
    };

    // ### Falcon: introduca a struct XY for points so lars is not so confused...
    QVectorPath(const qreal *points,
                int count,
                const QPainterPath::ElementType *elements = nullptr,
                uint hints = ArbitraryShapeHint)
        : m_elements(elements),
          m_points(points),
          m_count(count),
          m_hints(hints)
    {
    }

    ~QVectorPath();

    QRectF controlPointRect() const;

    inline Hint shape() const { return (Hint) (m_hints & ShapeMask); }
    inline bool isConvex() const { return (m_hints & NonConvexShapeMask) == 0; }
    inline bool isCurved() const { return m_hints & CurvedShapeMask; }

    inline bool isCacheable() const { return m_hints & ShouldUseCacheHint; }
    inline bool hasImplicitClose() const { return m_hints & ImplicitClose; }
    inline bool hasExplicitOpen() const { return m_hints & ExplicitOpen; }
    inline bool hasWindingFill() const { return m_hints & WindingFill; }

    inline void makeCacheable() const { m_hints |= ShouldUseCacheHint; m_cache = nullptr; }
    inline uint hints() const { return m_hints; }

    inline const QPainterPath::ElementType *elements() const { return m_elements; }
    inline const qreal *points() const { return m_points; }
    inline bool isEmpty() const { return m_points == nullptr; }

    inline int elementCount() const { return m_count; }
    inline const QPainterPath convertToPainterPath() const;

    static inline uint polygonFlags(QPaintEngine::PolygonDrawMode mode)
    {
        switch (mode) {
        case QPaintEngine::ConvexMode: return ConvexPolygonHint | ImplicitClose;
        case QPaintEngine::OddEvenMode: return PolygonHint | OddEvenFill | ImplicitClose;
        case QPaintEngine::WindingMode: return PolygonHint | WindingFill | ImplicitClose;
        case QPaintEngine::PolylineMode: return PolygonHint | ExplicitOpen;
        default: return 0;
        }
    }

    struct CacheEntry {
        QPaintEngineEx *engine;
        void *data;
        qvectorpath_cache_cleanup cleanup;
        CacheEntry *next;
    };

    CacheEntry *addCacheData(QPaintEngineEx *engine, void *data, qvectorpath_cache_cleanup cleanup) const;
    inline CacheEntry *lookupCacheData(QPaintEngineEx *engine) const {
        Q_ASSERT(m_hints & ShouldUseCacheHint);
        CacheEntry *e = m_cache;
        while (e) {
            if (e->engine == engine)
                return e;
            e = e->next;
        }
        return nullptr;
    }

    template <typename T> static inline bool isRect(const T *pts, int elementCount) {
        return (elementCount == 5 // 5-point polygon, check for closed rect
                && pts[0] == pts[8] && pts[1] == pts[9] // last point == first point
                && pts[0] == pts[6] && pts[2] == pts[4] // x values equal
                && pts[1] == pts[3] && pts[5] == pts[7] // y values equal...
                && pts[0] < pts[4] && pts[1] < pts[5]
                ) ||
               (elementCount == 4 // 4-point polygon, check for unclosed rect
                && pts[0] == pts[6] && pts[2] == pts[4] // x values equal
                && pts[1] == pts[3] && pts[5] == pts[7] // y values equal...
                && pts[0] < pts[4] && pts[1] < pts[5]
                );
    }

    inline bool isRect() const
    {
        const QPainterPath::ElementType * const types = elements();

        return (shape() == QVectorPath::RectangleHint)
                || (isRect(points(), elementCount())
                    && (!types || (types[0] == QPainterPath::MoveToElement
                                   && types[1] == QPainterPath::LineToElement
                                   && types[2] == QPainterPath::LineToElement
                                   && types[3] == QPainterPath::LineToElement)));
    }


private:
    Q_DISABLE_COPY_MOVE(QVectorPath)

    const QPainterPath::ElementType *m_elements;
    const qreal *m_points;
    const int m_count;

    mutable uint m_hints;
    mutable QRealRect m_cp_rect;

    mutable CacheEntry *m_cache;
};

Q_GUI_EXPORT const QVectorPath &qtVectorPathForPath(const QPainterPath &path);

QT_END_NAMESPACE

#endif
