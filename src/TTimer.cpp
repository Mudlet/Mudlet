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


#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <math.h>
#include <QDataStream>
#include <QRegExp>
#include <QString>
#include <QTextDocument>
#include "TTimer.h"
#include "Host.h"
#include "HostManager.h"
#include "mudlet.h"
#include "TDebug.h"

using namespace std;

TTimer::TTimer( TTimer * parent, Host * pHost )
: Tree<TTimer>( parent )
, mRegisteredAnonymousLuaFunction( false )
, mpHost( pHost )
, mNeedsToBeCompiled( true )
, mIsTempTimer( false )
, mpTimer( new QTimer )
{
}

TTimer::TTimer( QString name, QTime time, Host * pHost )
: Tree<TTimer>(0)
, mRegisteredAnonymousLuaFunction( false )
, mName( name )
, mTime( time )
, mpHost( pHost )
, mNeedsToBeCompiled( true )
, mIsTempTimer( false )
, mpTimer( new QTimer )
{
}

TTimer::~TTimer()
{
    mpTimer->stop();
    if( ! mpHost )
    {
        return;
    }
    mpHost->getTimerUnit()->unregisterTimer( this );
    mudlet::self()->unregisterTimer( mpTimer );
    mpTimer->deleteLater();
    //qDebug()<<"DELETING TIMER:"<<this;
}

bool TTimer::registerTimer()
{
    if( ! mpHost )
    {
        return false;
    }
    setTime( mTime );
    mudlet::self()->registerTimer( this, mpTimer );
    mpTimer->connect(mpTimer, SIGNAL(timeout()), mudlet::self(),SLOT(slot_timer_fires()));
    return mpHost->getTimerUnit()->registerTimer( this );
}

void TTimer::setName( QString name )
{
    // temp timers do not need to check for names referring to multiple
    // timer objects as names=ID -> much faster tempTimer creation
    if( ! mIsTempTimer )
    {
        mpHost->getTimerUnit()->mLookupTable.remove( mName, this );
    }
    mName = name;
    mpHost->getTimerUnit()->mLookupTable.insertMulti( name, this );
}


void TTimer::setTime( QTime time )
{
    QMutexLocker locker(& mLock);
    mTime = time;
    mpTimer->setInterval( mTime.msec()+(1000*mTime.second())+(1000*60*mTime.minute())+(1000*60*60*mTime.hour()));
}

// children of folder = regular timers
// children of timers = offset timers
//     offset timers: -> their time interval is interpreted as an offset to their parent timer
bool TTimer::isOffsetTimer()
{
    if( mpParent )
    {
        if( ! mpParent->isFolder() )
        {
            return true;
        }
        else
            return false;
    }
    else
    {
        return false;
    }
}

bool TTimer::setIsActive( bool b )
{
    bool condition1 = Tree<TTimer>::setIsActive( b );
    bool condition2 = canBeUnlocked(0);
    if( condition1 && condition2 )
    {
        start();
    }
    else
    {
        stop();
    }
    return condition1 && condition2;
}


void TTimer::start()
{
    if( mIsTempTimer )
        mpTimer->setSingleShot( true );
    else
        mpTimer->setSingleShot( false );
    mpTimer->start();
}

void TTimer::stop()
{
    mpTimer->stop();
}

void TTimer::compile()
{
    if( mNeedsToBeCompiled )
    {
        if( ! compileScript() )
        {
            if( mudlet::debugMode ) {TDebug(QColor(Qt::white),QColor(Qt::red))<<"ERROR: Lua compile error. compiling script of timer:"<<mName<<"\n">>0;}
            mOK_code = false;
        }
    }
    typedef list<TTimer *>::const_iterator I;
    for( I it = mpMyChildrenList->begin(); it != mpMyChildrenList->end(); it++)
    {
        TTimer * pChild = *it;
        pChild->compile();
    }
}

void TTimer::compileAll()
{
    mNeedsToBeCompiled = true;
    if( ! compileScript() )
    {
        if( mudlet::debugMode ) {TDebug(QColor(Qt::white),QColor(Qt::red))<<"ERROR: Lua compile error. compiling script of timer:"<<mName<<"\n">>0;}
        mOK_code = false;
    }
    typedef list<TTimer *>::const_iterator I;
    for( I it = mpMyChildrenList->begin(); it != mpMyChildrenList->end(); it++)
    {
        TTimer * pChild = *it;
        pChild->compileAll();
    }
}

bool TTimer::setScript( QString & script )
{
    mScript = script;
    if( script == "" )
    {
        mNeedsToBeCompiled = false;
        mOK_code = true;
    }
    else
    {
        mNeedsToBeCompiled = true;
        mOK_code = compileScript();
    }
    return mOK_code;
}

bool TTimer::compileScript()
{
    mFuncName = QString("Timer")+QString::number( mID );
    QString code = QString("function ")+ mFuncName + QString("()\n") + mScript + QString("\nend\n");
    QString error;
    if( mpHost->mLuaInterpreter.compile( code, error ) )
    {
        mNeedsToBeCompiled = false;
        mOK_code = true;
        return true;
    }
    else
    {
        mOK_code = false;
        setError( error );
        return false;
    }
}

bool TTimer::checkRestart()
{
    return ( ! mIsTempTimer
            && ! isOffsetTimer()
            && isActive()
            && ! mIsFolder );
}

