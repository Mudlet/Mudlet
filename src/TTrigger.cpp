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

#include <assert.h>
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
#include <sstream>

using namespace std;

TTrigger::TTrigger( TTrigger * parent, Host * pHost ) 
: Tree<TTrigger>( parent )
, mpHost( pHost )
, mNeedsToBeCompiled( true )
, mIsTempTrigger( false )
, mIsLineTrigger( false )
, mStartOfLineDelta( 0 )
, mLineDelta( 3 )
, mIsMultiline( false )
, mPerlSlashGOption( false )
, mTriggerType( REGEX_SUBSTRING )
, mTriggerContainsPerlRegex( false )
, mpLua( mpHost->getLuaInterpreter() )
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
, mLineDelta( 3 )
, mIsMultiline( isMultiline )
, mPerlSlashGOption( false )
, mTriggerType( REGEX_SUBSTRING )
, mTriggerContainsPerlRegex( false )
, mpLua( mpHost->getLuaInterpreter() )
{
    setRegexCodeList( regexList, regexProperyList );
}

TTrigger::~TTrigger()
{
    if( mpParent == 0 )
    {
        if( ! mpHost )
        {
            qDebug() << "ERROR: TTrigger::*UN*-registerTrigger() pHost=0";
            return;
        }
        mpHost->getTriggerUnit()->unregisterTrigger( this );     
    }
        
}

//FIXME: sperren, wenn code nicht compiliert werden kann *ODER* regex falsch
bool TTrigger::setRegexCodeList( QStringList regexList, QList<int> propertyList )
{
    mRegexCodeList.clear();
    mRegexMap.clear();
    mRegexCodePropertyList.clear();
    mLuaConditionMap.clear();
    mTriggerContainsPerlRegex = false;

    assert( propertyList.size() == regexList.size() );

    if( ( propertyList.size() < 1 ) && ( ! isFolder() ) )
    {
        setError("No patterns defined.");
        mOK_init = false;
        return false;
    }

    bool state = true;

    for( int i=0; i<regexList.size(); i++ )
    {
        if( regexList[i].size() < 1 ) continue;
        
        mRegexCodeList.append( regexList[i] );
        mRegexCodePropertyList.append( propertyList[i] );                  
        
        if( propertyList[i] == REGEX_PERL )
        {
            const char *error;
            char * pattern = regexList[i].toLatin1().data();
            int erroffset;
            
            pcre * re;
            re = pcre_compile( pattern,
                               0,
                               &error,
                               &erroffset,
                               0 );
            
            if (re == 0)
            {
                if( mudlet::debugMode ) TDebug()<<"REGEX_COMPILE_ERROR:"<<pattern>>0;
                setError( QString( "Pattern '" )+QString(pattern)+QString( "' failed to compile. Correct the pattern.") );
                state = false;
                //printf("PCRE compilation failed at offset %d: %s\n", erroffset, error);
            }
            else
            {
                if( mudlet::debugMode ) TDebug()<<"[OK]: REGEX_COMPILE OK">>0;
            }
            mRegexMap[i] = re; 
            mTriggerContainsPerlRegex = true;
            continue;
        }
        if( propertyList[i] == REGEX_LUA_CODE )
        {
             std::string funcName;
             std::stringstream func;
             func << "trigger" << mName.toLatin1().data() << "condition" << i;
             funcName = func.str();
             QString code = QString("function ")+funcName.c_str()+QString("()\n")+regexList[i]+QString("\nend\n");
             QString error;
             if( ! mpLua->compile( code, error ) )
             {
                 setError( QString("pattern type Lua condition function '")+regexList[i]+QString("' failed to compile.")+error);
                 state = false;
             }
             else
             {
                 mLuaConditionMap[i] = funcName;
             }
             continue;
        }
    }
    if( ! state )
    {
        mOK_init = false;
    }
    else
    {
        mOK_init = true;
    }
    return state;
}


