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
#include "TTrigger.h"
#include "Host.h"
#include "HostManager.h"
#include <map>
#include "mudlet.h"
#include "TDebug.h"
#include <pcre.h>


TTrigger::TTrigger( TTrigger * parent, Host * pHost ) 
: Tree<TTrigger>( parent )
, mpHost( pHost )
, mNeedsToBeCompiled( true )
, mIsTempTrigger( false )
, mIsLineTrigger( false )
, mStartOfLineDelta( 0 )
, mLineDelta( 0 )
, mIsMultiline( false )
, mTriggerType( REGEX_SUBSTRING )
, mTriggerContainsPerlRegex( false )
{
} 

TTrigger::TTrigger( QString name, QStringList regexList, QList<int> regexProperyList, bool isMultiline, Host * pHost ) 
: Tree<TTrigger>(0)
, mName( name )
, mRegexCodeList( regexList )
, mRegexCodePropertyList( regexProperyList )
, mpHost( pHost )
, mNeedsToBeCompiled( true )
, mIsTempTrigger( false )
, mIsLineTrigger( false )
, mStartOfLineDelta( 0 )
, mLineDelta( 0 )
, mIsMultiline( isMultiline )
, mTriggerType( REGEX_SUBSTRING )
, mTriggerContainsPerlRegex( false )
{
    setRegexCodeList( regexList, regexProperyList );
}

TTrigger::~TTrigger()
{
    if( mpParent == 0 )
    {
        if( ! mpHost )
        {
            qDebug() << "ERROR: TTrigger::**UN**registerTrigger() pHost=0";
            return;
        }
        mpHost->getTriggerUnit()->unregisterTrigger( this );     
    }
        
}

void TTrigger::setRegexCodeList( QStringList regexList, QList<int> propertyList )
{
    QMutexLocker locker(& mLock);
    mRegexCodeList.clear();
    mRegexMap.clear();
    mRegexCodePropertyList.clear();
    mTriggerContainsPerlRegex = false;
    
    if( propertyList.size() != regexList.size() )
    {
        qDebug()<<"\nCRITICAL ERROR: TTrigger::setRegexCodeList() regexlist.size() != propertyList.size()";
        return;
    }
    
    for( int i=0; i<regexList.size(); i++ )
    {
        if( regexList[i].size() < 1 ) continue;
        
        mRegexCodeList.append( regexList[i] );
        mRegexCodePropertyList.append( propertyList[i] );                  
        
        if( propertyList[i] == REGEX_WILDCARD )
        {
            const char *error;
            char * pattern = mRegexCodeList.last().toLatin1().data();
            int erroffset;
            
            pcre * re;
            re = pcre_compile( pattern,              /* the pattern */
                               0,                    /* default options */
                               &error,               /* for error message */
                               &erroffset,           /* for error offset */
                               NULL);                /* use default character tables */
            
            if (re == 0)
            {
                qDebug()<<"REGEX_COMPILE_ERROR:"<<pattern;
                printf("PCRE compilation failed at offset %d: %s\n", erroffset, error);
                re = pcre_compile( "regex doesnt compile",                   /* the pattern */
                                   0,                    /* default options */
                                   &error,               /* for error message */
                                   &erroffset,           /* for error offset */
                                   NULL);                /* use default character tables */
            }
            mRegexMap[i] = re; 
        }
        if( propertyList[i] == REGEX_PERL )
        {
            const char *error;
            char * pattern = mRegexCodeList.last().toLatin1().data();
            int erroffset;
            
            pcre * re;
            re = pcre_compile( pattern,              /* the pattern */
                               0,                    /* default options */
                               &error,               /* for error message */
                               &erroffset,           /* for error offset */
                               NULL);                /* use default character tables */
            
            if (re == NULL)
            {
                qDebug()<<"REGEX_COMPILE_ERROR:"<<pattern;
                printf("PCRE compilation failed at offset %d: %s\n", erroffset, error);
                re = pcre_compile( "regex doesnt compile",                   /* the pattern */
                                   0,                    /* default options */
                                   &error,               /* for error message */
                                   &erroffset,           /* for error offset */
                                   NULL);                /* use default character tables */
                
            }
            else
                qDebug()<<"[OK]: REGEX_COMPILE OK";
            
            mRegexMap[i] = re; 
            mTriggerContainsPerlRegex = true;
        }
                     
    } 
}

