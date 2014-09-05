#ifndef MUDLET_TSCRIPT_H
#define MUDLET_TSCRIPT_H

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

#include "pre_guard.h"
#include <QStringList>
#include "post_guard.h"

class Host;
class TEvent;


class TScript : public Tree<TScript>
{
    friend class XMLexport;
    friend class XMLimport;

public:


    virtual          ~TScript();
                     TScript( TScript * parent, Host * pHost );
                     TScript( QString name, Host * pHost );

    QString          getName()                                         { return mName; }
    void             setName( QString name )                           { mName = name; }
    void             compile();
    void             compileAll();
    bool             compileScript();
    void             execute();
    QString          getScript()                                       { return mScript; }
    bool             setScript( QString & script );
    bool             isFolder()                                        { return mIsFolder; }
    void             setIsFolder( bool b )                             { mIsFolder = b; }
    bool             registerScript();
    //bool             serialize( QDataStream & );
    //bool             restore( QDataStream & fs, bool );
    void             callEventHandler( const TEvent * );
    void             setEventHandlerList( QStringList handlerList );
    QStringList      getEventHandlerList()                             { return mEventHandlerList; }
    bool             exportItem;
    bool            mModuleMasterFolder;
private:

                     TScript(){};
    QString          mName;
    QString          mScript;
    QString          mFuncName;
    bool             mIsFolder;
    Host *           mpHost;
    bool             mNeedsToBeCompiled;
    QStringList      mEventHandlerList;
    bool                  mModuleMember;

};

#endif // MUDLET_TSCRIPT_H
