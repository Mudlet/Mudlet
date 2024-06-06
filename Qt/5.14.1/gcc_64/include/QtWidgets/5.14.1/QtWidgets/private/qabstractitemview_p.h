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

#ifndef QABSTRACTITEMVIEW_P_H
#define QABSTRACTITEMVIEW_P_H

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

#include <QtWidgets/private/qtwidgetsglobal_p.h>
#include "private/qabstractscrollarea_p.h"
#include "private/qabstractitemmodel_p.h"
#include "QtWidgets/qapplication.h"
#include "QtGui/qevent.h"
#include "QtCore/qmimedata.h"
#include "QtGui/qpainter.h"
#include "QtCore/qpair.h"
#include "QtGui/qregion.h"
#include "QtCore/qdebug.h"
#include "QtCore/qbasictimer.h"
#include "QtCore/qelapsedtimer.h"

QT_REQUIRE_CONFIG(itemviews);

QT_BEGIN_NAMESPACE

struct QEditorInfo {
    QEditorInfo(QWidget *e, bool s): widget(QPointer<QWidget>(e)), isStatic(s) {}
    QEditorInfo(): isStatic(false) {}

    QPointer<QWidget> widget;
    bool isStatic;
};

//  Fast associativity between Persistent editors and indices.
typedef QHash<QWidget *, QPersistentModelIndex> QEditorIndexHash;
typedef QHash<QPersistentModelIndex, QEditorInfo> QIndexEditorHash;

struct QItemViewPaintPair {
    QRect rect;
    QModelIndex index;
};
template <>
class QTypeInfo<QItemViewPaintPair> : public QTypeInfoMerger<QItemViewPaintPair, QRect, QModelIndex> {};

typedef QVector<QItemViewPaintPair> QItemViewPaintPairs;

class Q_AUTOTEST_EXPORT QAbstractItemViewPrivate : public QAbstractScrollAreaPrivate
{
    Q_DECLARE_PUBLIC(QAbstractItemView)

public:
    QAbstractItemViewPrivate();
    virtual ~QAbstractItemViewPrivate();

    void init();

    virtual void _q_rowsRemoved(const QModelIndex &parent, int start, int end);
    virtual void _q_rowsInserted(const QModelIndex &parent, int start, int end);
    virtual void _q_columnsAboutToBeRemoved(const QModelIndex &parent, int start, int end);
    virtual void _q_columnsRemoved(const QModelIndex &parent, int start, int end);
    virtual void _q_columnsInserted(const QModelIndex &parent, int start, int end);
    virtual void _q_modelDestroyed();
    virtual void _q_layoutChanged();
    virtual void _q_rowsMoved(const QModelIndex &source, int sourceStart, int sourceEnd, const QModelIndex &destination, int destinationStart);
    virtual void _q_columnsMoved(const QModelIndex &source, int sourceStart, int sourceEnd, const QModelIndex &destination, int destinationStart);

    void _q_headerDataChanged() { doDelayedItemsLayout(); }
    void _q_scrollerStateChanged();

    void fetchMore();

    bool shouldEdit(QAbstractItemView::EditTrigger trigger, const QModelIndex &index) const;
    bool shouldForwardEvent(QAbstractItemView::EditTrigger trigger, const QEvent *event) const;
    bool shouldAutoScroll(const QPoint &pos) const;
    void doDelayedItemsLayout(int delay = 0);
    void interruptDelayedItemsLayout() const;

    void updateGeometry();

    void startAutoScroll()
    {   // ### it would be nice to make this into a style hint one day
        int scrollInterval = (verticalScrollMode == QAbstractItemView::ScrollPerItem) ? 150 : 50;
        autoScrollTimer.start(scrollInterval, q_func());
        autoScrollCount = 0;
    }
    void stopAutoScroll() { autoScrollTimer.stop(); autoScrollCount = 0;}

#if QT_CONFIG(draganddrop)
    virtual bool dropOn(QDropEvent *event, int *row, int *col, QModelIndex *index);
#endif
    bool droppingOnItself(QDropEvent *event, const QModelIndex &index);

