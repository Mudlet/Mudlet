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
#include "LuaInterface.h"
#include "VarUnit.h"

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

void TTreeWidget::setHost( Host * pH )
{
    mpHost = pH;
}

void TTreeWidget::getAllChildren( QTreeWidgetItem *pItem, QList< QTreeWidgetItem * > & list)
{
    list.append( pItem );
    for(int i=0;i<pItem->childCount();++i)
        getAllChildren( pItem->child(i), list );
}

void TTreeWidget::mouseReleaseEvent( QMouseEvent *event )
{
    QModelIndex indexClicked = indexAt(event->pos());
    if( mIsVarTree && indexClicked.isValid() && mClickedItem == indexClicked )
    {
        QRect vrect = visualRect(indexClicked);
        int itemIndentation = vrect.x() - visualRect(rootIndex()).x();
        QRect rect = QRect(header()->sectionViewportPosition(0) + itemIndentation,
                           vrect.y(), style()->pixelMetric(QStyle::PM_IndicatorWidth), vrect.height());
        if(rect.contains(event->pos()))
        {
            QTreeWidgetItem * clicked = itemFromIndex( indexClicked );
            if ( ! ( clicked->flags()&Qt::ItemIsUserCheckable ) )
                return;
            if ( clicked->checkState( 0 ) == Qt::Unchecked )
            {
                clicked->setCheckState( 0, Qt::Checked );
                //get all children and see what ones we can save
                QList< QTreeWidgetItem * > list;
                getAllChildren( clicked, list );
                QListIterator< QTreeWidgetItem * > it(list);
                LuaInterface * lI = mpHost->getLuaInterface();
                VarUnit * vu = lI->getVarUnit();
                while( it.hasNext() )
                {
                    QTreeWidgetItem * item = it.next();
                    if ( ! vu->shouldSave( item ) )
                        item->setCheckState( 0, Qt::Unchecked );
                }
            }
            else
            {
                clicked->setCheckState( 0, Qt::Unchecked );
            }
            return;
        }
    }
    QTreeWidget::mouseReleaseEvent(event);
}

void TTreeWidget::mousePressEvent( QMouseEvent *event )
{
    QModelIndex indexClicked = indexAt(event->pos());
    if( mIsVarTree && indexClicked.isValid() )
    {
        QRect vrect = visualRect(indexClicked);
        int itemIndentation = vrect.x() - visualRect(rootIndex()).x();
        QRect rect = QRect(header()->sectionViewportPosition(0) + itemIndentation,
                           vrect.y(), style()->pixelMetric(QStyle::PM_IndicatorWidth), vrect.height());
        if(rect.contains(event->pos()))
        {
            mClickedItem = indexClicked;
            QTreeWidget::mousePressEvent(event);
            return;
        }
    }
    QTreeWidget::mousePressEvent(event);
}

void TTreeWidget::rowsAboutToBeRemoved( const QModelIndex & parent, int start, int end )
{
    if( parent.isValid() )
    {
        mOldParentID = parent.data( Qt::UserRole ).toInt();
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
    // determine position in parent list

    if( mIsDropAction )
    {
        QModelIndex child = parent.child( start, 0 );
        int parentPosition = parent.row();
        int childPosition = child.row();
        if( mChildID == 0 )
        {
            if( ! parent.model() ) goto END;
            if( ! mpHost ) goto END;
            mChildID = parent.model()->index( start, 0 ).data( Qt::UserRole ).toInt();
        }
        int newParentID = parent.data( Qt::UserRole ).toInt();
        if( mIsTriggerTree )
            mpHost->getTriggerUnit()->reParentTrigger( mChildID, mOldParentID, newParentID, parentPosition, childPosition );
        if( mIsAliasTree )
            mpHost->getAliasUnit()->reParentAlias( mChildID, mOldParentID, newParentID, parentPosition, childPosition );
        if( mIsKeyTree )
            mpHost->getKeyUnit()->reParentKey( mChildID, mOldParentID, newParentID, parentPosition, childPosition );

        if( mIsTimerTree )
        {
            mpHost->getTimerUnit()->reParentTimer( mChildID, mOldParentID, newParentID, parentPosition, childPosition );
            TTimer * pTChild = mpHost->getTimerUnit()->getTimer( mChildID );
            //TTimer * pTnewParent = mpHost->getTimerUnit()->getTimer( newParentID );
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
                if( ! pParent ) goto END;
                for( int i=0; i<pParent->childCount(); i++ )
                {
                    QTreeWidgetItem * pItem = pParent->child(i);
                    if( ! pItem ) goto END;
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
    END: QTreeWidget::rowsInserted( parent, start, end );
}

Qt::DropActions TTreeWidget::supportedDropActions() const
{
    return Qt::MoveAction;
}


void TTreeWidget::dragEnterEvent(QDragEnterEvent *event)
{
    mIsDropAction = true;
    QTreeWidget::dragEnterEvent( event );
}

void TTreeWidget::dropEvent(QDropEvent *event)
{
    QTreeWidgetItem * pItem = itemAt( event->pos() );

    if( ! pItem )
    {
        event->setDropAction( Qt::IgnoreAction );
        event->ignore();
    }

    if( pItem == topLevelItem(0) )
    {
        if( (dropIndicatorPosition() == QAbstractItemView::AboveItem )
         || (dropIndicatorPosition() == QAbstractItemView::BelowItem ) )
        {
            event->setDropAction( Qt::IgnoreAction );
            event->ignore();
        }
    }

    if ( mIsVarTree )
    {
        LuaInterface * lI = mpHost->getLuaInterface();        
        if ( ! lI->validMove( pItem ) )
        {
            event->setDropAction( Qt::IgnoreAction );
            event->ignore();
        }
        QTreeWidgetItem * newpItem = pItem;
        QTreeWidgetItem * cItem = selectedItems().first();
        QTreeWidgetItem * oldpItem = cItem->parent();
        if ( ! lI->reparentVariable( newpItem, cItem, oldpItem ) )
        {
            event->setDropAction( Qt::IgnoreAction );
            event->ignore();
        }
    }
    mIsDropAction = true;
    QTreeWidget::dropEvent( event );
    return;
}

void TTreeWidget::beginInsertRows ( const QModelIndex & parent, int first, int last )
{
}

void TTreeWidget::dragMoveEvent( QDragMoveEvent * e )
{
    QTreeWidget::dragMoveEvent( e );
}

void TTreeWidget::startDrag( Qt::DropActions supportedActions )
{
    QTreeWidget::startDrag( supportedActions );
}

bool TTreeWidget::dropMimeData ( QTreeWidgetItem * parent, int index, const QMimeData * data, Qt::DropAction action )
{
    return QTreeWidget::dropMimeData( parent, index, data, action );
}


