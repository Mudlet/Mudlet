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

#ifndef QPAINTENGINEEX_P_H
#define QPAINTENGINEEX_P_H

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
#include <private/qvectorpath_p.h>


QT_BEGIN_NAMESPACE


class QPainterState;
class QPaintEngineExPrivate;
class QStaticTextItem;
struct StrokeHandler;

#ifndef QT_NO_DEBUG_STREAM
QDebug Q_GUI_EXPORT &operator<<(QDebug &, const QVectorPath &path);
#endif

class Q_GUI_EXPORT QPaintEngineEx : public QPaintEngine
{
    Q_DECLARE_PRIVATE(QPaintEngineEx)
public:
    QPaintEngineEx();

    virtual QPainterState *createState(QPainterState *orig) const;

    virtual void draw(const QVectorPath &path);
    virtual void fill(const QVectorPath &path, const QBrush &brush) = 0;
    virtual void stroke(const QVectorPath &path, const QPen &pen);

    virtual void clip(const QVectorPath &path, Qt::ClipOperation op) = 0;
    virtual void clip(const QRect &rect, Qt::ClipOperation op);
    virtual void clip(const QRegion &region, Qt::ClipOperation op);
    virtual void clip(const QPainterPath &path, Qt::ClipOperation op);

    virtual void clipEnabledChanged() = 0;
    virtual void penChanged() = 0;
    virtual void brushChanged() = 0;
    virtual void brushOriginChanged() = 0;
    virtual void opacityChanged() = 0;
    virtual void compositionModeChanged() = 0;
    virtual void renderHintsChanged() = 0;
    virtual void transformChanged() = 0;

    virtual void fillRect(const QRectF &rect, const QBrush &brush);
    virtual void fillRect(const QRectF &rect, const QColor &color);

    virtual void drawRoundedRect(const QRectF &rect, qreal xrad, qreal yrad, Qt::SizeMode mode);

    virtual void drawRects(const QRect *rects, int rectCount) override;
    virtual void drawRects(const QRectF *rects, int rectCount) override;

    virtual void drawLines(const QLine *lines, int lineCount) override;
    virtual void drawLines(const QLineF *lines, int lineCount) override;

    virtual void drawEllipse(const QRectF &r) override;
    virtual void drawEllipse(const QRect &r) override;

    virtual void drawPath(const QPainterPath &path) override;

    virtual void drawPoints(const QPointF *points, int pointCount) override;
    virtual void drawPoints(const QPoint *points, int pointCount) override;

    virtual void drawPolygon(const QPointF *points, int pointCount, PolygonDrawMode mode) override;
    virtual void drawPolygon(const QPoint *points, int pointCount, PolygonDrawMode mode) override;

    virtual void drawPixmap(const QRectF &r, const QPixmap &pm, const QRectF &sr) override = 0;
    virtual void drawPixmap(const QPointF &pos, const QPixmap &pm);

    virtual void drawImage(const QRectF &r, const QImage &pm, const QRectF &sr,
                           Qt::ImageConversionFlags flags = Qt::AutoColor) override = 0;
    virtual void drawImage(const QPointF &pos, const QImage &image);

    virtual void drawTiledPixmap(const QRectF &r, const QPixmap &pixmap, const QPointF &s) override;

    virtual void drawPixmapFragments(const QPainter::PixmapFragment *fragments, int fragmentCount, const QPixmap &pixmap,
                                     QFlags<QPainter::PixmapFragmentHint> hints);

    virtual void updateState(const QPaintEngineState &state) override;

    virtual void drawStaticTextItem(QStaticTextItem *);

    virtual void setState(QPainterState *s);
    inline QPainterState *state() { return static_cast<QPainterState *>(QPaintEngine::state); }
    inline const QPainterState *state() const { return static_cast<const QPainterState *>(QPaintEngine::state); }

    virtual void sync() {}

    virtual void beginNativePainting() {}
    virtual void endNativePainting() {}

    // These flags are needed in the implementation of paint buffers.
    enum Flags
    {
        DoNotEmulate = 0x01,        // If set, QPainter will not wrap this engine in an emulation engine.
        IsEmulationEngine = 0x02    // If set, this object is a QEmulationEngine.
    };
    virtual uint flags() const {return 0;}
    virtual bool requiresPretransformedGlyphPositions(QFontEngine *fontEngine, const QTransform &m) const;
    virtual bool shouldDrawCachedGlyphs(QFontEngine *fontEngine, const QTransform &m) const;

protected:
    QPaintEngineEx(QPaintEngineExPrivate &data);
};

class Q_GUI_EXPORT QPaintEngineExPrivate : public QPaintEnginePrivate
{
    Q_DECLARE_PUBLIC(QPaintEngineEx)
public:
    QPaintEngineExPrivate();
    ~QPaintEngineExPrivate();

    void replayClipOperations();
    bool hasClipOperations() const;

    QStroker stroker;
    QDashStroker dasher;
    StrokeHandler *strokeHandler;
    QStrokerOps *activeStroker;
    QPen strokerPen;

    QRect exDeviceRect;
};

QT_END_NAMESPACE

#endif
