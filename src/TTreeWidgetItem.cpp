/***************************************************************************
 *   Copyright (C) 2008 by Heiko Koehn   *
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
    qDebug()<<"constructor 1";
}

TTreeWidgetItem::TTreeWidgetItem( int type )
: QTreeWidgetItem( type )
{
    qDebug()<<"constructor 2";
}

TTreeWidgetItem::TTreeWidgetItem( const QStringList & strings, int type )
: QTreeWidgetItem( strings, type )
{
    qDebug()<<"constructor 3";
}

TTreeWidgetItem::TTreeWidgetItem( TTreeWidget * parent, int type )
: QTreeWidgetItem( (QTreeWidgetItem*)parent, type )
{
    qDebug()<<"constructor 4";
}

TTreeWidgetItem::TTreeWidgetItem( TTreeWidget * parent, const QStringList & strings, int type )
: QTreeWidgetItem( (QTreeWidgetItem*)parent, strings, type )
{
    qDebug()<<"constructor 5";
}

TTreeWidgetItem::TTreeWidgetItem( TTreeWidget * parent, TTreeWidgetItem * preceding, int type )
: QTreeWidgetItem( (QTreeWidgetItem*)parent, (QTreeWidgetItem*)preceding, type )
{
    qDebug()<<"constructor 6";
}

TTreeWidgetItem::TTreeWidgetItem( TTreeWidgetItem * parent, int type )
: QTreeWidgetItem( (QTreeWidgetItem*)parent, type )
{
    qDebug()<<"constructor 7";
}



TTreeWidgetItem::TTreeWidgetItem( TTreeWidgetItem * parent, TTreeWidgetItem * preceding, int type )
: QTreeWidgetItem( (QTreeWidgetItem*)parent, (QTreeWidgetItem*)preceding, type )
{
    qDebug()<<"constructor 8";
}
    
TTreeWidgetItem::TTreeWidgetItem( const TTreeWidgetItem & other )
: QTreeWidgetItem( other )
{
    qDebug()<<"constructor 9";
}


void TTreeWidgetItem::removeChild( QTreeWidgetItem * child )
{
    qDebug()<<"REMOVING CHILD";
    QTreeWidgetItem::removeChild( child );
}

void TTreeWidgetItem::addChildren( const QList<QTreeWidgetItem *> & children )
{
    qDebug()<<"addChildren 1 called";
    QTreeWidgetItem::addChildren( children );
}

void TTreeWidgetItem::addChildren( const QList<TTreeWidgetItem *> & children )
{
    qDebug()<<"addChildren 2 called";
    //QTreeWidgetItem::addChildren( children );
}



TTreeWidgetItem * TTreeWidgetItem::takeChild ( int index )
{
    qDebug()<<"TAKE_CHILD called";
    QTreeWidgetItem::takeChild( index );
}

void TTreeWidgetItem::addChild( QTreeWidgetItem * child )
{
    qDebug()<<"ADDING CHILD";
    QTreeWidgetItem::addChild( child );
    
}