void TTimer::execute()
{
    if( mudlet::debugMode ) {TDebug(QColor(Qt::darkYellow),QColor(Qt::darkBlue)) << "\n[TIMER EXECUTES]: "<<mName<<" fired. Executing command="<<mCommand<<" and executing script:"<<mScript<<"\n" >> 0;}
    if( ! isActive() || mIsFolder )
    {
        mpTimer->stop();
        return;
    }

    if( mIsTempTimer )
    {
        if( mScript == "" )
        {
            mpHost->mLuaInterpreter.call_luafunction( this );
        }
        else
        {
            mpHost->mLuaInterpreter.compileAndExecuteScript( mScript );
        }
        mpTimer->stop();
        mpHost->mTimerUnit.markCleanup( this );
        return;
    }

    if( ( ! isFolder() && hasChildren() ) || ( isOffsetTimer() ) )
    {
        typedef list<TTimer *>::const_iterator I;
        for( I it = mpMyChildrenList->begin(); it != mpMyChildrenList->end(); it++)
        {
            TTimer * pChild = *it;
            if( pChild->isOffsetTimer() )
            {
                pChild->enableTimer( pChild->getID() );
            }
        }
        if( isOffsetTimer() )
        {
            disableTimer( mID );
            deactivate();
        }
    }

    if( mCommand.size() > 0 )
    {
        mpHost->send( mCommand );
    }

    if( mNeedsToBeCompiled )
    {
        if( ! compileScript() )
        {
            disableTimer();
            return;
        }
    }
    if( ! mpHost->mLuaInterpreter.call( mFuncName, mName ) )
    {
        mpTimer->stop();
    }
}

bool TTimer::canBeUnlocked( TTimer * pChild )
{
    if( shouldBeActive() )
    {
        if( ! mpParent )
        {
            return true;
        }
        else
            return mpParent->canBeUnlocked( 0 );
    }
    else
    {
        //DumpFamily();
        return false;
    }

}

void TTimer::enableTimer( qint64 id )
{

    if( mID == id )
    {
        if( canBeUnlocked( 0 ) )
        {
            if( activate() )
            {
                if( mScript.size() > 0 )
                {
                    mpTimer->start();
                }
            }
            else
            {
                deactivate();
                mpTimer->stop();
            }
        }
    }

    if( ! isOffsetTimer() )
    {
        typedef list<TTimer *>::const_iterator I;
        for( I it = mpMyChildrenList->begin(); it != mpMyChildrenList->end(); it++)
        {
            TTimer * pChild = *it;
            pChild->enableTimer( pChild->getID() );
        }
    }
}

void TTimer::disableTimer( qint64 id )
{
    if( mID == id )
    {
        deactivate();
        mpTimer->stop();
    }

    typedef list<TTimer *>::const_iterator I;
    for( I it = mpMyChildrenList->begin(); it != mpMyChildrenList->end(); it++)
    {
        TTimer * pChild = *it;
        if( ! pChild->isOffsetTimer() && pChild->shouldBeActive() )
        {
            pChild->disableTimer( pChild->getID() );
        }
    }
}

void TTimer::enableTimer()
{
    if( canBeUnlocked( 0 ) )
    {
        if( activate() )
        {
            if( mScript.size() > 0 )
            {
                mpTimer->start();
            }
        }
        else
        {
            deactivate();
            mpTimer->stop();
        }
    }
    if( ! isOffsetTimer() )
    {
        typedef list<TTimer *>::const_iterator I;
        for( I it = mpMyChildrenList->begin(); it != mpMyChildrenList->end(); it++)
        {
            TTimer * pChild = *it;
            if( ! pChild->isOffsetTimer() ) pChild->enableTimer();
        }
    }
}

void TTimer::disableTimer()
{
    deactivate();
    mpTimer->stop();
    //qDebug()<<"timer "<<mName<<" has been stopped stopping children...\n";
    typedef list<TTimer *>::const_iterator I;
    for( I it = mpMyChildrenList->begin(); it != mpMyChildrenList->end(); it++)
    {
        TTimer * pChild = *it;
        pChild->disableTimer();
    }
}



void TTimer::enableTimer( QString & name )
{
    if( mName == name )
    {
        if( canBeUnlocked( 0 ) )
        {
            if( activate() )
            {
                mpTimer->start();
            }
            else
            {
                deactivate();
                mpTimer->stop();
            }
        }
    }

    if( ! isOffsetTimer() )
    {
        typedef list<TTimer *>::const_iterator I;
        for( I it = mpMyChildrenList->begin(); it != mpMyChildrenList->end(); it++)
        {
            TTimer * pChild = *it;
            pChild->enableTimer( pChild->getName() );
        }
    }
}

void TTimer::disableTimer( QString & name )
{
    if( mName == name )
    {
        deactivate();
        mpTimer->stop();
    }

    typedef list<TTimer *>::const_iterator I;
    for( I it = mpMyChildrenList->begin(); it != mpMyChildrenList->end(); it++)
    {
        TTimer * pChild = *it;
        pChild->disableTimer( pChild->getName() );
    }
}


void TTimer::killTimer()
{
    deactivate();
    mpTimer->stop();
}


TTimer& TTimer::clone(const TTimer & b)
{
    mName = b.mName;
    mScript = b.mScript;
    mTime = b.mTime;
    mCommand = b.mCommand;
    mIsFolder = b.mIsFolder;
    mpHost = b.mpHost;
    mNeedsToBeCompiled = b.mNeedsToBeCompiled;
    mIsTempTimer = b.mIsTempTimer;
    return *this;
}

bool TTimer::isClone( TTimer & b ) const
{
    return( mName == b.mName
            && mScript == b.mScript
            && mTime == b.mTime
            && mCommand == b.mCommand
            && mIsFolder == b.mIsFolder
            && mpHost == b.mpHost
            && mNeedsToBeCompiled == b.mNeedsToBeCompiled
            && mIsTempTimer == b.mIsTempTimer );
}
