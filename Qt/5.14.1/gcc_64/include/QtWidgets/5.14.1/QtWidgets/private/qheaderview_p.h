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

#ifndef QHEADERVIEW_P_H
#define QHEADERVIEW_P_H

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
#include "private/qabstractitemview_p.h"

#include "QtCore/qbitarray.h"
#include "QtWidgets/qapplication.h"
#if QT_CONFIG(label)
#include "QtWidgets/qlabel.h"
#endif

QT_REQUIRE_CONFIG(itemviews);

QT_BEGIN_NAMESPACE

class QHeaderViewPrivate: public QAbstractItemViewPrivate
{
    Q_DECLARE_PUBLIC(QHeaderView)

public:
    enum StateVersion { VersionMarker = 0xff };

    QHeaderViewPrivate()
        : state(NoState),
          offset(0),
          sortIndicatorOrder(Qt::DescendingOrder),
          sortIndicatorSection(0),
          sortIndicatorShown(false),
          lastPos(-1),
          firstPos(-1),
          originalSize(-1),
          section(-1),
          target(-1),
          pressed(-1),
          hover(-1),
          length(0),
          preventCursorChangeInSetOffset(false),
          movableSections(false),
          clickableSections(false),
          highlightSelected(false),
          stretchLastSection(false),
          cascadingResizing(false),
          resizeRecursionBlock(false),
          allowUserMoveOfSection0(true), // will be false for QTreeView and true for QTableView
          customDefaultSectionSize(false),
          stretchSections(0),
          contentsSections(0),
          minimumSectionSize(-1),
          maximumSectionSize(-1),
          lastSectionSize(0),
          lastSectionLogicalIdx(-1), // Only trust when we stretch last section
          sectionIndicatorOffset(0),
#if QT_CONFIG(label)
          sectionIndicator(nullptr),
#endif
          globalResizeMode(QHeaderView::Interactive),
          sectionStartposRecalc(true),
          resizeContentsPrecision(1000)
    {}


    int lastVisibleVisualIndex() const;
    void restoreSizeOnPrevLastSection();
    void setNewLastSection(int visualIndexForLastSection);
    void maybeRestorePrevLastSectionAndStretchLast();
    int sectionHandleAt(int position);
    void setupSectionIndicator(int section, int position);
    void updateSectionIndicator(int section, int position);
    void updateHiddenSections(int logicalFirst, int logicalLast);
    void resizeSections(QHeaderView::ResizeMode globalMode, bool useGlobalMode = false);
    void _q_sectionsRemoved(const QModelIndex &,int,int);
    void _q_sectionsAboutToBeMoved(const QModelIndex &sourceParent, int logicalStart, int logicalEnd, const QModelIndex &destinationParent, int logicalDestination);
    void _q_sectionsMoved(const QModelIndex &sourceParent, int logicalStart, int logicalEnd, const QModelIndex &destinationParent, int logicalDestination);
    void _q_sectionsAboutToBeChanged(const QList<QPersistentModelIndex> &parents = QList<QPersistentModelIndex>(),
                                     QAbstractItemModel::LayoutChangeHint hint = QAbstractItemModel::NoLayoutChangeHint);
    void _q_sectionsChanged(const QList<QPersistentModelIndex> &parents = QList<QPersistentModelIndex>(),
                            QAbstractItemModel::LayoutChangeHint hint = QAbstractItemModel::NoLayoutChangeHint);

    bool isSectionSelected(int section) const;
    bool isFirstVisibleSection(int section) const;
    bool isLastVisibleSection(int section) const;

    inline bool rowIntersectsSelection(int row) const {
        return (selectionModel ? selectionModel->rowIntersectsSelection(row, root) : false);
    }

    inline bool columnIntersectsSelection(int column) const {
        return (selectionModel ? selectionModel->columnIntersectsSelection(column, root) : false);
    }

    inline bool sectionIntersectsSelection(int logical) const {
        return (orientation == Qt::Horizontal ? columnIntersectsSelection(logical) : rowIntersectsSelection(logical));
    }

    inline bool isRowSelected(int row) const {
        return (selectionModel ? selectionModel->isRowSelected(row, root) : false);
    }

    inline bool isColumnSelected(int column) const {
        return (selectionModel ? selectionModel->isColumnSelected(column, root) : false);
    }

