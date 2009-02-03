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
        qDebug()<<"CRITICAL ERROR: TTrigger::setRegexCodeList() regexlist.size() != propertyList.size()";
        return;
    }
    
    for( int i=0; i<regexList.size(); i++ )
    {
        if( regexList[i].size() < 1 ) continue;
        
        if( propertyList[i] == REGEX_WILDCARD )
        {
            QRegExp regex = QRegExp( regexList[i] );
            regex.setMinimal( false ); // greedy matching
            regex.setPatternSyntax( QRegExp::Wildcard ); 
            mRegexMap[i] = regex;
        }
        if( propertyList[i] == REGEX_PERL )
        {
            QRegExp regex = QRegExp( regexList[i] );
            regex.setMinimal( false ); // greedy matching
            regex.setPatternSyntax( QRegExp::RegExp );
            mRegexMap[i] = regex;
            mTriggerContainsPerlRegex = true;
        }
        mRegexCodeList.append( regexList[i] );
        mRegexCodePropertyList.append( propertyList[i] );                               
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
    if( mRegexMap[regexNumber].indexIn( toMatch ) > -1 )
    {
        if( mudlet::debugMode ) TDebug()<<"Trigger name="<<mName<<"("<<mRegexCodeList[regexNumber]<<") matched!">>0;
        
        if( mIsMultiline )
        {
            if( regexNumber == 0 )
            {
                if( mudlet::debugMode ) TDebug()<<"#0=true -> creating new MatchState">>0;
                // wird automatisch auf #1 gesetzt
                TMatchState * pCondition = new TMatchState( mRegexCodeList.size(), mConditionLineDelta );
                mConditionMap[mConditionMap.size()+2] = pCondition;    
                if( mudlet::debugMode ) TDebug()<<"new mConditionMap.size()="<<mConditionMap.size()>>0;
                return true;
            }
            else
            {
                
                int k=0;
                for( map<int, TMatchState *>::iterator it=mConditionMap.begin(); it!=mConditionMap.end(); ++it )
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
        QList<int> posList;
        for( int ii=1; ii<=mRegexMap[regexNumber].numCaptures(); ii++ )
        {
            captureList << mRegexMap[regexNumber].cap(ii);
            if( mudlet::debugMode ) TDebug()<<"capture group #"<<QString::number(ii)<<" = <"<<mRegexMap[regexNumber].cap(ii)<<">">>0;
            posList.append( mRegexMap[regexNumber].pos(ii) );
        }
        TLuaInterpreter * pL = mpHost->getLuaInterpreter();
        pL->setCaptureGroups( captureList, posList );
        // call lua trigger function with number of matches and matches itselves as arguments
        execute( captureList );
        pL->clearCaptureGroups();
        return true;
    }
    return false;
}

bool TTrigger::match_wildcard( QString & toMatch, int regexNumber )
{
    if( mRegexMap[regexNumber].exactMatch( toMatch ) )
    {
        if( mudlet::debugMode ) TDebug()<<"Trigger name ="<<mName<<"("<<mRegexCodeList[regexNumber]<<") matched!">>0;
        if( mIsMultiline )
        {
            if( regexNumber == 0 )
            {
                if( mudlet::debugMode ) TDebug()<<"condition #0 = true -> creating new MatchState">>0;
                // wird automatisch auf #1 gesetzt
                TMatchState * pCondition = new TMatchState( mRegexCodeList.size(), mConditionLineDelta );
                mConditionMap[mConditionMap.size()+2] = pCondition;    
                return true;
            }
            else
            {
                
                int k=0;
                for( map<int, TMatchState *>::iterator it=mConditionMap.begin(); it!=mConditionMap.end(); ++it )
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
    return false;
}

bool TTrigger::match_substring( QString & toMatch, QString & regex, int regexNumber )
{
    if( toMatch.indexOf( regex ) != -1 )
    {
        if( mudlet::debugMode ) TDebug()<<"Trigger name="<<mName<<"("<<mRegexCodeList[regexNumber]<<") matched!">>0;
        if( mIsMultiline )
        {
            if( regexNumber == 0 )
            {
                if( mudlet::debugMode ) TDebug()<<"#0=true -> creating new MatchState">>0;
                // wird automatisch auf #1 gesetzt
                TMatchState * pCondition = new TMatchState( mRegexCodeList.size(), mConditionLineDelta );
                mConditionMap[mConditionMap.size()+2] = pCondition;    
                return true;
            }
            else
            {
                int k=0;
                for( map<int, TMatchState *>::iterator it=mConditionMap.begin(); it!=mConditionMap.end(); ++it )
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
        if( mudlet::debugMode ) TDebug()<<"Trigger name="<<mName<<"("<<mRegexCodeList[regexNumber]<<") matched!">>0;
        if( mIsMultiline )
        {
            if( regexNumber == 0 )
            {
                if( mudlet::debugMode ) TDebug()<<"#0=true -> creating new MatchState">>0;
                // wird automatisch auf #1 gesetzt
                TMatchState * pCondition = new TMatchState( mRegexCodeList.size(), mConditionLineDelta );
                mConditionMap[mConditionMap.size()+2] = pCondition;    
                return true;
            }
            else
            {
                int k=0;
                for( map<int, TMatchState *>::iterator it=mConditionMap.begin(); it!=mConditionMap.end(); ++it )
                {
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

bool TTrigger::match( QString & toMatch )
{
    bool ret = false;
    if( mIsActive )
    {
        if( mIsLineTrigger )
        {
            if( mStartOfLineDelta > 0 )
            {
                mStartOfLineDelta--;
            }
            if( (mStartOfLineDelta == 0) && (mLineDelta > 0) )
            {
                mLineDelta--;
                QStringList captureList;
                execute( captureList );
                return true;
            }
            return false;
        }
        if( ! mIsFolder )
        {
            //FIXME: remove later
            if( mRegexCodeList.size() != mRegexCodePropertyList.size() )
            {
                qWarning()<<"CRITICAL ERROR: TTrigger::match( )size of regex and property list are not equal.";
            }
            if( mIsMultiline )
            {
                for( map<int, TMatchState *>::iterator it=mConditionMap.begin(); it!=mConditionMap.end(); ++it )
                {
                    (*it).second->newLineArrived();
                }
            }
            for( int i=0; i<mRegexCodeList.size(); i++ )
            {
                switch( mRegexCodePropertyList[i] )
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
                if( ! mIsMultiline ) if( ret ) break; //normal policy: one match is enough to fire trigger, in case of multiline *all* have to match in order to fire the trigger
            }
        }
        
        // in the case of multiline triggers: check our state
        if( mIsMultiline )
        {
            int k=0;     //weg
            
            list<int> removeList;
            bool triggerFires = false;
            for( map<int, TMatchState *>::iterator it=mConditionMap.begin(); it!=mConditionMap.end(); ++it )
            {
                k++;//weg
                TDebug()<<"EXECUTION CHECK: condition list.size()="<<mConditionMap.size()<<" CHECK:#"<<k>>0;
                if( (*it).second->isComplete() )
                {
                    if( mudlet::debugMode ) TDebug()<<"Multiline trigger name="<<mName<<" all conditons are fullfilled! executing script">>0;
                    removeList.push_back( (*it).first );
                    triggerFires = true;
                }
            
                if( ! (*it).second->newLine() )
                {
                    removeList.push_back( (*it).first );
                }
            }
            for( list<int>::iterator it=removeList.begin(); it!=removeList.end(); it++ )
            {
                delete mConditionMap[*it];
                mConditionMap.erase( mConditionMap.find( *it ) );
                //qDebug()<< "removing ID="<<*it<<" from mConditionMap";
            }
            if( triggerFires )
            {
                QStringList captureList;
                execute( captureList );
            }
        }
        
        typedef list<TTrigger *>::const_iterator I;
        for( I it = mpMyChildrenList->begin(); it != mpMyChildrenList->end(); it++)
        {
            TTrigger * pChild = *it;
            ret = pChild->match( toMatch );
        }
    }
    return false;
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

