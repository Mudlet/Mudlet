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
#include "TKey.h"
#include "Host.h"
#include "HostManager.h"
#include <iostream>
#include "TDebug.h"
#include "mudlet.h"

using namespace std;

TKey::TKey( TKey * parent, Host * pHost )
: Tree<TKey>( parent )
, mpHost( pHost )
, mModuleMember(false)
, mModuleMasterFolder(false)
, mNeedsToBeCompiled( true )
{
}

TKey::TKey( QString name, Host * pHost )
: Tree<TKey>(0)
, mName( name )
, mpHost( pHost )
, mModuleMember(false)
, mModuleMasterFolder(false)
, mNeedsToBeCompiled( true )
{
}

TKey::~TKey()
{
    if( ! mpHost )
    {
        return;
    }
    mpHost->getKeyUnit()->unregisterKey(this);
}


bool TKey::match( int key, int modifier )
{
    if( isActive() )
    {
        if( ! mIsFolder )
        {
            if( ( mKeyCode == key ) && ( mKeyModifier == modifier ) )
            {
                execute();
                return true;
            }
        }

        typedef list<TKey *>::const_iterator I;
        for( I it = mpMyChildrenList->begin(); it != mpMyChildrenList->end(); it++)
        {
            TKey * pChild = *it;
            if( pChild->match( key, modifier ) ) return true;
        }
    }
    return false;
}


bool TKey::registerKey()
{
    if( ! mpHost )
    {
        qDebug() << "ERROR: TAlias::registerTrigger() pHost=0";
        return false;
    }
    return mpHost->getKeyUnit()->registerKey( this );
}


void TKey::enableKey( QString & name )
{
    if( mName == name )
    {
        setIsActive( true );
    }
    typedef list<TKey *>::const_iterator I;
    for( I it = mpMyChildrenList->begin(); it != mpMyChildrenList->end(); it++)
    {
        TKey * pChild = *it;
        pChild->enableKey( name );
    }
}

void TKey::disableKey( QString & name )
{
    if( mName == name )
    {
        setIsActive( false );
    }
    typedef list<TKey *>::const_iterator I;
    for( I it = mpMyChildrenList->begin(); it != mpMyChildrenList->end(); it++)
    {
        TKey * pChild = *it;
        pChild->disableKey( name );
    }
}

void TKey::compileAll()
{
    mNeedsToBeCompiled = true;
    if( ! compileScript() )
    {
        if( mudlet::debugMode ) {TDebug(QColor(Qt::white),QColor(Qt::red))<<"ERROR: Lua compile error. compiling script of key binding:"<<mName<<"\n">>0;}
        mOK_code = false;
    }
    typedef list<TKey *>::const_iterator I;
    for( I it = mpMyChildrenList->begin(); it != mpMyChildrenList->end(); it++)
    {
        TKey * pChild = *it;
        pChild->compileAll();
    }
}

void TKey::compile()
{
    if( mNeedsToBeCompiled )
    {
        if( ! compileScript() )
        {
            if( mudlet::debugMode ) {TDebug(QColor(Qt::white),QColor(Qt::red))<<"ERROR: Lua compile error. compiling script of key binding:"<<mName<<"\n">>0;}
            mOK_code = false;
        }
    }
    typedef list<TKey *>::const_iterator I;
    for( I it = mpMyChildrenList->begin(); it != mpMyChildrenList->end(); it++)
    {
        TKey * pChild = *it;
        pChild->compile();
    }
}

bool TKey::setScript( QString & script )
{
    mScript = script;
    mNeedsToBeCompiled = true;
    mOK_code = compileScript();
    return mOK_code;
}

bool TKey::compileScript()
{
    mFuncName = QString("Key")+QString::number( mID );
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

void TKey::execute()
{
    if( mCommand.size() > 0 )
    {
        mpHost->send( mCommand );
    }
    if( mNeedsToBeCompiled )
    {
        if( ! compileScript() )
        {
            return;
        }
    }
    mpHost->mLuaInterpreter.call( mFuncName, mName );
}

TKey& TKey::clone(const TKey& b)
{
    mName = b.mName;
    mCommand = b.mCommand;
    mKeyCode = b.mKeyCode;
    mKeyModifier = b.mKeyModifier;
    mRegexCode = b.mRegexCode;
    mRegex = b.mRegex;
    mScript = b.mScript;
    mIsFolder = b.mIsFolder;
    mpHost = b.mpHost;
    mNeedsToBeCompiled = b.mNeedsToBeCompiled;
    return *this;
}

bool TKey::isClone(TKey &b) const
{
    return( mName == b.mName
            && mCommand == b.mCommand
            && mKeyCode == b.mKeyCode
            && mKeyModifier == b.mKeyModifier
            && mRegexCode == b.mRegexCode
            && mRegex == b.mRegex
            && mScript == b.mScript
            && mIsFolder == b.mIsFolder
            && mpHost == b.mpHost
            && mNeedsToBeCompiled == b.mNeedsToBeCompiled );
}