    QWidget *editor(const QModelIndex &index, const QStyleOptionViewItem &options);
    bool sendDelegateEvent(const QModelIndex &index, QEvent *event) const;
    bool openEditor(const QModelIndex &index, QEvent *event);
    void updateEditorData(const QModelIndex &topLeft, const QModelIndex &bottomRight);

    QItemSelectionModel::SelectionFlags multiSelectionCommand(const QModelIndex &index,
                                                              const QEvent *event) const;
    QItemSelectionModel::SelectionFlags extendedSelectionCommand(const QModelIndex &index,
                                                                 const QEvent *event) const;
    QItemSelectionModel::SelectionFlags contiguousSelectionCommand(const QModelIndex &index,
                                                                   const QEvent *event) const;
    virtual void selectAll(QItemSelectionModel::SelectionFlags command);

    void setHoverIndex(const QPersistentModelIndex &index);

    void checkMouseMove(const QPersistentModelIndex &index);
    inline void checkMouseMove(const QPoint &pos) { checkMouseMove(q_func()->indexAt(pos)); }

    inline QItemSelectionModel::SelectionFlags selectionBehaviorFlags() const
    {
        switch (selectionBehavior) {
        case QAbstractItemView::SelectRows: return QItemSelectionModel::Rows;
        case QAbstractItemView::SelectColumns: return QItemSelectionModel::Columns;
        case QAbstractItemView::SelectItems: default: return QItemSelectionModel::NoUpdate;
        }
    }

#if QT_CONFIG(draganddrop)
    virtual QAbstractItemView::DropIndicatorPosition position(const QPoint &pos, const QRect &rect, const QModelIndex &idx) const;

    inline bool canDrop(QDropEvent *event) {
        const QMimeData *mime = event->mimeData();

        // Drag enter event shall always be accepted, if mime type and action match.
        // Whether the data can actually be dropped will be checked in drag move.
        if (event->type() == QEvent::DragEnter && (event->dropAction() & model->supportedDropActions())) {
            const QStringList modelTypes = model->mimeTypes();
            for (const auto &modelType : modelTypes) {
                if (mime->hasFormat(modelType))
                    return true;
            }
        }

        QModelIndex index;
        int col = -1;
        int row = -1;
        if (dropOn(event, &row, &col, &index)) {
            return model->canDropMimeData(mime,
                                          dragDropMode == QAbstractItemView::InternalMove ? Qt::MoveAction : event->dropAction(),
                                          row, col, index);
        }
        return false;
    }

    inline void paintDropIndicator(QPainter *painter)
    {
        if (showDropIndicator && state == QAbstractItemView::DraggingState
#ifndef QT_NO_CURSOR
            && viewport->cursor().shape() != Qt::ForbiddenCursor
#endif
            ) {
            QStyleOption opt;
            opt.init(q_func());
            opt.rect = dropIndicatorRect;
            q_func()->style()->drawPrimitive(QStyle::PE_IndicatorItemViewItemDrop, &opt, painter, q_func());
        }
    }

#endif
    virtual QItemViewPaintPairs draggablePaintPairs(const QModelIndexList &indexes, QRect *r) const;
    // reimplemented in subclasses
    virtual void adjustViewOptionsForIndex(QStyleOptionViewItem*, const QModelIndex&) const {}

    inline void releaseEditor(QWidget *editor, const QModelIndex &index = QModelIndex()) const {
        if (editor) {
            QObject::disconnect(editor, SIGNAL(destroyed(QObject*)),
                                q_func(), SLOT(editorDestroyed(QObject*)));
            editor->removeEventFilter(itemDelegate);
            editor->hide();
            QAbstractItemDelegate *delegate = delegateForIndex(index);

            if (delegate)
                delegate->destroyEditor(editor, index);
            else
                editor->deleteLater();
        }
    }

    inline void executePostedLayout() const {
        if (delayedPendingLayout && state != QAbstractItemView::CollapsingState) {
            interruptDelayedItemsLayout();
            const_cast<QAbstractItemView*>(q_func())->doItemsLayout();
        }
    }

    inline void setDirtyRegion(const QRegion &visualRegion) {
        updateRegion += visualRegion;
        if (!updateTimer.isActive())
            updateTimer.start(0, q_func());
    }

