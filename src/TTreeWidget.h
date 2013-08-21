/***************************************************************************
 *   Copyright (C) 2008-2009 by Heiko Koehn   *
 *   KoehnHeiko@googlemail.com   *
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

#ifndef TTREE_WIDGET_H
#define TTREE_WIDGET_H

#include <QTreeWidget>
#include <QDebug>

class Host;

class TTreeWidget : public QTreeWidget
{
Q_OBJECT
        
public:
        TTreeWidget( QWidget * pW );
    Qt::DropActions supportedDropActions() const;
    void dragEnterEvent( QDragEnterEvent * event );
    void dragMoveEvent( QDragMoveEvent * event );
    void dropEvent( QDropEvent * event );
    void startDrag( Qt::DropActions supportedActions ); 
    bool dropMimeData( QTreeWidgetItem * parent, int index, const QMimeData * data, Qt::DropAction action );
    void rowsAboutToBeRemoved ( const QModelIndex & parent, int start, int end );   
    void rowsInserted( const QModelIndex & parent, int start, int end );
    void mousePressEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);
    void setHost( Host * pH );
    void setIsScriptTree();
    void setIsTimerTree();
    void setIsTriggerTree();
    void setIsAliasTree();
    void setIsActionTree();
    void setIsVarTree();
    void setIsKeyTree();
    void beginInsertRows ( const QModelIndex & parent, int first, int last );
    void getAllChildren( QTreeWidgetItem *, QList< QTreeWidgetItem * > & );
    
private:
    
    bool mIsDropAction;
    Host * mpHost;
    int  mOldParentID;
    int  mChildID;
    bool mIsTriggerTree;
    bool mIsAliasTree;
    bool mIsScriptTree;
    bool mIsTimerTree;
    bool mIsKeyTree;
    bool mIsVarTree;
    bool mIsActionTree;
    QModelIndex mClickedItem;
    
signals:
    
public slots:
    
};

#endif

