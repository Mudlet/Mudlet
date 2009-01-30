#ifndef _TKEY_H_
#define _TKEY_H_

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

using namespace std;



class TKey : public Tree<TKey>
{
public:
    
    
    virtual          ~TKey();
    TKey( TKey * parent, Host * pHost ); 
    TKey( QString name, Host * pHost ); 
    TKey& clone(const TKey& );
    
    QString          getName()                       { QMutexLocker locker(& mLock); return mName; }
    void             setName( QString name )         { QMutexLocker locker(& mLock); mName = name; }
    int              getKeyCode()                    { QMutexLocker locker(& mLock); return mKeyCode; }
    void             setKeyCode( int code )          { QMutexLocker locker(& mLock); mKeyCode = code; }
    int              getKeyModifiers()                  { QMutexLocker locker(& mLock); return mKeyModifier; }
    void             setKeyModifiers( int code )        { QMutexLocker locker(& mLock); mKeyModifier = code; }
    void             enableKey( QString & name );
    void             disableKey( QString & name );
    void             compile();
    void             execute(QStringList &);
    QString          getScript()                     { QMutexLocker locker(& mLock); return mScript; }
    void             setScript( QString & script )   { QMutexLocker locker(& mLock); mScript = script; mNeedsToBeCompiled=true; }
    QString          getRegexCode()                  { QMutexLocker locker(& mLock); return mRegexCode; }
    void             setRegexCode( QString regex )   { QMutexLocker locker(& mLock); mRegexCode = regex; mRegex = QRegExp( regex ); mRegex.setMinimal( false ); }
    void             setCommand( QString command )   { QMutexLocker locker(& mLock); mCommand = command; }
    QString          getCommand()                    { QMutexLocker locker(& mLock); return mCommand; }
    bool             isActive()                      { QMutexLocker locker(& mLock); return mIsActive; }  
    bool             isFolder()                      { QMutexLocker locker(& mLock); return mIsFolder; }
    void             setIsActive( bool b )           { QMutexLocker locker(& mLock); mIsActive = b; }
    void             setIsFolder( bool b )           { QMutexLocker locker(& mLock); mIsFolder = b; }
    bool             match( int, int );
    
    
    bool             registerKey();
    bool             serialize( QDataStream & );
    bool             restore( QDataStream & fs, bool );
    bool             isClone(TKey &b) const;
    
private:
    
    TKey(){};
    QString          mName;
    QString          mCommand;
    
    /*Qt::NoModifier 0x00000000 No modifier key is pressed.
      Qt::ShiftModifier 0x02000000 A Shift key on the keyboard is pressed.
      Qt::ControlModifier 0x04000000 A Ctrl key on the keyboard is pressed.
      Qt::AltModifier 0x08000000 An Alt key on the keyboard is pressed.
      Qt::MetaModifier 0x10000000 A Meta key on the keyboard is pressed.
      Qt::KeypadModifier 0x20000000 A keypad button is pressed. */
    
    int              mKeyCode;
    int              mKeyModifier;
    
    QString          mRegexCode;
    QRegExp          mRegex;
    QString          mScript;
    bool             mIsActive;
    bool             mIsFolder;
    Host *           mpHost;
    bool             mNeedsToBeCompiled;
    
    QMutex           mLock;
};

#endif

