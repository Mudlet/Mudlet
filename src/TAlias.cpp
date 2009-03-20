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

using namespace std;

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
    if( ! mIsActive ) return false;

    bool matchCondition = false;
    bool ret = false;
    bool conditionMet = false;
    pcre * re = mpRegex;
    if( re == NULL ) return false; //regex compile error
    
    const char *error;
    char * subject = (char *) malloc(strlen(toMatch.toLatin1().data())+20);
    strcpy( subject, toMatch.toLatin1().data() );
    unsigned char *name_table;
    int erroffset;
    int find_all;
    int namecount;
    int name_entry_size;
    
    int subject_length = strlen( subject );
    int rc, i;
    std::list<std::string> captureList;
    std::list<int> posList;
    int ovector[300]; // 100 capture groups max (can be increase nbGroups=1/3 ovector
    
    //cout <<" LINE="<<subject<<endl;
    if( mRegexCode.size() > 0 )
    {
        rc = pcre_exec( re,
                        0,
                        subject,
                        subject_length,
                        0,
                        0,
                        ovector,
                        100 );
    }
    else
        goto ERROR;

    if( rc < 0 )
    {
        switch(rc)
        {
        case PCRE_ERROR_NOMATCH: 
            goto ERROR;
            
        default: 
            goto ERROR;
        }
    }
    if( rc > 0 )
    {
        if( mudlet::debugMode ) TDebug()<<"Alias name="<<mName<<"("<<mRegexCode<<") matched!">>0;    
    }
    
    if( rc == 0 )
    {
        qDebug()<<"CRITICAL ERROR: SHOULD NOT HAPPEN->pcre_info() got wrong num of cap groups ovector only has room for %d captured substrings\n";
    }
    
    if( rc < 0 )
    {
        goto ERROR;
    }
    
    matchCondition = true; // alias has matched
    
    for( i=0; i < rc; i++ )
    {
        char * substring_start = subject + ovector[2*i];
        int substring_length = ovector[2*i+1] - ovector[2*i];
        if( substring_length < 1 ) continue;
        std::string match;
        match.append( substring_start, substring_length );
        captureList.push_back( match );
        posList.push_back( ovector[2*i] );
        if( mudlet::debugMode ) TDebug()<<"Alias: capture group #"<<i<<" = <"<<match.c_str()<<">">>0;
    }
    (void)pcre_fullinfo( re,                                              
                            NULL,                 
                            PCRE_INFO_NAMECOUNT,  
                            &namecount);                                          
    
    if (namecount <= 0) 
    {
    //cout << "no named substrings detected" << endl; 
    }
    else
    {
        unsigned char *tabptr;
        (void)pcre_fullinfo( re,
                                NULL,                     
                                PCRE_INFO_NAMETABLE,      
                                &name_table);             
        
        (void)pcre_fullinfo(
                                re,                       
                                NULL,                     
                                PCRE_INFO_NAMEENTRYSIZE,  
                                &name_entry_size);      
        
        tabptr = name_table;
        for (i = 0; i < namecount; i++)
        {
            int n = (tabptr[0] << 8) | tabptr[1];
        //printf("GOT:(%d) %*s: %.*s\n", n, name_entry_size - 3, tabptr + 2, ovector[2*n+1] - ovector[2*n], subject + ovector[2*n]);
            tabptr += name_entry_size;
        }
    } 
    //TODO: add named groups seperately later as Lua::namedGroups
    for(;;)
    {
        int options = 0;                
        int start_offset = ovector[1];  
        
        if (ovector[0] == ovector[1])
        {
            if (ovector[0] >= subject_length)
            {
                goto END;
            }
            options = PCRE_NOTEMPTY | PCRE_ANCHORED;
        }
        
        rc = pcre_exec( re,                                            
                        NULL,                                                                  
                        subject,              
                        subject_length,                                       
                        start_offset,         
                        options,                             
                        ovector,                                                           
                        30 ); 
        
        if (rc == PCRE_ERROR_NOMATCH)
        {
            if (options == 0) break;
            ovector[1] = start_offset + 1;
            continue; 
        }
        
        if (rc < 0)
        {
            goto END;
        }
        
        if( rc == 0 )
        {
            qDebug()<<"CRITICAL ERROR: SHOULD NOT HAPPEN->pcre_info() got wrong num of cap groups ovector only has room for %d captured substrings\n";
        }
        for (i = 0; i < rc; i++)
        {
            char * substring_start = subject + ovector[2*i];
            int substring_length = ovector[2*i+1] - ovector[2*i];
            if( substring_length < 1 ) continue;
            std::string match;
            match.append( substring_start, substring_length );
            captureList.push_back( match );
            posList.push_back( ovector[2*i] );
            if( mudlet::debugMode ) TDebug()<<"capture group #"<<i<<" = <"<<match.c_str()<<">">>0;
        }
    }      

END:
    {
        TLuaInterpreter * pL = mpHost->getLuaInterpreter();
        pL->setCaptureGroups( captureList, posList );
        // call lua trigger function with number of matches and matches itselves as arguments
        execute();
        pL->clearCaptureGroups();
    }

ERROR:
    typedef list<TAlias *>::const_iterator I;
    for( I it = mpMyChildrenList->begin(); it != mpMyChildrenList->end(); it++)
    {
        TAlias * pChild = *it;
        if( pChild->match( toMatch ) ) matchCondition = true;
    }

    free( subject );
    return matchCondition;
}

void TAlias::setRegexCode( QString code )
{
    QMutexLocker locker(& mLock); 
    mRegexCode = code; 
    
    const char *error;
    char * pattern = code.toLatin1().data();
    int erroffset;
    
    pcre * re;
    re = pcre_compile( pattern,              
                       0,                    
                       &error,               
                       &erroffset,           
                       NULL);                
    
    if (re == NULL)
    {
        qDebug()<<"REGEX_COMPILE_ERROR:"<<pattern;
        re = pcre_compile( "MUDLET ERROR: regex doesnt compile",        
                           0,                    
                           &error,               
                           &erroffset,           
                           NULL);                
        
    }
    else
        qDebug()<<"[OK]: REGEX_COMPILE OK";
    
    mpRegex = re; 
    qDebug()<<"regex code für alias:"<<mName<<"="<<code;
}

TAlias& TAlias::clone(const TAlias& b)
{
    mName = b.mName;
    mCommand = b.mCommand;
    mRegexCode = b.mRegexCode;
    mpRegex = b.mpRegex;
    mScript = b.mScript;
    mIsActive = b.mIsActive;
    mIsFolder = b.mIsFolder;
    mpHost = b.mpHost;
    mNeedsToBeCompiled = b.mNeedsToBeCompiled;
    return *this;
}

bool TAlias::isClone(TAlias &b) const {
    return (mName == b.mName 
            && mCommand == b.mCommand 
            && mRegexCode == b.mRegexCode 
            && mpRegex == b.mpRegex 
            && mScript == b.mScript 
            && mIsActive == b.mIsActive 
            && mIsFolder == b.mIsFolder 
            && mpHost == b.mpHost 
            && mNeedsToBeCompiled == b.mNeedsToBeCompiled );
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

void TAlias::execute()
{
    if( mCommand.size() > 0 )
    {
        mpHost->send( mCommand );
    }
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
    setRegexCode( mRegexCode );
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

