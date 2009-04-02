
#ifndef _TRIGGER_H_
#define _TRIGGER_H_

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
#include <list>
#include <string>
#include <QMutex>
#include <QString>
#include <QRegExp>
#include "Tree.h"
#include <QDataStream>
#include "Host.h"
#include <QTextBlock>
#include "TMatchState.h"
#include <pcre.h>

#define REGEX_SUBSTRING 0
#define REGEX_PERL 1
#define REGEX_BEGIN_OF_LINE_SUBSTRING 2
#define REGEX_EXACT_MATCH 3
#define REGEX_LUA_CODE 4

#define OVECCOUNT 30    // should be a multiple of 3 



class TTrigger : public Tree<TTrigger>
{
    
    friend class XMLexport;
    friend class XMLimport;
        
public:
    
                      
                      //TTrigger(const TTrigger &);
    virtual          ~TTrigger();
                     TTrigger( TTrigger * parent, Host * pHost ); 
                     TTrigger( QString name, QStringList regexList, QList<int> regexPorpertyList, bool isMultiline, Host * pHost ); //throws exeption ExObjNoCreate
                     TTrigger & clone( const TTrigger & );
                      //TTrigger & TTrigger( const TTrigger & ); //assignment operator not needed by now
                      //TTrigger( const TTrigger & ); //copyconstructor not needed so far all members have copyconstructors
    QString          getCommand()                    { return mCommand; }
    void             setCommand( QString b )         { mCommand = b; }
    QString          getName()                       { return mName; }
    void             setName( QString name )         { mName = name; }
    QStringList &    getRegexCodeList()              { return mRegexCodeList; }
    QList<int>       getRegexCodePropertyList()      { return mRegexCodePropertyList; }
    void             compile();
    void             execute();
    bool             isFilterChain();
    void             setRegexCodeList( QStringList regex, QList<int> regexPorpertyList );        
    QString          getScript()                     { return mScript; }
    void             setScript( QString & script )   { mScript = script; mNeedsToBeCompiled=true; }
    bool             match( char *, QString & );
    bool             isFolder()                      { return mIsFolder; }
    bool             isMultiline()                   { return mIsMultiline; }
    int              getTriggerType()                { return mTriggerType; }
    bool             isTempTrigger()                 { return mIsTempTrigger; }
    bool             isLineTrigger()                 { return mIsLineTrigger; }
    void             setIsLineTrigger( bool b )      { mIsLineTrigger = b; }
    void             setStartOfLineDelta( int b )    { mStartOfLineDelta = b; }
    void             setLineDelta( int b )           { mLineDelta = b; }
    void             setTriggerType( int b )         { mTriggerType = b; }
    void             setIsTempTrigger( bool b )      { mIsTempTrigger = b; }
    void             setIsMultiline( bool b )        { mIsMultiline = b; }
    void             setIsFolder( bool b )           { mIsFolder = b; }
    void             enableTrigger( QString & );
    void             disableTrigger( QString & );
    TTrigger *       killTrigger( QString & );
    bool             match_substring( QString &, QString &, int );
    bool             match_perl( char *, QString &, int );
    bool             match_wildcard( QString &, int );
    bool             match_exact_match( QString &, QString &, int );
    bool             match_begin_of_line_substring( QString & toMatch, QString & regex, int regexNumber );
    bool             match_lua_code( int );
    void             setConditionLineDelta( int delta )  { mConditionLineDelta = delta; }
    int              getConditionLineDelta() { return mConditionLineDelta; }
    bool             registerTrigger();
    
    bool             serialize( QDataStream & );
    bool             restore( QDataStream & fs, bool );
    bool             mTriggerContainsPerlRegex;
    bool             mPerlSlashGOption;
    bool             isClone( TTrigger & ) const;
    
private:
    
                                           TTrigger(){};
    void                                   updateMultistates( int regexNumber,
                                                              std::list<std::string> & captureList,
                                                              std::list<int> & posList );
    QString                                mName;
    QStringList                            mRegexCodeList;
    QList<int>                             mRegexCodePropertyList;
    QMap<int, pcre *>                      mRegexMap;
    Host *                                 mpHost;
    QString                                mScript;
    bool                                   mIsTempTrigger;
    bool                                   mIsFolder;
    bool                                   mNeedsToBeCompiled;
    int                                    mTriggerType;
    bool                                   mIsLineTrigger;
    int                                    mStartOfLineDelta;
    int                                    mLineDelta;
    bool                                   mIsMultiline;
    int                                    mConditionLineDelta;
    QString                                mCommand;
    std::map<TMatchState*, TMatchState*>   mConditionMap;
    std::list< std::list<std::string> >    mMultiCaptureGroupList;
    std::list< std::list<int> >            mMultiCaptureGroupPosList;
    TLuaInterpreter *                      mpLua;
    std::map<int, std::string>             mLuaConditionMap;
    QString                                mFuncName;

};

#endif

