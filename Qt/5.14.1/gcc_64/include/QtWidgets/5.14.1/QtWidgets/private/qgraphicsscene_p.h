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

#ifndef QGRAPHICSSCENE_P_H
#define QGRAPHICSSCENE_P_H

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
#include "qgraphicsscene.h"

#include "qgraphicssceneevent.h"
#include "qgraphicsview.h"
#include "qgraphicsview_p.h"
#include "qgraphicsitem_p.h"

#include <private/qobject_p.h>
#include <QtCore/qbitarray.h>
#include <QtCore/qlist.h>
#include <QtCore/qmap.h>
#include <QtCore/qset.h>
#include <QtGui/qfont.h>
#include <QtGui/qpalette.h>
#include <QtWidgets/qstyle.h>
#include <QtWidgets/qstyleoption.h>

#include <set>
#include <tuple>

QT_REQUIRE_CONFIG(graphicsview);

QT_BEGIN_NAMESPACE

class QGraphicsSceneIndex;
class QGraphicsView;
class QGraphicsWidget;

class Q_AUTOTEST_EXPORT QGraphicsScenePrivate : public QObjectPrivate
{
    Q_DECLARE_PUBLIC(QGraphicsScene)
public:
    QGraphicsScenePrivate();
    void init();

    static QGraphicsScenePrivate *get(QGraphicsScene *q);

    int changedSignalIndex;
    int processDirtyItemsIndex;
    int polishItemsIndex;

    QGraphicsScene::ItemIndexMethod indexMethod;
    QGraphicsSceneIndex *index;

    int lastItemCount;

    QRectF sceneRect;

    quint32 hasSceneRect : 1;
    quint32 dirtyGrowingItemsBoundingRect : 1;
    quint32 updateAll : 1;
    quint32 calledEmitUpdated : 1;
    quint32 processDirtyItemsEmitted : 1;
    quint32 needSortTopLevelItems : 1;
    quint32 holesInTopLevelSiblingIndex : 1;
    quint32 topLevelSequentialOrdering : 1;
    quint32 scenePosDescendantsUpdatePending : 1;
    quint32 stickyFocus : 1;
    quint32 hasFocus : 1;
    quint32 lastMouseGrabberItemHasImplicitMouseGrab : 1;
    quint32 allItemsIgnoreHoverEvents : 1;
    quint32 allItemsUseDefaultCursor : 1;
    quint32 painterStateProtection : 1;
    quint32 sortCacheEnabled : 1; // for compatibility
    quint32 allItemsIgnoreTouchEvents : 1;
    quint32 focusOnTouch : 1;
    quint32 padding : 14;

    qreal minimumRenderSize;

    QRectF growingItemsBoundingRect;

    void _q_emitUpdated();

    struct UpdatedRectsCmp
    {
        bool operator() (const QRectF &a, const QRectF &b) const noexcept
        {
            return std::make_tuple(a.y(), a.x(), a.height(), a.width())
                    < std::make_tuple(b.y(), b.x(), b.height(), b.width());
        }
    };

    // std::set was used here instead of std::unordered_set due to requiring only a comparator and
    // showing equivalent performance in empirical measurements within the ranges of interest...
    std::set<QRectF, UpdatedRectsCmp> updatedRects;

    QPainterPath selectionArea;
    int selectionChanging;
    QSet<QGraphicsItem *> selectedItems;
    QVector<QGraphicsItem *> unpolishedItems;
    QList<QGraphicsItem *> topLevelItems;

    QHash<QGraphicsItem *, QPointF> movingItemsInitialPositions;
    void registerTopLevelItem(QGraphicsItem *item);
    void unregisterTopLevelItem(QGraphicsItem *item);
    void _q_updateLater();
    void _q_polishItems();

    void _q_processDirtyItems();

    QSet<QGraphicsItem *> scenePosItems;
    void setScenePosItemEnabled(QGraphicsItem *item, bool enabled);
    void registerScenePosItem(QGraphicsItem *item);
    void unregisterScenePosItem(QGraphicsItem *item);
    void _q_updateScenePosDescendants();

    void removeItemHelper(QGraphicsItem *item);

    QBrush backgroundBrush;
    QBrush foregroundBrush;

    quint32 rectAdjust;
    QGraphicsItem *focusItem;
    QGraphicsItem *lastFocusItem;
    QGraphicsItem *passiveFocusItem;
    QGraphicsWidget *tabFocusFirst;
    QGraphicsItem *activePanel;
    QGraphicsItem *lastActivePanel;
    int activationRefCount;
    int childExplicitActivation;
    void setActivePanelHelper(QGraphicsItem *item, bool duringActivationEvent);
    void setFocusItemHelper(QGraphicsItem *item, Qt::FocusReason focusReason,
                            bool emitFocusChanged = true);

