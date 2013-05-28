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
#include "TScript.h"
#include "Host.h"
#include "HostManager.h"
#include "mudlet.h"
#include "TDebug.h"

using namespace std;


TScript::TScript( TScript * parent, Host * pHost )
: Tree<TScript>( parent )
, mpHost( pHost )
, mModuleMember(false)
, mModuleMasterFolder(false)
, mNeedsToBeCompiled( true )
, exportItem(true)
{
}

TScript::TScript( QString name, Host * pHost )
: Tree<TScript>(0)
, mName( name )
, mpHost( pHost )
, mModuleMember(false)
, mModuleMasterFolder(false)
, mNeedsToBeCompiled( true )
, exportItem(true)
{
}

TScript::~TScript()
{
    if( ! mpHost )
    {
        return;
    }
    for( int i=0; i<mEventHandlerList.size(); i++ )
    {
        mpHost->unregisterEventHandler( mEventHandlerList[i], this );
    }
    mpHost->getScriptUnit()->unregisterScript( this );
}



bool TScript::registerScript()
{
    if( ! mpHost )
    {
        return false;
    }
    return mpHost->getScriptUnit()->registerScript(this);
}

void TScript::setEventHandlerList( QStringList handlerList )
{
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

void TScript::callEventHandler( TEvent * pE )
{
    // Only call this event handler if this script and all its ancestors are active:
    if(isActive() && ancestorsActive())
    {
        mpHost->mLuaInterpreter.callEventHandler( mName, pE );
    }
}

void TScript::compile()
{
    if( mNeedsToBeCompiled || mpHost->mResetProfile )
    {
        if( ! compileScript() )
        {
            if( mudlet::debugMode ) {TDebug(QColor(Qt::white),QColor(Qt::red))<<"ERROR: Lua compile error. compiling script of script:"<<mName<<"\n">>0;}
            mOK_code = false;
        }
    }
    typedef list<TScript *>::const_iterator I;
    for( I it = mpMyChildrenList->begin(); it != mpMyChildrenList->end(); it++)
    {
        TScript * pChild = *it;
        pChild->compile();
    }
}

bool TScript::setScript( QString & script )
{
    mScript = script;
    mNeedsToBeCompiled = true;
    if( ! mpHost->blockScripts() ) mOK_code = compileScript();
    return mOK_code;
}

bool TScript::compileScript()
{
    QString error;
    if( mpHost->mLuaInterpreter.compile( mScript, error ) )
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

void TScript::execute()
{
    if( mNeedsToBeCompiled )
    {
        if( ! compileScript() )
        {
            return;
        }
    }
    mpHost->mLuaInterpreter.call( mFuncName, mName );
}




TScript& TScript::clone(const TScript& b)
{
    mName = b.mName;
    mScript = b.mScript;
    mIsFolder = b.mIsFolder;
    mpHost = b.mpHost;
    mNeedsToBeCompiled = b.mNeedsToBeCompiled;
    mEventHandlerList = b.mEventHandlerList;
    return *this;
}

bool TScript::isClone(TScript &b) const
{
    return ( mName == b.mName
             && mScript == b.mScript
             && mIsFolder == b.mIsFolder
             && mpHost == b.mpHost
             && mNeedsToBeCompiled == b.mNeedsToBeCompiled
             && mEventHandlerList == b.mEventHandlerList );
}
