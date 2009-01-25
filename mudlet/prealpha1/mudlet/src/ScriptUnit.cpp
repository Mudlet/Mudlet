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
#include "ScriptUnit.h"


void ScriptUnit::addScriptRootNode( TScript * pT )
{
    if( ! pT ) return;
    if( ! pT->getID() )
    {
        pT->setID( getNewID() );    
    }
    
    mScriptRootNodeList.push_back( pT );
    if( mScriptMap.find( pT->getID() ) == mScriptMap.end() )
    {
        mScriptMap[pT->getID()] = pT;
    }
    else
    {
        map<int,TScript*>::iterator it;
        for( it=mScriptMap.begin(); it!=mScriptMap.end(); it++  ) 
        {
            int id = it->first;
        }
    }
}

void ScriptUnit::reParentScript( int childID, int oldParentID, int newParentID )
{
    QMutexLocker locker(& mScriptUnitLock);
    
    TScript * pOldParent = getScriptPrivate( oldParentID );
    TScript * pNewParent = getScriptPrivate( newParentID );
    TScript * pChild = getScriptPrivate( childID );
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
        removeScriptRootNode( pChild );    
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
        addScriptRootNode( pChild );
    }
}

void ScriptUnit::removeScriptRootNode( TScript * pT )
{
    if( ! pT ) return;
    mScriptRootNodeList.remove( pT );
}

TScript * ScriptUnit::getScript( int id )
{ 
    QMutexLocker locker(& mScriptUnitLock); 
    if( mScriptMap.find( id ) != mScriptMap.end() )
    {
        return mScriptMap[id];
    }
    else
    {
        return 0;
    }
}

TScript * ScriptUnit::getScriptPrivate( int id )
{ 
    if( mScriptMap.find( id ) != mScriptMap.end() )
    {
        return mScriptMap[id];
    }
    else
    {
        return 0;
    }
}

bool ScriptUnit::registerScript( TScript * pT )
{
    if( ! pT ) return false;
    
    if( pT->getParent() )
    {
        addScript( pT );
        return true;
    }
    else
    {
        addScriptRootNode( pT );    
        return true;
    }
}

void ScriptUnit::unregisterScript( TScript * pT )
{
    if( ! pT ) return;
    if( pT->getParent() )
    {
        removeScript( pT );
        return;
    }
    else
    {
        removeScriptRootNode( pT );    
        return;
    }
}


void ScriptUnit::addScript( TScript * pT )
{
    qDebug()<<"adding script";
    if( ! pT ) return;
    
    QMutexLocker locker(& mScriptUnitLock); 
    
    if( ! pT->getID() )
    {
        pT->setID( getNewID() );
    }
    
    mScriptMap[pT->getID()] = pT;
    qDebug()<<"mScriptMap.size()="<<mScriptMap.size();
}

void ScriptUnit::removeScript( TScript * pT )
{
    if( ! pT ) return;
    
    //FIXME: warning: race condition
    //QMutexLocker locker(& mTriggerUnitLock); 
    mScriptMap.erase(pT->getID());    
}


qint64 ScriptUnit::getNewID()
{
    return ++mMaxID;
}

bool ScriptUnit::serialize( QDataStream & ofs )
{
    bool ret = true;
    ofs << (qint64)mMaxID;
    ofs << (qint64)mScriptRootNodeList.size();
    typedef list<TScript *>::const_iterator I;
    for( I it = mScriptRootNodeList.begin(); it != mScriptRootNodeList.end(); it++)
    {
        TScript * pChild = *it;
        ret = pChild->serialize( ofs );
    }
    return ret;
    
}


bool ScriptUnit::restore( QDataStream & ifs )
{
    ifs >> mMaxID;
    qint64 children;
    ifs >> children;
    bool ret = true;
    mMaxID = 0;
    for( qint64 i=0; i<children; i++ )
    {
        TScript * pChild = new TScript( 0, mpHost );
        ret = pChild->restore( ifs );
        pChild->Dump();
        registerScript( pChild );
    }
    
    return ret;
}

void ScriptUnit::compileAll()
{
    typedef list<TScript *>::const_iterator I;
    for( I it = mScriptRootNodeList.begin(); it != mScriptRootNodeList.end(); it++)
    {
        TScript * pChild = *it;
        pChild->compileAll();
    }
}



