/***************************************************************************
 *   Copyright (C) 2008-2013 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2014 by Ahmed Charles - acharles@outlook.com            *
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


#include "TKey.h"


#include "Profile.h"
#include "MainWindow.h"


using namespace std;

TKey::TKey( TKey * parent, Profile * pHost )
: Tree<TKey>( parent )
, exportItem(true)
, mModuleMasterFolder(false)
, mpHost( pHost )
, mNeedsToBeCompiled( true )
, mModuleMember(false)
{
}

TKey::TKey( QString name, Profile * pHost )
: Tree<TKey>(0)
, exportItem(true)
, mModuleMasterFolder(false)
, mName( name )
, mpHost( pHost )
, mNeedsToBeCompiled( true )
, mModuleMember(false)
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

        for(auto it = mpMyChildrenList->begin(); it != mpMyChildrenList->end(); it++)
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


void TKey::enableKey(const QString & name )
{
    if( mName == name )
    {
        setIsActive( true );
    }
    for(auto it = mpMyChildrenList->begin(); it != mpMyChildrenList->end(); it++)
    {
        TKey * pChild = *it;
        pChild->enableKey( name );
    }
}

void TKey::disableKey(const QString & name )
{
    if( mName == name )
    {
        setIsActive( false );
    }
    for(auto it = mpMyChildrenList->begin(); it != mpMyChildrenList->end(); it++)
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
        mOK_code = false;
    }
    for(auto it = mpMyChildrenList->begin(); it != mpMyChildrenList->end(); it++)
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
            mOK_code = false;
        }
    }
    for(auto it = mpMyChildrenList->begin(); it != mpMyChildrenList->end(); it++)
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

    //TODO probably delete whole TKey class
    return false;
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

}

