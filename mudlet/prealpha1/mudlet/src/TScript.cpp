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
#include "TScript.h"
#include "Host.h"
#include "HostManager.h"

using namespace std;


TScript::TScript( TScript * parent, Host * pHost ) 
: Tree<TScript>( parent ),
mpHost( pHost ),
mNeedsToBeCompiled( true )
{
} 

TScript::TScript( QString name, Host * pHost ) 
: Tree<TScript>(0),
mName( name ),
mpHost( pHost ),
mNeedsToBeCompiled( true )
{
}

TScript::~TScript()
{
    if( mpParent == 0 )
    {
        if( ! mpHost )
        {
            qDebug() << "ERROR: TTrigger::**UN**registerTrigger() pHost=0";
            return;
        }
        mpHost->getScriptUnit()->unregisterScript( this );     
    }
    
}



bool TScript::registerScript()
{
    if( ! mpHost )
    {
        qDebug() << "ERROR: TTrigger::registerTrigger() pHost=0";
        return false;
    }
    qDebug()<<"calling TriggerUnit->registerTrigger(this) ...";
    return mpHost->getScriptUnit()->registerScript(this);    
}

void TScript::setEventHandlerList( QStringList handlerList )
{
    QMutexLocker locker(& mLock);
    for( int i=0; i<mEventHandlerList.size(); i++ )
    {
        mpHost->unregisterEventHandler( mEventHandlerList[i], this );
    }
    mEventHandlerList.clear();
    for( int i=0; i<handlerList.size(); i++ )
    {
        if( handlerList[i].size() < 1 ) continue;
        
        mEventHandlerList.append( handlerList[i] );
        mpHost->registerEventHandler( handlerList[i], this );
    } 
}


void TScript::compile()
{
    TLuaInterpreter * pL = mpHost->getLuaInterpreter();    
    
    if( pL->compile( mScript ) )
    {
        mNeedsToBeCompiled = false;
    }
    else mNeedsToBeCompiled = true;
}

void TScript::compileAll()
{
    compile();
    typedef list<TScript *>::const_iterator I;
    for( I it = mpMyChildrenList->begin(); it != mpMyChildrenList->end(); it++)
    {
        TScript * pChild = *it;
        pChild->compileAll();
    }
}

void TScript::callEventHandler( QString & func, TEvent * pE )
{
    TLuaInterpreter * pL = mpHost->getLuaInterpreter();    
    QStringList argumentList = pE->mArgumentList;
    QList<int> typeList = pE->mArgumentTypeList;
    pL->callEventHandler( mName, argumentList, typeList );    
}

void TScript::execute()
{
    if( mNeedsToBeCompiled )
    {
        TLuaInterpreter * pL = mpHost->getLuaInterpreter();    
        if( pL->compile( mScript ) )
        {
            mNeedsToBeCompiled = false;
        }
        QStringList list; 
        pL->call( mName, mName );
    }
    else
    {
        TLuaInterpreter * pL = mpHost->getLuaInterpreter();    
        QStringList list;
        pL->call( mName, mName );
    }
}

bool TScript::serialize( QDataStream & ofs )
{
    QMutexLocker locker(& mLock);
    qDebug()<<"serializing:"<< mName;
    
    ofs << mName;
    ofs << mScript;
    ofs << mID;
    ofs << mIsActive;
    ofs << mIsFolder;
    ofs << mEventHandlerList;
    ofs << (qint64)mpMyChildrenList->size();
    bool ret = true;
    typedef list<TScript *>::const_iterator I;
    for( I it = mpMyChildrenList->begin(); it != mpMyChildrenList->end(); it++)
    {
        TScript * pChild = *it;
        ret = pChild->serialize( ofs );
    }
    return ret;
} 

bool TScript::restore( QDataStream & ifs, bool initMode )
{
    ifs >> mName;
    ifs >> mScript;
    ifs >> mID;
    ifs >> mIsActive;
    ifs >> mIsFolder;
    ifs >> mEventHandlerList;
    setEventHandlerList( mEventHandlerList );
    qint64 children;
    ifs >> children;
    
    mID = mpHost->getScriptUnit()->getNewID();
    
    bool ret = false;
    
    if( ifs.status() == QDataStream::Ok )
        ret = true;
    
    for( qint64 i=0; i<children; i++ )
    {
        TScript * pChild = new TScript( this, mpHost );
        ret = pChild->restore( ifs, initMode );
        if( initMode ) 
            pChild->registerScript();
    }

    if (getChildrenList()->size() > 0)
        mIsFolder = true;
    
    return ret;
}

TScript& TScript::clone(const TScript& b)
{
    mName = b.mName;
    mScript = b.mScript;
    mIsActive = b.mIsActive;
    mIsFolder = b.mIsFolder;
    mpHost = b.mpHost;
    mNeedsToBeCompiled = b.mNeedsToBeCompiled;
    mEventHandlerList = b.mEventHandlerList;
    return *this;
}

bool TScript::isClone(TScript &b) const {
    return (mName == b.mName && mScript == b.mScript && mIsActive == b.mIsActive && mIsFolder == b.mIsFolder && mpHost == b.mpHost && \
        mNeedsToBeCompiled == b.mNeedsToBeCompiled && mEventHandlerList == b.mEventHandlerList);
}
