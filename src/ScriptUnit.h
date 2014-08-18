#ifndef MUDLET_SCRIPTUNIT_H
#define MUDLET_SCRIPTUNIT_H

/***************************************************************************
 *   Copyright (C) 2008-2011 by Heiko Koehn - KoehnHeiko@googlemail.com    *
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


#include "pre_guard.h"
#include <QMap>
#include <QMutex>
#include <QString>
#include "post_guard.h"

#include <list>

class Host;
class TScript;


class ScriptUnit
{
    friend class XMLexport;
    friend class XMLimport;

public:

    ScriptUnit( Host * pHost ) : mpHost(pHost), mMaxID(0) {;}
    std::list<TScript *>  getScriptRootNodeList()   { QMutexLocker locker(& mScriptUnitLock); return mScriptRootNodeList; }
    TScript *             getScript( int id );
    void                  compileAll();
    bool                  registerScript( TScript * pT );
    void                  unregisterScript( TScript * pT );
    void                  reParentScript( int childID, int oldParentID, int newParentID, int parentPosition = -1, int childPosition = -1 );
    void                  stopAllTriggers();
    void                  uninstall( QString );
    void                  _uninstall( TScript * pChild, QString packageName );
    qint64                getNewID();
    QMutex                mScriptUnitLock;
    QList<TScript*>        uninstallList;

private:

    ScriptUnit(){;}
    TScript *             getScriptPrivate( int id );
    void                  addScriptRootNode( TScript * pT, int parentPosition = -1, int childPosition = -1 );
    void                  addScript( TScript * pT );
    void                  removeScriptRootNode( TScript * pT );
    void                  removeScript( TScript *);
    Host *                mpHost;
    QMap<int, TScript *>  mScriptMap;
    std::list<TScript *>  mScriptRootNodeList;
    qint64                mMaxID;


};

#endif // MUDLET_SCRIPTUNIT_H
