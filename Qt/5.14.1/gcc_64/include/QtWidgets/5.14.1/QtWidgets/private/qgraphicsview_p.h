/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtWidgets module of the Qt Toolkit.
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

#ifndef QGRAPHICSVIEW_P_H
#define QGRAPHICSVIEW_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of other Qt classes.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtWidgets/private/qtwidgetsglobal_p.h>
#include "qgraphicsview.h"

#include <QtGui/qevent.h>
#include <QtCore/qcoreapplication.h>
#include "qgraphicssceneevent.h"
#include <QtWidgets/qstyleoption.h>
#include <private/qabstractscrollarea_p.h>
#include <private/qapplication_p.h>

QT_REQUIRE_CONFIG(graphicsview);

QT_BEGIN_NAMESPACE

class Q_WIDGETS_EXPORT QGraphicsViewPrivate : public QAbstractScrollAreaPrivate
{
    Q_DECLARE_PUBLIC(QGraphicsView)
public:
    QGraphicsViewPrivate();
    ~QGraphicsViewPrivate();

    void recalculateContentSize();
    void centerView(QGraphicsView::ViewportAnchor anchor);

    QPainter::RenderHints renderHints;

    QGraphicsView::DragMode dragMode;

    quint32 sceneInteractionAllowed : 1;
    quint32 hasSceneRect : 1;
    quint32 connectedToScene : 1;
    quint32 useLastMouseEvent : 1;
    quint32 identityMatrix : 1;
    quint32 dirtyScroll : 1;
    quint32 accelerateScrolling : 1;
    quint32 keepLastCenterPoint : 1;
    quint32 transforming : 1;
    quint32 handScrolling : 1;
    quint32 mustAllocateStyleOptions : 1;
    quint32 mustResizeBackgroundPixmap : 1;
    quint32 fullUpdatePending : 1;
    quint32 hasUpdateClip : 1;
    quint32 padding : 18;

    QRectF sceneRect;
    void updateLastCenterPoint();

    qint64 horizontalScroll() const;
    qint64 verticalScroll() const;

    QRectF mapRectToScene(const QRect &rect) const;
    QRectF mapRectFromScene(const QRectF &rect) const;

    QRect updateClip;
    QPointF mousePressItemPoint;
    QPointF mousePressScenePoint;
    QPoint mousePressViewPoint;
    QPoint mousePressScreenPoint;
    QPointF lastMouseMoveScenePoint;
    QPointF lastRubberbandScenePoint;
    QPoint lastMouseMoveScreenPoint;
    QPoint dirtyScrollOffset;
    Qt::MouseButton mousePressButton;
    QTransform matrix;
    qint64 scrollX, scrollY;
    void updateScroll();

    qreal leftIndent;
    qreal topIndent;

    // Replaying mouse events
    QMouseEvent lastMouseEvent;
    void replayLastMouseEvent();
    void storeMouseEvent(QMouseEvent *event);
    void mouseMoveEventHandler(QMouseEvent *event);

    QPointF lastCenterPoint;
    Qt::Alignment alignment;

    QGraphicsView::ViewportAnchor transformationAnchor;
    QGraphicsView::ViewportAnchor resizeAnchor;
    QGraphicsView::ViewportUpdateMode viewportUpdateMode;
    QGraphicsView::OptimizationFlags optimizationFlags;

    QPointer<QGraphicsScene> scene;
#if QT_CONFIG(rubberband)
    QRect rubberBandRect;
    QRegion rubberBandRegion(const QWidget *widget, const QRect &rect) const;
    void updateRubberBand(const QMouseEvent *event);
    bool rubberBanding;
    Qt::ItemSelectionMode rubberBandSelectionMode;
    Qt::ItemSelectionOperation rubberBandSelectionOperation;
#endif
    int handScrollMotions;

    QGraphicsView::CacheMode cacheMode;

    QVector<QStyleOptionGraphicsItem> styleOptions;
    QStyleOptionGraphicsItem *allocStyleOptionsArray(int numItems);
    void freeStyleOptionsArray(QStyleOptionGraphicsItem *array);

    QBrush backgroundBrush;
    QBrush foregroundBrush;
    QPixmap backgroundPixmap;
    QRegion backgroundPixmapExposed;

#ifndef QT_NO_CURSOR
    QCursor originalCursor;
    bool hasStoredOriginalCursor;
    void _q_setViewportCursor(const QCursor &cursor);
    void _q_unsetViewportCursor();
#endif

    QGraphicsSceneDragDropEvent *lastDragDropEvent;
    void storeDragDropEvent(const QGraphicsSceneDragDropEvent *event);
    void populateSceneDragDropEvent(QGraphicsSceneDragDropEvent *dest,
                                    QDropEvent *source);

    QRect mapToViewRect(const QGraphicsItem *item, const QRectF &rect) const;
    QRegion mapToViewRegion(const QGraphicsItem *item, const QRectF &rect) const;
    QRegion dirtyRegion;
    QRect dirtyBoundingRect;
    void processPendingUpdates();
    inline void updateAll()
    {
        viewport->update();
        fullUpdatePending = true;
        dirtyBoundingRect = QRect();
        dirtyRegion = QRegion();
    }

    inline void dispatchPendingUpdateRequests()
    {
        if (qt_widget_private(viewport)->shouldPaintOnScreen())
            QCoreApplication::sendPostedEvents(viewport, QEvent::UpdateRequest);
        else
            QCoreApplication::sendPostedEvents(viewport->window(), QEvent::UpdateRequest);
    }

    void setUpdateClip(QGraphicsItem *);

    inline bool updateRectF(const QRectF &rect)
    {
        if (rect.isEmpty())
            return false;
        if (optimizationFlags & QGraphicsView::DontAdjustForAntialiasing)
            return updateRect(rect.toAlignedRect().adjusted(-1, -1, 1, 1));
        return updateRect(rect.toAlignedRect().adjusted(-2, -2, 2, 2));
    }

    bool updateRect(const QRect &rect);
    bool updateRegion(const QRectF &rect, const QTransform &xform);
    bool updateSceneSlotReimplementedChecked;
    QRegion exposedRegion;

    QList<QGraphicsItem *> findItems(const QRegion &exposedRegion, bool *allItems,
                                     const QTransform &viewTransform) const;

    QPointF mapToScene(const QPointF &point) const;
    QRectF mapToScene(const QRectF &rect) const;
    static void translateTouchEvent(QGraphicsViewPrivate *d, QTouchEvent *touchEvent);
    void updateInputMethodSensitivity();
};

QT_END_NAMESPACE

#endif