    QList<QGraphicsWidget *> popupWidgets;
    void addPopup(QGraphicsWidget *widget);
    void removePopup(QGraphicsWidget *widget, bool itemIsDying = false);

    QGraphicsItem *lastMouseGrabberItem;
    QList<QGraphicsItem *> mouseGrabberItems;
    void grabMouse(QGraphicsItem *item, bool implicit = false);
    void ungrabMouse(QGraphicsItem *item, bool itemIsDying = false);
    void clearMouseGrabber();

    QList<QGraphicsItem *> keyboardGrabberItems;
    void grabKeyboard(QGraphicsItem *item);
    void ungrabKeyboard(QGraphicsItem *item, bool itemIsDying = false);
    void clearKeyboardGrabber();

    QGraphicsItem *dragDropItem;
    QGraphicsWidget *enterWidget;
    Qt::DropAction lastDropAction;
    QList<QGraphicsItem *> cachedItemsUnderMouse;
    QList<QGraphicsItem *> hoverItems;
    QPointF lastSceneMousePos;
    void enableMouseTrackingOnViews();
    QMap<Qt::MouseButton, QPointF> mouseGrabberButtonDownPos;
    QMap<Qt::MouseButton, QPointF> mouseGrabberButtonDownScenePos;
    QMap<Qt::MouseButton, QPoint> mouseGrabberButtonDownScreenPos;
    QList<QGraphicsItem *> itemsAtPosition(const QPoint &screenPos,
                                           const QPointF &scenePos,
                                           QWidget *widget) const;
    void storeMouseButtonsForMouseGrabber(QGraphicsSceneMouseEvent *event);

    QList<QGraphicsView *> views;
    void addView(QGraphicsView *view);
    void removeView(QGraphicsView *view);

    QMultiMap<QGraphicsItem *, QGraphicsItem *> sceneEventFilters;
    void installSceneEventFilter(QGraphicsItem *watched, QGraphicsItem *filter);
    void removeSceneEventFilter(QGraphicsItem *watched, QGraphicsItem *filter);
    bool filterDescendantEvent(QGraphicsItem *item, QEvent *event);
    bool filterEvent(QGraphicsItem *item, QEvent *event);
    bool sendEvent(QGraphicsItem *item, QEvent *event);

    bool dispatchHoverEvent(QGraphicsSceneHoverEvent *hoverEvent);
    bool itemAcceptsHoverEvents_helper(const QGraphicsItem *item) const;
    void leaveScene(QWidget *viewport);

    void cloneDragDropEvent(QGraphicsSceneDragDropEvent *dest,
                           QGraphicsSceneDragDropEvent *source);
    void sendDragDropEvent(QGraphicsItem *item,
                           QGraphicsSceneDragDropEvent *dragDropEvent);
    void sendHoverEvent(QEvent::Type type, QGraphicsItem *item,
                        QGraphicsSceneHoverEvent *hoverEvent);
    void sendMouseEvent(QGraphicsSceneMouseEvent *mouseEvent);
    void mousePressEventHandler(QGraphicsSceneMouseEvent *mouseEvent);
    QGraphicsWidget *windowForItem(const QGraphicsItem *item) const;

    void drawItemHelper(QGraphicsItem *item, QPainter *painter,
                        const QStyleOptionGraphicsItem *option, QWidget *widget,
                        bool painterStateProtection);

    void drawItems(QPainter *painter, const QTransform *const viewTransform,
                   QRegion *exposedRegion, QWidget *widget);

    void drawSubtreeRecursive(QGraphicsItem *item, QPainter *painter, const QTransform *const,
                              QRegion *exposedRegion, QWidget *widget, qreal parentOpacity = qreal(1.0),
                              const QTransform *const effectTransform = nullptr);
    void draw(QGraphicsItem *, QPainter *, const QTransform *const, const QTransform *const,
              QRegion *, QWidget *, qreal, const QTransform *const, bool, bool);

    void markDirty(QGraphicsItem *item, const QRectF &rect = QRectF(), bool invalidateChildren = false,
                   bool force = false, bool ignoreOpacity = false, bool removingItemFromScene = false,
                   bool updateBoundingRect = false);
    void processDirtyItemsRecursive(QGraphicsItem *item, bool dirtyAncestorContainsChildren = false,
                                    qreal parentOpacity = qreal(1.0));

