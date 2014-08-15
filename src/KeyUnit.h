#ifndef MUDLET_KEYUNIT_H
#define MUDLET_KEYUNIT_H

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
class TKey;


class KeyUnit
{
    friend class XMLexport;
    friend class XMLimport;

public:

                          KeyUnit( Host * pHost );
    std::list<TKey *>     getKeyRootNodeList()   { QMutexLocker locker(& mKeyUnitLock); return mKeyRootNodeList; }
    TKey *                getKey( int id );
    void                  compileAll();
    bool                  enableKey( QString & name );
    bool                  disableKey( QString & name );
    bool                  registerKey( TKey * pT );
    void                  unregisterKey( TKey * pT );
    void                  reParentKey( int childID, int oldParentID, int newParentID, int parentPosition = -1, int childPosition = -1 );
    qint64                getNewID();
    QString               getKeyName( int keyCode, int modifier );
    void                  setupKeyNames();
    void                  uninstall( QString );
    void                  _uninstall( TKey * pChild, QString packageName );
    bool                  processDataStream( int, int );
    QMutex                mKeyUnitLock;
    QList<TKey*>        uninstallList;

private:
    KeyUnit(){;}
    TKey *                getKeyPrivate( int id );
    void                  addKeyRootNode( TKey * pT, int parentPosition = -1, int childPosition = -1 );
    void                  addKey( TKey * pT );
    void                  removeKeyRootNode( TKey * pT );
    void                  removeKey( TKey *);
    Host *                mpHost;
    QMap<int, TKey *>     mKeyMap;
    std::list<TKey *>     mKeyRootNodeList;
    qint64                mMaxID;
    bool                  mModuleMember;
    QMap<int, QString>    mKeys;

};

#endif // MUDLET_KEYUNIT_H