TTrigger& TTrigger::clone(const TTrigger& b)
{
    mName = b.mName;
    mRegexCodeList = b.mRegexCodeList;
    mRegexCodePropertyList = b.mRegexCodePropertyList;
    mRegexMap = b.mRegexMap;
    mpHost = b.mpHost;
    mScript = b.mScript;
    mIsActive = b.mIsActive;
    mIsTempTrigger = b.mIsTempTrigger;
    mIsFolder = b.mIsFolder;
    mNeedsToBeCompiled = b.mNeedsToBeCompiled;
    mTriggerType = b.mTriggerType;
    mIsLineTrigger = b.mIsLineTrigger;
    mStartOfLineDelta = b.mStartOfLineDelta;
    mLineDelta = b.mLineDelta;
    mIsMultiline = b.mIsMultiline;
    mConditionLineDelta = b.mConditionLineDelta;
    mConditionMap = b.mConditionMap;
    return *this;
}

bool TTrigger::isClone(TTrigger &b) const {
    return (mName == b.mName && mRegexCodeList == b.mRegexCodeList && mRegexCodePropertyList == b.mRegexCodePropertyList && mRegexMap == b.mRegexMap && mpHost == b.mpHost && \
        mScript == b.mScript && mIsActive == b.mIsActive && mIsTempTrigger == b.mIsTempTrigger && mIsFolder == b.mIsFolder && mNeedsToBeCompiled == b.mNeedsToBeCompiled && \
        mTriggerType == b.mTriggerType && mIsLineTrigger == b.mIsLineTrigger && mStartOfLineDelta == b.mStartOfLineDelta && mLineDelta == b.mLineDelta && \
        mIsMultiline == b.mIsMultiline && mConditionLineDelta == b.mConditionLineDelta && mConditionMap == b.mConditionMap);
}

