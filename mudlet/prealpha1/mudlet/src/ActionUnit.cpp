/***************************************************************************
 *   Copyright (C) 2008 by Heiko Koehn                                     *
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

#include <iomanip>
#include <iostream>
#include <fstream>
#include <string>
#include <cstddef> // NULL
#include <iomanip>
#include <iostream>
#include <fstream>
#include <string>
#include "Host.h"
#include "TLuaInterpreter.h"
#include <QDebug>
#include "ActionUnit.h"
#include "mudlet.h"

void ActionUnit::processDataStream( QString & data )
{
    TLuaInterpreter * Lua = mpHost->getLuaInterpreter();
    QString lua_command_string = "command";
    Lua->set_lua_string( lua_command_string, data );
    typedef list<TAction *>::const_iterator I;
    for( I it = mActionRootNodeList.begin(); it != mActionRootNodeList.end(); it++)
    {
        TAction * pChild = *it;
        pChild->match( data );
    }
    //data = Lua->get_lua_string( lua_command_string );
}


void ActionUnit::addActionRootNode( TAction * pT )
{
    if( ! pT ) return;
    if( ! pT->getID() )
    {
        pT->setID( getNewID() );    
    }
    mActionRootNodeList.push_back( pT );
        
    mActionMap.insert( pT->getID(), pT );
}

void ActionUnit::reParentAction( int childID, int oldParentID, int newParentID )
{
    QMutexLocker locker(& mActionUnitLock);
    
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
        pNewParent->addChild( pChild );
        if( pChild ) pChild->Tree<TAction>::setParent( pNewParent );
        //cout << "dumping family of newParent:"<<endl;
        //pNewParent->Dump();
    }
    if( ! pNewParent )
    {
        addActionRootNode( pChild );
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
    if( pT->getParent() )
    {
        removeAction( pT );
        return;
    }
    else
    {
        removeActionRootNode( pT );    
        return;
    }
}


void ActionUnit::addAction( TAction * pT )
{
    if( ! pT ) return;
    
    QMutexLocker locker(& mActionUnitLock); 
    
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


qint64 ActionUnit::getNewID()
{
    return ++mMaxID;
}

std::list<QToolBar *> ActionUnit::getToolBarList()
{
    typedef list<TAction *>::iterator I;
    for( I it = mActionRootNodeList.begin(); it != mActionRootNodeList.end(); it++)
    {
        bool found = false;
        QToolBar * pTB;
        typedef list<QToolBar *>::iterator I2;
        for( I2 it2 = mToolBarList.begin(); it2!=mToolBarList.end(); it2++ )
        {
            if( *it2 == (*it)->mpToolBar )
            {
                found = true;
                pTB = *it2;
            }
        }
        if( ! found )
        {
            pTB = new QToolBar( (*it)->getName(), mudlet::self() );
            mToolBarList.push_back( pTB );
        }
        
        //pTB->setOrientation( Qt::Vertical );
        constructToolbar( *it, mudlet::self(), pTB );
        
        (*it)->mpToolBar = pTB;
    }    
    
    return mToolBarList;
}

TAction * ActionUnit::getHeadAction( QToolBar * pT )
{
    typedef list<TAction *>::iterator I;
    for( I it = mActionRootNodeList.begin(); it != mActionRootNodeList.end(); it++)
    {
        bool found = false;
        typedef list<QToolBar *>::iterator I2;
        for( I2 it2 = mToolBarList.begin(); it2!=mToolBarList.end(); it2++ )
        {
            if( pT == (*it)->mpToolBar )
            {
                found = true;
                return *it;
            }
        }
    }
    return 0;
}
    
void ActionUnit::constructToolbar( TAction * pA, mudlet * pMainWindow, QToolBar * pTB )
{
    pTB->clear();
    pA->expandToolbar( pMainWindow, pTB, 0 );
}

void ActionUnit::updateToolbar()
{
    getToolBarList();
    /*typedef list<TAction *>::const_iterator I;
    I it = mActionRootNodeList.begin();
    for( ; it != mActionRootNodeList.end(); it++ )
    {
        //constructToolbar( *it, mudlet::self(), (*it)->mpToolBar );
        
    } */        
    
}

bool ActionUnit::serialize( QDataStream & ofs )
{
    bool ret = true;
    ofs << (qint64)mMaxID;
    ofs << (qint64)mActionRootNodeList.size();
    typedef list<TAction *>::const_iterator I;
    for( I it = mActionRootNodeList.begin(); it != mActionRootNodeList.end(); it++)
    {
        TAction * pChild = *it;
        ret = pChild->serialize( ofs );
    }
    return ret;
    
}

bool ActionUnit::restore( QDataStream & ifs, bool initMode )
{
    ifs >> mMaxID;
    qint64 children;
    ifs >> children;
    
    bool ret1 = false;
    bool ret2 = true;
    
    if( ifs.status() == QDataStream::Ok )
        ret1 = true;
    
    mMaxID = 0;
    for( qint64 i=0; i<children; i++ )
    {
        TAction * pChild = new TAction( 0, mpHost );
        ret2 = pChild->restore( ifs, initMode );
        
        if( ! initMode ) 
        {
            delete pChild;
        }
        else 
            registerAction( pChild );
    }
    
    return ret1 && ret2;
}

