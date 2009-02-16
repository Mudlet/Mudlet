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
#include "TAlias.h"
#include "Host.h"
#include "HostManager.h"
#include "mudlet.h"
#include "TDebug.h"


TAlias::TAlias( TAlias * parent, Host * pHost ) 
: Tree<TAlias>( parent ),
mpHost( pHost ),
mNeedsToBeCompiled( true )
{
} 

TAlias::TAlias( QString name, Host * pHost ) 
: Tree<TAlias>(0),
mName( name ),
mpHost( pHost ),
mNeedsToBeCompiled( true )
{
}

TAlias::~TAlias()
{
    if( mpParent == 0 )
    {
        if( ! mpHost )
        {
            qDebug() << "ERROR: TAlias::**UN**registerTrigger() pHost=0";
            return;
        }
        mpHost->getAliasUnit()->unregisterAlias(this);     
    }
    
}

bool TAlias::match( QString & toMatch )
{
    bool matchCondition = false;
    if( mIsActive )
    {
        if( ( mRegexCode.size() > 0 ) && ( mRegex.isValid() ) )
        {
            if( mRegex.indexIn( toMatch ) == -1 )
            {
qDebug()<<"alias::match() no match for:<"<<toMatch<<">";
                return false; // regex didn't match
            }
            else
            {
                if( mCommand.size() > 0 )
                {
                    mpHost->sendRaw( mCommand );
                }
                QStringList captureList;
                
                int pos = 0;
                while( (pos = mRegex.indexIn( toMatch, pos )) != -1 )
                {
qDebug()<<"alias::match(): regex matched txt="<<toMatch;
                     for( int i=1; i<=mRegex.numCaptures(); i++ )
                     {
                         qDebug()<<"capture#"<<i;
                        captureList << mRegex.cap( i );
                        //if( mudlet::debugMode ) TDebug()<<"Alias capture group #"<<QString::number(captureList.size()+1)<<" = <"<<mRegex.cap( i )<<">">>0;
                     }
                     pos += mRegex.matchedLength();
                }
qDebug()<<"alias::match() captureList:"<<captureList;
                // call lua alias function with number of matches and matches itselves as arguments
                execute( captureList );    
                matchCondition = true;
            }
        }
        
        typedef list<TAlias *>::const_iterator I;
        for( I it = mpMyChildrenList->begin(); it != mpMyChildrenList->end(); it++)
        {
            TAlias * pChild = *it;
            if( pChild->match( toMatch ) ) matchCondition = true;
        }
    }
    return matchCondition;
}

void TAlias::setRegexCode( QString code )
{
    QMutexLocker locker(& mLock); 
    mRegexCode = code; 
    mRegex = QRegExp( code ); 
    qDebug()<<"regex code für alias:"<<mName<<"="<<code;
    mRegex.setMinimal( false );
    mRegex.setPatternSyntax( QRegExp::RegExp2 );
}

TAlias& TAlias::clone(const TAlias& b)
{
    mName = b.mName;
    mCommand = b.mCommand;
    mRegexCode = b.mRegexCode;
    mRegex = b.mRegex;
    mScript = b.mScript;
    mIsActive = b.mIsActive;
    mIsFolder = b.mIsFolder;
    mpHost = b.mpHost;
    mNeedsToBeCompiled = b.mNeedsToBeCompiled;
    return *this;
}

bool TAlias::isClone(TAlias &b) const {
    return (mName == b.mName && mCommand == b.mCommand && mRegexCode == b.mRegexCode && mRegex == b.mRegex && mScript == b.mScript && mIsActive == b.mIsActive && \
        mIsFolder == b.mIsFolder && mpHost == b.mpHost && mNeedsToBeCompiled == b.mNeedsToBeCompiled);
}

bool TAlias::registerAlias()
{
    if( ! mpHost )
    {
        qDebug() << "ERROR: TAlias::registerTrigger() pHost=0";
        return false;
    }
    return mpHost->getAliasUnit()->registerAlias( this );    
}

void TAlias::compile()
{
}

void TAlias::execute(QStringList & list)
{
    if( mNeedsToBeCompiled )
    {
        TLuaInterpreter * pL = mpHost->getLuaInterpreter();    
        QString funcName = QString("function Alias") + QString::number( mID ) + QString("()\n"); 
        QString code = funcName + mScript + QString("\nend\n");
        if( pL->compile( code ) )
        {
            mNeedsToBeCompiled = false;
        }
        funcName = QString("Alias") + QString::number( mID ); 
        pL->call( funcName, mName );
    }
    else
    {
        TLuaInterpreter * pL = mpHost->getLuaInterpreter();    
        QString funcName = QString("Alias") + QString::number( mID ); 
        pL->call( funcName, mName );
    }
}

bool TAlias::serialize( QDataStream & ofs )
{
    QMutexLocker locker(& mLock);
    qDebug()<<"serializing:"<< mName;
    
    ofs << mName;
    ofs << mScript;
    ofs << mID;
    ofs << mCommand;
    ofs << mRegexCode;
    ofs << mIsActive;
    ofs << mIsFolder;
    ofs << (qint64)mpMyChildrenList->size();
    
    bool ret = true;
    typedef list<TAlias *>::const_iterator I;
    for( I it = mpMyChildrenList->begin(); it != mpMyChildrenList->end(); it++)
    {
        TAlias * pChild = *it;
        ret = pChild->serialize( ofs );
    }
    return ret;
} 

bool TAlias::restore( QDataStream & ifs, bool initMode )
{
    ifs >> mName;
    ifs >> mScript;
    ifs >> mID;
    ifs >> mCommand;
    ifs >> mRegexCode;
    mRegex = QRegExp( mRegexCode );
    mRegex.setMinimal( false );
    ifs >> mIsActive;
    ifs >> mIsFolder;
    qint64 children;
    ifs >> children;
    mID = mpHost->getAliasUnit()->getNewID();
    
    bool ret = false;
    
    if( ifs.status() == QDataStream::Ok )
        ret = true;
    
    for( qint64 i=0; i<children; i++ )
    {
        TAlias * pChild = new TAlias( this, mpHost );
        ret = pChild->restore( ifs, initMode );
        if( initMode ) 
            pChild->registerAlias();
    }

    if (getChildrenList()->size() > 0)
        mIsFolder = true;
    return ret;
}

