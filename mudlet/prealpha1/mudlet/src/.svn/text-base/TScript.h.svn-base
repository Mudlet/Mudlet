
#ifndef _TSCRIPT_H_
#define _TSCRIPT_H_

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
#include <QTimer>
#include <QString>
#include <QRegExp>
#include "Tree.h"
#include <QDataStream>
#include "Host.h"
#include <QTextBlock>

class TEvent;
using namespace std;



class TScript : public Tree<TScript>
{
public:
    
    
    virtual          ~TScript();
    TScript( TScript * parent, Host * pHost ); 
    TScript( QString name, Host * pHost ); 
    
    QString          getName()                       { QMutexLocker locker(& mLock); return mName; }
    void             setName( QString name )         { QMutexLocker locker(& mLock); mName = name; }
    void             compile();
    void             compileAll();
    void             execute();
    QString          getScript()                     { QMutexLocker locker(& mLock); return mScript; }
    void             setScript( QString & script )   { QMutexLocker locker(& mLock); mScript = script; mNeedsToBeCompiled=true; }
    bool             isActive()                      { QMutexLocker locker(& mLock); return mIsActive; }  
    bool             isFolder()                      { QMutexLocker locker(& mLock); return mIsFolder; }
    void             setIsActive( bool b )           { QMutexLocker locker(& mLock); mIsActive = b; }
    void             setIsFolder( bool b )           { QMutexLocker locker(& mLock); mIsFolder = b; }
    bool             registerScript();
    bool             serialize( QDataStream & );
    bool             restore( QDataStream & fs, bool );
    void             callEventHandler( QString &, TEvent * );
    void             setEventHandlerList( QStringList handlerList );
    QStringList      getEventHandlerList()            { QMutexLocker locker(& mLock); return mEventHandlerList; }
    
    
private:
    
    TScript(){};
    QString          mName;
    QString          mScript;
    bool             mIsActive;
    bool             mIsFolder;
    Host *           mpHost;
    bool             mNeedsToBeCompiled;
    QStringList      mEventHandlerList;
    
    QMutex           mLock;
};

#endif

