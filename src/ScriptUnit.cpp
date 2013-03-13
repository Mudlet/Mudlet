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

#include <QMapIterator>
#include <QDebug>
#include "ScriptUnit.h"

using namespace std;

void ScriptUnit::_uninstall( TScript * pChild, QString packageName )
{
    typedef list<TScript *>::const_iterator I;
    list<TScript*> * childrenList = pChild->mpMyChildrenList;
    for( I it2 = childrenList->begin(); it2 != childrenList->end(); it2++)
    {
        TScript * pT = *it2;
        _uninstall( pT, packageName );
        uninstallList.append( pT );
    }
}


void ScriptUnit::uninstall( QString packageName )
{
    typedef std::list<TScript *>::iterator IT;
    for( IT it = mScriptRootNodeList.begin(); it != mScriptRootNodeList.end(); it ++ )
    {
        TScript * pT = *it;

        if( pT->mPackageName == packageName )
        {
            _uninstall( pT, packageName );
            uninstallList.append( pT );
        }
    }
    for( int i=0; i<uninstallList.size(); i++ )
    {
        unregisterScript(uninstallList[i]);
    }
     uninstallList.clear();
}

void ScriptUnit::stopAllTriggers()
{
    typedef list<TScript *>::const_iterator I;
    for( I it = mScriptRootNodeList.begin(); it != mScriptRootNodeList.end(); it++)
    {
        TScript * pChild = *it;
        pChild->setIsActive( false );
    }
}

void ScriptUnit::addScriptRootNode( TScript * pT, int parentPosition, int childPosition )
{
    if( ! pT ) return;
    if( ! pT->getID() )
    {
        pT->setID( getNewID() );
    }

    if( ( parentPosition == -1 ) || ( childPosition >= static_cast<int>(mScriptRootNodeList.size()) ) )
    {
        mScriptRootNodeList.push_back( pT );
    }
    else
    {
        // insert item at proper position
        int cnt = 0;
        typedef std::list<TScript *>::iterator IT;
        for( IT it = mScriptRootNodeList.begin(); it != mScriptRootNodeList.end(); it ++ )
        {
            if( cnt >= childPosition )
            {
                mScriptRootNodeList.insert( it, pT );
                break;
            }
            cnt++;
        }
    }

    mScriptMap.insert( pT->getID(), pT );
}

void ScriptUnit::reParentScript( int childID, int oldParentID, int newParentID, int parentPosition, int childPosition )
{
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
        pNewParent->addChild( pChild, parentPosition, childPosition );
        if( pChild ) pChild->setParent( pNewParent );
        //cout << "dumping family of newParent:"<<endl;
        //pNewParent->Dump();
    }
    else
    {
        pChild->Tree<TScript>::setParent( 0 );
        addScriptRootNode( pChild, parentPosition, childPosition );
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
        return mScriptMap.value( id );
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
        return mScriptMap.value( id );
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
    if( ! pT ) return;

    QMutexLocker locker(& mScriptUnitLock);

    if( ! pT->getID() )
    {
        pT->setID( getNewID() );
    }

    mScriptMap.insert( pT->getID(), pT );
}

void ScriptUnit::removeScript( TScript * pT )
{
    if( ! pT ) return;
    QMapIterator<QString, QList<TScript *> > it(mpHost->mEventHandlerMap);
    while( it.hasNext() )
    {
        it.next();
        mpHost->mEventHandlerMap[it.key()].removeAll(pT);
    }
    mScriptMap.remove(pT->getID());
}


qint64 ScriptUnit::getNewID()
{
    return ++mMaxID;
}

void ScriptUnit::compileAll()
{
    typedef list<TScript *>::const_iterator I;
    for( I it = mScriptRootNodeList.begin(); it != mScriptRootNodeList.end(); it++)
    {
        TScript * pChild = *it;
        if( pChild->isActive() )
        {
            pChild->compileAll();
        }
    }
}



