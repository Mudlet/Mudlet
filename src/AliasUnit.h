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

#ifndef _ALIAS_UNIT_H
#define _ALIAS_UNIT_H

#include "TAlias.h"
#include <list>
#include <map>
#include <QMutex>
#include <QDataStream>

class TAlias;
class Host;

class AliasUnit
{
    friend class XMLexport;
    friend class XMLimport;
    
public:
    AliasUnit( Host * pHost ) : mpHost(pHost), mMaxID(0) {;}
    std::list<TAlias *>   getAliasRootNodeList()   { QMutexLocker locker(& mAliasUnitLock); return mAliasRootNodeList; }
    TAlias *              getAlias( int id );
    bool                  registerAlias( TAlias * pT );
    void                  unregisterAlias( TAlias * pT );
    bool                  serialize( QDataStream & );
    bool                  restore( QDataStream &, bool );
    void                  reParentAlias( int childID, int oldParentID, int newParentID );
    qint64                getNewID();
    bool                  processDataStream( QString & );
    void                  stopAllTriggers();
    
    QMutex                mAliasUnitLock;
    
private: 
    AliasUnit(){;}
    TAlias *              getAliasPrivate( int id );
    void                  addAliasRootNode( TAlias * pT );
    void                  addAlias( TAlias * pT );
    void                  removeAliasRootNode( TAlias * pT );
    void                  removeAlias( TAlias *);
    Host *                mpHost;
    QMap<int, TAlias *>   mAliasMap;
    std::list<TAlias *>   mAliasRootNodeList;
    qint64                mMaxID;
    
    
};


#endif

