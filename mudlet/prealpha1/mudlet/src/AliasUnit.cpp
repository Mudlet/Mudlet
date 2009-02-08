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

void AliasUnit::stopAllTriggers()
{    
    typedef list<TAlias *>::const_iterator I;
    for( I it = mAliasRootNodeList.begin(); it != mAliasRootNodeList.end(); it++)
    {
        TAlias * pChild = *it;
        pChild->setIsActive( false );
    }
}

bool AliasUnit::processDataStream( QString & data )
{
    TLuaInterpreter * Lua = mpHost->getLuaInterpreter();
    QString lua_command_string = "command";
    Lua->set_lua_string( lua_command_string, data );
    typedef list<TAlias *>::const_iterator I;
    for( I it = mAliasRootNodeList.begin(); it != mAliasRootNodeList.end(); it++)
    {
        TAlias * pChild = *it;
        if( pChild->match( data ) )
        {
            //TODO: mudlet::self()->mConsoleMap[mpHost]->scrollDown();
            return true;
        }
    }
    
    //data = Lua->get_lua_string( lua_command_string );
    return false;
}


void AliasUnit::addAliasRootNode( TAlias * pT )
{
    if( ! pT ) return;
    if( ! pT->getID() )
    {
        pT->setID( getNewID() );    
    }
    mAliasRootNodeList.push_back( pT );
    mAliasMap.insert( pT->getID(), pT );
}

void AliasUnit::reParentAlias( int childID, int oldParentID, int newParentID )
{
    QMutexLocker locker(& mAliasUnitLock);
    
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
        pNewParent->addChild( pChild );
        if( pChild ) pChild->setParent( pNewParent );
        //cout << "dumping family of newParent:"<<endl;
        //pNewParent->Dump();
    }
    if( ! pNewParent )
    {
        addAliasRootNode( pChild );
    }
}

void AliasUnit::removeAliasRootNode( TAlias * pT )
{
    if( ! pT ) return;
    mAliasRootNodeList.remove( pT );
}

TAlias * AliasUnit::getAlias( int id )
{ 
    QMutexLocker locker(& mAliasUnitLock); 
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
    
    QMutexLocker locker(& mAliasUnitLock); 
    
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

bool AliasUnit::serialize( QDataStream & ofs )
{
    bool ret = true;
    ofs << (qint64)mMaxID;
    ofs << (qint64)mAliasRootNodeList.size();
    typedef list<TAlias *>::const_iterator I;
    for( I it = mAliasRootNodeList.begin(); it != mAliasRootNodeList.end(); it++)
    {
        TAlias * pChild = *it;
        ret = pChild->serialize( ofs );
    }
    return ret;
    
}


bool AliasUnit::restore( QDataStream & ifs, bool initMode )
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
        TAlias * pChild = new TAlias( 0, mpHost );
        ret2 = pChild->restore( ifs, initMode );
        
        if( ! initMode )
        {
            delete pChild;
        }
        else 
            registerAlias( pChild );
    }
    
    return ret1 && ret2;
}

