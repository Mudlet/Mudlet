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

#ifndef _TRIGGER_UNIT_H
#define _TRIGGER_UNIT_H

#include "TTrigger.h"
#include <list>
#include <map>
#include <QMutex>
#include <QDataStream>
#include <QTextBlock>

class TTrigger;
class Host;

class TriggerUnit
{
    friend class XMLexport;
    friend class XMLimport;

public:
                          TriggerUnit( Host * pHost ) : mpHost(pHost), mMaxID(0) { initStats();}
    std::list<TTrigger *> getTriggerRootNodeList()   { QMutexLocker locker(& mTriggerUnitLock); return mTriggerRootNodeList; }
    TTrigger *            getTrigger( int id );
    void                  removeAllTempTriggers();
    TTrigger *            findTrigger( QString & );
    bool                  enableTrigger( QString & );
    bool                  disableTrigger( QString & );
    bool                  killTrigger( QString & name );
    bool                  registerTrigger( TTrigger * pT );
    void                  unregisterTrigger( TTrigger * pT );
    bool                  serialize( QDataStream & );
    bool                  restore( QDataStream &, bool );
    void                  reParentTrigger( int childID, int oldParentID, int newParentID, int parentPosition = -1, int childPosition = -1 );
    void                  processDataStream( QString &, int );
    void                  dump();
    void                  compileAll();
    void                  setTriggerStayOpen( QString, int );
    void                  stopAllTriggers();
    void                  reenableAllTriggers();
    QString               assembleReport();
    std::list<TTrigger *> mCleanupList;
    qint64                getNewID();
    QMultiMap<QString, TTrigger *> mLookupTable;
    QMutex                mTriggerUnitLock;
    void                  markCleanup( TTrigger * pT );
    void                  doCleanup();
    void                  uninstall( QString );
    void                  _uninstall( TTrigger * pChild, QString packageName );

    int                   statsTriggerTotal;
    int                   statsTempTriggers;
    int                   statsPatterns;
    int                   statsActiveTriggers;
    int                   statsActiveTriggersMax;
    int                   statsActiveTriggersMin;
    int                   statsActiveTriggersAverage;
    int                   statsTempTriggersCreated;
    int                   statsTempTriggersKilled;
    int                   statsAverageLineProcessingTime;
    int                   statsMaxLineProcessingTime;
    int                   statsMinLineProcessingTime;
    int                   statsRegexTriggers;
    QList<TTrigger*>        uninstallList;

private:
                              TriggerUnit(){;}
    void                      initStats();
    void                      _assembleReport( TTrigger * );
    TTrigger *                getTriggerPrivate( int id );
    void                      addTriggerRootNode( TTrigger * pT, int parentPosition = -1, int childPosition = -1, bool moveTrigger = false );
    void                      addTrigger( TTrigger * pT );
    void                      removeTriggerRootNode( TTrigger * pT );
    void                      removeTrigger( TTrigger *);

    Host *                    mpHost;
    QMap<int, TTrigger *>     mTriggerMap;
    std::list<TTrigger *>     mTriggerRootNodeList;
    qint64                    mMaxID;
    bool                  mModuleMember;


};


#endif

