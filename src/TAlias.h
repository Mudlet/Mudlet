#ifndef MUDLET_TALIAS_H
#define MUDLET_TALIAS_H

/***************************************************************************
 *   Copyright (C) 2008-2012 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2014 by Ahmed Charles - acharles@outlook.com            *
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


#include "Tree.h"

#include <pcre.h>

class Host;


class TAlias : public Tree<TAlias>
{
    friend class XMLexport;
    friend class XMLimport;

public:


    virtual          ~TAlias();
                     TAlias( TAlias * parent, Host * pHost );
                     TAlias( QString name, Host * pHost );
    void             compileAll();
    QString          getName()                       { return mName; }
    void             setName( QString name );
    void             compile();
    bool             compileScript();
    void             execute();
    QString          getScript()                     { return mScript; }
    bool             setScript( QString & script );
    QString          getRegexCode()                  { return mRegexCode; }
    void             setRegexCode( QString );
    void             setCommand( QString command )   { mCommand = command; }
    QString          getCommand()                    { return mCommand; }
    bool             isFolder()                      { return mIsFolder; }
    void             setIsFolder( bool b )           { mIsFolder = b; }
    bool             match( QString & toMatch );
    bool             registerAlias();
    bool             isTempAlias()                   { return mIsTempAlias; }
    void             setIsTempAlias( bool b )        { mIsTempAlias = b; }



                     TAlias(){};
    QString          mName;
    QString          mCommand;
    QString          mRegexCode;
    pcre *           mpRegex;
    QString          mScript;
    bool             mIsFolder;
    Host *           mpHost;
    bool             mNeedsToBeCompiled;
    bool             mIsTempAlias;
    bool                  mModuleMember;
    bool            mModuleMasterFolder;
    QString          mFuncName;
    bool             exportItem;
};

#endif // MUDLET_TALIAS_H