bool TTrigger::match_perl( QString & toMatch, int regexNumber )
{
    cout << "match_perl() enter ..." << endl;
    
    if( ! mRegexMap.contains(regexNumber ) ) return false;
    
    pcre * re = mRegexMap[regexNumber];
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
    QStringList captureList;
    QList<int> posList;
    //int numOfCaptureGroups = pcre_info( re, 0, 0 )*3;
    int ovector[300];
    
    cout <<" LINE="<<subject<<endl;
    
    rc = pcre_exec( re,                    /* the compiled pattern */
                    0,                     /* no extra data - we didn't study the pattern */
                    subject,               /* the subject string */
                    subject_length,        /* the length of the subject */
                    0,                     /* start at offset 0 in the subject */
                    0,                     /* default options */
                    ovector,               /* output vector for substring information */
                    100 );                 /* number of elements in the output vector */
    
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
        if( mudlet::debugMode ) TDebug()<<"Trigger name="<<mName<<"("<<mRegexCodeList.value(regexNumber)<<") matched!">>0;    
    }

    
    if( rc == 0 )
    {
        qDebug()<<"CRITICAL ERROR: SHOULD NOT HAPPEN->pcre_info() got wrong num of cap groups ovector only has room for %d captured substrings\n";
    }
    
    
    //qDebug()<<"EXPAND_ALL: numOfCaptureGroups="<<numOfCaptureGroups<<" regex="<<mRegexCodeList[regexNumber];
    cout << "pcre_exec() returned rc="<<rc<<endl;
    if( rc < 0 )
    {
        goto ERROR;
    }
    for( i=0; i < rc; i++ )
    {
        char * substring_start = subject + ovector[2*i];
        int substring_length = ovector[2*i+1] - ovector[2*i];
        printf("GOT:%2d: %.*s\n", i, substring_length, substring_start); 
        if( substring_length < 1 ) continue;
        char * buffer = (char *)malloc(substring_length + 10);
        memcpy( buffer, substring_start, substring_length );
        buffer[substring_length] = '\0';
        QString captureGroup = buffer;
        captureList << buffer;
        posList.append( ovector[2*i] );
        if( mudlet::debugMode ) TDebug()<<"capture group #"<<i<<" = <"<<captureList[i]<<">">>0;
    }
    (void)pcre_fullinfo( re,                                              
                         NULL,                 
                         PCRE_INFO_NAMECOUNT,  
                         &namecount);                                          
    
    if (namecount <= 0) 
    {
        cout << "no named substrings detected" << endl; 
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
            printf("GOT:(%d) %*s: %.*s\n", n, name_entry_size - 3, tabptr + 2,
                   ovector[2*n+1] - ovector[2*n], subject + ovector[2*n]);
            tabptr += name_entry_size;
        }
    } 
        
    for(;;)
    {
        int options = 0;                
        int start_offset = ovector[1];  
        
        if (ovector[0] == ovector[1])
        {
            if (ovector[0] >= subject_length)
            {
                cout << "subject end reached. aborting matching"<<endl;
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
        cout << "SUBSEQUENT MATCHING: pcre_exec() returned rc="<<rc<<endl;    
        
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
            printf("GOT:%2d: %.*s\n", i, substring_length, substring_start); 
            if( substring_length < 1 ) continue;
            char * buffer = (char *)malloc(substring_length + 10);
            memcpy( buffer, substring_start, substring_length );
            buffer[substring_length] = '\0';
            captureList << buffer;
            free( buffer );
            posList.append( ovector[2*i] );
            if( mudlet::debugMode ) TDebug()<<"capture group #"<<i<<" = <"<<captureList[i]<<">">>0;
        }
    }      

END:
{
    if( mIsMultiline )
    {
        if( regexNumber == 0 )
        {
            if( mudlet::debugMode ) TDebug()<<"Trigger is a multiline trigger condition #0=true -> creating new MatchState number of matchstates="<<mConditionMap.size()+1>>0;
            // wird automatisch auf #1 gesetzt
            TMatchState * pCondition = new TMatchState( mRegexCodeList.size(), mConditionLineDelta );
            mConditionMap[pCondition] = pCondition;  
            goto EXIT_OK;//DON'T RUN SCRIPTS
        }
        else
        {
            int k=0;
            for( map<TMatchState *, TMatchState *>::iterator it=mConditionMap.begin(); it!=mConditionMap.end(); ++it )
            {
                k++;//weg
                if( mudlet::debugMode ) TDebug()<<"LOOKING FOR condition="<<regexNumber<<" MatchState condition="<< (*it).second->nextCondition()>>0;
                if( (*it).second->nextCondition() == regexNumber )
                {
                    if( mudlet::debugMode ) TDebug()<<"MatchState["<<k<<"] conditon #"<<regexNumber<<"=true "<<"nextCondition="<<(*it).second->nextCondition()<<" regex="<<regexNumber<<" updating MatchState["<<k<<"].">>0;
                    (*it).second->conditionMatched();    
                }
            }
            goto EXIT_OK; //DON'T RUN SCRIPTS
        }                               
    }
                
    TLuaInterpreter * pL = mpHost->getLuaInterpreter();
    pL->setCaptureGroups( captureList, posList );
    // call lua trigger function with number of matches and matches itselves as arguments
    execute( captureList );
    pL->clearCaptureGroups();
}

EXIT_OK:    
    free( subject );
    return true;
   
    
ERROR: 
    free( subject );
    return false;
    
}

