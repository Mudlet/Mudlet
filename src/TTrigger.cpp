/***************************************************************************
 *   Copyright (C) 2008-2011 by Heiko Koehn  KoehnHeiko@googlemail.com     *
 *                                                                         *
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
#include <QSound>
#include <phonon>

using namespace std;

const QString nothing = "";

TTrigger::TTrigger( TTrigger * parent, Host * pHost )
: Tree<TTrigger>( parent )
, mTriggerContainsPerlRegex( false )
, mPerlSlashGOption( false )
, mFilterTrigger( false )
, mSoundTrigger( false )
, mStayOpen( 0 )
, mColorTrigger( false )
, mColorTriggerFg( false )
, mColorTriggerBg( false )
, mKeepFiring( 0 )
, mpHost( pHost )
, mIsTempTrigger( false )
, mModuleMember(false)
, mModuleMasterFolder(false)
, mNeedsToBeCompiled( true )
, mTriggerType( REGEX_SUBSTRING )
, exportItem(true)
, mIsLineTrigger( false )
, mStartOfLineDelta( 0 )
, mLineDelta( 3 )
, mConditionLineDelta( 0 )
, mIsMultiline( false )
, mpLua( mpHost->getLuaInterpreter() )
, mFgColor( QColor(255,0,0) )
, mBgColor( QColor(255,255,0) )
, mIsColorizerTrigger( false )
{
}

TTrigger::TTrigger( QString name, QStringList regexList, QList<int> regexProperyList, bool isMultiline, Host * pHost )
: Tree<TTrigger>(0)
, mTriggerContainsPerlRegex( false )
, mPerlSlashGOption( false )
, mFilterTrigger( false )
, mSoundTrigger( false )
, mStayOpen( 0 )
, mColorTrigger( false )
, mColorTriggerFg( false )
, mColorTriggerBg( false )
, mKeepFiring( 0 )
, mpHost( pHost )
, mName( name )
, mIsTempTrigger( false )
, mModuleMember(false)
, mModuleMasterFolder(false)
, mRegexCodeList( regexList )
, mRegexCodePropertyList( regexProperyList )
, mNeedsToBeCompiled( true )
, exportItem(true)
, mTriggerType( REGEX_SUBSTRING )
, mIsLineTrigger( false )
, mStartOfLineDelta( 0 )
, mLineDelta( 3 )
, mConditionLineDelta( 0 )
, mIsMultiline( isMultiline )
, mpLua( mpHost->getLuaInterpreter() )
, mFgColor( QColor(255,0,0) )
, mBgColor( QColor(255,255,0) )
, mIsColorizerTrigger( false )

{
    setRegexCodeList( regexList, regexProperyList );
}

TTrigger::~TTrigger()
{
    if( ! mpHost )
    {
        return;
    }
    mpHost->getTriggerUnit()->unregisterTrigger( this );
}

void TTrigger::setName( QString name )
{
    if( ! mIsTempTrigger )
    {
        mpHost->getTriggerUnit()->mLookupTable.remove( mName, this );
    }
    mName = name;
    mpHost->getTriggerUnit()->mLookupTable.insertMulti( name, this );
}

//FIXME: sperren, wenn code nicht compiliert werden kann *ODER* regex falsch
bool TTrigger::setRegexCodeList( QStringList regexList, QList<int> propertyList )
{
    regexList.replaceInStrings( "\n", "" );
    mRegexCodeList.clear();
    mRegexMap.clear();
    mRegexCodePropertyList.clear();
    mLuaConditionMap.clear();
    mColorPatternList.clear();
    mTriggerContainsPerlRegex = false;

    if( propertyList.size() != regexList.size() )
    {
        //FIXME: ronny hat das irgendwie geschafft
        qDebug()<<"[CRITICAL ERROR (plz report):] Trigger name="<<mName<<" aborting reason: propertyList.size() != regexList.size()";
        //assert( propertyList.size() == regexList.size() );
    }

    if( ( propertyList.size() < 1 ) && ( ! isFolder() ) && ( ! mColorTrigger ) )
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
            char * pattern = (char *) malloc( strlen( regexList[i].toLocal8Bit().data() ) + 48 );
            strcpy( pattern, regexList[i].toLocal8Bit().data() );

            int erroffset;

            pcre * re;
            re = pcre_compile( pattern,
                               0,
                               &error,
                               &erroffset,
                               0 );

            if (re == 0)
            {
                if( mudlet::debugMode )
                {
                    TDebug(QColor(Qt::white),QColor(Qt::red))<<"REGEX COMPILE ERROR:">>0;
                    TDebug(QColor(Qt::red),QColor(Qt::gray))<<pattern<<"\n">>0;
                }
                setError( QString( "Pattern '" )+QString(pattern)+QString( "' failed to compile. Correct the pattern.") );
                state = false;
                //printf("PCRE compilation failed at offset %d: %s\n", erroffset, error);
            }
            else
            {
                if( mudlet::debugMode )
                {
                    TDebug(QColor(Qt::white),QColor(Qt::darkGreen))<<"[OK]: REGEX_COMPILE OK\n">>0;
                }
            }
            mRegexMap[i] = re;
            mTriggerContainsPerlRegex = true;
        }
        if( propertyList[i] == REGEX_LUA_CODE )
        {
             std::string funcName;
             std::stringstream func;
             func << "trigger" << mID << "condition" << i;
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
        }
        if( propertyList[i] == REGEX_COLOR_PATTERN )
        {
            QRegExp regex = QRegExp("FG(\\d+)BG(\\d+)");
            int _pos = regex.indexIn( regexList[i] );
            if( _pos == -1 )
            {
                mColorPatternList.push_back( 0 );
                state = false;
                continue;
            }
            int ansiFg = regex.cap(1).toInt();
            int ansiBg = regex.cap(2).toInt();

            if( ! setupColorTrigger( ansiFg, ansiBg ) )
            {
                mColorPatternList.push_back( 0 );
                state = false;
                continue;
            }
//            else
//            {
//                qDebug()<<"[OK] color pattern initialized:"<<regexList[i];
//            }
        }
        else
        {
            mColorPatternList.push_back( 0 );
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

bool TTrigger::match_perl( char * subject, QString & toMatch, int regexNumber, int posOffset )
{
    assert( mRegexMap.contains(regexNumber ) );

    pcre * re = mRegexMap[regexNumber];

    if( ! re )
    {
        if( mudlet::debugMode ){ TDebug(QColor(Qt::white),QColor(Qt::red))<<"ERROR:"<<0; TDebug(QColor(Qt::darkRed),QColor(Qt::darkGray))<<" the regex of trigger "<<mName<<" does not compile. Please correct the expression. This trigger will never match until it is fixed.\n">>0;}
        return false; //regex compile error
    }

    int numberOfCaptureGroups = 0;
    unsigned char *name_table;
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
             return false;
        }
    }
    if( rc > 0 )
    {
        if( mudlet::debugMode ){ TDebug(QColor(Qt::blue),QColor(Qt::black))<<"Trigger name="<<mName<<"("<<mRegexCodeList.value(regexNumber)<<") matched.\n">>0;}
    }

    if( rc == 0 )
    {
        qDebug()<<"CRITICAL ERROR: SHOULD NOT HAPPEN->pcre_info() got wrong num of cap groups ovector only has room for %d captured substrings\n";
    }

    if( rc < 0 )
    {
        return false;
    }

    for( i=0; i < rc; i++ )
    {
        char * substring_start = subject + ovector[2*i];
        int substring_length = ovector[2*i+1] - ovector[2*i];
        std::string match;
        if( substring_length < 1 )
        {
            captureList.push_back( match );
            posList.push_back( -1 );
            continue;
        }

        match.append( substring_start, substring_length );
        captureList.push_back( match );
        posList.push_back( ovector[2*i] + posOffset );
        if( mudlet::debugMode ){ TDebug(QColor(Qt::darkCyan),QColor(Qt::black))<<"capture group #"<<(i+1)<<" = ">>0; TDebug(QColor(Qt::darkMagenta),QColor(Qt::black))<<"<"<<match.c_str()<<">\n">>0;}
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
            //int n = (tabptr[0] << 8) | tabptr[1];
            //printf("GOT:(%d) %*s: %.*s\n", n, name_entry_size - 3, tabptr + 2, ovector[2*n+1] - ovector[2*n], subject + ovector[2*n]);
            tabptr += name_entry_size;
        }
    }
    //TODO: add named groups seperately later as Lua::namedGroups
    if( mIsColorizerTrigger || mFilterTrigger )
    {
        numberOfCaptureGroups = captureList.size();
    }
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

            std::string match;
            if( substring_length < 1 )
            {
                captureList.push_back( match );
                posList.push_back( -1 );
                continue;
            }
            match.append( substring_start, substring_length );
            captureList.push_back( match );
            posList.push_back( ovector[2*i] + posOffset );
            if( mudlet::debugMode )
            {
                TDebug(QColor(Qt::darkCyan),QColor(Qt::black))<<"<regex mode: match all> capture group #"<<(i+1)<<" = ">>0; TDebug(QColor(Qt::darkMagenta),QColor(Qt::black))<<"<"<<match.c_str()<<">\n">>0;
            }
        }
    }

END:
{
    if( mIsColorizerTrigger )
    {
        int r1 = mBgColor.red();
        int g1 = mBgColor.green();
        int b1 = mBgColor.blue();
        int r2 = mFgColor.red();
        int g2 = mFgColor.green();
        int b2 = mFgColor.blue();
        int total = captureList.size();
        TConsole * pC = mpHost->mpConsole;
        pC->deselect();
        std::list<std::string>::iterator its = captureList.begin();
        std::list<int>::iterator iti = posList.begin();
        for( int i=1; iti!=posList.end(); ++iti, ++its, i++ )
        {
            int begin = *iti;
            std::string & s = *its;
            int length = s.size();
            if( total > 1 )
            {
                // skip complete match in Perl /g option type of triggers
                // to enable people to highlight capture groups if there are any
                // otherwise highlight complete expression match
                if( i % numberOfCaptureGroups != 1 )
                {
                    pC->selectSection( begin, length );
                    pC->setBgColor( r1, g1, b1 );
                    pC->setFgColor( r2, g2, b2 );
                }
            }
            else
            {
                pC->selectSection( begin, length );
                pC->setBgColor( r1, g1, b1 );
                pC->setFgColor( r2, g2, b2 );
            }
        }
        pC->reset();
    }
    if( mIsMultiline )
    {
        updateMultistates( regexNumber, captureList, posList );
        return true;
    }
    else
    {
        TLuaInterpreter * pL = mpHost->getLuaInterpreter();
        pL->setCaptureGroups( captureList, posList );
        execute();
        pL->clearCaptureGroups();
        if( mFilterTrigger )
        {
             if( captureList.size() > 1 )
             {
                 int total = captureList.size();
                 std::list<std::string>::iterator its = captureList.begin();
                 std::list<int>::iterator iti = posList.begin();
                 for( int i=1; iti!=posList.end(); ++iti, ++its, i++ )
                 {
                     int begin = *iti;
                     std::string & s = *its;
                     if( total > 1 )
                     {
                         // skip complete match in Perl /g option type of triggers
                         // to enable people to highlight capture groups if there are any
                         // otherwise highlight complete expression match
                         if( i % numberOfCaptureGroups != 1 )
                         {
                             filter( s, begin );
                         }
                     }
                     else
                     {
                         filter( s, begin );
                     }
                 }
             }
        }
        return true;
    }
}
    return true;


//ERROR:

//    return false;

}

bool TTrigger::match_begin_of_line_substring( QString & toMatch, QString & regex, int regexNumber, int posOffset )
{
    if( toMatch.startsWith( regex ) )
    {
        std::list<std::string> captureList;
        std::list<int> posList;
        captureList.push_back( regex.toLatin1().data() );
        posList.push_back( 0 + posOffset );
        if( mudlet::debugMode ){ TDebug(QColor(Qt::darkCyan),QColor(Qt::black))<<"Trigger name="<<mName<<"("<<mRegexCodeList.value(regexNumber)<<") matched.\n">>0;}
        if( mIsColorizerTrigger )
        {
            int r1 = mBgColor.red();
            int g1 = mBgColor.green();
            int b1 = mBgColor.blue();
            int r2 = mFgColor.red();
            int g2 = mFgColor.green();
            int b2 = mFgColor.blue();
            TConsole * pC = mpHost->mpConsole;
            std::list<std::string>::iterator its = captureList.begin();
            std::list<int>::iterator iti = posList.begin();
            for( ; iti!=posList.end(); ++iti, ++its )
            {
                int begin = *iti;
                std::string & s = *its;
                int length = s.size();
                pC->selectSection( begin, length );
                pC->setBgColor( r1, g1, b1 );
                pC->setFgColor( r2, g2, b2 );
            }
            pC->reset();
        }
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
            if( mFilterTrigger )
            {
                if( captureList.size() > 0 )
                {
                    filter( captureList.front(), posList.front() );
                }
            }
            return true;
        }
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
        if( mudlet::debugMode )
        {
            TDebug(QColor(Qt::darkYellow),QColor(Qt::black)) << "match state " << mConditionMap.size() << "/" << mConditionMap.size() <<" condition #" << regexNumber << "=true (" << regexNumber << "/" << mRegexCodeList.size() << ") regex=" << mRegexCodeList[regexNumber] <<"\n" >> 0;
        }
    }
    else
    {
        int k=0;
        for( map<TMatchState *, TMatchState *>::iterator it=mConditionMap.begin(); it!=mConditionMap.end(); it++ )
        {
            k++;
            if( (*it).second->nextCondition() == regexNumber )
            {
                if( mudlet::debugMode )
                {
                    TDebug(QColor(Qt::darkYellow),QColor(Qt::black)) << "match state " << k << "/" << mConditionMap.size() <<" condition #" << regexNumber << "=true (" << regexNumber << "/" << mRegexCodeList.size() << ") regex=" << mRegexCodeList[regexNumber] <<"\n" >> 0;
                }
                (*it).second->conditionMatched();
                (*it).second->multiCaptureList.push_back( captureList );
                (*it).second->multiCapturePosList.push_back( posList );
            }
        }
    }
}

inline void TTrigger::filter( std::string & capture, int & posOffset )
{
    if( capture.size() < 1 ) return;
    char * filterSubject = (char *) malloc( capture.size() + 2048 );
    if( filterSubject )
    {
        strcpy( filterSubject, capture.c_str() );
    }
    else
    {
        return;
    }
    QString text = capture.c_str();
    typedef list<TTrigger *>::const_iterator I;
    for( I it = mpMyChildrenList->begin(); it != mpMyChildrenList->end(); it++)
    {
        (*it)->match( filterSubject, text, -1, posOffset );
    }
    free( filterSubject );
}

bool TTrigger::match_substring( QString & toMatch, QString & regex, int regexNumber, int posOffset )
{
    int where = toMatch.indexOf( regex );
    if( where != -1 )
    {
        std::list<std::string> captureList;
        std::list<int> posList;
        captureList.push_back( regex.toLatin1().data() );
        posList.push_back( where + posOffset );
        if( mPerlSlashGOption )
        {
            while( (where = toMatch.indexOf( regex, where + 1 )) != -1 )
            {
                captureList.push_back( regex.toLatin1().data() );
                posList.push_back( where + posOffset );
            }
        }
        if( mudlet::debugMode ) {TDebug(QColor(Qt::cyan),QColor(Qt::black))<<"Trigger name="<<mName<<"("<<mRegexCodeList.value(regexNumber)<<") matched.\n">>0;}
        if( mIsColorizerTrigger )
        {
            int r1 = mBgColor.red();
            int g1 = mBgColor.green();
            int b1 = mBgColor.blue();
            int r2 = mFgColor.red();
            int g2 = mFgColor.green();
            int b2 = mFgColor.blue();
            TConsole * pC = mpHost->mpConsole;
            pC->deselect();
            std::list<std::string>::iterator its = captureList.begin();
            std::list<int>::iterator iti = posList.begin();
            for( ; iti!=posList.end(); ++iti, ++its )
            {
                int begin = *iti;
                std::string & s = *its;
                int length = s.size();
                pC->selectSection( begin, length );
                pC->setBgColor( r1, g1, b1 );
                pC->setFgColor( r2, g2, b2 );
            }
            pC->reset();
        }
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
            if( mFilterTrigger )
            {
                if( captureList.size() > 0 )
                {
                    filter( captureList.front(), posList.front() );
                }
            }
            return true;
        }
    }
    return false;
}

bool TTrigger::match_color_pattern( int line, int regexNumber )
{
    if( regexNumber >= mColorPatternList.size() ) return false;
    if( line == -1 ) return false;
    //bool bgColorMatch = false;
    //bool fgColorMatch = false;
    bool canExecute = false;
    std::list<std::string> captureList;
    std::list<int> posList;
    if( line >= static_cast<int>(mpHost->mpConsole->buffer.buffer.size()) ) return false;
    std::deque<TChar> & bufferLine = mpHost->mpConsole->buffer.buffer[line];
    QString & lineBuffer = mpHost->mpConsole->buffer.lineBuffer[line];
    typedef std::deque<TChar>::iterator IT;
    int pos = 0;
    //bool fgColorChange = false;
    int matchBegin = -1;
    bool matching = false;

    TColorTable * pCT = mColorPatternList[regexNumber];
    if( ! pCT ) return false; //no color pattern created
    int mFgR = pCT->fgR;
    int mFgG = pCT->fgG;
    int mFgB = pCT->fgB;
    int mBgR = pCT->bgR;
    int mBgG = pCT->bgG;
    int mBgB = pCT->bgB;
    for( IT it=bufferLine.begin(); it!=bufferLine.end(); it++, pos++ )
    {
        if( ( (*it).fgR == mFgR )
         && ( (*it).fgG == mFgG )
         && ( (*it).fgB == mFgB )
         && ( (*it).bgR == mBgR )
         && ( (*it).bgG == mBgG )
         && ( (*it).bgB == mBgB ) )
        {
            if( matchBegin == -1 )
                matchBegin = pos;
            matching = true;
        }
        else
        {
            matching = false;
        }
        if( ( ! matching ) || ( matching && ( pos+1 >= static_cast<int>(bufferLine.size()) ) ) )
        {
            if( matchBegin > -1 )
            {
                std::string got;
                if( matching )
                    got = lineBuffer.mid(matchBegin, pos-matchBegin+1).toLatin1().data();
                else
                    got = lineBuffer.mid(matchBegin, pos-matchBegin).toLatin1().data();
                captureList.push_back( got );
                posList.push_back( matchBegin );
                matchBegin = -1;
                canExecute = true;
                matching = false;
            }
        }
    }

    if( canExecute )
    {
        if( mIsColorizerTrigger )
        {
            int r1 = mBgColor.red();
            int g1 = mBgColor.green();
            int b1 = mBgColor.blue();
            int r2 = mFgColor.red();
            int g2 = mFgColor.green();
            int b2 = mFgColor.blue();
            TConsole * pC = mpHost->mpConsole;
            pC->deselect();
            std::list<std::string>::iterator its = captureList.begin();
            std::list<int>::iterator iti = posList.begin();
            for( ; iti!=posList.end(); ++iti, ++its )
            {
                int begin = *iti;
                std::string & s = *its;
                cout<<"CTgot<"<<s<<"> bis:"<<s.size()<<endl;

                int length = s.size();
                pC->selectSection( begin, length );
                pC->setBgColor( r1, g1, b1 );
                pC->setFgColor( r2, g2, b2 );
            }
            pC->reset();
        }
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
            if( mFilterTrigger )
            {
                if( captureList.size() > 0 )
                {
                    typedef std::list<std::string>::iterator IT;
                    typedef std::list<int>::iterator IT2;
                    IT it1 = captureList.begin();
                    IT2 it2 = posList.begin();
                    for( ; it1!=captureList.end(); it1++, it2++ )
                    {
                        filter( *it1, *it2 );
                    }
                }
            }
            return true;
        }
    }
    return false;
}

bool TTrigger::match_line_spacer( int regexNumber )
{
    if( mIsMultiline )
    {
        int k=0;

        for( map<TMatchState *, TMatchState *>::iterator it=mConditionMap.begin(); it!=mConditionMap.end(); it++ )
        {
            k++;
            if( (*it).second->nextCondition() == regexNumber )
            {
                if( (*it).second->lineSpacerMatch( mRegexCodeList.value(regexNumber).toInt() ) )
                {
                    if( mudlet::debugMode )
                    {
                        TDebug(QColor(Qt::yellow),QColor(Qt::black))<<"Trigger name="<<mName<<"("<<mRegexCodeList.value(regexNumber)<<") condition #"<<regexNumber<<"=true ">>0;
                        TDebug(QColor(Qt::darkYellow),QColor(Qt::black)) << "match state " << k << "/" << mConditionMap.size() <<" condition #" << regexNumber << "=true (" << regexNumber+1 << "/" << mRegexCodeList.size() << ") line spacer=" << mRegexCodeList.value(regexNumber) <<"lines\n" >> 0;
                    }
                    (*it).second->conditionMatched();
                    std::list<string> captureList;
                    std::list<int> posList;
                    (*it).second->multiCaptureList.push_back( captureList );
                    (*it).second->multiCapturePosList.push_back( posList );
                }
            }
        }
    }

    return true; //line spacers don't make sense outside of AND triggers -> ignore them
}

bool TTrigger::match_lua_code( int regexNumber )
{
    if( mLuaConditionMap.find( regexNumber ) == mLuaConditionMap.end() ) return false;

    if( mpLua->callConditionFunction( mLuaConditionMap[regexNumber], mName ) )
    {
        if( mudlet::debugMode ){ TDebug(QColor(Qt::yellow),QColor(Qt::black))<<"Trigger name="<<mName<<"("<<mRegexCodeList.value(regexNumber)<<") matched.\n">>0;}
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

bool TTrigger::match_exact_match( QString & toMatch, QString & line, int regexNumber, int posOffset )
{
    QString text = toMatch;
    if( text.endsWith(QChar('\n')) ) text.chop(1); //TODO: speed optimization
    if( text == line )
    {
        std::list<std::string> captureList;
        std::list<int> posList;
        captureList.push_back( line.toLatin1().data() );
        posList.push_back( 0 + posOffset );
        if( mudlet::debugMode ) {TDebug(QColor(Qt::yellow),QColor(Qt::black))<<"Trigger name="<<mName<<"("<<mRegexCodeList.value(regexNumber)<<") matched.\n">>0;}
        if( mIsColorizerTrigger )
        {
            int r1 = mBgColor.red();
            int g1 = mBgColor.green();
            int b1 = mBgColor.blue();
            int r2 = mFgColor.red();
            int g2 = mFgColor.green();
            int b2 = mFgColor.blue();
            TConsole * pC = mpHost->mpConsole;
            std::list<std::string>::iterator its = captureList.begin();
            std::list<int>::iterator iti = posList.begin();
            for( ; iti!=posList.end(); ++iti, ++its )
            {
                int begin = *iti;
                std::string & s = *its;
                int length = s.size();
                pC->selectSection( begin, length );
                pC->setBgColor( r1, g1, b1 );
                pC->setFgColor( r2, g2, b2 );
            }
            pC->reset();
        }
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
            if( mFilterTrigger )
            {
                if( captureList.size() > 0 )
                {
                    filter( captureList.front(), posList.front() );
                }
            }
            return true;
        }
    }
    return false;
}

bool TTrigger::match( char * subject, QString & toMatch, int line, int posOffset )
{
    bool ret = false;
    if( isActive() )
    {
        if( mIsLineTrigger )
        {
            if( --mStartOfLineDelta < 0 )
            {
                execute();
                if( --mLineDelta <= 0 )
                {
                    deactivate();
                    mpHost->getTriggerUnit()->markCleanup( this );
                }
                return true;
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
            //int matchStateCnt = 0;
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
                    ret = match_substring( toMatch, mRegexCodeList[i], i, posOffset );
                    break;

                case REGEX_PERL:
                    ret = match_perl( subject, toMatch, i, posOffset );
                    break;

                case REGEX_BEGIN_OF_LINE_SUBSTRING:
                    ret = match_begin_of_line_substring( toMatch, mRegexCodeList[i], i, posOffset );
                    break;

                case REGEX_EXACT_MATCH:
                    ret = match_exact_match( toMatch, mRegexCodeList[i], i, posOffset );
                    break;

                case REGEX_LUA_CODE:
                    ret = match_lua_code( i );
                    break;

                case REGEX_LINE_SPACER:
                    ret = match_line_spacer( i );
                    break;

                case REGEX_COLOR_PATTERN:
                    ret = match_color_pattern( line, i );
                    break;
            }
            // policy: one match is enough to fire on OR-trigger, but in the case of
            //         an AND-trigger all conditions have to be met in order to fire the trigger
            if( ! mIsMultiline )
            {
                if( ret )
                {
                    conditionMet = true;
                    mKeepFiring = mStayOpen;
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
            //bool triggerFires = false;
            conditionMet = false; //invalidate conditionMet as it has no meaning for multiline triggers
            list<TMatchState*> removeList;

            for( map<TMatchState*, TMatchState *>::iterator it=mConditionMap.begin(); it!=mConditionMap.end(); ++it )
            {
                k++;
                //qDebug()<<"TMatchState #"<<k<<" lineCount="<<(*it).second->mLineCount<<" delta="<<(*it).second->mDelta<<" conditon ("<<(*it).second->mNextCondition<<"/"<<(*it).second->mNumberOfConditions<<")";
                if( (*it).second->isComplete() )
                {
                    mKeepFiring = mStayOpen;
                    if( mudlet::debugMode ){ TDebug(QColor(Qt::yellow),QColor(Qt::darkMagenta))<<"multiline trigger name="<<mName<<" *FIRES* all conditons are fullfilled. Executing script.\n">>0;}
                    removeList.push_back( (*it).first );
                    conditionMet = true;
                    TLuaInterpreter * pL = mpHost->getLuaInterpreter();
                    pL->setMultiCaptureGroups( (*it).second->multiCaptureList, (*it).second->multiCapturePosList );
                    execute();
                    pL->clearCaptureGroups();
                    if( mFilterTrigger )
                    {
                        std::list< std::list<std::string> > multiCaptureList;
                        multiCaptureList = (*it).second->multiCaptureList;
                        if( multiCaptureList.size() > 0 )
                        {
                            std::list< std::list<std::string> >::iterator mit = multiCaptureList.begin();
                            for( ; mit!=multiCaptureList.end(); mit++, k++ )
                            {
                                int total = (*mit).size();
                                std::list<std::string>::iterator its = (*mit).begin();
                                for( int i=1; its!=(*mit).end(); ++its, i++ )
                                {
                                    std::string s = *its;
                                    int p = 0;
                                    if( total > 1 )
                                    {
                                        if( i % total != 1 )
                                        {
                                            filter( s, p );
                                        }
                                    }
                                    else
                                    {
                                        filter( s, p );
                                    }
                                }
                            }
                        }
                    }
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
                    if( mudlet::debugMode )
                    {
                        TDebug(QColor(Qt::darkBlue),QColor(Qt::black))<< "removing condition from conditon table.\n">>0;
                    }
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
        if( ! mFilterTrigger )
        {
            if( conditionMet || ( mRegexCodeList.size() < 1 ) )
            {
                typedef list<TTrigger *>::const_iterator I;
                for( I it = mpMyChildrenList->begin(); it != mpMyChildrenList->end(); it++)
                {
                    TTrigger * pChild = *it;
                    ret = pChild->match( subject, toMatch, line );
                    if( ret ) conditionMet = true;
                }
            }
        }

        if( ( mKeepFiring > 0 ) && ( ! conditionMet ) )
        {
            mKeepFiring--;
            if( ( mKeepFiring == mStayOpen ) || ( mpMyChildrenList->size() == 0 ) )
            {
                execute();
            }
            bool conditionMet = false;
            typedef list<TTrigger *>::const_iterator I;
            for( I it = mpMyChildrenList->begin(); it != mpMyChildrenList->end(); it++)
            {
                TTrigger * pChild = *it;
                ret = pChild->match( subject, toMatch, line );
                if( ret ) conditionMet = true;
            }
            return true;
        }


        return conditionMet;
    }
    return false;
}


// Die Musternummer wird ID im color-pattern lookup table
TColorTable * TTrigger::createColorPattern( int ansiFg, int ansiBg )
{
    /* Mudlet simplified ANSI color codes
      -----------------------------------
      0  default text color
      1  light black
      2  dark black
      3  light red
      4  dark red
      5  light green
      6  dark green
      7  light yellow
      8  dark yellow
      9  light blue
      10 dark blue
      11 light magenta
      12 dark magenta
      13 light cyan
      14 dark cyan
      15 light white
      16 dark white */

    bool invalidColorCode = false;

    int fgColorR = 0;
    int fgColorG = 0;
    int fgColorB = 0;
    int bgColorR = 0;
    int bgColorG = 0;
    int bgColorB = 0;

    int tag = ansiFg;
    if( tag <= 16 )
    {
        QColor c;
        switch( tag )
        {
            case 0: c = mpHost->mFgColor;  break;
            case 1: c = mpHost->mLightBlack; break;
            case 2: c = mpHost->mBlack; break;
            case 3: c = mpHost->mLightRed; break;
            case 4: c = mpHost->mRed; break;
            case 5: c = mpHost->mLightGreen; break;
            case 6: c = mpHost->mGreen; break;
            case 7: c = mpHost->mLightYellow; break;
            case 8: c = mpHost->mYellow; break;
            case 9: c = mpHost->mLightBlue; break;
            case 10: c = mpHost->mBlue; break;
            case 11: c = mpHost->mLightMagenta; break;
            case 12: c = mpHost->mMagenta; break;
            case 13: c = mpHost->mLightCyan; break;
            case 14: c = mpHost->mCyan; break;
            case 15: c = mpHost->mLightWhite; break;
            case 16: c = mpHost->mWhite; break;
        }
        fgColorR = c.red();
        fgColorG = c.green();
        fgColorB = c.blue();
    }
    else
    {
        if( tag < 232 )
        {
            tag-=16; // because color 1-15 behave like normal ANSI colors
            // 6x6 RGB color space
            int r = tag / 36;
            int g = (tag-(r*36)) / 6;
            int b = (tag-(r*36))-(g*6);
            fgColorR = r*42;
            fgColorG = g*42;
            fgColorB = b*42;
        }
        else if( tag < 256 )
        {
            // black + 23 tone grayscale from dark to light gray
            tag -= 232;
            fgColorR = tag*10;
            fgColorG = tag*10;
            fgColorB = tag*10;
        }
        else
        {
            //return invalid color error
            invalidColorCode = true;
        }
    }

    tag = ansiBg;
    if( tag <= 16 )
    {
        QColor c;
        switch( tag )
        {
            case 0: c = mpHost->mBgColor;  break;
            case 1: c = mpHost->mLightBlack; break;
            case 2: c = mpHost->mBlack; break;
            case 3: c = mpHost->mLightRed; break;
            case 4: c = mpHost->mRed; break;
            case 5: c = mpHost->mLightGreen; break;
            case 6: c = mpHost->mGreen; break;
            case 7: c = mpHost->mLightYellow; break;
            case 8: c = mpHost->mYellow; break;
            case 9: c = mpHost->mLightBlue; break;
            case 10: c = mpHost->mBlue; break;
            case 11: c = mpHost->mLightMagenta; break;
            case 12: c = mpHost->mMagenta; break;
            case 13: c = mpHost->mLightCyan; break;
            case 14: c = mpHost->mCyan; break;
            case 15: c = mpHost->mLightWhite; break;
            case 16: c = mpHost->mWhite; break;
        }
        bgColorR = c.red();
        bgColorG = c.green();
        bgColorB = c.blue();
    }
    else
    {
        if( tag < 232 )
        {
            tag-=16; // because color 1-15 behave like normal ANSI colors
            // 6x6 RGB color space
            int r = tag / 36;
            int g = (tag-(r*36)) / 6;
            int b = (tag-(r*36))-(g*6);
            bgColorR = r*42;
            bgColorG = g*42;
            bgColorB = b*42;
        }
        else if( tag < 256 )
        {
            // black + 23 tone grayscale from dark to light gray
            tag -= 232;
            bgColorR = tag*10;
            bgColorG = tag*10;
            bgColorB = tag*10;
        }
        else
        {
            //return invalid color error
            invalidColorCode = true;
        }
    }

    if( invalidColorCode ) return 0;

    TColorTable * pCT = new TColorTable;
    if( !pCT ) return 0;

    pCT->ansiBg = ansiBg;
    pCT->ansiFg = ansiFg;
    pCT->bgB = bgColorB;
    pCT->bgG = bgColorG;
    pCT->bgR = bgColorR;
    pCT->fgB = fgColorB;
    pCT->fgG = fgColorG;
    pCT->fgR = fgColorR;
    return pCT;
}

