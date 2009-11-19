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

#include <QTreeWidget>
#include <QTreeWidgetItem>
#include "TTreeWidget.h"
#include <QtGui>
#include "Host.h"
#include "HostManager.h"
#include "TDebug.h"

TTreeWidget::TTreeWidget( QWidget * pW ) : QTreeWidget( pW )
{
    setSelectionMode( QAbstractItemView::SingleSelection );
    setSelectionBehavior( QAbstractItemView::SelectRows );
    setDragEnabled( true );
    setAcceptDrops( true );
    setDropIndicatorShown( true );
    viewport()->setAcceptDrops( true );
    setDragDropMode( QAbstractItemView::InternalMove );
    mIsDropAction = false;
    mpHost = 0;
    mOldParentID = 0;
    
    mIsTriggerTree = false;
    mIsScriptTree = false;
    mIsTimerTree = false;
    mIsAliasTree = false;
    mIsActionTree = false;
    mIsKeyTree = false;
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

void TTreeWidget::setHost( Host * pH )
{
    mpHost = pH;    
}

void TTreeWidget::rowsAboutToBeRemoved( const QModelIndex & parent, int start, int end )
{
    //TDebug()<<"rowsAboutToBeRemoved">>0;
    if( parent.isValid() )
    {
        mOldParentID = parent.data( Qt::UserRole ).toInt();
        //TDebug()<<"mOldParentID="<<mOldParentID>>0;
    }
    else 
        mOldParentID = 0;
    
    if( mOldParentID == 0 )
    {
        mOldParentID = parent.sibling( start, 0 ).data( Qt::UserRole ).toInt();
    }
        
    if( parent.isValid() )
    {
        QModelIndex child = parent.child( start, 0 );
        mChildID = child.data( Qt::UserRole ).toInt();
        //TDebug()<<"mChildID="<<mChildID>>0;
        if( mChildID == 0 )
        {
            if( parent.isValid() )
            {
                child = parent.model()->index( start, 0, QModelIndex() );
            }
            if( child.isValid() )        
            {
                mChildID = child.data( Qt::UserRole ).toInt();
            }
            else
                mChildID = 0;
        }
    }
    //QTreeWidget::rowsAboutToBeRemoved( parent, start, end );
}


void TTreeWidget::rowsInserted( const QModelIndex & parent, int start, int end )
{
    // determin position in parent list

    if( mIsDropAction )
    {
        QModelIndex child = parent.child( start, 0 );
        int parentPosition = parent.row();
        int childPosition = child.row();
        if( mChildID == 0 )
        {
            mChildID = parent.model()->index( start, 0 ).data( Qt::UserRole ).toInt();
            //qDebug()<<"item has parent: parent row="<<parent.row()<<" child.row="<<child.row()<<" pos="<<child.row()-parent.row();
        }
        //else
        //    qDebug()<<"iten has no parent: parent row="<<parent.row()<<" child.row="<<child.row()<<" pos="<<child.row()-parent.row();
        int newParentID = parent.data( Qt::UserRole ).toInt();
        //TDebug()<<"rowsInserted() newParentID="<<newParentID>>0;
        if( mIsTriggerTree ) 
            mpHost->getTriggerUnit()->reParentTrigger( mChildID, mOldParentID, newParentID, parentPosition, childPosition );
        if( mIsAliasTree ) 
            mpHost->getAliasUnit()->reParentAlias( mChildID, mOldParentID, newParentID, parentPosition, childPosition );
        if( mIsTimerTree )
        {
            mpHost->getTimerUnit()->reParentTimer( mChildID, mOldParentID, newParentID, parentPosition, childPosition );
            TTimer * pTChild = mpHost->getTimerUnit()->getTimer( mChildID );
            TTimer * pTnewParent = mpHost->getTimerUnit()->getTimer( newParentID );
            if( pTChild )
            {
                QIcon icon;
                if( pTChild->isOffsetTimer() )
                {
                    if( pTChild->shouldBeActive() )
                    {
                        icon.addPixmap(QPixmap(QString::fromUtf8(":/icons/offsettimer-on.png")), QIcon::Normal, QIcon::Off);
                    }
                    else
                    {
                        icon.addPixmap(QPixmap(QString::fromUtf8(":/icons/offsettimer-off.png")), QIcon::Normal, QIcon::Off);            
                    }
                }
                else
                {
                    if( pTChild->shouldBeActive() )
                    {
                        icon.addPixmap(QPixmap(QString::fromUtf8(":/icons/tag_checkbox_checked.png")), QIcon::Normal, QIcon::Off);
                    }
                    else
                    {
                        icon.addPixmap(QPixmap(QString::fromUtf8(":/icons/tag_checkbox.png")), QIcon::Normal, QIcon::Off);            
                    }
                }   
                QTreeWidgetItem * pParent = itemFromIndex( parent );
                for( int i=0; i<pParent->childCount(); i++ )
                {
                    QTreeWidgetItem * pItem = pParent->child(i);
                    int id = pItem->data(0, Qt::UserRole).toInt();
                    if( id == mChildID )
                    {
                        pItem->setIcon(0, icon); 
                    }
                }
            }
        }
        if( mIsScriptTree ) 
            mpHost->getScriptUnit()->reParentScript( mChildID, mOldParentID, newParentID, parentPosition, childPosition );
        if( mIsActionTree ) 
        {
            mpHost->getActionUnit()->reParentAction( mChildID, mOldParentID, newParentID, parentPosition, childPosition );
            mpHost->getActionUnit()->updateToolbar();
        }
        
        mChildID = 0;
        mOldParentID = 0;
        mIsDropAction = false;
    }
    QTreeWidget::rowsInserted( parent, start, end );
}

Qt::DropActions TTreeWidget::supportedDropActions() const
{
    return Qt::MoveAction;
}


void TTreeWidget::dragEnterEvent(QDragEnterEvent *event)
{
    mIsDropAction = true;
    QTreeWidget::dragEnterEvent( event );
    //event->accept();
}

void TTreeWidget::dragMoveEvent(QDragMoveEvent *event)
{
    QTreeWidget::dragMoveEvent( event );
    //event->accept();
}

void TTreeWidget::dropEvent(QDropEvent *event)
{
    QTreeWidgetItem * pItem = itemAt( event->pos() );

    if( ! pItem )
    {
        event->ignore();
        return;
    }

    if( ! pItem->parent() )
    {
        event->ignore();
        return;
    }

    mIsDropAction = true;
    QTreeWidget::dropEvent( event );
    //event->accept();
    return;
}


void TTreeWidget::startDrag( Qt::DropActions supportedActions )
{
    QTreeWidgetItem * item = currentItem();
    
    QTreeWidget::startDrag( supportedActions );
}

bool TTreeWidget::dropMimeData ( QTreeWidgetItem * parent, int index, const QMimeData * data, Qt::DropAction action )
{
    return QTreeWidget::dropMimeData( parent, index, data, action );
}