bool TTrigger::match_wildcard( QString & toMatch, int regexNumber )
{
    return false; //FIXME
    /*
    if( mRegexMap.value(regexNumber).exactMatch( toMatch ) )
    {
        if( mudlet::debugMode ) TDebug()<<"nTrigger name ="<<mName<<"("<<mRegexCodeList.value(regexNumber)<<") matched!">>0;
        if( mIsMultiline )
        {
            if( regexNumber == 0 )
            {
                if( mudlet::debugMode ) TDebug()<<"condition #0 = true -> creating new MatchState">>0;
                // wird automatisch auf #1 gesetzt
                TMatchState * pCondition = new TMatchState( mRegexCodeList.size(), mConditionLineDelta );
                mConditionMap[pCondition] = pCondition;    
                return true;
            }
            else
            {
                
                int k=0;
                for( map<TMatchState*, TMatchState *>::iterator it=mConditionMap.begin(); it!=mConditionMap.end(); ++it )
                {
                    k++;//weg
                    if( mudlet::debugMode ) TDebug()<<"LOOKING FOR condition="<<QString::number(regexNumber)<<" MatchState condition="<< QString::number((*it).second->nextCondition())>>0;
                    if( (*it).second->nextCondition() == regexNumber )
                    {
                        if( mudlet::debugMode )  TDebug()<<"MatchState["<<QString::number(k)<<"] conditon #"<<regexNumber<<"=true "<<"nextCondition="<<QString::number((*it).second->nextCondition())<<" regex="<<QString::number(regexNumber)<<" updating MatchState["<<QString::number(k)<<"].">>0;
                        (*it).second->conditionMatched();    
                    }
                }
                return true;
            }
        }
        QStringList captureList;
        TLuaInterpreter * pL = mpHost->getLuaInterpreter();
        // call lua trigger function with number of matches and matches itselves as arguments
        execute( captureList );    
        return true;
    }
    return false;*/
}

bool TTrigger::match_substring( QString & toMatch, QString & regex, int regexNumber )
{
    if( toMatch.indexOf( regex ) != -1 )
    {
        if( mudlet::debugMode ) TDebug()<<"Trigger name="<<mName<<"("<<mRegexCodeList.value(regexNumber)<<") matched!">>0;
        if( mIsMultiline )
        {
            if( regexNumber == 0 )
            {
                if( mudlet::debugMode ) TDebug()<<"#0=true -> creating new MatchState">>0;
                // wird automatisch auf #1 gesetzt
                TMatchState * pCondition = new TMatchState( mRegexCodeList.size(), mConditionLineDelta );
                mConditionMap[pCondition] = pCondition;    
                return true;
            }
            else
            {
                int k=0;
                for( map<TMatchState*, TMatchState *>::iterator it=mConditionMap.begin(); it!=mConditionMap.end(); ++it )
                {
                    k++;//weg
                    if( mudlet::debugMode ) TDebug()<<"LOOKING FOR condition="<<regexNumber<<" MatchState condition="<< (*it).second->nextCondition()>>0;
                    if( (*it).second->nextCondition() == regexNumber )
                    {
                        if( mudlet::debugMode ) TDebug()<<"MatchState["<<k<<"] conditon #"<<regexNumber<<"=true "<<"nextCondition="<<(*it).second->nextCondition()<<" regex="<<regexNumber<<" updating MatchState["<<k<<"].">>0;
                        (*it).second->conditionMatched();    
                    }
                }
                return true;
            }
        }
        QStringList captureList;
        execute( captureList );    
        return true;
    }    
    return false;
}