    inline void prepareSectionSelected() {
        if (!selectionModel || !selectionModel->hasSelection())
            sectionSelected.clear();
        else if (sectionSelected.count() != sectionCount() * 2)
            sectionSelected.fill(false, sectionCount() * 2);
        else sectionSelected.fill(false);
    }

    inline int sectionCount() const {return sectionItems.count();}

    inline bool reverse() const {
        return orientation == Qt::Horizontal && q_func()->isRightToLeft();
    }

    inline int logicalIndex(int visualIndex) const {
        return logicalIndices.isEmpty() ? visualIndex : logicalIndices.at(visualIndex);
    }

    inline int visualIndex(int logicalIndex) const {
        return visualIndices.isEmpty() ? logicalIndex : visualIndices.at(logicalIndex);
    }

    inline void setDefaultValues(Qt::Orientation o) {
        orientation = o;
        updateDefaultSectionSizeFromStyle();
        defaultAlignment = (o == Qt::Horizontal
                            ? Qt::Alignment(Qt::AlignCenter)
                            : Qt::AlignLeft|Qt::AlignVCenter);
    }

    inline bool isVisualIndexHidden(int visual) const {
        return sectionItems.at(visual).isHidden;
    }

    inline void setVisualIndexHidden(int visual, bool hidden) {
        sectionItems[visual].isHidden = hidden;
    }

    inline bool hasAutoResizeSections() const {
        return stretchSections || stretchLastSection || contentsSections;
    }

    QStyleOptionHeader getStyleOption() const;

    inline void invalidateCachedSizeHint() const {
        cachedSizeHint = QSize();
    }

    inline void initializeIndexMapping() const {
        if (visualIndices.count() != sectionCount()
            || logicalIndices.count() != sectionCount()) {
            visualIndices.resize(sectionCount());
            logicalIndices.resize(sectionCount());
            for (int s = 0; s < sectionCount(); ++s) {
                visualIndices[s] = s;
                logicalIndices[s] = s;
            }
        }
    }

    inline void clearCascadingSections() {
        firstCascadingSection = sectionItems.count();
        lastCascadingSection = 0;
        cascadingSectionSize.clear();
    }

    inline void saveCascadingSectionSize(int visual, int size) {
        if (!cascadingSectionSize.contains(visual)) {
            cascadingSectionSize.insert(visual, size);
            firstCascadingSection = qMin(firstCascadingSection, visual);
            lastCascadingSection = qMax(lastCascadingSection, visual);
        }
    }

    inline bool sectionIsCascadable(int visual) const {
        return headerSectionResizeMode(visual) == QHeaderView::Interactive;
    }

    inline int modelSectionCount() const {
        return (orientation == Qt::Horizontal
                ? model->columnCount(root)
                : model->rowCount(root));
    }

    inline void doDelayedResizeSections() {
        if (!delayedResize.isActive())
            delayedResize.start(0, q_func());
    }

    inline void executePostedResize() const {
        if (delayedResize.isActive() && state == NoState) {
            const_cast<QHeaderView*>(q_func())->resizeSections();
        }
    }

    void clear();
    void flipSortIndicator(int section);
    void cascadingResize(int visual, int newSize);

    enum State { NoState, ResizeSection, MoveSection, SelectSections, NoClear } state;

    int offset;
    Qt::Orientation orientation;
    Qt::SortOrder sortIndicatorOrder;
    int sortIndicatorSection;
    bool sortIndicatorShown;

    mutable QVector<int> visualIndices; // visualIndex = visualIndices.at(logicalIndex)
    mutable QVector<int> logicalIndices; // logicalIndex = row or column in the model
    mutable QBitArray sectionSelected; // from logical index to bit
    mutable QHash<int, int> hiddenSectionSize; // from logical index to section size
    mutable QHash<int, int> cascadingSectionSize; // from visual index to section size
    mutable QSize cachedSizeHint;
    mutable QBasicTimer delayedResize;

    int firstCascadingSection;
    int lastCascadingSection;

    int lastPos;
    int firstPos;
    int originalSize;
    int section; // used for resizing and moving sections
    int target;
    int pressed;
    int hover;

