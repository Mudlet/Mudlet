/***************************************************************************
 *   Copyright (C) 2008-2009 by Heiko Koehn                                     *
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

using namespace std;

void TimerUnit::stopAllTriggers()
{
    typedef list<TTimer *>::const_iterator I;
    for( I it = mTimerRootNodeList.begin(); it != mTimerRootNodeList.end(); it++)
    {
        TTimer * pChild = *it;
        pChild->disableTimer( pChild->getID() );
    }
}

void TimerUnit::reenableAllTriggers()
{
    typedef list<TTimer *>::const_iterator I;
    for( I it = mTimerRootNodeList.begin(); it != mTimerRootNodeList.end(); it++)
    {
        TTimer * pChild = *it;
        pChild->enableTimer( pChild->getID() );
    }
}


void TimerUnit::addTimerRootNode( TTimer * pT, int parentPosition, int childPosition )
{
    if( ! pT ) return;
    if( ! pT->getID() )
    {
        pT->setID( getNewID() );    
    }
    
    if( ( parentPosition == -1 ) || ( childPosition >= mTimerRootNodeList.size() ) )
    {
        mTimerRootNodeList.push_back( pT );
    }
    else
    {
        // insert item at proper position
        int cnt = 0;
        typedef std::list<TTimer *>::iterator IT;
        for( IT it = mTimerRootNodeList.begin(); it != mTimerRootNodeList.end(); it ++ )
        {
            if( cnt >= childPosition )
            {
                mTimerRootNodeList.insert( it, pT );
                break;
            }
            cnt++;
        }
    }

    // -> schneller; sicherheitsabfrage nicht noetig
    //if( mTimerMap.find( pT->getID() ) == mTimerMap.end() )
    //{
    mTimerMap.insert( pT->getID(), pT );
    //}
}

void TimerUnit::reParentTimer( int childID, int oldParentID, int newParentID, int parentPosition, int childPosition )
{
    TTimer * pOldParent = getTimerPrivate( oldParentID );
    TTimer * pNewParent = getTimerPrivate( newParentID );
    TTimer * pChild = getTimerPrivate( childID );
    if( ! pChild )
    {
        return;
    }
    
    pChild->disableTimer( childID );
    
    if( pOldParent )
    {
        pOldParent->popChild( pChild );
    }
    if( ! pOldParent )
    {
        //removeTimerRootNode( pChild );
        mTimerRootNodeList.remove( pChild );
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
        pChild->Tree<TTimer>::setParent( 0 );
        addTimerRootNode( pChild, parentPosition, childPosition );
    }
    
    pChild->enableTimer( childID );
}

void TimerUnit::_removeTimerRootNode( TTimer * pT )
{
    if( ! pT ) return;
    // temp timers do not need to check for names referring to multiple different
    // objects as names=ID -> much faster tempTimer creation
    if( ! pT->mIsTempTimer )
    {
        QString key;
        key = mpHost->getTimerUnit()->mLookupTable.key( pT );
        if( key != "" )
            mpHost->getTimerUnit()->mLookupTable.remove( key );
    }
    else
        mLookupTable.remove( pT->getName() );

    mTimerMap.remove( pT->getID() );
    mTimerRootNodeList.remove( pT );
}

TTimer * TimerUnit::getTimer( int id )
{ 
    if( mTimerMap.find( id ) != mTimerMap.end() )
    {
        return mTimerMap.value( id );
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
        return mTimerMap.value( id );
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
        pT->setIsActive( false );
        return true;
    }
    else
    {
        addTimerRootNode( pT );   
        pT->setIsActive( false );
        return true;
    }
}

void TimerUnit::unregisterTimer( TTimer * pT )
{
    if( ! pT ) return;
    if( pT->getParent() )
    {
        _removeTimer( pT );
        return;
    }
    else
    {
        _removeTimerRootNode( pT );
        return;
    }
}



void TimerUnit::addTimer( TTimer * pT )
{
    if( ! pT ) return;
    
    if( ! pT->getID() )
    {
        pT->setID( getNewID() );
    }
    
    mTimerMap.insert( pT->getID(), pT );
}

void TimerUnit::_removeTimer( TTimer * pT )
{
    if( ! pT ) return;

    // temp timers do not need to check for names referring to multiple different
    // objects as names=ID -> much faster tempTimer creation
    if( ! pT->mIsTempTimer )
    {
        QString key;
        key = mpHost->getTimerUnit()->mLookupTable.key( pT );
        if( key != "" )
            mpHost->getTimerUnit()->mLookupTable.remove( key );
    }
    else
        mLookupTable.remove( pT->getName() );
    mTimerMap.remove( pT->getID() );
}


void TimerUnit::enableTimer( QString & name )
{
    QMap<QString, TTimer *>::const_iterator it = mLookupTable.find( name );
    while( it != mLookupTable.end() && it.key() == name )
    {
        TTimer * pT = it.value();

        bool ret = false;

        if( ! pT->isOffsetTimer() )
            ret = pT->setIsActive( true );
        else
            pT->setShouldBeActive( true );

        qDebug()<<"trying to enable name="<<name<<" acutally using name="<<pT->getName()<<" setIsActive()="<<ret;
        if( pT->isFolder() )
        {
            // disable or enable all timers in the respective branch
            // irrespective of the user defined state.
            if( pT->shouldBeActive() )
            {
                pT->enableTimer();
            }
            else
            {
                pT->disableTimer();
            }
        }
        else
        {
            if( pT->isOffsetTimer() )
            {
                // state of offset timers is managed by the trigger engine
                if( pT->shouldBeActive() )
                {
                    pT->enableTimer();
                }
                else
                {
                    pT->disableTimer();
                }
            }
            /*else
            {
                if( pT->shouldBeActive() )
                {
                    pT->enableTimer();
                }
                else
                {
                    pT->disableTimer();
                }
            }*/
        }

        ++it;
    }
}

