#ifndef MUDLET_TIMERUNIT_H
#define MUDLET_TIMERUNIT_H

/***************************************************************************
 *   Copyright (C) 2008-2013 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2014 by Ahmed Charles - acharles@outlook.com            *
 *   Copyright (C) 2019, 2022-2023, 2026 by Stephen Lyons                  *
 *                                               - slysven@virginmedia.com *
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


#include "utils.h"

#include <QList>
#include <QMap>
#include <QMultiMap>
#include <QPointer>
#include <QSet>
#include <QString>
#include <QtGlobal>

#include <list>
#include <tuple>
#include <vector>

class Host;
class TTimer;
class QTimer;

// Note: mProcessingDepth tracks TTimer::execute() nesting (via begin/endProcessing())
// so that uninstall() can defer deletion of a package's timers while one of them is
// still on the call stack - a timer script calling uninstallPackage() on its own
// package would otherwise free the very TTimer execute() is running on. Deferred
// items are flushed by doCleanup() once no timer script is executing - primarily
// by mudlet::slot_timerFires() right after it finishes with the fired timer, so
// the "uninstalled" objects do not outlive the event loop iteration. This
// complements the guard in TTimer::execute() and the re-verification of timer
// existence after execute() in mudlet::slot_timerFires().
class TimerUnit
{
    friend class XMLexport;
    friend class XMLimport;

public:
    explicit TimerUnit(Host*);
    ~TimerUnit();

    void resetStats();
    void removeAllTempTimers();
    std::list<TTimer*> getTimerRootNodeList() { return mTimerRootNodeList; }
    TTimer* getTimer(int id);
    TTimer* findFirstTimer(const QString&) const;
    std::vector<int> findItems(const QString& name, const bool exactMatch = true, const bool caseSensitive = true);
    void compileAll();
    bool enableTimer(const QString&);
    bool disableTimer(const QString&);
    bool killTimer(const QString& name);
    int remainingTime(const QString& name) const;
    int remainingTime(const int id) const;
    bool registerTimer(TTimer* pT);
    void unregisterTimer(TTimer* pT);
    void reParentTimer(int childID, int oldParentID, int newParentID, int parentPosition = -1, int childPosition = -1);
    void reParentTimer(int childID, int oldParentID, int newParentID, TreeItemInsertMode mode, int position = 0);
    void stopAllTriggers();
    void reenableAllTriggers();
    void markCleanup(TTimer*);
    void doCleanup();
    void beginProcessing() { ++mProcessingDepth; }
    void endProcessing()
    {
        --mProcessingDepth;
        Q_ASSERT(mProcessingDepth >= 0);
    }
    std::tuple<QString, int, int, int> assembleReport();
    int getNewID();
    void uninstall(const QString&);
    void _uninstall(TTimer* pChild, const QString& packageName);
    void changeHostName(const QString&);


    QMultiMap<QString, TTimer*> mLookupTable;
    QList<TTimer*> uninstallList;
    QSet<TTimer*> mCleanupSet;

    // This will contain all the QTimers associated with the TTimer instances
    // it is needed so that should mpHost be renamed we can update them to have
    // the correct name (which is needed when they fire so the mudlet class
    // knows when profile they belong to and where to find the TTimer that they
    // are part of):
    QSet<QTimer*> mQTimerSet;

private:
    TimerUnit() = default;

    void assembleReport(TTimer*);
    TTimer* getTimerPrivate(int id);
    void addTimerRootNode(TTimer* pT, int parentPosition = -1, int childPosition = -1);
    void addTimer(TTimer* pT);
    void _removeTimerRootNode(TTimer* pT);
    void _removeTimer(TTimer*);


    QPointer<Host> mpHost;
    QMap<int, TTimer*> mTimerMap;
    std::list<TTimer*> mTimerRootNodeList;
    int mMaxID = 0;
    bool mModuleMember = false;
    // > 0 whilst a TTimer::execute() is on the call stack; uninstall() and
    // doCleanup() must not delete timers then - see the note above the class:
    int mProcessingDepth = 0;
    int statsActiveItems = 0;
    int statsItemsTotal = 0;
    int statsTempItems = 0;
};

#endif // MUDLET_TIMERUNIT_H
