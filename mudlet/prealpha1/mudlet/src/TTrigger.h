
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

#define REGEX_SUBSTRING 0
#define REGEX_PERL 1
#define REGEX_WILDCARD 2
#define REGEX_EXACT_MATCH 3

using namespace std;



class TTrigger : public Tree<TTrigger>
{
public:
    
                      
                      //TTrigger(const TTrigger &);
    virtual          ~TTrigger();
                     TTrigger( TTrigger * parent, Host * pHost ); 
                     TTrigger( QString name, QStringList regexList, QList<int> regexPorpertyList, bool isMultiline, Host * pHost ); //throws exeption ExObjNoCreate
                     TTrigger& clone(const TTrigger& );
                      //TTrigger & TTrigger( const TTrigger & ); //assignment operator not needed by now
                      //TTrigger( const TTrigger & ); //copyconstructor not needed so far all members have copyconstructors
     
    QString          getName()                       { QMutexLocker locker(& mLock); return mName; }
    void             setName( QString name )         { QMutexLocker locker(& mLock); mName = name; }
    QStringList &    getRegexCodeList()              { QMutexLocker locker(& mLock); return mRegexCodeList; }
    QList<int>       getRegexCodePropertyList()      { QMutexLocker locker(& mLock); return mRegexCodePropertyList; }
    void             compile();
    void             execute( QStringList & matches );
    void             setRegexCodeList( QStringList regex, QList<int> regexPorpertyList );        
    QString          getScript()                     { QMutexLocker locker(& mLock); return mScript; }
    void             setScript( QString & script )   { QMutexLocker locker(& mLock); mScript = script; mNeedsToBeCompiled=true; }
    bool             match( QString & );
    bool             isActive()                      { QMutexLocker locker(& mLock); return mIsActive; }  
    bool             isFolder()                      { QMutexLocker locker(& mLock); return mIsFolder; }
    bool             isMultiline()                   { QMutexLocker locker(& mLock); return mIsMultiline; }
    int              getTriggerType()                { QMutexLocker locker(& mLock); return mTriggerType; }
    bool             isTempTrigger()                 { QMutexLocker locker(& mLock); return mIsTempTrigger; }
    bool             isLineTrigger()                 { QMutexLocker locker(& mLock); return mIsLineTrigger; }
    void             setIsLineTrigger( bool b )      { QMutexLocker locker(& mLock); mIsLineTrigger = b; }
    void             setStartOfLineDelta( int b )    { QMutexLocker locker(& mLock); mStartOfLineDelta = b; }
    void             setLineDelta( int b )           { QMutexLocker locker(& mLock); mLineDelta = b; }
    void             setTriggerType( int b )         { QMutexLocker locker(& mLock); mTriggerType = b; }    
    void             setIsTempTrigger( bool b )      { QMutexLocker locker(& mLock); mIsTempTrigger = b; }
    void             setIsMultiline( bool b )        { QMutexLocker locker(& mLock); mIsMultiline = b; }    
    void             setIsActive( bool b )           { QMutexLocker locker(& mLock); mIsActive = b; }
    void             setIsFolder( bool b )           { QMutexLocker locker(& mLock); mIsFolder = b; }
    void             enableTrigger( QString & );
    void             disableTrigger( QString & );
    TTrigger *       killTrigger( QString & );
    bool             match_substring( QString &, QString &, int );
    bool             match_perl( QString &, int );
    bool             match_wildcard( QString &, int );
    bool             match_exact_match( QString &, QString &, int );
    void             setConditionLineDelta( int delta )  { QMutexLocker locker(& mLock); mConditionLineDelta = delta; }
    int              getConditionLineDelta() { QMutexLocker locker(& mLock); return mConditionLineDelta; }
    bool             registerTrigger();
    
    bool             serialize( QDataStream & );
    bool             restore( QDataStream & fs );
    bool             mTriggerContainsPerlRegex;
    bool             isClone(TTrigger &b) const;
    
private:
    
                     TTrigger(){};
    QString          mName;
    QStringList      mRegexCodeList;
    QList<int>       mRegexCodePropertyList;
    QMap<int, QRegExp>   mRegexMap;
    Host *           mpHost;
    QString          mScript;
    bool             mIsActive;
    bool             mIsTempTrigger;
    bool             mIsFolder;
    bool             mNeedsToBeCompiled;
    int              mTriggerType;
    bool             mIsLineTrigger;
    int              mStartOfLineDelta;
    int              mLineDelta;
    bool             mIsMultiline;
    int              mConditionLineDelta;
    std::map<int,TMatchState*> mConditionMap;
    
    QMutex           mLock;
};

#endif

