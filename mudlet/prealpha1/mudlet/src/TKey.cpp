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
#include "TKey.h"
#include "Host.h"
#include "HostManager.h"
#include <iostream>



TKey::TKey( TKey * parent, Host * pHost ) 
: Tree<TKey>( parent )
, mpHost( pHost )
, mNeedsToBeCompiled( true )
{
} 

TKey::TKey( QString name, Host * pHost ) 
: Tree<TKey>(0)
, mName( name )
, mpHost( pHost )
, mNeedsToBeCompiled( true )
{
}

TKey::~TKey()
{
    if( mpParent == 0 )
    {
        if( ! mpHost )
        {
            qDebug() << "ERROR: TAlias::**UN**registerTrigger() pHost=0";
            return;
        }
        mpHost->getKeyUnit()->unregisterKey(this);     
    }
    
}

bool TKey::match( int key, int modifier )
{
    if( mIsActive )
    {
        if( ! mIsFolder )
        {
            if( ( mKeyCode == key ) && ( mKeyModifier == modifier ) )
            {
                if( mCommand.size() > 0 )
                {
                    // when a command is specified we use it instead of the script
                    mpHost->sendRaw( mCommand );
                    return true;
                }
                else
                {
                    QStringList captureList;
                    captureList << QString("");
                    execute( captureList );    
                    return true;
                }
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

void TKey::compile()
{
}

void TKey::enableKey( QString & name )
{
    if( mName == name )
    {
        mIsActive = true;
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
        mIsActive = false;
    }
    typedef list<TKey *>::const_iterator I;
    for( I it = mpMyChildrenList->begin(); it != mpMyChildrenList->end(); it++)
    {
        TKey * pChild = *it;
        pChild->disableKey( name );
    }
}

void TKey::execute(QStringList & list)
{
    if( mCommand.size() > 0 )
    {
        mpHost->send( mCommand );
    }
    if( mNeedsToBeCompiled )
    {
        TLuaInterpreter * pL = mpHost->getLuaInterpreter();    
        QString funcName = QString("function Key") + QString::number( mID ) + QString("()\n"); 
        QString code = funcName + mScript + QString("\nend\n");
        if( pL->compile( code ) )
        {
            mNeedsToBeCompiled = false;
        }
        funcName = QString("Key") + QString::number( mID ); 
        pL->call( funcName, 0, list, mName );
    }
    else
    {
        TLuaInterpreter * pL = mpHost->getLuaInterpreter();    
        QString funcName = QString("Key") + QString::number( mID ); 
        pL->call( funcName, 0, list, mName );
    }
}

bool TKey::serialize( QDataStream & ofs )
{
    QMutexLocker locker(& mLock);
    ofs << mName;
    ofs << mKeyCode;
    ofs << mKeyModifier;
    ofs << mScript;
    ofs << mID;
    ofs << mCommand;
    ofs << mIsActive;
    ofs << mIsFolder;
    ofs << (qint64)mpMyChildrenList->size();
    
    bool ret = true;
    typedef list<TKey *>::const_iterator I;
    for( I it = mpMyChildrenList->begin(); it != mpMyChildrenList->end(); it++)
    {
        TKey * pChild = *it;
        ret = pChild->serialize( ofs );
    }
    return ret;
} 

bool TKey::restore( QDataStream & ifs, bool initMode )
{
    ifs >> mName;
    ifs >> mKeyCode;
    ifs >> mKeyModifier;
    ifs >> mScript;
    ifs >> mID;
    ifs >> mCommand;
    ifs >> mIsActive;
    ifs >> mIsFolder;
    qint64 children;
    ifs >> children;
    mID = mpHost->getKeyUnit()->getNewID();
    
    bool ret = false;
    
    if( ifs.status() == QDataStream::Ok )
        ret = true;
    
    for( qint64 i=0; i<children; i++ )
    {
        TKey * pChild = new TKey( this, mpHost );
        ret = pChild->restore( ifs, initMode );
        if( initMode ) 
            pChild->registerKey();
    }

    if (getChildrenList()->size() > 0)
        mIsFolder = true;
    return ret;
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
    mIsActive = b.mIsActive;
    mIsFolder = b.mIsFolder;
    mpHost = b.mpHost;
    mNeedsToBeCompiled = b.mNeedsToBeCompiled;
    return *this;
}

bool TKey::isClone(TKey &b) const {
    return (mName == b.mName && mCommand == b.mCommand && mKeyCode == b.mKeyCode && mKeyModifier == b.mKeyModifier && mRegexCode == b.mRegexCode && mRegex == b.mRegex && \
        mScript == b.mScript && mIsActive == b.mIsActive && mIsFolder == b.mIsFolder && mpHost == b.mpHost && mNeedsToBeCompiled == b.mNeedsToBeCompiled);
}
