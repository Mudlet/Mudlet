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
#include "AliasUnit.h"
#include "mudlet.h"

using namespace std;

void AliasUnit::stopAllTriggers()
{
    typedef list<TAlias *>::const_iterator I;
    for( I it = mAliasRootNodeList.begin(); it != mAliasRootNodeList.end(); it++)
    {
        TAlias * pChild = *it;
        pChild->disableFamily();
    }
}

void AliasUnit::reenableAllTriggers()
{
    typedef list<TAlias *>::const_iterator I;
    for( I it = mAliasRootNodeList.begin(); it != mAliasRootNodeList.end(); it++)
    {
        TAlias * pChild = *it;
        pChild->enableFamily();
    }
}

bool AliasUnit::processDataStream( QString & data )
{
    TLuaInterpreter * Lua = mpHost->getLuaInterpreter();
    QString lua_command_string = "command";
    Lua->set_lua_string( lua_command_string, data );
    bool state = false;
    typedef list<TAlias *>::const_iterator I;
    for( I it = mAliasRootNodeList.begin(); it != mAliasRootNodeList.end(); it++)
    {
        TAlias * pChild = *it;
        // = data.replace( "\n", "" );
        if( pChild->match( data ) )
        {
            state = true;
        }
    }
    // the idea to get "command" after alias processing was done and send its value
    // was too difficult for users because if multiple alias change the value of command it becomes too difficult to handle for many users
    // it's easier if we simply intercepts the command and hand responsibility for
    // sending a command to the user scripts.
    //data = Lua->get_lua_string( lua_command_string );
    return state;
}


void AliasUnit::addAliasRootNode( TAlias * pT, int parentPosition, int childPosition )
{
    if( ! pT ) return;
    if( ! pT->getID() )
    {
        pT->setID( getNewID() );    
    }

    if( ( parentPosition == -1 ) || ( childPosition >= mAliasRootNodeList.size() ) )
    {
        mAliasRootNodeList.push_back( pT );
    }
    else
    {
        // insert item at proper position
        int cnt = 0;
        typedef std::list<TAlias *>::iterator IT;
        for( IT it = mAliasRootNodeList.begin(); it != mAliasRootNodeList.end(); it ++ )
        {
            if( cnt >= childPosition )
            {
                mAliasRootNodeList.insert( it, pT );
                break;
            }
            cnt++;
        }
    }

    if( mAliasMap.find( pT->getID() ) == mAliasMap.end() )
    {
        mAliasMap.insert( pT->getID(), pT );
    }
}

void AliasUnit::reParentAlias( int childID, int oldParentID, int newParentID, int parentPosition, int childPosition )
{
    TAlias * pOldParent = getAliasPrivate( oldParentID );
    TAlias * pNewParent = getAliasPrivate( newParentID );
    TAlias * pChild = getAliasPrivate( childID );
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
        removeAliasRootNode( pChild );    
    }

    if( pNewParent )
    {
        pNewParent->addChild( pChild, parentPosition, childPosition );
        if( pChild ) pChild->setParent( pNewParent );
        //cout << "dumping family of newParent:"<<endl;
        //pNewParent->Dump();
    }
    else
    {
        pChild->Tree<TAlias>::setParent( 0 );
        addAliasRootNode( pChild, parentPosition, childPosition );
    }
}

void AliasUnit::removeAliasRootNode( TAlias * pT )
{
    if( ! pT ) return;
    mAliasRootNodeList.remove( pT );
}

TAlias * AliasUnit::getAlias( int id )
{ 
    if( mAliasMap.find( id ) != mAliasMap.end() )
    {
        return mAliasMap.value( id );
    }
    else
    {
        return 0;
    }
}

TAlias * AliasUnit::getAliasPrivate( int id )
{ 
    if( mAliasMap.find( id ) != mAliasMap.end() )
    {
        return mAliasMap.value( id );
    }
    else
    {
        return 0;
    }
}

bool AliasUnit::registerAlias( TAlias * pT )
{
    if( ! pT ) return false;
    
    if( pT->getParent() )
    {
        addAlias( pT );
        return true;
    }
    else
    {
        addAliasRootNode( pT );    
        return true;
    }
}

void AliasUnit::unregisterAlias( TAlias * pT )
{
    if( ! pT ) return;
    if( pT->getParent() )
    {
        removeAlias( pT );
        return;
    }
    else
    {
        removeAliasRootNode( pT );    
        return;
    }
}


void AliasUnit::addAlias( TAlias * pT )
{
    if( ! pT ) return;
    
    if( ! pT->getID() )
    {
        pT->setID( getNewID() );
    }
    
    mAliasMap.insert( pT->getID(), pT );
}

void AliasUnit::removeAlias( TAlias * pT )
{
    if( ! pT ) return;
    
    //FIXME: warning: race condition
    //QMutexLocker locker(& mTriggerUnitLock); 
    mAliasMap.remove(pT->getID());    
}


qint64 AliasUnit::getNewID()
{
    return ++mMaxID;
}


