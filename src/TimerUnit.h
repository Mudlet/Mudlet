#ifndef MUDLET_TIMERUNIT_H
#define MUDLET_TIMERUNIT_H

/***************************************************************************
 *   Copyright (C) 2008-2013 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2014 by Ahmed Charles - acharles@outlook.com            *
 *   Copyright (C) 2019 by Stephen Lyons - slysven@virginmedia.com         *
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
#include <QMultiMap>
#include <QMutex>
#include <QPointer>
#include <QSet>
#include <QString>
#include "post_guard.h"

#include <list>

class Host;
class TTimer;
class QTimer;

class TimerUnit
{
    friend class XMLexport;
    friend class XMLimport;

public:
    TimerUnit(Host* pHost) : statsActiveTriggers(0), statsTriggerTotal(0), statsTempTriggers(0), mpHost(pHost), mMaxID(0), mModuleMember() {}
    void removeAllTempTimers();
    std::list<TTimer*> getTimerRootNodeList() { return mTimerRootNodeList; }
    TTimer* getTimer(int id);
    TTimer* findFirstTimer(const QString&) const;
    QList<TTimer*> findTimers(const QString&);
    void compileAll();
    bool enableTimer(const QString&);
    bool disableTimer(const QString&);
    bool killTimer(const QString& name);
    int remainingTime(const QString& name) const;
    int remainingTime(const int id) const;
    bool registerTimer(TTimer* pT);
    void unregisterTimer(TTimer* pT);
    void reParentTimer(int childID, int oldParentID, int newParentID, int parentPosition = -1, int childPosition = -1);
    void stopAllTriggers();
    void reenableAllTriggers();
    void markCleanup(TTimer*);
    void doCleanup();
    QString assembleReport();
    int getNewID();
    void uninstall(const QString&);
    void _uninstall(TTimer* pChild, const QString& packageName);
    void changeHostName(const QString&);


    QMultiMap<QString, TTimer*> mLookupTable;
    int statsActiveTriggers;
    int statsTriggerTotal;
    int statsTempTriggers;
    QList<TTimer*> uninstallList;

    // This will contain all the QTimers associated with the TTimer instances
    // it is needed so that should mpHost be renamed we can update them to have
    // the correct name (which is needed when they fire so the mudlet class
    // knows when profile they belong to and where to find the TTimer that they
    // are part of):
    QSet<QTimer*> mQTimerSet;

private:
    TimerUnit() = default;

    void _assembleReport(TTimer*);
    TTimer* getTimerPrivate(int id);
    void addTimerRootNode(TTimer* pT, int parentPosition = -1, int childPosition = -1);
    void addTimer(TTimer* pT);
    void _removeTimerRootNode(TTimer* pT);
    void _removeTimer(TTimer*);
    QPointer<Host> mpHost;
    QMap<int, TTimer*> mTimerMap;
    std::list<TTimer*> mTimerRootNodeList;
    int mMaxID;
    bool mModuleMember;
    QSet<TTimer*> mCleanupSet;
};

#endif // MUDLET_TIMERUNIT_H