    inline void resetDirtyItem(QGraphicsItem *item, bool recursive = false)
    {
        Q_ASSERT(item);
        item->d_ptr->dirty = 0;
        item->d_ptr->paintedViewBoundingRectsNeedRepaint = 0;
        item->d_ptr->geometryChanged = 0;
        if (!item->d_ptr->dirtyChildren)
            recursive = false;
        item->d_ptr->dirtyChildren = 0;
        item->d_ptr->needsRepaint = QRectF();
        item->d_ptr->allChildrenDirty = 0;
        item->d_ptr->fullUpdatePending = 0;
        item->d_ptr->ignoreVisible = 0;
        item->d_ptr->ignoreOpacity = 0;
#if QT_CONFIG(graphicseffect)
        QGraphicsEffect::ChangeFlags flags;
        if (item->d_ptr->notifyBoundingRectChanged) {
            flags |= QGraphicsEffect::SourceBoundingRectChanged;
            item->d_ptr->notifyBoundingRectChanged = 0;
        }
        if (item->d_ptr->notifyInvalidated) {
            flags |= QGraphicsEffect::SourceInvalidated;
            item->d_ptr->notifyInvalidated = 0;
        }
#endif // QT_CONFIG(graphicseffect)
        if (recursive) {
            for (int i = 0; i < item->d_ptr->children.size(); ++i)
                resetDirtyItem(item->d_ptr->children.at(i), recursive);
        }
#if QT_CONFIG(graphicseffect)
        if (flags && item->d_ptr->graphicsEffect)
            item->d_ptr->graphicsEffect->sourceChanged(flags);
#endif // QT_CONFIG(graphicseffect)
    }

    inline void ensureSortedTopLevelItems()
    {
        if (needSortTopLevelItems) {
            std::sort(topLevelItems.begin(), topLevelItems.end(), qt_notclosestLeaf);
            topLevelSequentialOrdering = false;
            needSortTopLevelItems = false;
        }
    }

    void ensureSequentialTopLevelSiblingIndexes();

    QStyle *style;
    QFont font;
    void setFont_helper(const QFont &font);
    void resolveFont();
    void updateFont(const QFont &font);
    QPalette palette;
    void setPalette_helper(const QPalette &palette);
    void resolvePalette();
    void updatePalette(const QPalette &palette);

    QStyleOptionGraphicsItem styleOptionTmp;

    QMap<int, QTouchEvent::TouchPoint> sceneCurrentTouchPoints;
    QMap<int, QGraphicsItem *> itemForTouchPointId;
    static void updateTouchPointsForItem(QGraphicsItem *item, QTouchEvent *touchEvent);
    int findClosestTouchPointId(const QPointF &scenePos);
    void touchEventHandler(QTouchEvent *touchEvent);
    bool sendTouchBeginEvent(QGraphicsItem *item, QTouchEvent *touchEvent);
    void enableTouchEventsOnViews();

    QList<QGraphicsObject *> cachedTargetItems;
#ifndef QT_NO_GESTURES
    QHash<QGraphicsObject *, QSet<QGesture *> > cachedItemGestures;
    QHash<QGraphicsObject *, QSet<QGesture *> > cachedAlreadyDeliveredGestures;
    QHash<QGesture *, QGraphicsObject *> gestureTargets;
    QHash<Qt::GestureType, int>  grabbedGestures;
    void gestureEventHandler(QGestureEvent *event);
    void gestureTargetsAtHotSpots(const QSet<QGesture *> &gestures,
                           Qt::GestureFlag flag,
                           QHash<QGraphicsObject *, QSet<QGesture *> > *targets,
                           QSet<QGraphicsObject *> *itemsSet = nullptr,
                           QSet<QGesture *> *normal = nullptr,
                           QSet<QGesture *> *conflicts = nullptr);
    void cancelGesturesForChildren(QGesture *original);
    void grabGesture(QGraphicsItem *, Qt::GestureType gesture);
    void ungrabGesture(QGraphicsItem *, Qt::GestureType gesture);
#endif // QT_NO_GESTURES

    void updateInputMethodSensitivityInViews();

    QList<QGraphicsItem *> modalPanels;
    void enterModal(QGraphicsItem *item,
                    QGraphicsItem::PanelModality panelModality = QGraphicsItem::NonModal);
    void leaveModal(QGraphicsItem *item);
};

// QRectF::intersects() returns false always if either the source or target
// rectangle's width or height are 0. This works around that problem.
static inline void _q_adjustRect(QRectF *rect)
{
    Q_ASSERT(rect);
    if (!rect->width())
        rect->adjust(qreal(-0.00001), 0, qreal(0.00001), 0);
    if (!rect->height())
        rect->adjust(0, qreal(-0.00001), 0, qreal(0.00001));
}

static inline QRectF adjustedItemBoundingRect(const QGraphicsItem *item)
{
    Q_ASSERT(item);
    QRectF boundingRect(item->boundingRect());
    _q_adjustRect(&boundingRect);
    return boundingRect;
}

static inline QRectF adjustedItemEffectiveBoundingRect(const QGraphicsItem *item)
{
    Q_ASSERT(item);
    QRectF boundingRect(QGraphicsItemPrivate::get(item)->effectiveBoundingRect());
    _q_adjustRect(&boundingRect);
    return boundingRect;
}

QT_END_NAMESPACE

#endif
