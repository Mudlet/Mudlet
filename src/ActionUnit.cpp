/***************************************************************************
 *   Copyright (C) 2008-2013 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2014 by Ahmed Charles - acharles@outlook.com            *
 *   Copyright (C) 2017 by Stephen Lyons - slysven@virginmedia.com         *
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


#include "ActionUnit.h"


#include "Host.h"
#include "mudlet.h"
#include "TAction.h"
#include "TCommandLine.h"
#include "TConsole.h"
#include "TEasyButtonBar.h"
#include "TToolBar.h"


using namespace std;

void ActionUnit::_uninstall( TAction * pChild, const QString& packageName )
{
    list<TAction*> * childrenList = pChild->mpMyChildrenList;
    for(auto it2 = childrenList->begin(); it2 != childrenList->end(); it2++)
    {
        TAction * pT = *it2;
        _uninstall( pT, packageName );
        uninstallList.append( pT );
    }
}


void ActionUnit::uninstall(const QString& packageName )
{
    for(auto it = mActionRootNodeList.begin(); it != mActionRootNodeList.end(); it ++ )
    {
        TAction * pT = *it;

        if( pT->mPackageName == packageName )
        {
            _uninstall( pT, packageName );
            uninstallList.append( pT );
        }
    }
    for( int i=0; i<uninstallList.size(); i++ )
    {
        delete uninstallList[i];
    }
    uninstallList.clear();
}

void ActionUnit::compileAll()
{
    for(auto it = mActionRootNodeList.begin(); it != mActionRootNodeList.end(); it++)
    {
        TAction * pChild = *it;
        if( pChild->isActive() )
        {
            pChild->compileAll();
        }
    }
}

TAction * ActionUnit::findAction(const QString & name )
{
    //QMap<int, TAction *>  mActionMap;

    QMapIterator<int,TAction *> it(mActionMap);
    while( it.hasNext() )
    {
        it.next();
        if (it.value()->getName() == name){
            qDebug()<<it.value()->getName();
            TAction * pT = it.value();
            return pT;
        }
    }
    return 0;
}

void ActionUnit::addActionRootNode( TAction * pT, int parentPosition, int childPosition )
{
    if( ! pT ) return;
    if( ! pT->getID() )
    {
        pT->setID( getNewID() );
    }
    if( ( parentPosition == -1 ) || ( childPosition >= static_cast<int>(mActionRootNodeList.size()) ) )
    {
        mActionRootNodeList.push_back( pT );
    }
    else
    {
         // insert item at proper position
        int cnt = 0;
        for(auto it = mActionRootNodeList.begin(); it != mActionRootNodeList.end(); it ++ )
        {
            if( cnt >= childPosition )
            {
                mActionRootNodeList.insert( it, pT );
                break;
            }
            cnt++;
        }
    }

    mActionMap.insert( pT->getID(), pT );

}

void ActionUnit::reParentAction( int childID, int oldParentID, int newParentID, int parentPosition, int childPosition )
{
    TAction * pOldParent = getActionPrivate( oldParentID );
    TAction * pNewParent = getActionPrivate( newParentID );
    TAction * pChild = getActionPrivate( childID );
    if( ! pChild )
    {
        return;
    }
    if( pOldParent )
    {
        pOldParent->popChild( pChild );
    }
    if( ! pOldParent )
    {
        removeActionRootNode( pChild );
    }

    if( pNewParent )
    {
        pNewParent->Tree<TAction>::addChild( pChild, parentPosition, childPosition );
        if( pChild )
            pChild->Tree<TAction>::setParent( pNewParent );
        //cout << "dumping family of newParent:"<<endl;
        //pNewParent->Dump();
    }
    else
    {
        pChild->Tree<TAction>::setParent( 0 );
        addActionRootNode( pChild, parentPosition, childPosition );
    }



    if( ( ! pOldParent ) && ( pNewParent ) )
    {
        if( pChild->mpEasyButtonBar )
        {
            if( pChild->mLocation == 0 ) mpHost->mpConsole->mpTopToolBar->layout()->removeWidget( pChild->mpEasyButtonBar );
            if( pChild->mLocation == 2 ) mpHost->mpConsole->mpLeftToolBar->layout()->removeWidget( pChild->mpEasyButtonBar );
            if( pChild->mLocation == 3 ) mpHost->mpConsole->mpRightToolBar->layout()->removeWidget( pChild->mpEasyButtonBar );
            if( pChild->mLocation == 4 )
            {
                if( pChild->mpToolBar )
                {
                    pChild->mpToolBar->setFloating( false );
                    mudlet::self()->removeDockWidget( pChild->mpToolBar );
                }
            }
        }
    }
}

void ActionUnit::removeActionRootNode( TAction * pT )
{
    if( ! pT ) return;
    mActionRootNodeList.remove( pT );
}

TAction * ActionUnit::getAction( int id )
{
    if( mActionMap.contains( id ) )
    {
        return mActionMap.value( id );
    }
    else
    {
        return 0;
    }
}

TAction * ActionUnit::getActionPrivate( int id )
{
    if( mActionMap.find( id ) != mActionMap.end() )
    {
        return mActionMap.value( id );
    }
    else
    {
        return 0;
    }
}

bool ActionUnit::registerAction( TAction * pT )
{
    if( ! pT ) return false;

    if( pT->getParent() )
    {
        addAction( pT );
        return true;
    }
    else
    {
        addActionRootNode( pT );
        return true;
    }
}

void ActionUnit::unregisterAction( TAction * pT )
{
    if( ! pT ) return;
    if( pT->getParent() && pT->getParent()->mPackageName.isEmpty() )
    {
        removeAction( pT );
        updateToolbar();
        return;
    }
    else
    {
        if( pT->mpEasyButtonBar && pT->mPackageName.isEmpty() )
        {
            if( pT->mLocation == 0 ) mpHost->mpConsole->mpTopToolBar->layout()->removeWidget( pT->mpEasyButtonBar );
            if( pT->mLocation == 2 ) mpHost->mpConsole->mpLeftToolBar->layout()->removeWidget( pT->mpEasyButtonBar );
            if( pT->mLocation == 3 ) mpHost->mpConsole->mpRightToolBar->layout()->removeWidget( pT->mpEasyButtonBar );
            if( pT->mLocation == 4 )
            {
                if( pT->mpToolBar )
                {
                    pT->mpToolBar->setFloating( false );
                    mudlet::self()->removeDockWidget( pT->mpToolBar );
                }
            }
        }
        if( ! pT->getParent() )
            removeActionRootNode( pT );
        else
            removeAction( pT );
        updateToolbar();
        return;
    }
}


void ActionUnit::addAction( TAction * pT )
{
    if( ! pT ) return;

    if( ! pT->getID() )
    {
        pT->setID( getNewID() );
    }

    mActionMap.insert(pT->getID(), pT);
}

void ActionUnit::removeAction( TAction * pT )
{
    if( ! pT ) return;

    mActionMap.remove( pT->getID() );
}


int ActionUnit::getNewID()
{
    return ++mMaxID;
}

std::list<TToolBar *> ActionUnit::getToolBarList()
{
    for(auto it = mActionRootNodeList.begin(); it != mActionRootNodeList.end(); it++)
    {
        if( (*it)->mPackageName.size() > 0 )
        {
            for(auto it3 = (*it)->mpMyChildrenList->begin(); it3 != (*it)->mpMyChildrenList->end(); it3++)
            {
                bool found = false;
                TToolBar * pTB = 0;
                for(auto it2 = mToolBarList.begin(); it2!=mToolBarList.end(); it2++ )
                {
                    if( *it2 == (*it3)->mpToolBar )
                    {
                        found = true;
                        pTB = *it2;
                    }
                }
                if( ! found )
                {
                    pTB = new TToolBar( *it3, (*it3)->getName(), mudlet::self() );
                    mToolBarList.push_back( pTB );
                }
                if( (*it3)->mOrientation == 1 )
                {
                    pTB->setVerticalOrientation();
                }
                else
                {
                    pTB->setHorizontalOrientation();
                }
                constructToolbar( *it3, pTB );
                (*it3)->mpToolBar = pTB;
                pTB->setStyleSheet( pTB->mpTAction->css );
            }
            continue; //action package
        }
        bool found = false;
        TToolBar * pTB = 0;
        for(auto it2 = mToolBarList.begin(); it2!=mToolBarList.end(); it2++ )
        {
            if( *it2 == (*it)->mpToolBar )
            {
                found = true;
                pTB = *it2;
            }
        }
        if( ! found )
        {
            pTB = new TToolBar( *it, (*it)->getName(), mudlet::self() );
            mToolBarList.push_back( pTB );
        }
        if( (*it)->mOrientation == 1 )
        {
            pTB->setVerticalOrientation();
        }
        else
        {
            pTB->setHorizontalOrientation();
        }
        constructToolbar( *it, pTB );
        (*it)->mpToolBar = pTB;
        pTB->setStyleSheet( pTB->mpTAction->css );
    }

    return mToolBarList;
}

std::list<TEasyButtonBar *> ActionUnit::getEasyButtonBarList()
{
    for(auto it = mActionRootNodeList.begin(); it != mActionRootNodeList.end(); it++)
    {

        if( (*it)->mPackageName.size() > 0 )
        {
            for(auto it3 = (*it)->mpMyChildrenList->begin(); it3 != (*it)->mpMyChildrenList->end(); it3++)
            {
                bool found = false;
                TEasyButtonBar * pTB = 0;
                for(auto it2 = mEasyButtonBarList.begin(); it2!=mEasyButtonBarList.end(); it2++ )
                {
                    if( *it2 == (*it3)->mpEasyButtonBar )
                    {
                        found = true;
                        pTB = *it2;
                    }
                }
                if( ! found )
                {
                    pTB = new TEasyButtonBar( *it, (*it3)->getName(), mpHost->mpConsole->mpTopToolBar );
                    mpHost->mpConsole->mpTopToolBar->layout()->addWidget( pTB );
                    mEasyButtonBarList.push_back( pTB );
                    (*it3)->mpEasyButtonBar = pTB; // wird fuer drag&drop gebraucht
                }
                if( (*it3)->mOrientation == 1 )
                {
                    pTB->setVerticalOrientation();
                }
                else
                {
                    pTB->setHorizontalOrientation();
                }
                constructToolbar( *it3, pTB );
                (*it3)->mpEasyButtonBar = pTB;
                pTB->setStyleSheet( pTB->mpTAction->css );
            }
            continue; //action package
        }
        bool found = false;
        TEasyButtonBar * pTB = 0;
        for(auto it2 = mEasyButtonBarList.begin(); it2!=mEasyButtonBarList.end(); it2++ )
        {
            if( *it2 == (*it)->mpEasyButtonBar )
            {
                found = true;
                pTB = *it2;
            }
        }
        if( ! found )
        {
            pTB = new TEasyButtonBar( *it, (*it)->getName(), mpHost->mpConsole->mpTopToolBar );
            mpHost->mpConsole->mpTopToolBar->layout()->addWidget( pTB );
            mEasyButtonBarList.push_back( pTB );
            (*it)->mpEasyButtonBar = pTB; // wird fuer drag&drop gebraucht
        }
        if( (*it)->mOrientation == 1 )
        {
            pTB->setVerticalOrientation();
        }
        else
        {
            pTB->setHorizontalOrientation();
        }
        constructToolbar( *it, pTB );
        (*it)->mpEasyButtonBar = pTB;
        pTB->setStyleSheet( pTB->mpTAction->css );
    }

    return mEasyButtonBarList;
}

TAction * ActionUnit::getHeadAction( TToolBar * pT )
{
    for(auto it = mActionRootNodeList.begin(); it != mActionRootNodeList.end(); it++)
    {
// N/U:         bool found = false;
        for(auto it2 = mToolBarList.begin(); it2!=mToolBarList.end(); it2++ )
        {
            if( pT == (*it)->mpToolBar )
            {
// N/U:                 found = true;
                return *it;
            }
        }
    }
    return 0;
}

void ActionUnit::showToolBar(const QString & name )
{
    for(auto it = mEasyButtonBarList.begin(); it!=mEasyButtonBarList.end(); it++ )
    {
        if( (*it)->mpTAction->mName == name )
        {
            (*it)->mpTAction->setIsActive( true );
            updateToolbar();
        }
    }
    mudlet::self()->processEventLoopHack();
    mpHost->mpConsole->mpCommandLine->setFocus();
}

void ActionUnit::hideToolBar(const QString & name )
{
    for(auto it = mEasyButtonBarList.begin(); it!=mEasyButtonBarList.end(); it++ )
    {
        if( (*it)->mpTAction->mName == name )
        {
            (*it)->mpTAction->setIsActive( false );
            updateToolbar();
        }
    }
    mudlet::self()->processEventLoopHack();

}

void ActionUnit::constructToolbar( TAction * pA, TToolBar * pTB )
{
    pTB->clear();
    if( ( pA->mLocation != 4 ) || ( ! pA->isActive() ) )
    {
        pTB->setFloating( false );
        mudlet::self()->removeDockWidget( pTB );
        return;
    }

    if( pA->mLocation == 4 )
    {
        pA->expandToolbar( pTB );
        pTB->setTitleBarWidget( 0 );
    }
    /*else
    {
        pA->expandToolbar( pMainWindow, pTB, 0 );
        QWidget * test = new QWidget;
        pTB->setTitleBarWidget( test );
    }*/

    pTB->finalize();

    if( pA->mOrientation == 0 )
        pTB->setHorizontalOrientation();
    else
        pTB->setVerticalOrientation();

    pTB->setTitleBarWidget( 0 );
    pTB->setFeatures( QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable );
    if( pA->mLocation == 4 )
    {
        mudlet::self()->addDockWidget( Qt::LeftDockWidgetArea, pTB ); //float toolbar
        pTB->setFloating( true );
        QPoint pos = QPoint( pA->mPosX, pA->mPosY );
        pTB->show();
        pTB->move( pos );
        pTB->mpTAction = pA;
        pTB->recordMove();
    }
    else
        pTB->show();

    pTB->setStyleSheet( pTB->mpTAction->css );
}