    inline void scrollDirtyRegion(int dx, int dy) {
        scrollDelayOffset = QPoint(-dx, -dy);
        updateDirtyRegion();
        scrollDelayOffset = QPoint(0, 0);
    }

    inline void scrollContentsBy(int dx, int dy) {
        scrollDirtyRegion(dx, dy);
        viewport->scroll(dx, dy);
    }

    void updateDirtyRegion() {
        updateTimer.stop();
        viewport->update(updateRegion);
        updateRegion = QRegion();
    }

    void clearOrRemove();
    void checkPersistentEditorFocus();

    QPixmap renderToPixmap(const QModelIndexList &indexes, QRect *r) const;

    inline QPoint offset() const {
        const Q_Q(QAbstractItemView);
        return QPoint(q->isRightToLeft() ? -q->horizontalOffset()
                      : q->horizontalOffset(), q->verticalOffset());
    }

    const QEditorInfo &editorForIndex(const QModelIndex &index) const;
    bool hasEditor(const QModelIndex &index) const;

    QModelIndex indexForEditor(QWidget *editor) const;
    void addEditor(const QModelIndex &index, QWidget *editor, bool isStatic);
    void removeEditor(QWidget *editor);

    inline bool isAnimating() const {
        return state == QAbstractItemView::AnimatingState;
    }

    inline QAbstractItemDelegate *delegateForIndex(const QModelIndex &index) const {
        QMap<int, QPointer<QAbstractItemDelegate> >::ConstIterator it;

        it = rowDelegates.find(index.row());
        if (it != rowDelegates.end())
            return it.value();

        it = columnDelegates.find(index.column());
        if (it != columnDelegates.end())
            return it.value();

        return itemDelegate;
    }

    inline bool isIndexValid(const QModelIndex &index) const {
         return (index.row() >= 0) && (index.column() >= 0) && (index.model() == model);
    }
    inline bool isIndexSelectable(const QModelIndex &index) const {
        return (model->flags(index) & Qt::ItemIsSelectable);
    }
    inline bool isIndexEnabled(const QModelIndex &index) const {
        return (model->flags(index) & Qt::ItemIsEnabled);
    }
    inline bool isIndexDropEnabled(const QModelIndex &index) const {
        return (model->flags(index) & Qt::ItemIsDropEnabled);
    }
    inline bool isIndexDragEnabled(const QModelIndex &index) const {
        return (model->flags(index) & Qt::ItemIsDragEnabled);
    }

    virtual bool selectionAllowed(const QModelIndex &index) const {
        // in some views we want to go ahead with selections, even if the index is invalid
        return isIndexValid(index) && isIndexSelectable(index);
    }

    // reimplemented from QAbstractScrollAreaPrivate
    QPoint contentsOffset() const override {
        Q_Q(const QAbstractItemView);
        return QPoint(q->horizontalOffset(), q->verticalOffset());
    }

    /**
     * For now, assume that we have few editors, if we need a more efficient implementation
     * we should add a QMap<QAbstractItemDelegate*, int> member.
     */
    int delegateRefCount(const QAbstractItemDelegate *delegate) const
    {
        int ref = 0;
        if (itemDelegate == delegate)
            ++ref;

        for (int maps = 0; maps < 2; ++maps) {
            const QMap<int, QPointer<QAbstractItemDelegate> > *delegates = maps ? &columnDelegates : &rowDelegates;
            for (QMap<int, QPointer<QAbstractItemDelegate> >::const_iterator it = delegates->begin();
                it != delegates->end(); ++it) {
                    if (it.value() == delegate) {
                        ++ref;
                        // optimization, we are only interested in the ref count values 0, 1 or >=2
                        if (ref >= 2) {
                            return ref;
                        }
                    }
            }
        }
        return ref;
    }

    /**
     * return true if the index is registered as a QPersistentModelIndex
     */
    inline bool isPersistent(const QModelIndex &index) const
    {
        return static_cast<QAbstractItemModelPrivate *>(model->d_ptr.data())->persistent.indexes.contains(index);
    }

    QModelIndexList selectedDraggableIndexes() const;

    QStyleOptionViewItem viewOptionsV1() const;

