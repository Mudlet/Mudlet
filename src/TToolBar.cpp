/***************************************************************************
 *   Copyright (C) 2008-2009 by Heiko Koehn                                *
 *   KoehnHeiko@googlemail.com                                             *
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

#include <QWidget>
#include <QtGui>
#include "Host.h"
#include "HostManager.h"
#include "TToolBar.h"
#include "mudlet.h"

TToolBar::TToolBar( TAction * pA, QString name, QWidget * pW )
: QDockWidget( pW )
, mpTAction( pA )
, mVerticalOrientation( false )
, mpWidget( new QWidget( this ) )
, mName( name )
, mRecordMove( false )
, mpLayout( 0 )
, mItemCount( 0 )

{
    setFeatures( QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable );
    setWidget( mpWidget );

    if( ! mpTAction->mUseCustomLayout )
    {
        mpLayout = new QGridLayout( mpWidget );
        setContentsMargins(0,0,0,0);
        mpLayout->setContentsMargins(0,0,0,0);
        mpLayout->setSpacing(0);
        QSizePolicy sizePolicy( QSizePolicy::Preferred, QSizePolicy::Preferred);
        mpWidget->setSizePolicy( sizePolicy );
    }
    QWidget * test = new QWidget;
    setTitleBarWidget(test);
    setStyleSheet( mpTAction->css );
    mpWidget->setStyleSheet( mpTAction->css );
}

void TToolBar::moveEvent( QMoveEvent * e )
{
    if( mRecordMove )
    {
        if( ! mpTAction ) return;
        mpTAction->mPosX = e->pos().x();
        mpTAction->mPosY = e->pos().y();
    }
    e->ignore();
}

void TToolBar::addButton( TFlipButton * pB )
{
    if( ! mpTAction->mUseCustomLayout )
    {
        QSize size = pB->minimumSizeHint();
        if( pB->mpTAction->getButtonRotation() > 0 )
        {
            size.transpose();
        }
        pB->setMaximumSize( size );
        pB->setMinimumSize( size );
    }
    else
    {
        QSize size = QSize(pB->mpTAction->mSizeX, pB->mpTAction->mSizeY );
        pB->setMaximumSize( size );
        pB->setMinimumSize( size );
        pB->setParent( mpWidget );
        pB->setGeometry( pB->mpTAction->mPosX, pB->mpTAction->mPosY, pB->mpTAction->mSizeX, pB->mpTAction->mSizeY );
    }

    pB->setStyleSheet( pB->mpTAction->css );
    pB->setFlat( pB->mpTAction->getButtonFlat() );
    int rotation = pB->mpTAction->getButtonRotation();
    switch( rotation )
    {
        case 0: pB->setOrientation( Qt::Horizontal ); break;
        case 1: pB->setOrientation( Qt::Vertical ); break;
        case 2: pB->setOrientation( Qt::Vertical ); pB->setMirrored( true ); break;
    }

    if( ! mpTAction->mUseCustomLayout )
    {
        // tool bar mButtonColumns > 0 -> autolayout
        // case == 0: use individual button placment for user defined layouts
        int columns = mpTAction->getButtonColumns();
        if( columns <= 0 ) columns = 1;
        if( columns > 0 )
        {
            mItemCount++;
            int row = mItemCount / columns;
            int col = mItemCount % columns;
            if( mVerticalOrientation )
            {
                mpLayout->addWidget( pB, row, col );
            }
            else
            {
                mpLayout->addWidget( pB, col, row );
            }
        }
    }
    else
    {
        pB->move( pB->mpTAction->mPosX, pB->mpTAction->mPosY );
    }
    connect( pB, SIGNAL(pressed()), this, SLOT(slot_pressed()) );
}

void TToolBar::finalize()
{
    if( mpTAction->mUseCustomLayout )
    {
        return;
    }
    QWidget * fillerWidget = new QWidget;
    QSizePolicy sizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding );
    fillerWidget->setSizePolicy( sizePolicy );
    int columns = mpTAction->getButtonColumns();
    if( columns <= 0 ) columns = 1;
    mpLayout->addWidget( fillerWidget, ++mItemCount/columns, mItemCount%columns );
}

void TToolBar::slot_pressed()
{
    TFlipButton * pB = dynamic_cast<TFlipButton *>( sender() );
    if( ! pB )
    {
        return;
    }

    TAction * pA = pB->mpTAction;
    pB->showMenu();

    if( pB->isChecked() )
    {
        pA->mButtonState = 2;
    }
    else
    {
        pA->mButtonState = 1;
    }
    if( pB->isChecked() )
        pA->mpHost->mpConsole->mButtonState = 1;
    else
        pA->mpHost->mpConsole->mButtonState = 0;
    QStringList sL;
    pA->_execute( sL );
}

void TToolBar::clear()
{
    QWidget * pW = new QWidget( this );
    setWidget( pW );
    mpWidget->deleteLater();
    mpWidget = pW;

    if( ! mpTAction->mUseCustomLayout )
    {
        mpLayout = new QGridLayout( mpWidget );
        mpLayout->setContentsMargins(0,0,0,0);
        mpLayout->setSpacing(0);
        QSizePolicy sizePolicy( QSizePolicy::Expanding, QSizePolicy::Expanding );
        mpWidget->setSizePolicy( sizePolicy );
    }
    else
        mpLayout = 0;
    QWidget * test = new QWidget;
    setStyleSheet( mpTAction->css );
    mpWidget->setStyleSheet( mpTAction->css );
    setTitleBarWidget( test );

    mudlet::self()->removeDockWidget( this );
}