    int length;
    bool preventCursorChangeInSetOffset;
    bool movableSections;
    bool clickableSections;
    bool highlightSelected;
    bool stretchLastSection;
    bool cascadingResizing;
    bool resizeRecursionBlock;
    bool allowUserMoveOfSection0;
    bool customDefaultSectionSize;
    int stretchSections;
    int contentsSections;
    int defaultSectionSize;
    int minimumSectionSize;
    int maximumSectionSize;
    int lastSectionSize;
    int lastSectionLogicalIdx; // Only trust if we stretch LastSection
    int sectionIndicatorOffset;
    Qt::Alignment defaultAlignment;
#if QT_CONFIG(label)
    QLabel *sectionIndicator;
#endif
    QHeaderView::ResizeMode globalResizeMode;
    mutable bool sectionStartposRecalc;
    int resizeContentsPrecision;
    // header sections

    struct SectionItem {
        uint size : 20;
        uint isHidden : 1;
        uint resizeMode : 5;  // (holding QHeaderView::ResizeMode)
        uint currentlyUnusedPadding : 6;

        union { // This union is made in order to save space and ensure good vector performance (on remove)
            mutable int calculated_startpos; // <- this is the primary used member.
            mutable int tmpLogIdx;         // When one of these 'tmp'-members has been used we call
            int tmpDataStreamSectionCount; // recalcSectionStartPos() or set sectionStartposRecalc to true
        };                                 // to ensure that calculated_startpos will be calculated afterwards.

        inline SectionItem() : size(0), isHidden(0), resizeMode(QHeaderView::Interactive) {}
        inline SectionItem(int length, QHeaderView::ResizeMode mode)
            : size(length), isHidden(0), resizeMode(mode), calculated_startpos(-1) {}
        inline int sectionSize() const { return size; }
        inline int calculatedEndPos() const { return calculated_startpos + size; }
#ifndef QT_NO_DATASTREAM
        inline void write(QDataStream &out) const
        { out << static_cast<int>(size); out << 1; out << (int)resizeMode; }
        inline void read(QDataStream &in)
        { int m; in >> m; size = m; in >> tmpDataStreamSectionCount; in >> m; resizeMode = m; }
#endif
    };

    QVector<SectionItem> sectionItems;
    struct LayoutChangeItem {
        QPersistentModelIndex index;
        SectionItem section;
    };
    QVector<LayoutChangeItem> layoutChangePersistentSections;

    void createSectionItems(int start, int end, int size, QHeaderView::ResizeMode mode);
    void removeSectionsFromSectionItems(int start, int end);
    void resizeSectionItem(int visualIndex, int oldSize, int newSize);
    void setDefaultSectionSize(int size);
    void updateDefaultSectionSizeFromStyle();
    void recalcSectionStartPos() const; // not really const

    inline int headerLength() const { // for debugging
        int len = 0;
        for (const auto &section : sectionItems)
            len += section.size;
        return len;
    }

    QBitArray sectionsHiddenToBitVector() const
    {
        QBitArray sectionHidden;
        if (!hiddenSectionSize.isEmpty()) {
            sectionHidden.resize(sectionItems.size());
            for (int u = 0; u < sectionItems.size(); ++u)
                sectionHidden[u] = sectionItems.at(u).isHidden;
        }
        return sectionHidden;
    }

    void setHiddenSectionsFromBitVector(const QBitArray &sectionHidden) {
        SectionItem *sectionData = sectionItems.data();
        for (int i = 0; i < sectionHidden.count(); ++i)
            sectionData[i].isHidden = sectionHidden.at(i);
    }

    int headerSectionSize(int visual) const;
    int headerSectionPosition(int visual) const;
    int headerVisualIndexAt(int position) const;

    // resize mode
    void setHeaderSectionResizeMode(int visual, QHeaderView::ResizeMode mode);
    QHeaderView::ResizeMode headerSectionResizeMode(int visual) const;
    void setGlobalHeaderResizeMode(QHeaderView::ResizeMode mode);

    // other
    int viewSectionSizeHint(int logical) const;
    int adjustedVisualIndex(int visualIndex) const;
    void setScrollOffset(const QScrollBar *scrollBar, QAbstractItemView::ScrollMode scrollMode);

#ifndef QT_NO_DATASTREAM
    void write(QDataStream &out) const;
    bool read(QDataStream &in);
#endif

};
Q_DECLARE_TYPEINFO(QHeaderViewPrivate::SectionItem, Q_PRIMITIVE_TYPE);
Q_DECLARE_TYPEINFO(QHeaderViewPrivate::LayoutChangeItem, Q_MOVABLE_TYPE);

QT_END_NAMESPACE

#endif // QHEADERVIEW_P_H
