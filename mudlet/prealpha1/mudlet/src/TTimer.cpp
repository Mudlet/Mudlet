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



TTimer::TTimer( TTimer * parent, Host * pHost ) 
: Tree<TTimer>( parent ),
mpHost( pHost ),
mNeedsToBeCompiled( true ),
mIsTempTimer( false )
{
} 

TTimer::TTimer( QString name, QTime time, Host * pHost ) 
: Tree<TTimer>(0),
mName( name ),
mTime( time ),
mpHost( pHost ),
mNeedsToBeCompiled( true ),
mIsTempTimer( false )
{
}

TTimer::~TTimer()
{
    if( mpParent == 0 )
    {
        if( ! mpHost )
        {
            qDebug() << "ERROR: TTimer::**UN**registerTrigger() mpHost=0";
            return;
        }
        mpHost->getTimerUnit()->unregisterTimer( this );
        mudlet::self()->unregisterTimer( this, &mTimer );
    }
    mTimer.stop();
}

bool TTimer::registerTimer()
{
    if( ! mpHost )
    {
        qDebug() << "ERROR: TTrigger::registerTrigger() pHost=0";
        return false;
    }
    setTime( mTime );
    mudlet::self()->registerTimer( this, &mTimer );
    mTimer.connect(&mTimer, SIGNAL(timeout()), mudlet::self(),SLOT(slot_timer_fires()));
    return mpHost->getTimerUnit()->registerTimer( this );    
}

void TTimer::setTime( QTime time )
{
    QMutexLocker locker(& mLock); 
    mTime = time; 
    mTimer.setInterval( mTime.msec()+(1000*mTime.second())+(1000*60*mTime.minute())+(1000*60*60*mTime.hour()));
}       

void TTimer::setIsActive( bool b )
{ 
    QMutexLocker locker(& mLock); 
    mIsActive = b; 
    if( mIsActive )
    {
        start(); 
    }
    else
    {
        stop(); 
    }
}

void TTimer::slot_timer_fires()
{
    execute();    
}

void TTimer::compile()
{
}

void TTimer::start()
{
    if( mIsTempTimer ) mTimer.setSingleShot( true );
    mTimer.start();
}

void TTimer::stop()
{
    mTimer.stop();    
}

void TTimer::execute()
{
    if( mIsTempTimer )
    {
        TLuaInterpreter * pL = mpHost->getLuaInterpreter();    
        pL->compileAndExecuteScript( mScript );
        mTimer.stop();
        return;
    }
    if( mCommand.size() > 0 )
    {
        mpHost->send( mCommand );
    }
    if( mNeedsToBeCompiled )
    {
        TLuaInterpreter * pL = mpHost->getLuaInterpreter();    
        QStringList list;
        QString funcName = QString("function Timer") + QString::number( mID ) + QString("()\n"); 
        QString code = funcName + mScript + QString("\nend\n");
        if( pL->compile( code ) )
        {
            mNeedsToBeCompiled = false;
        }
        funcName = QString("Timer") + QString::number( mID ); 
        pL->call( funcName, 0, list, mName );
    }
    else
    {
        TLuaInterpreter * pL = mpHost->getLuaInterpreter();    
        QString funcName = QString("Timer") + QString::number( mID ); 
        QStringList list;
        pL->call( funcName, 0, list, mName );
    }
}


void TTimer::enableTimer( QString & name )
{
    if( mName == name )
    {
        mIsActive = true;
        mTimer.start();
    }
    typedef list<TTimer *>::const_iterator I;
    for( I it = mpMyChildrenList->begin(); it != mpMyChildrenList->end(); it++)
    {
        TTimer * pChild = *it;
        pChild->enableTimer( name );
    }
}

void TTimer::disableTimer( QString & name )
{
    if( mName == name )
    {
        mIsActive = false;
        mTimer.stop();
    }
    typedef list<TTimer *>::const_iterator I;
    for( I it = mpMyChildrenList->begin(); it != mpMyChildrenList->end(); it++)
    {
        TTimer * pChild = *it;
        pChild->disableTimer( name );
    }
}

TTimer * TTimer::killTimer( QString & name )
{
    if( mName == name )
    {
        mIsActive = false;
        mTimer.stop();
        return this;
    }
    typedef list<TTimer *>::const_iterator I;
    for( I it = mpMyChildrenList->begin(); it != mpMyChildrenList->end(); it++)
    {
        TTimer * pChild = *it;
        TTimer * ret = pChild->killTimer( name );
        if( ret ) return ret;
    }
    return 0;
}

bool TTimer::serialize( QDataStream & ofs )
{
    QMutexLocker locker(& mLock);
    qDebug()<<"serializing:"<< mName;
    
    ofs << mName;
    ofs << mScript;
    ofs << mTime;
    ofs << mCommand;
    ofs << mID;
    ofs << mIsActive;
    ofs << mIsFolder;
    ofs << mIsTempTimer;
    ofs << (qint64)mpMyChildrenList->size();
    bool ret = true;
    typedef list<TTimer *>::const_iterator I;
    for( I it = mpMyChildrenList->begin(); it != mpMyChildrenList->end(); it++)
    {
        TTimer * pChild = *it;
        ret = pChild->serialize( ofs );
    }
    return ret;
} 


bool TTimer::restore( QDataStream & ifs, bool initMode )
{
    ifs >> mName;
    ifs >> mScript;
    ifs >> mTime;
    ifs >> mCommand;
    ifs >> mID;
    ifs >> mIsActive;
    ifs >> mIsFolder;
    ifs >> mIsTempTimer;
    qint64 children;
    ifs >> children;
    mID = mpHost->getTimerUnit()->getNewID();
    
    bool ret = false;
    
    if( ifs.status() == QDataStream::Ok )
        ret = true;
    
    for( qint64 i=0; i<children; i++ )
    {
        TTimer * pChild = new TTimer( this, mpHost );
        ret = pChild->restore( ifs, initMode );
        if( initMode )
            pChild->registerTimer();
    }

    if (getChildrenList()->size() > 0)
        mIsFolder = true;
    
    return ret;
}

TTimer& TTimer::clone(const TTimer& b)
{
    mName = b.mName;
    mScript = b.mScript;
    mTime = b.mTime;
    mCommand = b.mCommand;
    mIsActive = b.mIsActive;
    mIsFolder = b.mIsFolder;
    mpHost = b.mpHost;
    mNeedsToBeCompiled = b.mNeedsToBeCompiled;
    mIsTempTimer = b.mIsTempTimer;
    return *this;
}

bool TTimer::isClone(TTimer &b) const {
    return (mName == b.mName && mScript == b.mScript && mTime == b.mTime && mCommand == b.mCommand && mIsActive == b.mIsActive && \
        mIsFolder == b.mIsFolder && mpHost == b.mpHost && mNeedsToBeCompiled == b.mNeedsToBeCompiled && mIsTempTimer == b.mIsTempTimer);
}