bool TTrigger::match_exact_match( QString & toMatch, QString & line, int regexNumber )
{
    QString text = toMatch;
    if( text.endsWith(QChar('\n')) ) text.chop(1); //TODO: speed optimization
    if( text == line )
    {
        if( mudlet::debugMode ) TDebug()<<"Trigger name="<<mName<<"("<<mRegexCodeList.value(regexNumber)<<") matched!">>0;
        if( mIsMultiline )
        {
            if( regexNumber == 0 )
            {
                if( mudlet::debugMode ) TDebug()<<"#0=true -> creating new MatchState">>0;
                // wird automatisch auf #1 gesetzt
                TMatchState * pCondition = new TMatchState( mRegexCodeList.size(), mConditionLineDelta );
                mConditionMap[pCondition] = pCondition;    
                return true;
            }
            else
            {
                int k=0;
                for( map<TMatchState*, TMatchState *>::iterator it=mConditionMap.begin(); it!=mConditionMap.end(); ++it )
                {
                    if( mudlet::debugMode ) TDebug() << "LOOKING FOR condition="<<regexNumber<<" MatchState condition="<< (*it).second->nextCondition()>>0;
                    if( (*it).second->nextCondition() == regexNumber )
                    {
                        if( mudlet::debugMode ) TDebug() << "MatchState[" << k << "] conditon #"<<regexNumber<<"=true "<<"nextCondition="<<(*it).second->nextCondition()<<" regex="<<regexNumber<<" updating MatchState["<<k<<"].">>0;
                        (*it).second->conditionMatched();    
                    }
                }
                return true;
            }
        }
        QStringList captureList;
        execute( captureList );    
        return true;
    }    
    return false;
}

bool TTrigger::match( QString & toMatch )
{
    cout << "enter TTrigger::match()"<<endl;
    bool ret = false;
    if( mIsActive )
    {
        cout << "mIsActive enter"<<endl;
        if( mIsLineTrigger )
        {
            cout << "mIsLineTrigger enter ..."<<endl;
            if( mStartOfLineDelta > 0 )
            {
                mStartOfLineDelta--;
            }
            if( (mStartOfLineDelta == 0) && (mLineDelta > 0) )
            {
                mLineDelta--;
                QStringList captureList;
                execute( captureList );
                cout << "linetrigger leaving #1"<<endl;
                return true;
            }
            cout << "linetrigger #2 leaving"<<endl;
            return false;
        }
      
        bool conditionMet = false;
        
        if( mRegexCodeList.size() != mRegexCodePropertyList.size() )
        {
            qWarning()<<"\nCRITICAL ERROR: TTrigger::match( )size of regex and property list are not equal.";
        }
        if( mIsMultiline )
        {
            cout << "mIsMultiline #1 main"<<endl;
            int matchStateCnt=0;
            for( map<TMatchState*, TMatchState *>::iterator it=mConditionMap.begin(); it!=mConditionMap.end(); ++it )
            {
                qDebug()<<"TMatchState #"<<++matchStateCnt<<" lineCount="<<(*it).second->mLineCount<<" delta="<<(*it).second->mDelta<<" conditon ("<<(*it).second->mNextCondition<<"/"<<(*it).second->mNumberOfConditions<<")";
                (*it).second->newLineArrived();
            }
        }
        cout << "checking mRegexCodeList... big for loop ... enter"<<endl;
        for( int i=0; i<mRegexCodeList.size(); i++ )
        {
            ret = false;
            switch( mRegexCodePropertyList.value(i) )
            {
        
                case REGEX_SUBSTRING:
                    ret = match_substring( toMatch, mRegexCodeList[i], i );
                    break;
                
                case REGEX_PERL:
                    ret = match_perl( toMatch, i );
                    break;
                
                case REGEX_WILDCARD:
                    ret = match_wildcard( toMatch, i );
                    break;
                
                case REGEX_EXACT_MATCH:
                    ret = match_exact_match( toMatch, mRegexCodeList[i], i );
                    break;
            }
            //normal policy: one match is enough to fire trigger, 
            //in case of multiline *all* have to match in order to fire the trigger
            if( ! mIsMultiline ) 
            {
                if( ret )
                {
                    conditionMet = true;
                    break; 
                }
            }
        }
        
        // in the case of multiline triggers: check our state
        if( mIsMultiline )
        {
            cout << "mIsMultiline #2 main enter ..."<<endl;
            int k = 0;    
            bool triggerFires = false;            
            conditionMet = false; //invalidate conditionMet as it has no meaning for multiline triggers
            list<TMatchState*> removeList;
            
            for( map<TMatchState*, TMatchState *>::iterator it=mConditionMap.begin(); it!=mConditionMap.end(); ++it )
            {
                k++;
                if( mudlet::debugMode ) TDebug()<<"---> multiline conditons: condition total="<<mConditionMap.size()<<" checking conditon #"<<k>>0;
                qDebug()<<"TMatchState #"<<k<<" lineCount="<<(*it).second->mLineCount<<" delta="<<(*it).second->mDelta<<" conditon ("<<(*it).second->mNextCondition<<"/"<<(*it).second->mNumberOfConditions<<")";
                if( (*it).second->isComplete() )
                {
                    if( mudlet::debugMode ) TDebug()<<"multiline trigger name="<<mName<<" *FIRES* all conditons are fullfilled! executing script">>0;
                    removeList.push_back( (*it).first );
                    triggerFires = true;
                }
            
                if( ! (*it).second->newLine() )
                {
                    removeList.push_back( (*it).first );
                }
            }
            for( list<TMatchState*>::iterator it=removeList.begin(); it!=removeList.end(); it++ )
            {
                if( mConditionMap.find( *it ) != mConditionMap.end() )
                {
                    delete mConditionMap[*it];
                    if( mudlet::debugMode ) TDebug()<< "removing condition from conditon table.";
                    mConditionMap.erase( *it );
                }
            }
            if( triggerFires )
            {
                cout << "firing trigger main"<<endl;
                conditionMet = true;
                QStringList captureList;
                // FIXME: decision: do we want this: clear condition map after first match to prevent built-up mass triggering
                //mConditionMap.clear();
                execute( captureList );
            }
        }
        
        // definition trigger chain: a folder is part of a trigger chain if it has a regex defined
        // a trigger chain only lets data pass if the condition matches or in case of multiline all
        // all conditions are fullfilled
        //
        // a folder can also be a simple structural element in which case all data passes through
        // if at least one regex is defined a folder is considered a trigger chain otherwise a structural element
        
        cout << "descending in children ... main "<<endl;
        if( conditionMet || ( mRegexCodeList.size() < 1 ) )
        {
            typedef list<TTrigger *>::const_iterator I;
            for( I it = mpMyChildrenList->begin(); it != mpMyChildrenList->end(); it++)
            {
                TTrigger * pChild = *it;
                ret = pChild->match( toMatch );
                if( ret ) conditionMet = true;
            }
        }
        cout << "trigger::match() main ends#1"<<endl;
        return conditionMet;
    }
    cout << "trigger::match() main ends#2"<<endl;
    return false;
}

