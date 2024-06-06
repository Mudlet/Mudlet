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

#ifndef QPIXMAP_BLITTER_P_H
#define QPIXMAP_BLITTER_P_H

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
#include <qpa/qplatformpixmap.h>
#include <private/qpaintengine_blitter_p.h>

#ifndef QT_NO_BLITTABLE
QT_BEGIN_NAMESPACE

class Q_GUI_EXPORT  QBlittablePlatformPixmap : public QPlatformPixmap
{
//     Q_DECLARE_PRIVATE(QBlittablePlatformPixmap)
public:
    QBlittablePlatformPixmap();
    ~QBlittablePlatformPixmap();

    virtual QBlittable *createBlittable(const QSize &size, bool alpha) const = 0;
    QBlittable *blittable() const;
    void setBlittable(QBlittable *blittable);

    void resize(int width, int height) override;
    int metric(QPaintDevice::PaintDeviceMetric metric) const override;
    void fill(const QColor &color) override;
    QImage *buffer() override;
    QImage toImage() const override;
    bool hasAlphaChannel() const override;
    void fromImage(const QImage &image, Qt::ImageConversionFlags flags) override;
    qreal devicePixelRatio() const override;
    void setDevicePixelRatio(qreal scaleFactor) override;

    QPaintEngine *paintEngine() const override;

    void markRasterOverlay(const QRectF &);
    void markRasterOverlay(const QPointF &, const QTextItem &);
    void markRasterOverlay(const QVectorPath &);
    void markRasterOverlay(const QPainterPath &);
    void markRasterOverlay(const QRect *rects, int rectCount);
    void markRasterOverlay(const QRectF *rects, int rectCount);
    void markRasterOverlay(const QPointF *points, int pointCount);
    void markRasterOverlay(const QPoint *points, int pointCount);
    void unmarkRasterOverlay(const QRectF &);

#ifdef QT_BLITTER_RASTEROVERLAY
    void mergeOverlay();
    void unmergeOverlay();
    QImage *overlay();

#endif //QT_BLITTER_RASTEROVERLAY
protected:
    QScopedPointer<QBlitterPaintEngine> m_engine;
    QScopedPointer<QBlittable> m_blittable;
    bool m_alpha;
    qreal m_devicePixelRatio;

#ifdef QT_BLITTER_RASTEROVERLAY
    QImage *m_rasterOverlay;
    QImage *m_unmergedCopy;
    QColor m_overlayColor;

    void markRasterOverlayImpl(const QRectF &);
    void unmarkRasterOverlayImpl(const QRectF &);
    QRectF clipAndTransformRect(const QRectF &) const;
#endif //QT_BLITTER_RASTEROVERLAY

};

inline void QBlittablePlatformPixmap::markRasterOverlay(const QRectF &rect)
{
#ifdef QT_BLITTER_RASTEROVERLAY
    markRasterOverlayImpl(rect);
#else
   Q_UNUSED(rect)
#endif
}

inline void QBlittablePlatformPixmap::markRasterOverlay(const QVectorPath &path)
{
#ifdef QT_BLITTER_RASTEROVERLAY
    markRasterOverlayImpl(path.convertToPainterPath().boundingRect());
#else
    Q_UNUSED(path)
#endif
}

inline void QBlittablePlatformPixmap::markRasterOverlay(const QPointF &pos, const QTextItem &ti)
{
#ifdef QT_BLITTER_RASTEROVERLAY
    QFontMetricsF fm(ti.font());
    QRectF rect = fm.tightBoundingRect(ti.text());
    rect.moveBottomLeft(pos);
    markRasterOverlay(rect);
#else
    Q_UNUSED(pos)
    Q_UNUSED(ti)
#endif
}

inline void QBlittablePlatformPixmap::markRasterOverlay(const QRect *rects, int rectCount)
{
#ifdef QT_BLITTER_RASTEROVERLAY
    for (int i = 0; i < rectCount; i++) {
        markRasterOverlay(rects[i]);
    }
#else
    Q_UNUSED(rects)
    Q_UNUSED(rectCount)
#endif
}
inline void QBlittablePlatformPixmap::markRasterOverlay(const QRectF *rects, int rectCount)
{
#ifdef QT_BLITTER_RASTEROVERLAY
    for (int i = 0; i < rectCount; i++) {
        markRasterOverlay(rects[i]);
    }
#else
    Q_UNUSED(rects)
    Q_UNUSED(rectCount)
#endif
}

inline void QBlittablePlatformPixmap::markRasterOverlay(const QPointF *points, int pointCount)
{
#ifdef QT_BLITTER_RASTEROVERLAY
#error "not ported yet"
#else
    Q_UNUSED(points);
    Q_UNUSED(pointCount);
#endif
}

inline void QBlittablePlatformPixmap::markRasterOverlay(const QPoint *points, int pointCount)
{
#ifdef QT_BLITTER_RASTEROVERLAY
#error "not ported yet"
#else
    Q_UNUSED(points);
    Q_UNUSED(pointCount);
#endif
}

inline void QBlittablePlatformPixmap::markRasterOverlay(const QPainterPath& path)
{
#ifdef QT_BLITTER_RASTEROVERLAY
#error "not ported yet"
#else
    Q_UNUSED(path);
#endif
}

inline void QBlittablePlatformPixmap::unmarkRasterOverlay(const QRectF &rect)
{
#ifdef QT_BLITTER_RASTEROVERLAY
    unmarkRasterOverlayImpl(rect);
#else
    Q_UNUSED(rect)
#endif
}

QT_END_NAMESPACE
#endif // QT_NO_BLITTABLE
#endif // QPIXMAP_BLITTER_P_H