TAction * ActionUnit::getHeadAction( TEasyButtonBar * pT )
{
    for(auto it = mActionRootNodeList.begin(); it != mActionRootNodeList.end(); it++)
    {
// N/U:         bool found = false;
        for(auto it2 = mEasyButtonBarList.begin(); it2!=mEasyButtonBarList.end(); it2++ )
        {
            if( pT == (*it)->mpEasyButtonBar )
            {
// N/U:                 found = true;
                return *it;
            }
        }
    }
    return 0;
}

void ActionUnit::constructToolbar( TAction * pA, TEasyButtonBar * pTB )
{
    pTB->clear();
    if( pA->mLocation == 4 ) return; //floating toolbars are handled differently
    if( ! pA->isActive() )
    {
        pTB->hide();
        return;
    }

    pA->expandToolbar( pTB );
    pTB->finalize();
    if( pA->mOrientation == 0 )
        pTB->setHorizontalOrientation();
    else
        pTB->setVerticalOrientation();
    switch( pA->mLocation )
    {
        case 0: mpHost->mpConsole->mpTopToolBar->layout()->addWidget( pTB ); break;
        //case 1: mpHost->mpConsole->mpTopToolBar->layout()->addWidget( pTB ); break;
        case 2: mpHost->mpConsole->mpLeftToolBar->layout()->addWidget( pTB ); break;
        case 3: mpHost->mpConsole->mpRightToolBar->layout()->addWidget( pTB ); break;
    }

    pTB->setStyleSheet( pTB->mpTAction->css );
    pTB->show();

}


void ActionUnit::updateToolbar()
{
    getToolBarList();
    getEasyButtonBarList();
}