void TimerUnit::disableTimer( QString & name )
{
    QMap<QString, TTimer *>::const_iterator it = mLookupTable.find( name );
    while( it != mLookupTable.end() && it.key() == name )
    {
        TTimer * pT = it.value();

        if( ! pT->isOffsetTimer() )
            pT->setIsActive( false );
        else
            pT->setShouldBeActive( false );

        if( pT->isFolder() )
        {
            // disable or enable all timers in the respective branch
            // irrespective of the user defined state.
            if( pT->shouldBeActive() )
            {
                pT->enableTimer();
            }
            else
            {
                pT->disableTimer();
            }
        }
        else
        {
            if( pT->isOffsetTimer() )
            {
                // state of offset timers is managed by the trigger engine
                if( pT->shouldBeActive() )
                {
                    pT->enableTimer();
                }
                else
                {
                    pT->disableTimer();
                }
            }
            else
            {
                if( pT->shouldBeActive() )
                {
                    pT->enableTimer();
                }
                else
                {
                    pT->disableTimer();
                }
            }
        }

        ++it;
    }
}


bool TimerUnit::killTimer( QString & name )
{
    typedef list<TTimer *>::const_iterator I;
    for( I it = mTimerRootNodeList.begin(); it != mTimerRootNodeList.end(); it++)
    {
        TTimer * pChild = *it;
        if( pChild->getName() == name )
        {
            // only temporary timers are allowed to be killed
            if( ! pChild->isTempTimer() ) return false;
            pChild->killTimer();
            //removeTimer( pChild );
            markCleanup( pChild );
            return true;
        }
    }
    return false;
}

qint64 TimerUnit::getNewID()
{
    return ++mMaxID;
}

void TimerUnit::doCleanup()
{
    typedef list<TTimer *>::iterator I;
    for( I it = mCleanupList.begin(); it != mCleanupList.end(); it++)
    {
        delete *it;
    }
    mCleanupList.clear();
}

void TimerUnit::markCleanup( TTimer * pT )
{
    typedef list<TTimer *>::iterator I;
    for( I it = mCleanupList.begin(); it != mCleanupList.end(); it++)
    {
        if( *it == pT )
        {
            return;
        }
    }
    mCleanupList.push_back( pT );
}