bool TTrigger::isFilterChain()
{
    if( ( mRegexCodeList.size() > 0 ) && ( hasChildren() ) )
    {
        return true;
    }
    else
    {
        return false;
    }
}

bool TTrigger::registerTrigger()
{
    if( ! mpHost )
    {
        qDebug() << "ERROR: TTrigger::registerTrigger() mpHost=0";
        return false;
    }
    return mpHost->getTriggerUnit()->registerTrigger(this);    
}

void TTrigger::compile()
{
    if( mNeedsToBeCompiled )
    {
        TLuaInterpreter * pL = mpHost->getLuaInterpreter();    
        QString funcName = QString("function Trigger") + QString::number( mID ) + QString("()\n"); 
        QString code = funcName + mScript + QString("\nend\n");
        if( pL->compile( code ) )
        {
            mNeedsToBeCompiled = false;//FIXME isnt sure that compile actually worked!
        }
        else
        {
            if( mudlet::debugMode ) TDebug()<<"ERROR: Lua compile error. compiling script of Trigger:"<<mName>>0;
        }
    }
    typedef list<TTrigger *>::const_iterator I;
    for( I it = mpMyChildrenList->begin(); it != mpMyChildrenList->end(); it++)
    {
        TTrigger * pChild = *it;
        pChild->compile();
    }
}