    void doDelayedReset()
    {
        //we delay the reset of the timer because some views (QTableView)
        //with headers can't handle the fact that the model has been destroyed
        //all _q_modelDestroyed slots must have been called
        if (!delayedReset.isActive())
            delayedReset.start(0, q_func());
    }

    QAbstractItemModel *model;
    QPointer<QAbstractItemDelegate> itemDelegate;
    QMap<int, QPointer<QAbstractItemDelegate> > rowDelegates;
    QMap<int, QPointer<QAbstractItemDelegate> > columnDelegates;
    QPointer<QItemSelectionModel> selectionModel;
    QItemSelectionModel::SelectionFlag ctrlDragSelectionFlag;
    bool noSelectionOnMousePress;

    QAbstractItemView::SelectionMode selectionMode;
    QAbstractItemView::SelectionBehavior selectionBehavior;

    QEditorIndexHash editorIndexHash;
    QIndexEditorHash indexEditorHash;
    QSet<QWidget*> persistent;
    QWidget *currentlyCommittingEditor;

    QPersistentModelIndex enteredIndex;
    QPersistentModelIndex pressedIndex;
    QPersistentModelIndex currentSelectionStartIndex;
    Qt::KeyboardModifiers pressedModifiers;
    QPoint pressedPosition;
    bool pressedAlreadySelected;

    //forces the next mouseMoveEvent to send the viewportEntered signal
    //if the mouse is over the viewport and not over an item
    bool viewportEnteredNeeded;

    QAbstractItemView::State state;
    QAbstractItemView::State stateBeforeAnimation;
    QAbstractItemView::EditTriggers editTriggers;
    QAbstractItemView::EditTrigger lastTrigger;

    QPersistentModelIndex root;
    QPersistentModelIndex hover;

    bool tabKeyNavigation;

#if QT_CONFIG(draganddrop)
    bool showDropIndicator;
    QRect dropIndicatorRect;
    bool dragEnabled;
    QAbstractItemView::DragDropMode dragDropMode;
    bool overwrite;
    QAbstractItemView::DropIndicatorPosition dropIndicatorPosition;
    Qt::DropAction defaultDropAction;
#endif

    QString keyboardInput;
    QElapsedTimer keyboardInputTime;

    bool autoScroll;
    QBasicTimer autoScrollTimer;
    int autoScrollMargin;
    int autoScrollCount;
    bool shouldScrollToCurrentOnShow; //used to know if we should scroll to current on show event
    bool shouldClearStatusTip; //if there is a statustip currently shown that need to be cleared when leaving.

    bool alternatingColors;

    QSize iconSize;
    Qt::TextElideMode textElideMode;

    QRegion updateRegion; // used for the internal update system
    QPoint scrollDelayOffset;

    QBasicTimer updateTimer;
    QBasicTimer delayedEditing;
    QBasicTimer delayedAutoScroll; //used when an item is clicked
    QBasicTimer delayedReset;

    QAbstractItemView::ScrollMode verticalScrollMode;
    QAbstractItemView::ScrollMode horizontalScrollMode;

#ifndef QT_NO_GESTURES
    // the selection before the last mouse down. In case we have to restore it for scrolling
    QItemSelection oldSelection;
    QModelIndex oldCurrent;
#endif

    bool currentIndexSet;

    bool wrapItemText;
    mutable bool delayedPendingLayout;
    bool moveCursorUpdatedView;

    // Whether scroll mode has been explicitly set or its value come from SH_ItemView_ScrollMode
    bool verticalScrollModeSet;
    bool horizontalScrollModeSet;

private:
    mutable QBasicTimer delayedLayout;
    mutable QBasicTimer fetchMoreTimer;
};

QT_BEGIN_INCLUDE_NAMESPACE
#include <qvector.h>
QT_END_INCLUDE_NAMESPACE

template <typename T>
inline int qBinarySearch(const QVector<T> &vec, const T &item, int start, int end)
{
    int i = (start + end + 1) >> 1;
    while (end - start > 0) {
        if (vec.at(i) > item)
            end = i - 1;
        else
            start = i;
        i = (start + end + 1) >> 1;
    }
    return i;
}

QT_END_NAMESPACE

#endif // QABSTRACTITEMVIEW_P_H
