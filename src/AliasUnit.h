#ifndef MUDLET_ALIASUNIT_H
#define MUDLET_ALIASUNIT_H

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
#include <QMutex>
#include <QMultiMap>
#include <QString>
#include "post_guard.h"

#include <list>

class Host;
class TAlias;


class AliasUnit
{
    friend class XMLexport;
    friend class XMLimport;

public:
                                    AliasUnit( Host * pHost ) : mpHost(pHost), mMaxID(0) { initStats();}
    std::list<TAlias *>             getAliasRootNodeList()   { return mAliasRootNodeList; }
    TAlias *                        getAlias( int id );
    void                            compileAll();
    TAlias *                        findAlias( QString & name );
    bool                            enableAlias( QString & );
    bool                            disableAlias( QString & );
    bool                            killAlias( QString & name );
    bool                            registerAlias( TAlias * pT );
    void                            unregisterAlias( TAlias * pT );
    void                            uninstall( QString );
    void                            _uninstall( TAlias * pChild, QString packageName );
    void                            reParentAlias( int childID, int oldParentID, int newParentID, int parentPosition = -1, int childPosition = -1 );
    bool                            processDataStream( QString & );
    void                            setAliasStayOpen( QString, int );
    void                            stopAllTriggers();
    void                            reenableAllTriggers();
    QString                         assembleReport();
    std::list<TAlias *>             mCleanupList;
    qint64                          getNewID();
    QMultiMap<QString, TAlias *>    mLookupTable;
    QMutex                          mAliasUnitLock;
    void                            markCleanup( TAlias * pT );
    void                            doCleanup();

    int                             statsAliasTotal;
    int                             statsTempAliass;
    int                             statsActiveAliass;
    int                             statsActiveAliassMax;
    int                             statsActiveAliassMin;
    int                             statsActiveAliassAverage;
    int                             statsTempAliassCreated;
    int                             statsTempAliassKilled;
    int                             statsAverageLineProcessingTime;
    int                             statsMaxLineProcessingTime;
    int                             statsMinLineProcessingTime;
    int                             statsRegexAliass;
    QList<TAlias*>                  uninstallList;


private:
                                    AliasUnit(){;}
    void                            initStats();
    void                            _assembleReport( TAlias * );
    TAlias *                        getAliasPrivate( int id );
    void                            addAliasRootNode( TAlias * pT, int parentPosition = -1, int childPosition = -1, bool moveAlias = false );
    void                            addAlias( TAlias * pT );
    void                            removeAliasRootNode( TAlias * pT );
    void                            removeAlias( TAlias *);

    Host *                          mpHost;
    QMap<int, TAlias *>             mAliasMap;
    std::list<TAlias *>             mAliasRootNodeList;
    qint64                          mMaxID;
    bool                  mModuleMember;
};

#endif // MUDLET_ALIASUNIT_H
