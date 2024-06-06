/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtCore module of the Qt Toolkit.
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

#ifndef QGRAPHICSSCENEINDEX_H
#define QGRAPHICSSCENEINDEX_H

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
#include "qgraphicsscene_p.h"
#include "qgraphicsscene.h"
#include <private/qobject_p.h>

#include <QtCore/qnamespace.h>
#include <QtCore/qobject.h>
#include <QtGui/qtransform.h>

QT_REQUIRE_CONFIG(graphicsview);

QT_BEGIN_NAMESPACE

class QGraphicsSceneIndexPrivate;
class QPointF;
class QRectF;
template<typename T> class QList;

typedef bool (*QGraphicsSceneIndexIntersector)(const QGraphicsItem *item, const QRectF &exposeRect, Qt::ItemSelectionMode mode,
                                               const QTransform &deviceTransform, const void *data);

class Q_AUTOTEST_EXPORT QGraphicsSceneIndex : public QObject
{
    Q_OBJECT

public:
    QGraphicsSceneIndex(QGraphicsScene *scene = nullptr);
    virtual ~QGraphicsSceneIndex();

    QGraphicsScene *scene() const;

    virtual QList<QGraphicsItem *> items(Qt::SortOrder order = Qt::DescendingOrder) const  = 0;
    virtual QList<QGraphicsItem *> items(const QPointF &pos, Qt::ItemSelectionMode mode,
                                         Qt::SortOrder order, const QTransform &deviceTransform = QTransform()) const;
    virtual QList<QGraphicsItem *> items(const QRectF &rect, Qt::ItemSelectionMode mode,
                                         Qt::SortOrder order, const QTransform &deviceTransform = QTransform()) const;
    virtual QList<QGraphicsItem *> items(const QPolygonF &polygon, Qt::ItemSelectionMode mode,
                                         Qt::SortOrder order, const QTransform &deviceTransform = QTransform()) const;
    virtual QList<QGraphicsItem *> items(const QPainterPath &path, Qt::ItemSelectionMode mode,
                                         Qt::SortOrder order, const QTransform &deviceTransform = QTransform()) const;
    virtual QList<QGraphicsItem *> estimateItems(const QPointF &point, Qt::SortOrder order) const;
    virtual QList<QGraphicsItem *> estimateItems(const QRectF &rect, Qt::SortOrder order) const = 0;
    virtual QList<QGraphicsItem *> estimateTopLevelItems(const QRectF &, Qt::SortOrder order) const;

protected Q_SLOTS:
    virtual void updateSceneRect(const QRectF &rect);

protected:
    virtual void clear();
    virtual void addItem(QGraphicsItem *item) = 0;
    virtual void removeItem(QGraphicsItem *item) = 0;
    virtual void deleteItem(QGraphicsItem *item);

    virtual void itemChange(const QGraphicsItem *item, QGraphicsItem::GraphicsItemChange, const void *const value);
    virtual void prepareBoundingRectChange(const QGraphicsItem *item);

    QGraphicsSceneIndex(QGraphicsSceneIndexPrivate &dd, QGraphicsScene *scene);

    friend class QGraphicsScene;
    friend class QGraphicsScenePrivate;
    friend class QGraphicsItem;
    friend class QGraphicsItemPrivate;
    friend class QGraphicsSceneBspTreeIndex;
private:
    Q_DISABLE_COPY_MOVE(QGraphicsSceneIndex)
    Q_DECLARE_PRIVATE(QGraphicsSceneIndex)
};

class QGraphicsSceneIndexPrivate : public QObjectPrivate
{
    Q_DECLARE_PUBLIC(QGraphicsSceneIndex)
public:
    QGraphicsSceneIndexPrivate(QGraphicsScene *scene);
    ~QGraphicsSceneIndexPrivate();

    void init();
    static bool itemCollidesWithPath(const QGraphicsItem *item, const QPainterPath &path, Qt::ItemSelectionMode mode);

    void recursive_items_helper(QGraphicsItem *item, QRectF exposeRect,
                                QGraphicsSceneIndexIntersector intersect, QList<QGraphicsItem *> *items,
                                const QTransform &viewTransform,
                                Qt::ItemSelectionMode mode, qreal parentOpacity, const void *intersectData) const;
    inline void items_helper(const QRectF &rect, QGraphicsSceneIndexIntersector intersect,
                             QList<QGraphicsItem *> *items, const QTransform &viewTransform,
                             Qt::ItemSelectionMode mode, Qt::SortOrder order, const void *intersectData) const;

    QGraphicsScene *scene;
};

inline void QGraphicsSceneIndexPrivate::items_helper(const QRectF &rect, QGraphicsSceneIndexIntersector intersect,
                                                     QList<QGraphicsItem *> *items, const QTransform &viewTransform,
                                                     Qt::ItemSelectionMode mode, Qt::SortOrder order, const void *intersectData) const
{
    Q_Q(const QGraphicsSceneIndex);
    const QList<QGraphicsItem *> tli = q->estimateTopLevelItems(rect, Qt::AscendingOrder);
    for (int i = 0; i < tli.size(); ++i)
        recursive_items_helper(tli.at(i), rect, intersect, items, viewTransform, mode, 1.0, intersectData);
    if (order == Qt::DescendingOrder) {
        const int n = items->size();
        for (int i = 0; i < n / 2; ++i)
            items->swapItemsAt(i, n - i - 1);
    }
}

QT_END_NAMESPACE

#endif // QGRAPHICSSCENEINDEX_H
