/***************************************************************************
 *   Copyright (C) 2008-2010 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2014 by Ahmed Charles - acharles@outlook.com            *
 *   Copyright (C) 2022-2023 by Stephen Lyons - slysven@virginmedia.com    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/


#include "TTreeWidget.h"

#include "Host.h"
#include "LuaInterface.h"
#include "TTimer.h"
#include "VarUnit.h"

#include <QtEvents>
#include <QHeaderView>
#include <QToolTip>

TTreeWidget::TTreeWidget(QWidget* pW)
: QTreeWidget(pW)
, mChildID()
{
    setSelectionMode(QAbstractItemView::ExtendedSelection);
    setSelectionBehavior(QAbstractItemView::SelectRows);
    setDragEnabled(true);
    setAcceptDrops(true);
    setDropIndicatorShown(true);
    viewport()->setAcceptDrops(true);
    setDragDropMode(QAbstractItemView::InternalMove);
    mIsDropAction = false;
    mpHost = nullptr;
    mOldParentID = 0;

    mIsTriggerTree = false;
    mIsScriptTree = false;
    mIsTimerTree = false;
    mIsAliasTree = false;
    mIsActionTree = false;
    mIsKeyTree = false;
    mIsVarTree = false;
}

void TTreeWidget::setIsAliasTree()
{
    mIsAliasTree = true;
    mIsTriggerTree = false;
    mIsScriptTree = false;
    mIsTimerTree = false;
    mIsActionTree = false;
    mIsKeyTree = false;
}

void TTreeWidget::setIsVarTree()
{
    mIsVarTree = true;
    mIsAliasTree = false;
    mIsTriggerTree = false;
    mIsScriptTree = false;
    mIsTimerTree = false;
    mIsActionTree = false;
    mIsKeyTree = false;
}

void TTreeWidget::setIsTriggerTree()
{
    mIsTriggerTree = true;
    mIsAliasTree = false;
    mIsScriptTree = false;
    mIsTimerTree = false;
    mIsActionTree = false;
    mIsKeyTree = false;
}

void TTreeWidget::setIsActionTree()
{
    mIsTriggerTree = false;
    mIsAliasTree = false;
    mIsScriptTree = false;
    mIsTimerTree = false;
    mIsKeyTree = false;
    mIsActionTree = true;
}

void TTreeWidget::setIsKeyTree()
{
    mIsTriggerTree = false;
    mIsAliasTree = false;
    mIsScriptTree = false;
    mIsTimerTree = false;
    mIsActionTree = false;
    mIsKeyTree = true;
}

void TTreeWidget::setIsTimerTree()
{
    mIsTimerTree = true;
    mIsTriggerTree = false;
    mIsScriptTree = false;
    mIsAliasTree = false;
    mIsActionTree = false;
    mIsKeyTree = false;
}

void TTreeWidget::setIsScriptTree()
{
    mIsScriptTree = true;
    mIsTriggerTree = false;
    mIsAliasTree = false;
    mIsTimerTree = false;
    mIsActionTree = false;
    mIsKeyTree = false;
}

void TTreeWidget::setHost(Host* pH)
{
    mpHost = pH;
}

void TTreeWidget::getAllChildren(QTreeWidgetItem* pItem, QList<QTreeWidgetItem*>& list)
{
    list.append(pItem);
    for (int i = 0; i < pItem->childCount(); ++i) {
        getAllChildren(pItem->child(i), list);
    }
}

void TTreeWidget::mouseReleaseEvent(QMouseEvent* event)
{
    QModelIndex indexClicked = indexAt(event->pos());
    if (mIsVarTree && indexClicked.isValid() && indexClicked.row() != 0 && mClickedItem == indexClicked) {
        QRect vrect = visualRect(indexClicked);
        int itemIndentation = vrect.x() - visualRect(rootIndex()).x();
        QRect rect = QRect(header()->sectionViewportPosition(0) + itemIndentation, vrect.y(), style()->pixelMetric(QStyle::PM_IndicatorWidth), vrect.height());
        if (rect.contains(event->pos())) {
            QTreeWidgetItem* clicked = itemFromIndex(indexClicked);
            if (!clicked) {
                return;
            }
            if (!(clicked->flags() & Qt::ItemIsUserCheckable)) {
                return;
            }
            if (clicked->checkState(0) == Qt::Unchecked) {
                clicked->setCheckState(0, Qt::Checked);
                //get all children and see what ones we can save
                QList<QTreeWidgetItem*> list;
                getAllChildren(clicked, list);
                QListIterator<QTreeWidgetItem*> it(list);
                LuaInterface* lI = mpHost->getLuaInterface();
                VarUnit* vu = lI->getVarUnit();
                while (it.hasNext()) {
                    QTreeWidgetItem* item = it.next();
                    if (!vu->shouldSave(item)) {
                        item->setCheckState(0, Qt::Unchecked);
                    }
                }
            } else {
                clicked->setCheckState(0, Qt::Unchecked);
            }
            return;
        }
    }
    QTreeWidget::mouseReleaseEvent(event);
}

void TTreeWidget::mousePressEvent(QMouseEvent* event)
{
    QModelIndex indexClicked = indexAt(event->pos());
    if (mIsVarTree && indexClicked.isValid()) {
        QRect vrect = visualRect(indexClicked);
        int itemIndentation = vrect.x() - visualRect(rootIndex()).x();
        QRect rect = QRect(header()->sectionViewportPosition(0) + itemIndentation, vrect.y(), style()->pixelMetric(QStyle::PM_IndicatorWidth), vrect.height());
        if (rect.contains(event->pos())) {
            mClickedItem = indexClicked;
            QTreeWidget::mousePressEvent(event);
            return;
        }
    }

    QTreeWidget::mousePressEvent(event);
}

void TTreeWidget::rowsAboutToBeRemoved(const QModelIndex& parent, int start, int end)
{
    // We only move one element (though it may have its own children) at a time
    // so end is always the same as start (?)
    Q_UNUSED(end)

    if (!mIsDropAction) {
        return;
    }

    // Store information about this item being moved
    MoveInfo moveInfo;
    moveInfo.oldPosition = start;

    if (parent.isValid()) {
        moveInfo.oldParentID = parent.data(Qt::UserRole).toInt();
    } else {
        moveInfo.oldParentID = 0;
    }

    if (!moveInfo.oldParentID) {
        moveInfo.oldParentID = parent.sibling(start, 0).data(Qt::UserRole).toInt();
    }

    if (parent.isValid()) {
        QModelIndex child = parent.model()->index(start, 0, parent);
        moveInfo.childID = child.data(Qt::UserRole).toInt();
        if (!moveInfo.childID) {
            if (parent.isValid()) {
                // This if seems redundant - as it has already been done once
                // and "parent" hasn't changed - so it will always be true:
                child = parent.model()->index(start, 0, QModelIndex());
            }

            if (child.isValid()) {
                moveInfo.childID = child.data(Qt::UserRole).toInt();
            } else {
                moveInfo.childID = 0;
            }
        }
    }

    // Add to the list of pending moves
    mPendingMoves.append(moveInfo);

    // Keep backward compatibility by setting the old member variables to the last item
    mChildID = moveInfo.childID;
    mOldParentID = moveInfo.oldParentID;
    mOldPosition = moveInfo.oldPosition;
}


void TTreeWidget::rowsInserted(const QModelIndex& parent, int start, int end)
{
    // determine position in parent list

    if (mIsDropAction && !mPendingMoves.isEmpty()) {
        // If parent.isValid() is false for the item being considered then that
        // item is a top-level item. The obsolete parent.child(start, 0) that we
        // used to use would return a null "QModelIndex" directly but now,
        // since we must get the (const QAbstractModel*) from parent.model()
        // and use that, we have to handle the case where that returns a
        // nullptr - see: https://github.com/Mudlet/Mudlet/issues/6313
        QModelIndex child = parent.isValid() ? parent.model()->index(start, 0, parent) : QModelIndex();
        int parentPosition = parent.row();
        int childPosition = child.row();

        int newParentID = parent.data(Qt::UserRole).toInt();

        // If moving multiple items, signal start of batch operation for undo system
        if (mPendingMoves.size() > 1) {
            emit batchMoveStarted();
        }

        // Make a copy to avoid iterator invalidation if signals cause re-entry or container reallocation
        const QList<MoveInfo> pendingMovesCopy = mPendingMoves;

        // Process all pending moves
        for (const MoveInfo& moveInfo : pendingMovesCopy) {
            int childID = moveInfo.childID;

            if (!childID) {
                if (!parent.model()) {
                    continue;
                }
                if (!mpHost) {
                    continue;
                }
                childID = parent.model()->index(start, 0).data(Qt::UserRole).toInt();
            }

            // Emit signal for undo system before performing the move
            emit itemMoved(childID, moveInfo.oldParentID, newParentID, moveInfo.oldPosition, childPosition);

            if (mIsTriggerTree) {
                mpHost->getTriggerUnit()->reParentTrigger(childID, moveInfo.oldParentID, newParentID, parentPosition, childPosition);
            } else if (mIsAliasTree) {
                mpHost->getAliasUnit()->reParentAlias(childID, moveInfo.oldParentID, newParentID, parentPosition, childPosition);
            } else if (mIsKeyTree) {
                mpHost->getKeyUnit()->reParentKey(childID, moveInfo.oldParentID, newParentID, parentPosition, childPosition);
            } else if (mIsTimerTree) {
                mpHost->getTimerUnit()->reParentTimer(childID, moveInfo.oldParentID, newParentID, parentPosition, childPosition);
                TTimer* pTChild = mpHost->getTimerUnit()->getTimer(childID);
                if (pTChild) {
                    QIcon icon;
                    if (pTChild->isOffsetTimer()) {
                        if (pTChild->shouldBeActive()) {
                            icon.addPixmap(QPixmap(qsl(":/icons/offsettimer-on.png")), QIcon::Normal, QIcon::Off);
                        } else {
                            icon.addPixmap(QPixmap(qsl(":/icons/offsettimer-off.png")), QIcon::Normal, QIcon::Off);
                        }
                    } else {
                        if (pTChild->shouldBeActive()) {
                            icon.addPixmap(QPixmap(qsl(":/icons/tag_checkbox_checked.png")), QIcon::Normal, QIcon::Off);
                        } else {
                            icon.addPixmap(QPixmap(qsl(":/icons/tag_checkbox.png")), QIcon::Normal, QIcon::Off);
                        }
                    }
                    QTreeWidgetItem* pParent = itemFromIndex(parent);
                    if (pParent) {
                        for (int i = 0; i < pParent->childCount(); i++) {
                            QTreeWidgetItem* pItem = pParent->child(i);
                            if (pItem) {
                                int id = pItem->data(0, Qt::UserRole).toInt();
                                if (id == childID) {
                                    pItem->setIcon(0, icon);
                                }
                            }
                        }
                    }
                }
            } else if (mIsScriptTree) {
                mpHost->getScriptUnit()->reParentScript(childID, moveInfo.oldParentID, newParentID, parentPosition, childPosition);
            } else if (mIsActionTree) {
                mpHost->getActionUnit()->reParentAction(childID, moveInfo.oldParentID, newParentID, parentPosition, childPosition);
                mpHost->getActionUnit()->updateToolbar();
            } else {
                qWarning().nospace().noquote() << "TTreeWidget::rowsInserted(...) WARNING - a TTreeWidget item which has not been classified as a mudlet type detected.";
                // Consider marking this:
                // Q_UNREACHABLE();
            }
        }

        // If moving multiple items, signal end of batch operation for undo system
        if (pendingMovesCopy.size() > 1) {
            emit batchMoveEnded();
        }

        // Clear the pending moves list
        mPendingMoves.clear();

        // Reset backward compatibility variables
        mChildID = 0;
        mOldParentID = 0;
        mOldPosition = 0;
        mIsDropAction = false;
    }

    QTreeWidget::rowsInserted(parent, start, end);
}

Qt::DropActions TTreeWidget::supportedDropActions() const
{
    return Qt::MoveAction;
}


void TTreeWidget::dragEnterEvent(QDragEnterEvent* event)
{
    mIsDropAction = true;
    QTreeWidget::dragEnterEvent(event);
}

void TTreeWidget::dragLeaveEvent(QDragLeaveEvent* event)
{
    // Reset flag when drag is cancelled (user presses Esc, drags outside, etc.)
    mIsDropAction = false;
    QTreeWidget::dragLeaveEvent(event);
}

void TTreeWidget::dropEvent(QDropEvent* event)
{
    QTreeWidgetItem* pItem = itemAt(event->position().toPoint());

    if (!pItem || pItem == topLevelItem(0)) {
        event->setDropAction(Qt::IgnoreAction);
        event->ignore();
    }

    if (mIsVarTree) {
        LuaInterface* lI = mpHost->getLuaInterface();
        auto [isValid, errorMsg] = lI->validMove(pItem);
        if (!isValid) {
            event->setDropAction(Qt::IgnoreAction);
            event->ignore();
            if (!errorMsg.isEmpty()) {
                QToolTip::showText(QCursor::pos(), errorMsg, this);
            }
            return;
        }
    }
    mIsDropAction = true;
    QTreeWidget::dropEvent(event);

    // Reset flag after drop completes
    mIsDropAction = false;
}

void TTreeWidget::beginInsertRows(const QModelIndex& parent, int first, int last)
{
    Q_UNUSED(parent)
    Q_UNUSED(first)
    Q_UNUSED(last)
}

void TTreeWidget::dragMoveEvent(QDragMoveEvent* e)
{
    QTreeWidget::dragMoveEvent(e);
}

void TTreeWidget::startDrag(Qt::DropActions supportedActions)
{
    QTreeWidget::startDrag(supportedActions);
}

bool TTreeWidget::dropMimeData(QTreeWidgetItem* parent, int index, const QMimeData* data, Qt::DropAction action)
{
    return QTreeWidget::dropMimeData(parent, index, data, action);
}
