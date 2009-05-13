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

#ifndef TTREE_WIDGET_ITEM_H
#define TTREE_WIDGET_ITEM_H

#include <QTreeWidget>

#include <QTreeWidgetItem>
class TTreeWidget;

class TTreeWidgetItem : public QTreeWidgetItem
{
    //Q_OBJECT
        
public:
        TTreeWidgetItem( TTreeWidgetItem * parent, const QStringList & strings, int type = QTreeWidgetItem::UserType );
    TTreeWidgetItem( int type = QTreeWidgetItem::UserType );
    TTreeWidgetItem( const QStringList & strings, int type = QTreeWidgetItem::UserType );
    TTreeWidgetItem( TTreeWidget * parent, int type = QTreeWidgetItem::UserType );
    TTreeWidgetItem( TTreeWidget * parent, const QStringList & strings, int type = QTreeWidgetItem::UserType );
    TTreeWidgetItem( TTreeWidget * parent, TTreeWidgetItem * preceding, int type = QTreeWidgetItem::UserType );
    TTreeWidgetItem( TTreeWidgetItem * parent, int type = QTreeWidgetItem::UserType );
    TTreeWidgetItem( TTreeWidgetItem * parent, TTreeWidgetItem * preceding, int type = QTreeWidgetItem::UserType );
    TTreeWidgetItem( const TTreeWidgetItem & other );
    
    void addChildren( const QList<QTreeWidgetItem *> & children );
    void addChildren( const QList<TTreeWidgetItem *> & children );
        
        
    void removeChild( QTreeWidgetItem * child );
    TTreeWidgetItem * takeChild ( int index );
    void addChild( QTreeWidgetItem * child );
    //Qt::ItemFlags flags(const QModelIndex &index) const;
    //bool removeRows(int row, int count, const QModelIndex &parent);
    Qt::DropActions supportedDropActions() const;
    //void dragEnterEvent( QDragEnterEvent * event );
    //void dragMoveEvent( QDragMoveEvent * event );
    //void dropEvent( QDropEvent * event );
    //void startDrag( Qt::DropActions supportedActions ); 
    //signals:
    //public slots:
    
};

#endif

