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
#include "TriggerUnit.h"

using namespace std;

void TriggerUnit::addTriggerRootNode( TTrigger * pT )
{
    if( ! pT ) return;
    if( ! pT->getID() )
    {
        pT->setID( getNewID() );    
    }
    
    mTriggerRootNodeList.push_back( pT );
    mTriggerMap.insert( pT->getID(), pT );
}

void TriggerUnit::reParentTrigger( int childID, int oldParentID, int newParentID )
{
    QMutexLocker locker(& mTriggerUnitLock);
    
    TTrigger * pOldParent = getTriggerPrivate( oldParentID );
    TTrigger * pNewParent = getTriggerPrivate( newParentID );
    TTrigger * pChild = getTriggerPrivate( childID );
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
        removeTriggerRootNode( pChild );    
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
        addTriggerRootNode( pChild );
    }
}

void TriggerUnit::removeTriggerRootNode( TTrigger * pT )
{
    if( ! pT ) return;
    mTriggerRootNodeList.remove( pT );
}

TTrigger * TriggerUnit::getTrigger( int id )
{ 
    QMutexLocker locker(& mTriggerUnitLock); 
    if( mTriggerMap.find( id ) != mTriggerMap.end() )
    {
        return mTriggerMap.value( id );
    }
    else
    {
        return 0;
    }
}

TTrigger * TriggerUnit::getTriggerPrivate( int id )
{ 
    if( mTriggerMap.find( id ) != mTriggerMap.end() )
    {
        return mTriggerMap.value( id );
    }
    else
    {
        return 0;
    }
}

bool TriggerUnit::registerTrigger( TTrigger * pT )
{
    if( ! pT ) return false;
    
    if( pT->getParent() )
    {
        addTrigger( pT );
        return true;
    }
    else
    {
        addTriggerRootNode( pT );    
        return true;
    }
}

void TriggerUnit::unregisterTrigger( TTrigger * pT )
{
    if( ! pT ) return;
    if( pT->getParent() )
    {
        removeTrigger( pT );
        return;
    }
    else
    {
        removeTriggerRootNode( pT );    
        return;
    }
}


void TriggerUnit::addTrigger( TTrigger * pT )
{
    if( ! pT ) return;
    
    QMutexLocker locker(& mTriggerUnitLock); 
    
    if( ! pT->getID() )
    {
        pT->setID( getNewID() );
    }
    
    mTriggerMap.insert( pT->getID(), pT );
}

void TriggerUnit::removeTrigger( TTrigger * pT )
{
    if( ! pT ) return;
    
    mTriggerMap.remove(pT->getID());    
}


qint64 TriggerUnit::getNewID()
{
    return ++mMaxID;
}

void TriggerUnit::processDataStream( QString & data )
{    
    char * subject = (char *) malloc( strlen( data.toLatin1().data() ) + 2048 );
    strcpy( subject, data.toLatin1().data() );

    typedef list<TTrigger *>::const_iterator I;
    for( I it = mTriggerRootNodeList.begin(); it != mTriggerRootNodeList.end(); it++)
    {
        TTrigger * pChild = *it;
        pChild->match( subject, data );
    }
    free( subject );

    for( I it = mCleanupList.begin(); it != mCleanupList.end(); it++ )
    {
        delete *it;
    }
    mCleanupList.clear();
}


void TriggerUnit::stopAllTriggers()
{    
    typedef list<TTrigger *>::const_iterator I;
    for( I it = mTriggerRootNodeList.begin(); it != mTriggerRootNodeList.end(); it++)
    {
        TTrigger * pChild = *it;
        QString name = pChild->getName();
        pChild->disableFamily();
    }
}

void TriggerUnit::reenableAllTriggers()
{
    typedef list<TTrigger *>::const_iterator I;
    for( I it = mTriggerRootNodeList.begin(); it != mTriggerRootNodeList.end(); it++)
    {
        TTrigger * pChild = *it;
        pChild->enableFamily();
    }
}

bool TriggerUnit::serialize( QDataStream & ofs )
{
    bool ret = true;
    ofs << (qint64)mMaxID;
    ofs << (qint64)mTriggerRootNodeList.size();
    typedef list<TTrigger *>::const_iterator I;
    for( I it = mTriggerRootNodeList.begin(); it != mTriggerRootNodeList.end(); it++)
    {
        TTrigger * pChild = *it;
        ret = pChild->serialize( ofs );
    }
    return ret;
}


bool TriggerUnit::restore( QDataStream & ifs, bool initMode )
{
    ifs >> mMaxID;
    qint64 children;
    ifs >> children;
    
    if( initMode ) qDebug()<<"TriggerUnit::restore() mMaxID="<<mMaxID<<" children="<<children;
    
    bool ret1 = false;
    bool ret2 = true;
    
    if( ifs.status() == QDataStream::Ok )
        ret1 = true;
    
    mMaxID = 0;
    for( qint64 i=0; i<children; i++ )
    {
        TTrigger * pChild = new TTrigger( 0, mpHost );
        ret2 = pChild->restore( ifs, initMode );
        
        if( ( pChild->isTempTrigger() ) || ( ! initMode ) ) 
        {
            delete pChild;
        }
        else 
            registerTrigger( pChild );
    }
    
    return ret1 && ret2;
}

void TriggerUnit::enableTrigger( QString & name )
{
    QMutexLocker locker(& mTriggerUnitLock); 
    typedef list<TTrigger *>::const_iterator I;
    for( I it = mTriggerRootNodeList.begin(); it != mTriggerRootNodeList.end(); it++)
    {
        TTrigger * pChild = *it;
        pChild->enableTrigger( name );
    } 
}

void TriggerUnit::disableTrigger( QString & name )
{
    QMutexLocker locker(& mTriggerUnitLock); 
    typedef list<TTrigger *>::const_iterator I;
    for( I it = mTriggerRootNodeList.begin(); it != mTriggerRootNodeList.end(); it++)
    {
        TTrigger * pChild = *it;
        pChild->disableTrigger( name );
    } 
}


void TriggerUnit::killTrigger( QString & name )
{
    QMutexLocker locker(& mTriggerUnitLock); 
    RERUN: TTrigger * ret = 0;
    typedef list<TTrigger *>::const_iterator I;
    for( I it = mTriggerRootNodeList.begin(); it != mTriggerRootNodeList.end(); it++)
    {
        TTrigger * pChild = *it;
        ret = pChild->killTrigger( name );
        if( ret )
        {
            delete ret;
            goto RERUN;
        }
    } 
}


void TriggerUnit::dump()
{
    bool ret = true;
    
    typedef list<TTrigger *>::const_iterator I;
    cout << "TriggerUnit::dump() entries="<<mTriggerRootNodeList.size()<<endl;
    
    for( I it = mTriggerRootNodeList.begin(); it != mTriggerRootNodeList.end(); it++)
    {
        TTrigger * pChild = *it;
        pChild->DumpFamily();
    }
}





