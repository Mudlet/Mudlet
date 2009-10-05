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


#include <QtGui>
#include <QTreeWidgetItem>
#include "Host.h"
#include "HostManager.h"
#include "TTreeWidgetItem.h"

TTreeWidgetItem::TTreeWidgetItem( TTreeWidgetItem * parent, const QStringList & strings, int type )
: QTreeWidgetItem( (QTreeWidgetItem*)parent, strings, type )
{
}

TTreeWidgetItem::TTreeWidgetItem( int type )
: QTreeWidgetItem( type )
{
}

TTreeWidgetItem::TTreeWidgetItem( const QStringList & strings, int type )
: QTreeWidgetItem( strings, type )
{
}

TTreeWidgetItem::TTreeWidgetItem( TTreeWidget * parent, int type )
: QTreeWidgetItem( (QTreeWidgetItem*)parent, type )
{
}

TTreeWidgetItem::TTreeWidgetItem( TTreeWidget * parent, const QStringList & strings, int type )
: QTreeWidgetItem( (QTreeWidgetItem*)parent, strings, type )
{
}

TTreeWidgetItem::TTreeWidgetItem( TTreeWidget * parent, TTreeWidgetItem * preceding, int type )
: QTreeWidgetItem( (QTreeWidgetItem*)parent, (QTreeWidgetItem*)preceding, type )
{
}

TTreeWidgetItem::TTreeWidgetItem( TTreeWidgetItem * parent, int type )
: QTreeWidgetItem( (QTreeWidgetItem*)parent, type )
{
}



TTreeWidgetItem::TTreeWidgetItem( TTreeWidgetItem * parent, TTreeWidgetItem * preceding, int type )
: QTreeWidgetItem( (QTreeWidgetItem*)parent, (QTreeWidgetItem*)preceding, type )
{
}
    
TTreeWidgetItem::TTreeWidgetItem( const TTreeWidgetItem & other )
: QTreeWidgetItem( other )
{
}


void TTreeWidgetItem::removeChild( QTreeWidgetItem * child )
{
    QTreeWidgetItem::removeChild( child );
}

void TTreeWidgetItem::addChildren( const QList<QTreeWidgetItem *> & children )
{
    QTreeWidgetItem::addChildren( children );
}

void TTreeWidgetItem::addChildren( const QList<TTreeWidgetItem *> & children )
{
    //QTreeWidgetItem::addChildren( children );
}



TTreeWidgetItem * TTreeWidgetItem::takeChild ( int index )
{
    return static_cast<TTreeWidgetItem *>(QTreeWidgetItem::takeChild( index ));
}

void TTreeWidgetItem::addChild( QTreeWidgetItem * child )
{
    QTreeWidgetItem::addChild( child );
}

