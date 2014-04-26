#ifndef _TKEY_H_
#define _TKEY_H_

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

class TLuaInterpreter;

class TKey : public Tree<TKey>
{
    friend class XMLexport;
    friend class XMLimport;

public:


    virtual          ~TKey();
                     TKey( TKey * parent, Host * pHost );
                     TKey( QString name, Host * pHost );
    void             compileAll();
    QString          getName()                          { return mName; }
    void             setName( QString name )            { mName = name; }
    int              getKeyCode()                       { return mKeyCode; }
    void             setKeyCode( int code )             { mKeyCode = code; }
    int              getKeyModifiers()                  { return mKeyModifier; }
    void             setKeyModifiers( int code )        { mKeyModifier = code; }
    void             enableKey( QString & name );
    void             disableKey( QString & name );
    void             compile();
    bool             compileScript();
    void             execute();
    QString          getScript()                        { return mScript; }
    bool             setScript( QString & script );
    void             setCommand( QString command )      { mCommand = command; }
    QString          getCommand()                       { return mCommand; }
    bool             isFolder()                         { return mIsFolder; }
    void             setIsFolder( bool b )              { mIsFolder = b; }
    bool             match( int, int );
    bool             registerKey();
    //bool             serialize( QDataStream & );
    //bool             restore( QDataStream & fs, bool );
    bool             exportItem;
    bool            mModuleMasterFolder;
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
    QString          mFuncName;
    bool             mIsFolder;
    Host *           mpHost;
    bool             mNeedsToBeCompiled;
    bool                  mModuleMember;

};

#endif

