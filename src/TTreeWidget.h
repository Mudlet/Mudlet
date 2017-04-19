#ifndef MUDLET_TTREEWIDGET_H
#define MUDLET_TTREEWIDGET_H

/***************************************************************************
 *   Copyright (C) 2008-2009 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2014 by Ahmed Charles - acharles@outlook.com            *
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


#include "pre_guard.h"
#include <QPointer>
#include <QTreeWidget>
#include "post_guard.h"

class Host;


class TTreeWidget : public QTreeWidget
{
    Q_OBJECT

    Q_DISABLE_COPY(TTreeWidget)

public:
    TTreeWidget(QWidget* pW);
    Qt::DropActions supportedDropActions() const override;
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dragMoveEvent(QDragMoveEvent* event) override;
    void dropEvent(QDropEvent* event) override;
    void startDrag(Qt::DropActions supportedActions) override;
    bool dropMimeData(QTreeWidgetItem* parent, int index, const QMimeData* data, Qt::DropAction action) override;
    void rowsAboutToBeRemoved(const QModelIndex& parent, int start, int end) override;
    void rowsInserted(const QModelIndex& parent, int start, int end) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void setHost(Host* pH);
    void setIsScriptTree();
    void setIsTimerTree();
    void setIsTriggerTree();
    void setIsAliasTree();
    void setIsActionTree();
    void setIsVarTree();
    void setIsKeyTree();
    void beginInsertRows(const QModelIndex& parent, int first, int last);
    void getAllChildren(QTreeWidgetItem*, QList<QTreeWidgetItem*>&);

private:
    bool mIsDropAction;
    QPointer<Host> mpHost;
    int mOldParentID;
    int mChildID;
    bool mIsTriggerTree;
    bool mIsAliasTree;
    bool mIsScriptTree;
    bool mIsTimerTree;
    bool mIsKeyTree;
    bool mIsVarTree;
    bool mIsActionTree;
    QModelIndex mClickedItem;
};

#endif // MUDLET_TTREEWIDGET_H
