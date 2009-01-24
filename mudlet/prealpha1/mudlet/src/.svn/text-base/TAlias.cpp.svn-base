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
    if( mIsActive )
    {
        if( ( ! mIsFolder ) && ( mRegexCode.size() > 0 ) )
        {
            if( mRegex.indexIn( toMatch ) == -1 )
            {
                //qDebug()<<"text="<<toMatch;
                //qDebug()<<"Alias("<<mRegexCode<<") did NOT match.";
                //qDebug()<<"mCommand="<<mCommand<<" mRegexCode="<<mRegexCode;
                return false; // regex didn't match
            }
            else
            {
                //qDebug()<<"text="<<toMatch;
                //qDebug()<<"Alias("<<mRegexCode<<") *DID* match!"<<" mCommand="<<mCommand;
                if( mCommand.size() > 0 )
                {
                    //qDebug()<<"sending command="<<mCommand;
                    // when a command is specified we use it instead of the script
                    mpHost->sendRaw( mCommand );
                    return true;
                }
                else
                {
                    QStringList captureList;
                    for( int i=1; i<=mRegex.numCaptures(); i++ )
                    {
                        captureList << mRegex.cap(i);
                        //qDebug()<<"captured #"<<i<<":"<<mRegex.cap(i);
                    }
                    // call lua alias function with number of matches and matches itselves as arguments
                    execute( captureList );    
                    return true;
                }
            }
        }
        
        typedef list<TAlias *>::const_iterator I;
        for( I it = mpMyChildrenList->begin(); it != mpMyChildrenList->end(); it++)
        {
            TAlias * pChild = *it;
            if( pChild->match( toMatch ) ) return true;
        }
    }
    return false;
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
        pL->call( funcName, 0, list, mName );
    }
    else
    {
        TLuaInterpreter * pL = mpHost->getLuaInterpreter();    
        QString funcName = QString("Alias") + QString::number( mID ); 
        pL->call( funcName, 0, list, mName );
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

bool TAlias::restore( QDataStream & ifs )
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
    
    bool ret = true;
    for( qint64 i=0; i<children; i++ )
    {
        TAlias * pChild = new TAlias( this, mpHost );
        ret = pChild->restore( ifs );
        pChild->registerAlias();
    }

    if (getChildrenList()->size() > 0)
        mIsFolder = true;
    return ret;
}

