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
#include "TimerUnit.h"


void TimerUnit::addTimerRootNode( TTimer * pT )
{
    if( ! pT ) return;
    if( ! pT->getID() )
    {
        pT->setID( getNewID() );    
    }
    
    mTimerRootNodeList.push_back( pT );
    if( mTimerMap.find( pT->getID() ) == mTimerMap.end() )
    {
        mTimerMap[pT->getID()] = pT;
    }
    /*else
    {
        map<int,TTimer*>::iterator it;
        for( it=mTimerMap.begin(); it!=mTimerMap.end(); it++  ) 
        {
            int id = it->first;
        }
    }        */
}

void TimerUnit::reParentTimer( int childID, int oldParentID, int newParentID )
{
    QMutexLocker locker(& mTimerUnitLock);
    
    TTimer * pOldParent = getTimerPrivate( oldParentID );
    TTimer * pNewParent = getTimerPrivate( newParentID );
    TTimer * pChild = getTimerPrivate( childID );
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
        removeTimerRootNode( pChild );    
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
        addTimerRootNode( pChild );
    }
}

void TimerUnit::removeTimerRootNode( TTimer * pT )
{
    if( ! pT ) return;
    mTimerRootNodeList.remove( pT );
}

TTimer * TimerUnit::getTimer( int id )
{ 
    QMutexLocker locker(& mTimerUnitLock); 
    if( mTimerMap.find( id ) != mTimerMap.end() )
    {
        return mTimerMap[id];
    }
    else
    {
        return 0;
    }
}

TTimer * TimerUnit::getTimerPrivate( int id )
{ 
    if( mTimerMap.find( id ) != mTimerMap.end() )
    {
        return mTimerMap[id];
    }
    else
    {
        return 0;
    }
}

bool TimerUnit::registerTimer( TTimer * pT )
{
    if( ! pT ) return false;
    
    if( pT->getParent() )
    {
        addTimer( pT );
        return true;
    }
    else
    {
        addTimerRootNode( pT );    
        return true;
    }
}

void TimerUnit::unregisterTimer( TTimer * pT )
{
    if( ! pT ) return;
    if( pT->getParent() )
    {
        removeTimer( pT );
        return;
    }
    else
    {
        removeTimerRootNode( pT );    
        return;
    }
}



void TimerUnit::addTimer( TTimer * pT )
{
    qDebug()<<"adding timer";
    if( ! pT ) return;
    
    QMutexLocker locker(& mTimerUnitLock); 
    
    if( ! pT->getID() )
    {
        pT->setID( getNewID() );
    }
    
    mTimerMap[pT->getID()] = pT;
    qDebug()<<"mTimerMap.size()="<<mTimerMap.size();
}

void TimerUnit::removeTimer( TTimer * pT )
{
    if( ! pT ) return;
    
    //FIXME: warning: race condition
    //QMutexLocker locker(& mTriggerUnitLock); 
    mTimerMap.erase(pT->getID());    
}

void TimerUnit::enableTimer( QString & name )
{
    QMutexLocker locker(& mTimerUnitLock); 
    typedef list<TTimer *>::const_iterator I;
    for( I it = mTimerRootNodeList.begin(); it != mTimerRootNodeList.end(); it++)
    {
        TTimer * pChild = *it;
        pChild->enableTimer( name );
    } 
}

void TimerUnit::disableTimer( QString & name )
{
    QMutexLocker locker(& mTimerUnitLock); 
    typedef list<TTimer *>::const_iterator I;
    for( I it = mTimerRootNodeList.begin(); it != mTimerRootNodeList.end(); it++)
    {
        TTimer * pChild = *it;
        pChild->disableTimer( name );
    } 
}


void TimerUnit::killTimer( QString & name )
{
    QMutexLocker locker(& mTimerUnitLock); 
    qDebug()<<"timerList.size()="<<mTimerRootNodeList.size();
    RERUN: TTimer * ret = 0;
    typedef list<TTimer *>::const_iterator I;
    for( I it = mTimerRootNodeList.begin(); it != mTimerRootNodeList.end(); it++)
    {
        TTimer * pChild = *it;
        ret = pChild->killTimer( name );
        if( ret )
        {
            delete ret;
            goto RERUN;
        }
    } 
}

qint64 TimerUnit::getNewID()
{
    return ++mMaxID;
}

bool TimerUnit::serialize( QDataStream & ofs )
{
    bool ret = true;
    int tempTimer = 0;
    ofs << (qint64)mMaxID;
    ofs << (qint64)mTimerRootNodeList.size();
    typedef list<TTimer *>::const_iterator I;
    for( I it = mTimerRootNodeList.begin(); it != mTimerRootNodeList.end(); it++)
    {
        TTimer * pChild = *it;
        ret = pChild->serialize( ofs );
    }

    return ret;
    
}

/*void TimerUnit::processDataStream( QString & data )
{
    typedef list<TTimer *>::const_iterator I;
    for( I it = mTimerRootNodeList.begin(); it != mTimerRootNodeList.end(); it++)
    {
        TTimer * pChild = *it;
        pChild->match( data );
    }
} */

bool TimerUnit::restore( QDataStream & ifs, bool initMode )
{
    ifs >> mMaxID;
    qint64 children;
    ifs >> children;
    bool ret1 = false;
    bool ret2 = false;
    
    if( ifs.status() == QDataStream::Ok )
        ret1 = true;
    
    mMaxID = 0;
    
    for( qint64 i=0; i<children; i++ )
    {
        TTimer * pChild = new TTimer( 0, mpHost );
        ret2 = pChild->restore( ifs, initMode );
        if( ( pChild->isTempTimer() ) || ( ! initMode ) ) 
        {
            delete pChild;
        }
        else 
            registerTimer( pChild );
    }
    
    return ret1 & ret2;
}


