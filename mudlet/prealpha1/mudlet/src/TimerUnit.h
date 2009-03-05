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

#ifndef _TIMER_UNIT_H
#define _TIMER_UNIT_H

#include "TTimer.h"
#include <list>
#include <map>
#include <QMutex>
#include <QDataStream>
#include <QTimer>
#include <QTime>

class TTimer;
class Host;

class TimerUnit
{
    friend class XMLexport;
    friend class XMLimport;
    
public:
    TimerUnit( Host * pHost ) : mpHost(pHost), mMaxID(0) {;}
    std::list<TTimer *>   getTimerRootNodeList()   { QMutexLocker locker(& mTimerUnitLock); return mTimerRootNodeList; }
    TTimer *              getTimer( int id );
    void                  enableTimer( QString & );
    void                  disableTimer( QString & );
    void                  killTimer( QString & name );
    bool                  registerTimer( TTimer * pT );
    void                  unregisterTimer( TTimer * pT );
    bool                  serialize( QDataStream & );
    bool                  restore( QDataStream &, bool );
    void                  reParentTimer( int childID, int oldParentID, int newParentID );
    void                  stopAllTriggers();
    
    qint64                getNewID();
    QMutex                mTimerUnitLock;
    
private: 
    TimerUnit(){;}
    TTimer *              getTimerPrivate( int id );
    void                  addTimerRootNode( TTimer * pT );
    void                  addTimer( TTimer * pT );
    void                  removeTimerRootNode( TTimer * pT );
    void                  removeTimer( TTimer *);
    Host *                mpHost;
    QMap<int, TTimer *>   mTimerMap;
    std::list<TTimer *>   mTimerRootNodeList;
    qint64                mMaxID;
  
    
};


#endif

