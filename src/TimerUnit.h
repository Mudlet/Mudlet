#ifndef MUDLET_TIMERUNIT_H
#define MUDLET_TIMERUNIT_H

/***************************************************************************
 *   Copyright (C) 2008-2013 by Heiko Koehn - KoehnHeiko@googlemail.com    *
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
class TTimer;


class TimerUnit
{
    friend class XMLexport;
    friend class XMLimport;

public:
                          TimerUnit( Host * pHost ) : statsActiveTriggers(0), statsTriggerTotal(0), statsTempTriggers(0), mpHost(pHost), mMaxID(0) {;}
    void                  removeAllTempTimers();
    std::list<TTimer *>   getTimerRootNodeList()   { return mTimerRootNodeList; }
    TTimer *              getTimer( int id );
    TTimer *              findTimer( QString & name );
    void                  compileAll();
    bool                  enableTimer( QString & );
    bool                  disableTimer( QString & );
    bool                  killTimer( QString & name );
    bool                  registerTimer( TTimer * pT );
    void                  unregisterTimer( TTimer * pT );
    void                  reParentTimer( int childID, int oldParentID, int newParentID, int parentPosition = -1, int childPosition = -1 );
    void                  stopAllTriggers();
    void                  reenableAllTriggers();
    void                  markCleanup( TTimer * );
    void                  doCleanup();
    QString               assembleReport();
    qint64                getNewID();
    void                  uninstall( QString );
    void                  _uninstall( TTimer * pChild, QString packageName );


    QMultiMap<QString, TTimer *> mLookupTable;
    QMutex                mTimerUnitLock;
    int                   statsActiveTriggers;
    int                   statsTriggerTotal;
    int                   statsTempTriggers;
    QList<TTimer*>        uninstallList;

private:
    TimerUnit(){;}
    void                  _assembleReport(TTimer *);
    TTimer *              getTimerPrivate( int id );
    void                  addTimerRootNode( TTimer * pT, int parentPosition = -1, int childPosition = -1 );
    void                  addTimer( TTimer * pT );
    void                  _removeTimerRootNode( TTimer * pT );
    void                  _removeTimer( TTimer *);
    Host *                mpHost;
    QMap<int, TTimer *>   mTimerMap;
    std::list<TTimer *>   mTimerRootNodeList;
    qint64                mMaxID;
    bool                  mModuleMember;
    std::list<TTimer *>   mCleanupList;


};

#endif // MUDLET_TIMERUNIT_H
