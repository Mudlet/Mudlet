
#ifndef _TIMER_H_
#define _TIMER_H_

/***************************************************************************
 *   Copyright (C) 2008-2009 by Heiko Koehn                                     *
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
#include <QTimer>
#include <QString>
#include <QRegExp>
#include "Tree.h"
#include <QDataStream>
#include "Host.h"
#include <QTextBlock>
#include <QTime>



class TTimer : public Tree<TTimer>
{
    friend class TimerUnit;
    friend class XMLexport;
    friend class XMLimport;

public:

                    ~TTimer();
                     TTimer( TTimer * parent, Host * pHost );
                     TTimer( QString name, QTime time, Host * pHost );
                     TTimer& clone(const TTimer& );
    void             compileAll();
    QString &        getName()                       { return mName; }
    void             setName( QString name );
    QTime &          getTime()                       { return mTime; }
    void             compile();
    bool             checkRestart();
    bool             compileScript();
    void             execute();
    void             setTime( QTime time );
    QString          getCommand()                    { return mCommand; }
    void             setCommand( QString & cmd )     { mCommand = cmd; }
    QString          getScript()                     { return mScript; }
    bool             setScript( QString & script );
    bool             canBeUnlocked( TTimer * );
    bool             isFolder()                      { return mIsFolder; }
    void             setIsTempTimer( bool b )        { mIsTempTimer = b; }
    bool             isTempTimer()                   { return mIsTempTimer; }
    void             setIsFolder( bool b )           { mIsFolder = b; }
    bool             registerTimer();
    bool             setIsActive( bool );
    void             stop();
    void             start();
    void             enableTimer();
    void             disableTimer();
    void             enableTimer( QString & );
    void             disableTimer( QString & );
    void             enableTimer( qint64 );
    void             disableTimer( qint64 );
    void             killTimer();

    bool             isClone(TTimer &b) const;
    bool             isOffsetTimer();
    bool             mRegisteredAnonymousLuaFunction;
    bool             exportItem;
    bool            mModuleMasterFolder;
private:

                       TTimer(){};
    QString            mName;
    QString            mScript;
    QTime              mTime;
    QString            mCommand;
    QString            mFuncName;
    bool               mIsFolder;
    Host *             mpHost;
    bool               mNeedsToBeCompiled;
    bool               mIsTempTimer;
    QMutex             mLock;
    QTimer *           mpTimer;
    bool                  mModuleMember;
    //TLuaInterpreter *  mpLua;

};

#endif