void TTrigger::execute( QStringList & matches )
{
    if( mIsTempTrigger )
    {
        TLuaInterpreter * pL = mpHost->getLuaInterpreter();    
        pL->compileAndExecuteScript( mScript );
        return;
    }
    if( mCommand.size() > 0 )
    {
        mpHost->send( mCommand );
    }
    if( mNeedsToBeCompiled )
    {
        TLuaInterpreter * pL = mpHost->getLuaInterpreter();    
        int numOfMatches = matches.size();
        QString funcName = QString("function Trigger") + QString::number( mID ) + QString("()\n"); 
        QString code = funcName + mScript + QString("\nend\n");
        if( pL->compile( code ) )
        {
            mNeedsToBeCompiled = false;
        }
        funcName = QString("Trigger") + QString::number( mID ); 
        pL->call( funcName, numOfMatches, matches, mName );
    }
    else
    {
        TLuaInterpreter * pL = mpHost->getLuaInterpreter();    
        QString funcName = QString("Trigger") + QString::number( mID ); 
        int numOfMatches = matches.size();
        pL->call( funcName, numOfMatches, matches, mName );
    }
}

void TTrigger::enableTrigger( QString & name )
{
    if( mName == name )
    {
        mIsActive = true;
    }
    typedef list<TTrigger *>::const_iterator I;
    for( I it = mpMyChildrenList->begin(); it != mpMyChildrenList->end(); it++)
    {
        TTrigger * pChild = *it;
        pChild->enableTrigger( name );
    }
}

void TTrigger::disableTrigger( QString & name )
{
    if( mName == name )
    {
        mIsActive = false;
    }
    typedef list<TTrigger *>::const_iterator I;
    for( I it = mpMyChildrenList->begin(); it != mpMyChildrenList->end(); it++)
    {
        TTrigger * pChild = *it;
        pChild->disableTrigger( name );
    }
}

TTrigger * TTrigger::killTrigger( QString & name )
{
    if( mName == name )
    {
        mIsActive = false;
    }
    typedef list<TTrigger *>::const_iterator I;
    for( I it = mpMyChildrenList->begin(); it != mpMyChildrenList->end(); it++)
    {
        TTrigger * pChild = *it;
        pChild->killTrigger( name );
    }
    return 0;
}


bool TTrigger::serialize( QDataStream & ofs )
{
    QMutexLocker locker(& mLock);
        
    ofs << mName;
    ofs << mScript;
    ofs << mRegexCodeList;
    ofs << mRegexCodePropertyList;
    qDebug()<<"serializing:"<< mName;
    ofs << mID;
    ofs << mIsActive;
    ofs << mIsFolder;
    ofs << mTriggerType;
    ofs << mIsTempTrigger; 
    ofs << mIsMultiline;
    ofs << mConditionLineDelta;
    ofs << (qint64)mpMyChildrenList->size();
    
    bool ret = true;
    typedef list<TTrigger *>::const_iterator I;
    for( I it = mpMyChildrenList->begin(); it != mpMyChildrenList->end(); it++)
    {
        TTrigger * pChild = *it;
        ret = pChild->serialize( ofs );
    }
    return ret;
} 

bool TTrigger::restore( QDataStream & ifs, bool initMode )
{
    ifs >> mName;
    ifs >> mScript;
    QStringList regexList;
    QList<int> propertyList;
    ifs >> regexList;
    ifs >> propertyList;
    setRegexCodeList( regexList, propertyList );
    ifs >> mID;
    ifs >> mIsActive;
    ifs >> mIsFolder;
    ifs >> mTriggerType;
    ifs >> mIsTempTrigger;
    ifs >> mIsMultiline;
    ifs >> mConditionLineDelta;
    qint64 children;
    ifs >> children;
    
    mID = mpHost->getTriggerUnit()->getNewID();
    
    if( initMode ) qDebug()<<"TTrigger::restore() mName="<<mName<<" mID="<<mID<<" children="<<children;
    
    bool ret = false;
    
    if( ifs.status() == QDataStream::Ok )
        ret = true;
    
    for( qint64 i=0; i<children; i++ )
    {
        TTrigger * pChild = new TTrigger( this, mpHost );
        ret = pChild->restore( ifs, initMode );
        if( initMode ) 
            pChild->registerTrigger();
    }

    if( getChildrenList()->size() > 0 )
        mIsFolder = true;
    
    return ret;
}

