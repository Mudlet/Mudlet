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
#include "TTrigger.h"
#include "TVar.h"
#include "VarUnit.h"
#include "utils.h"

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
}

void TTreeWidget::setTreeType(TreeType type)
{
    mTreeType = type;

    // A variables tree is a view of what Lua holds, and moving an item in it
    // moves nothing in Lua - the item lands inside the table it was dropped on
    // while the variable stays where it was, so the view ends up showing
    // something that is not true (#9958). Until a move can be carried through to
    // Lua, the tree does not offer one.
    if (mTreeType == TreeType::Var) {
        setDragDropMode(QAbstractItemView::NoDragDrop);
        setDragEnabled(false);
        setAcceptDrops(false);
        setDropIndicatorShown(false);
        viewport()->setAcceptDrops(false);
    }
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
    if (mTreeType == TreeType::Var && indexClicked.isValid() && indexClicked.row() != 0 && mClickedItem == indexClicked) {
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
                    if (!rowCanBeSaved(vu, item)) {
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
    if (mTreeType == TreeType::Var && indexClicked.isValid()) {
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

            switch (mTreeType) {
            case TreeType::Trigger: {
                mpHost->getTriggerUnit()->reParentTrigger(childID, moveInfo.oldParentID, newParentID, parentPosition, childPosition);

                // Update icons for affected triggers after the move
                // 1. Update the new parent's icon (may become a filter chain)
                QTreeWidgetItem* pNewParentItem = itemFromIndex(parent);
                if (pNewParentItem && newParentID != 0) {
                    updateTriggerIcon(pNewParentItem, newParentID);
                }

                // 2. Update the old parent's icon (may stop being a filter chain)
                if (moveInfo.oldParentID != 0 && moveInfo.oldParentID != newParentID) {
                    QTreeWidgetItem* pOldParentItem = findItemByTriggerID(invisibleRootItem(), moveInfo.oldParentID);
                    if (pOldParentItem) {
                        updateTriggerIcon(pOldParentItem, moveInfo.oldParentID);
                    }
                }

                // 3. Update the moved child and all its descendants (ancestors may have changed)
                if (pNewParentItem) {
                    for (int i = 0; i < pNewParentItem->childCount(); ++i) {
                        QTreeWidgetItem* pChildItem = pNewParentItem->child(i);
                        if (pChildItem && pChildItem->data(0, Qt::UserRole).toInt() == childID) {
                            updateTriggerIconsRecursively(pChildItem);
                            break;
                        }
                    }
                }
                break;
            }
            case TreeType::Alias:
                mpHost->getAliasUnit()->reParentAlias(childID, moveInfo.oldParentID, newParentID, parentPosition, childPosition);
                break;
            case TreeType::Key:
                mpHost->getKeyUnit()->reParentKey(childID, moveInfo.oldParentID, newParentID, parentPosition, childPosition);
                break;
            case TreeType::Timer: {
                mpHost->getTimerUnit()->reParentTimer(childID, moveInfo.oldParentID, newParentID, parentPosition, childPosition);
                TTimer* pTChild = mpHost->getTimerUnit()->getTimer(childID);
                if (pTChild) {
                    QIcon icon;
                    if (pTChild->isFolder()) {
                        // Timer folder
                        if (pTChild->shouldBeActive()) {
                            if (pTChild->ancestorsActive()) {
                                if (!pTChild->mPackageName.isEmpty()) {
                                    icon.addPixmap(QPixmap(qsl(":/icons/folder-brown.png")), QIcon::Normal, QIcon::Off);
                                } else {
                                    icon.addPixmap(QPixmap(qsl(":/icons/folder-green.png")), QIcon::Normal, QIcon::Off);
                                }
                            } else {
                                icon.addPixmap(QPixmap(qsl(":/icons/folder-grey.png")), QIcon::Normal, QIcon::Off);
                            }
                        } else {
                            if (pTChild->ancestorsActive()) {
                                if (!pTChild->mPackageName.isEmpty()) {
                                    icon.addPixmap(QPixmap(qsl(":/icons/folder-brown-locked.png")), QIcon::Normal, QIcon::Off);
                                } else {
                                    icon.addPixmap(QPixmap(qsl(":/icons/folder-green-locked.png")), QIcon::Normal, QIcon::Off);
                                }
                            } else {
                                icon.addPixmap(QPixmap(qsl(":/icons/folder-grey-locked.png")), QIcon::Normal, QIcon::Off);
                            }
                        }
                    } else if (pTChild->isOffsetTimer()) {
                        // Offset timer
                        if (pTChild->shouldBeActive()) {
                            if (pTChild->ancestorsActive()) {
                                icon.addPixmap(QPixmap(qsl(":/icons/offsettimer-on.png")), QIcon::Normal, QIcon::Off);
                            } else {
                                icon.addPixmap(QPixmap(qsl(":/icons/offsettimer-on-grey.png")), QIcon::Normal, QIcon::Off);
                            }
                        } else {
                            if (pTChild->ancestorsActive()) {
                                icon.addPixmap(QPixmap(qsl(":/icons/offsettimer-off.png")), QIcon::Normal, QIcon::Off);
                            } else {
                                icon.addPixmap(QPixmap(qsl(":/icons/offsettimer-off-grey.png")), QIcon::Normal, QIcon::Off);
                            }
                        }
                    } else {
                        // Regular timer
                        if (pTChild->shouldBeActive()) {
                            if (pTChild->ancestorsActive()) {
                                icon.addPixmap(QPixmap(qsl(":/icons/tag_checkbox_checked.png")), QIcon::Normal, QIcon::Off);
                            } else {
                                icon.addPixmap(QPixmap(qsl(":/icons/tag_checkbox_checked_grey.png")), QIcon::Normal, QIcon::Off);
                            }
                        } else {
                            if (pTChild->ancestorsActive()) {
                                icon.addPixmap(QPixmap(qsl(":/icons/tag_checkbox.png")), QIcon::Normal, QIcon::Off);
                            } else {
                                icon.addPixmap(QPixmap(qsl(":/icons/tag_checkbox_grey.png")), QIcon::Normal, QIcon::Off);
                            }
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
                break;
            }
            case TreeType::Script:
                mpHost->getScriptUnit()->reParentScript(childID, moveInfo.oldParentID, newParentID, parentPosition, childPosition);
                break;
            case TreeType::Action:
                mpHost->getActionUnit()->reParentAction(childID, moveInfo.oldParentID, newParentID, parentPosition, childPosition);
                mpHost->getActionUnit()->updateAllToolbars();
                break;
            case TreeType::Var:
            case TreeType::None:
                qWarning().nospace().noquote() << "TTreeWidget::rowsInserted(...) WARNING - a TTreeWidget item which has not been classified as a mudlet type detected.";
                break;
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

    if (mTreeType == TreeType::Var) {
        LuaInterface* lI = mpHost->getLuaInterface();
        auto [isValid, errorMsg] = lI->validMove(variableForRow(lI->getVarUnit(), pItem));
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

// Helper to find a tree widget item by trigger ID
QTreeWidgetItem* TTreeWidget::findItemByTriggerID(QTreeWidgetItem* pParent, int triggerID)
{
    if (!pParent) {
        return nullptr;
    }

    for (int i = 0; i < pParent->childCount(); ++i) {
        QTreeWidgetItem* pChild = pParent->child(i);
        if (pChild->data(0, Qt::UserRole).toInt() == triggerID) {
            return pChild;
        }
        QTreeWidgetItem* pFound = findItemByTriggerID(pChild, triggerID);
        if (pFound) {
            return pFound;
        }
    }
    return nullptr;
}

void TTreeWidget::buildVariableRows(VarUnit* pVarUnit, QTreeWidgetItem* pParent, TVar* pVariable, bool showHidden)
{
    // rows from the tree that came before stand for variables this walk is
    // about to replace, so they go rather than be re-validated by the stamp
    clearVariableRows();
    mVariablesGeneration = pVarUnit->treeGeneration();
    addVariableRows(pVarUnit, pParent, pVariable, showHidden);
}

void TTreeWidget::addVariableRows(VarUnit* pVarUnit, QTreeWidgetItem* pParent, TVar* pVariable, bool showHidden)
{
    QList<QTreeWidgetItem*> cList;
    for (TVar* child : pVariable->getChildren(true)) {
        if (!showHidden && pVarUnit->isHidden(child)) {
            continue;
        }
        auto pItem = new QTreeWidgetItem(QStringList() << child->getName());
        pItem->setText(0, child->getName());
        pItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsDropEnabled | Qt::ItemIsDragEnabled | Qt::ItemIsAutoTristate | Qt::ItemIsUserCheckable);
        //: Tooltip on a row in the editor's Variables view, offering to keep that variable between sessions
        pItem->setToolTip(0, utils::richText(tr("Checked variables will be saved and loaded with your profile.")));
        pItem->setCheckState(0, Qt::Unchecked);
        if (pVarUnit->isSaved(child)) {
            pItem->setCheckState(0, Qt::Checked);
        }
        if (!pVarUnit->shouldSave(child)) {
            pItem->setFlags(pItem->flags() & ~(Qt::ItemIsDropEnabled | Qt::ItemIsDragEnabled | Qt::ItemIsUserCheckable));
            pItem->setForeground(0, QBrush(QColor("grey")));
            const QString reason = pVarUnit->getUnsaveableReason(child);
            pItem->setToolTip(0, reason.isEmpty() ? QString() : utils::richText(reason));
        }
        pItem->setData(0, Qt::UserRole, child->getValueType());
        QIcon icon;
        switch (child->getValueType()) {
        case LUA_TTABLE:
            icon.addPixmap(QPixmap(qsl(":/icons/table.png")), QIcon::Normal, QIcon::Off);
            break;
        case LUA_TFUNCTION:
            icon.addPixmap(QPixmap(qsl(":/icons/function.png")), QIcon::Normal, QIcon::Off);
            break;
        default:
            icon.addPixmap(QPixmap(qsl(":/icons/variable.png")), QIcon::Normal, QIcon::Off);
            break;
        }
        pItem->setIcon(0, icon);
        mVariableForRow.insert(pItem, child);
        cList.append(pItem);
        if (child->getValueType() == LUA_TTABLE) {
            addVariableRows(pVarUnit, pItem, child, showHidden);
        }
    }
    pParent->addChildren(cList);
}

void TTreeWidget::clearVariableRows()
{
    mVariableForRow.clear();
    mNewVariableForRow.clear();
}

// The rows outlive the variables they stand for: resetting a profile builds a
// fresh variable tree and frees every TVar without the editor hearing about it.
// Answering from rows built against a tree that is gone would hand back a freed
// pointer, so they only answer while the tree they were built from is the
// current one.
bool TTreeWidget::rowsStandForCurrentVariables(VarUnit* pVarUnit) const
{
    return pVarUnit->treeGeneration() == mVariablesGeneration;
}

// Rows left over from a tree that has since been replaced stand for variables
// that are gone, so they go rather than sit beside entries for the current tree.
void TTreeWidget::adoptVariableTree(VarUnit* pVarUnit)
{
    if (rowsStandForCurrentVariables(pVarUnit)) {
        return;
    }
    mVariableForRow.clear();
    mNewVariableForRow.clear();
    mVariablesGeneration = pVarUnit->treeGeneration();
}

TVar* TTreeWidget::variableForRow(VarUnit* pVarUnit, QTreeWidgetItem* pItem) const
{
    return rowsStandForCurrentVariables(pVarUnit) ? mVariableForRow.value(pItem) : nullptr;
}

TVar* TTreeWidget::newVariableForRow(VarUnit* pVarUnit, QTreeWidgetItem* pItem) const
{
    return rowsStandForCurrentVariables(pVarUnit) ? mNewVariableForRow.value(pItem) : nullptr;
}

void TTreeWidget::setVariableForRow(VarUnit* pVarUnit, QTreeWidgetItem* pItem, TVar* pVariable)
{
    adoptVariableTree(pVarUnit);
    mVariableForRow.insert(pItem, pVariable);
}

void TTreeWidget::setNewVariableForRow(VarUnit* pVarUnit, QTreeWidgetItem* pItem, TVar* pVariable)
{
    adoptVariableTree(pVarUnit);
    mNewVariableForRow.insert(pItem, pVariable);
}

void TTreeWidget::forgetRow(QTreeWidgetItem* pItem)
{
    mVariableForRow.remove(pItem);
    mNewVariableForRow.remove(pItem);
}

void TTreeWidget::forgetNewVariableForRow(QTreeWidgetItem* pItem)
{
    mNewVariableForRow.remove(pItem);
}

// The same question VarUnit::shouldSave() answers for a variable, asked of the
// row standing for it - so it has to give the same answer. It used to leave out
// the size limit, and Qt's tristate cascade ticks a child whose
// ItemIsUserCheckable flag buildVariableRows() stripped, so a table over the
// limit reached by ticking its parent stayed ticked, went into savedVars and was
// written into the profile (#9957).
bool TTreeWidget::rowCanBeSaved(VarUnit* pVarUnit, QTreeWidgetItem* pItem) const
{
    TVar* var = variableForRow(pVarUnit, pItem);

    return var && pVarUnit->shouldSave(var);
}

// Update a single trigger item's icon based on its current state
void TTreeWidget::updateTriggerIcon(QTreeWidgetItem* pItem, int triggerID)
{
    if (!pItem || !mpHost) {
        return;
    }

    TTrigger* pT = mpHost->getTriggerUnit()->getTrigger(triggerID);
    if (!pT) {
        return;
    }

    QIcon icon;
    if (pT->state()) {
        if (pT->isFilterChain()) {
            if (pT->isActive()) {
                if (pT->ancestorsActive()) {
                    icon.addPixmap(QPixmap(qsl(":/icons/filter.png")), QIcon::Normal, QIcon::Off);
                } else {
                    icon.addPixmap(QPixmap(qsl(":/icons/filter-grey.png")), QIcon::Normal, QIcon::Off);
                }
            } else {
                if (pT->ancestorsActive()) {
                    icon.addPixmap(QPixmap(qsl(":/icons/filter-locked.png")), QIcon::Normal, QIcon::Off);
                } else {
                    icon.addPixmap(QPixmap(qsl(":/icons/filter-grey-locked.png")), QIcon::Normal, QIcon::Off);
                }
            }
        } else if (pT->isFolder()) {
            if (pT->isActive()) {
                if (pT->ancestorsActive()) {
                    icon.addPixmap(QPixmap(qsl(":/icons/folder-blue.png")), QIcon::Normal, QIcon::Off);
                } else {
                    icon.addPixmap(QPixmap(qsl(":/icons/folder-grey.png")), QIcon::Normal, QIcon::Off);
                }
            } else {
                if (pT->ancestorsActive()) {
                    icon.addPixmap(QPixmap(qsl(":/icons/folder-blue-locked.png")), QIcon::Normal, QIcon::Off);
                } else {
                    icon.addPixmap(QPixmap(qsl(":/icons/folder-grey-locked.png")), QIcon::Normal, QIcon::Off);
                }
            }
        } else {
            if (pT->isActive()) {
                if (pT->ancestorsActive()) {
                    icon.addPixmap(QPixmap(qsl(":/icons/tag_checkbox_checked.png")), QIcon::Normal, QIcon::Off);
                } else {
                    icon.addPixmap(QPixmap(qsl(":/icons/tag_checkbox_checked_grey.png")), QIcon::Normal, QIcon::Off);
                }
            } else {
                if (pT->ancestorsActive()) {
                    icon.addPixmap(QPixmap(qsl(":/icons/tag_checkbox.png")), QIcon::Normal, QIcon::Off);
                } else {
                    icon.addPixmap(QPixmap(qsl(":/icons/tag_checkbox-grey.png")), QIcon::Normal, QIcon::Off);
                }
            }
        }
    } else {
        icon.addPixmap(QPixmap(qsl(":/icons/tools-report-bug.png")), QIcon::Normal, QIcon::Off);
    }
    pItem->setIcon(0, icon);
}

// Recursively update trigger icons for an item and all its children
void TTreeWidget::updateTriggerIconsRecursively(QTreeWidgetItem* pItem)
{
    if (!pItem || !mpHost) {
        return;
    }

    int triggerID = pItem->data(0, Qt::UserRole).toInt();
    updateTriggerIcon(pItem, triggerID);

    for (int i = 0; i < pItem->childCount(); ++i) {
        updateTriggerIconsRecursively(pItem->child(i));
    }
}
