#ifndef MUDLET_TTREEWIDGET_H
#define MUDLET_TTREEWIDGET_H

/***************************************************************************
 *   Copyright (C) 2008-2009 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2014 by Ahmed Charles - acharles@outlook.com            *
 *   Copyright (C) 2022 by Stephen Lyons - slysven@virginmedia.com         *
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


#include <QMap>
#include <QPointer>
#include <QStyleOptionViewItem>
#include <QTreeWidget>

#include <map>
#include <memory>

class Host;
class TVar;
class VarUnit;

enum class TreeType {
    None,
    Trigger,
    Alias,
    Timer,
    Script,
    Key,
    Action,
    Var
};

class TTreeWidget : public QTreeWidget
{
    Q_OBJECT

public:
    Q_DISABLE_COPY(TTreeWidget)
    explicit TTreeWidget(QWidget* pW);
    ~TTreeWidget() override;
    Qt::DropActions supportedDropActions() const override;
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dragLeaveEvent(QDragLeaveEvent* event) override;
    void dragMoveEvent(QDragMoveEvent* event) override;
    void dropEvent(QDropEvent* event) override;
    void startDrag(Qt::DropActions supportedActions) override;
    bool dropMimeData(QTreeWidgetItem* parent, int index, const QMimeData* data, Qt::DropAction action) override;
    void rowsAboutToBeRemoved(const QModelIndex& parent, int start, int end) override;
    void rowsInserted(const QModelIndex& parent, int start, int end) override;
    // QAbstractItemView::initViewItemOption() is protected, and the delegates measure rows from outside the view
    [[nodiscard]] QStyleOptionViewItem viewItemOption() const;
    void setHost(Host* pH);
    void setTreeType(TreeType type);
    void beginInsertRows(const QModelIndex& parent, int first, int last);
    void getAllChildren(QTreeWidgetItem*, QList<QTreeWidgetItem*>&);
    // For a caller that made a row current with the selection model's signals blocked, which keeps the
    // view from telling a screen reader where its focus went
    void reportCurrentRow();

    // Variables view rows. They are this widget's children, so the bookkeeping dies with them.
    void buildVariableRows(VarUnit* pVarUnit, QTreeWidgetItem* pParent, TVar* pVariable, bool showHidden);
    void clearVariableRows();
    TVar* variableForRow(VarUnit* pVarUnit, QTreeWidgetItem* pItem) const;
    TVar* newVariableForRow(VarUnit* pVarUnit, QTreeWidgetItem* pItem) const;
    void setVariableForRow(VarUnit* pVarUnit, QTreeWidgetItem* pItem, TVar* pVariable);
    void setNewVariableForRow(VarUnit* pVarUnit, QTreeWidgetItem* pItem, std::unique_ptr<TVar> pVariable);
    void forgetRow(QTreeWidgetItem* pItem);
    // Gives up the row's unsaved variable without deleting it, for when the variable tree adopts it
    void forgetNewVariableForRow(QTreeWidgetItem* pItem);
    bool rowCanBeSaved(VarUnit* pVarUnit, QTreeWidgetItem* pItem) const;

signals:
    void itemMoved(int itemID, int oldParentID, int newParentID, int oldPosition, int newPosition);
    void batchMoveStarted();
    void batchMoveEnded();

private:
    // Structure to hold information about items being moved
    struct MoveInfo {
        int childID;
        int oldParentID;
        int oldPosition;
    };

    struct VariableRowLook;
    void addVariableRows(VarUnit* pVarUnit, QTreeWidgetItem* pParent, TVar* pVariable, bool showHidden, const VariableRowLook& look);
    bool rowsStandForCurrentVariables(VarUnit* pVarUnit) const;
    void adoptVariableTree(VarUnit* pVarUnit);

    bool mIsDropAction;
    QPointer<Host> mpHost;
    QList<MoveInfo> mPendingMoves;  // Stores info for all items being moved
    int mOldParentID;  // Deprecated: kept for compatibility, will be removed
    int mOldPosition;  // Deprecated: kept for compatibility, will be removed
    int mChildID;      // Deprecated: kept for compatibility, will be removed
    TreeType mTreeType = TreeType::None;
    QMap<QTreeWidgetItem*, TVar*> mVariableForRow;
    // An unsaved new variable is in no table yet, so its row's entry is the only owner it has
    std::map<QTreeWidgetItem*, std::unique_ptr<TVar>> mNewVariableForRow;
    quint64 mVariablesGeneration = 0;
};

#endif // MUDLET_TTREEWIDGET_H