bool TTrigger::setupColorTrigger( int ansiFg, int ansiBg )
{
    TColorTable * pCT = createColorPattern( ansiFg, ansiBg );
    if( ! pCT ) return false;
    mColorPatternList.push_back( pCT );
    return true;
}

bool TTrigger::setupTmpColorTrigger( int ansiFg, int ansiBg )
{
    TColorTable * pCT = createColorPattern( ansiFg, ansiBg );
    if( ! pCT ) return false;
    QString code;
    code = QString("FG%1BG%2").arg(ansiFg).arg(ansiBg);
    mRegexCodeList << code;
    mRegexCodePropertyList << REGEX_COLOR_PATTERN;
    mColorPatternList.push_back( pCT );
    return true;
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
        return false;
    }
    return mpHost->getTriggerUnit()->registerTrigger(this);
}

void TTrigger::compileAll()
{
    mNeedsToBeCompiled = true;
    if( ! compileScript() )
    {
        if( mudlet::debugMode ){ TDebug(QColor(Qt::white),QColor(Qt::red))<<"ERROR: Lua compile error. compiling script of Trigger:"<<mName<<"\n">>0;}
        mOK_code = false;
    }
    setRegexCodeList(  mRegexCodeList, mRegexCodePropertyList );
    typedef list<TTrigger *>::const_iterator I;
    for( I it = mpMyChildrenList->begin(); it != mpMyChildrenList->end(); it++)
    {
        TTrigger * pChild = *it;
        pChild->compileAll();
    }
}


void TTrigger::compile()
{
    if( mNeedsToBeCompiled )
    {
        if( ! compileScript() )
        {
            if( mudlet::debugMode ){ TDebug(QColor(Qt::white),QColor(Qt::red))<<"ERROR: Lua compile error. compiling script of Trigger:"<<mName<<"\n">>0;}
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
    if( mSoundTrigger )
    {
        mudlet::self()->playSound( mSoundFile );
    }
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

    //if( initMode ) qDebug()<<"TTrigger::restore() mName="<<mName<<" mID="<<mID<<" children="<<children;

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