TTrigger & TTrigger::clone(const TTrigger& b)
{
    mName = b.mName;
    mRegexCodeList = b.mRegexCodeList;
    mRegexCodePropertyList = b.mRegexCodePropertyList;
    mRegexMap = b.mRegexMap;
    mpHost = b.mpHost;
    mScript = b.mScript;
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

bool TTrigger::isClone( TTrigger & b ) const 
{
    return (    mName == b.mName 
             && mRegexCodeList == b.mRegexCodeList 
             && mRegexCodePropertyList == b.mRegexCodePropertyList 
             && mRegexMap == b.mRegexMap 
             && mpHost == b.mpHost 
             && mScript == b.mScript 
             && mIsTempTrigger == b.mIsTempTrigger 
             && mIsFolder == b.mIsFolder 
             && mNeedsToBeCompiled == b.mNeedsToBeCompiled 
             && mTriggerType == b.mTriggerType 
             && mIsLineTrigger == b.mIsLineTrigger 
             && mStartOfLineDelta == b.mStartOfLineDelta 
             && mLineDelta == b.mLineDelta 
             && mIsMultiline == b.mIsMultiline 
             && mConditionLineDelta == b.mConditionLineDelta 
             && mConditionMap == b.mConditionMap );
}

bool TTrigger::match_perl( char * subject, QString & toMatch, int regexNumber )
{
    assert( mRegexMap.contains(regexNumber ) );
    
    pcre * re = mRegexMap[regexNumber];

    if( ! re )
    {
        if( mudlet::debugMode ) TDebug()<<"ERROR: the regex of trigger "<<mName<<" does not compile. Please correct the expression. This trigger will never match until it is fixed.">>0;
        return false; //regex compile error
    }
    
    const char *error;
    unsigned char *name_table;
    int erroffset;
    int find_all;
    int namecount;
    int name_entry_size;
    
    int subject_length = strlen( subject );
    int rc, i;
    std::list<std::string> captureList;
    std::list<int> posList;
    //int numOfCaptureGroups = pcre_info( re, 0, 0 )*3;
    int ovector[300]; // 100 capture groups max (can be increase nbGroups=1/3 ovector
    
    //qDebug() <<"TTrigger::match_perl() regex="<<mRegexCodeList[regexNumber]<<" LINE="<<subject;
    
    rc = pcre_exec( re,                    
                    0,                                                                      
                    subject,                                              
                    subject_length,                                       
                    0,                                                               
                    0,                     
                    ovector,                                                               
                    100 );            
    
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
    
    if( rc < 0 )
    {
        goto ERROR;
    }
    
    for( i=0; i < rc; i++ )
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
    (void)pcre_fullinfo( re,                                              
                         NULL,                 
                         PCRE_INFO_NAMECOUNT,  
                         &namecount);                                          
    
    if( namecount <= 0 )
    {
        ;
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
        for( i = 0; i < namecount; i++ )
        {
            int n = (tabptr[0] << 8) | tabptr[1];
            //printf("GOT:(%d) %*s: %.*s\n", n, name_entry_size - 3, tabptr + 2, ovector[2*n+1] - ovector[2*n], subject + ovector[2*n]);
            tabptr += name_entry_size;
        }
    } 
    //TODO: add named groups seperately later as Lua::namedGroups
    for( ; mPerlSlashGOption ; )
    {
        int options = 0;                
        int start_offset = ovector[1];  
        
        if( ovector[0] == ovector[1] )
        {
            if( ovector[0] >= subject_length )
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
        
        if( rc == PCRE_ERROR_NOMATCH )
        {
            if( options == 0 ) break;
            ovector[1] = start_offset + 1;
            continue; 
        }
        
        if( rc < 0 )
        {
            goto END;
        }
        
        if( rc == 0 )
        {
            qDebug()<<"CRITICAL ERROR: SHOULD NOT HAPPEN->pcre_info() got wrong num of cap groups ovector only has room for %d captured substrings\n";
        }
        for( i = 0; i < rc; i++ )
        {
            char * substring_start = subject + ovector[2*i];
            int substring_length = ovector[2*i+1] - ovector[2*i];
            if( substring_length < 1 ) continue;
            std::string match;
            match.append( substring_start, substring_length );
            captureList.push_back( match );
            posList.push_back( ovector[2*i] );
            if( mudlet::debugMode ) TDebug()<<"<Perl /g switch mode:> capture group #"<<i<<" = <"<<match.c_str()<<">">>0;
        }
    }      

END:
{
    if( mIsMultiline )
    {
        updateMultistates( regexNumber, captureList, posList );
        return true;
    }
    else
    {
        TLuaInterpreter * pL = mpHost->getLuaInterpreter();
        pL->setCaptureGroups( captureList, posList );
        // call lua trigger function with number of matches and matches itselves as arguments
        execute();
        pL->clearCaptureGroups();
        return true;
    }
}

EXIT_OK:    

    return true;
   
    
ERROR: 

    return false;
    
}

bool TTrigger::match_begin_of_line_substring( QString & toMatch, QString & regex, int regexNumber )
{
    if( toMatch.startsWith( regex ) )
    {
        std::list<std::string> captureList;
        std::list<int> posList;
        captureList.push_back( regex.toLatin1().data() );
        posList.push_back( 0 );
        if( mudlet::debugMode ) TDebug()<<"Trigger name="<<mName<<"("<<mRegexCodeList.value(regexNumber)<<") matched!">>0;
        if( mIsMultiline )
        {
            updateMultistates( regexNumber, captureList, posList );
            return true;
        }
        execute();
        return true;
    }
    return false;
}

inline void TTrigger::updateMultistates( int regexNumber,
                                         std::list<std::string> & captureList,
                                         std::list<int> & posList )
{
    if( regexNumber == 0 )
    {
        // wird automatisch auf #1 gesetzt
        TMatchState * pCondition = new TMatchState( mRegexCodeList.size(), mConditionLineDelta );
        mConditionMap[pCondition] = pCondition;
        pCondition->multiCaptureList.push_back( captureList );
        pCondition->multiCapturePosList.push_back( posList );
        if( mudlet::debugMode ) TDebug() << "match state " << mConditionMap.size() << "/" << mConditionMap.size() <<" condition #" << regexNumber << "=true (" << regexNumber << "/" << mRegexCodeList.size() << ") regex=" << mRegexCodeList[regexNumber] >> 0;
    }
    else
    {
        int k=0;
        for( map<TMatchState *, TMatchState *>::iterator it=mConditionMap.begin(); it!=mConditionMap.end(); it++ )
        {
            k++;
            if( (*it).second->nextCondition() == regexNumber )
            {
                if( mudlet::debugMode ) TDebug() << "match state " << k << "/" << mConditionMap.size() <<" condition #" << regexNumber << "=true (" << regexNumber << "/" << mRegexCodeList.size() << ") regex=" << mRegexCodeList[regexNumber] >> 0;
                (*it).second->conditionMatched();
                (*it).second->multiCaptureList.push_back( captureList );
                (*it).second->multiCapturePosList.push_back( posList );
            }
        }
    }
}

bool TTrigger::match_substring( QString & toMatch, QString & regex, int regexNumber )
{
    int where = toMatch.indexOf( regex );
    if( where != -1 )
    {
        std::list<std::string> captureList;
        std::list<int> posList;
        captureList.push_back( regex.toLatin1().data() );
        posList.push_back( where );
        if( mPerlSlashGOption )
        {
            while( (where = toMatch.indexOf( regex, where )) != -1 )
            {
                captureList.push_back( regex.toLatin1().data() );
                posList.push_back( where );
            }
        }
        if( mudlet::debugMode ) TDebug()<<"Trigger name="<<mName<<"("<<mRegexCodeList.value(regexNumber)<<") matched!">>0;
        if( mIsMultiline )
        {
            updateMultistates( regexNumber, captureList, posList );
            return true;
        }
        execute();    
        return true;
    }    
    return false;
}

bool TTrigger::match_lua_code( int regexNumber )
{
    if( mLuaConditionMap.find( regexNumber ) == mLuaConditionMap.end() ) return false;

    if( mpLua->callConditionFunction( mLuaConditionMap[regexNumber], mName ) )
    {
        if( mudlet::debugMode ) TDebug()<<"Trigger name="<<mName<<"("<<mRegexCodeList.value(regexNumber)<<") matched!">>0;
        if( mIsMultiline )
        {
            std::list<std::string> captureList;
            std::list<int> posList;
            updateMultistates( regexNumber, captureList, posList );
            return true;
        }
        execute();
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
        std::list<std::string> captureList;
        std::list<int> posList;
        captureList.push_back( line.toLatin1().data() );
        posList.push_back( 0 );
        if( mudlet::debugMode ) TDebug()<<"Trigger name="<<mName<<"("<<mRegexCodeList.value(regexNumber)<<") matched!">>0;
        if( mIsMultiline )
        {
            updateMultistates( regexNumber, captureList, posList );
            return true;
        }
        execute();    
        return true;
    }    
    return false;
}

bool TTrigger::match( char * subject, QString & toMatch )
{
    bool ret = false;
    if( isActive() )
    {
        if( mIsLineTrigger )
        {
            if( (mStartOfLineDelta == 0) && (mLineDelta > 0) )
            {
                mLineDelta--;
                execute();
                mpHost->getTriggerUnit()->mCleanupList.push_back( this );
                return true;
            }
            if( mStartOfLineDelta > 0 )
            {
                mStartOfLineDelta--;
            }

            return false;
        }
              
        if( toMatch.size() < 1 )
        {
            return false;
        }
        
        bool conditionMet = false;
        int highestCondition = 0;
        if( mIsMultiline )
        {
            int matchStateCnt = 0;
            for( map<TMatchState*, TMatchState *>::iterator it=mConditionMap.begin(); it!=mConditionMap.end(); ++it )
            {

                (*it).second->newLineArrived();
                int next = (*it).second->nextCondition();
                if( next > highestCondition )
                {
                    highestCondition = next;
                }
            }
        }

        int size = mRegexCodePropertyList.size();
        for( int i=0; ; i++ )
        {
            if( i >= size ) break;
            ret = false;
            switch( mRegexCodePropertyList.value(i) )
            {
                case REGEX_SUBSTRING:
                    ret = match_substring( toMatch, mRegexCodeList[i], i );
                    break;
                
                case REGEX_PERL:
                    ret = match_perl( subject, toMatch, i );
                    break;
                
                case REGEX_BEGIN_OF_LINE_SUBSTRING:
                    ret = match_begin_of_line_substring( toMatch, mRegexCodeList[i], i );
                    break;
                
                case REGEX_EXACT_MATCH:
                    ret = match_exact_match( toMatch, mRegexCodeList[i], i );
                    break;

                case REGEX_LUA_CODE:
                    ret = match_lua_code( i );
                    break;
            }
            // policy: one match is enough to fire trigger, but in the case of
            //         multiline *all* have to match in order to fire the trigger
            if( ! mIsMultiline ) 
            {
                if( ret )
                {
                    conditionMet = true;
                    break; 
                }
            }
            else
            {
                if( ( ! ret ) && ( i >= highestCondition ) ) break;
            }

        }
        
        // in the case of multiline triggers: check our state
        if( mIsMultiline )
        {
            int k = 0;    
            bool triggerFires = false;            
            conditionMet = false; //invalidate conditionMet as it has no meaning for multiline triggers
            list<TMatchState*> removeList;
            
            for( map<TMatchState*, TMatchState *>::iterator it=mConditionMap.begin(); it!=mConditionMap.end(); ++it )
            {
                k++;
                //qDebug()<<"TMatchState #"<<k<<" lineCount="<<(*it).second->mLineCount<<" delta="<<(*it).second->mDelta<<" conditon ("<<(*it).second->mNextCondition<<"/"<<(*it).second->mNumberOfConditions<<")";
                if( (*it).second->isComplete() )
                {
                    if( mudlet::debugMode ) TDebug()<<"multiline trigger name="<<mName<<" *FIRES* all conditons are fullfilled! executing script">>0;
                    removeList.push_back( (*it).first );
                    conditionMet = true;
                    TLuaInterpreter * pL = mpHost->getLuaInterpreter();
                    pL->setMultiCaptureGroups( (*it).second->multiCaptureList, (*it).second->multiCapturePosList );
                    execute();
                    pL->clearCaptureGroups();
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
        }
        
        // definition trigger chain: a folder is part of a trigger chain if it has a regex defined
        // a trigger chain only lets data pass if the condition matches or in case of multiline all
        // all conditions are fullfilled
        //
        // a folder can also be a simple structural element in which case all data passes through
        // if at least one regex is defined a folder is considered a trigger chain otherwise a structural element
        
        if( conditionMet || ( mRegexCodeList.size() < 1 ) )
        {
            typedef list<TTrigger *>::const_iterator I;
            for( I it = mpMyChildrenList->begin(); it != mpMyChildrenList->end(); it++)
            {
                TTrigger * pChild = *it;
                ret = pChild->match( subject, toMatch );
                if( ret ) conditionMet = true;
            }
        }
        return conditionMet;
    }
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
    cout << "TTrigger::compile() mNeedsToBeCompiled="<<mNeedsToBeCompiled<<endl;
    if( mNeedsToBeCompiled )
    {
        if( ! compileScript() )
        {
            if( mudlet::debugMode ) TDebug()<<"ERROR: Lua compile error. compiling script of Trigger:"<<mName>>0;
            mOK_code = false;
        }
    }
    typedef list<TTrigger *>::const_iterator I;
    for( I it = mpMyChildrenList->begin(); it != mpMyChildrenList->end(); it++)
    {
        TTrigger * pChild = *it;
        pChild->compile();
    }
}

bool TTrigger::setScript( QString & script )
{
    mScript = script;
    mNeedsToBeCompiled = true;
    mOK_code = compileScript();
    return mOK_code;
}

bool TTrigger::compileScript()
{
    mFuncName = QString("Trigger")+QString::number( mID );
    QString code = QString("function ")+ mFuncName + QString("()\n") + mScript + QString("\nend\n");
    QString error;
    if( mpLua->compile( code, error ) )
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

void TTrigger::execute()
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

    if( mIsMultiline )
    {
        mpLua->callMulti( mFuncName, mName );
    }
    else
    {
        mpLua->call( mFuncName, mName );
    }
}

void TTrigger::enableTrigger( QString & name )
{
    if( mName == name )
    {
        setIsActive( true );
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
        setIsActive( false );
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
        setIsActive( false );
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
    ofs << mName;
    ofs << mScript;
    ofs << mRegexCodeList;
    ofs << mRegexCodePropertyList;
    qDebug()<<"serializing:"<< mName;
    ofs << mID;
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

