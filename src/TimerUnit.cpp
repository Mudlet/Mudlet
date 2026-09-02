/***************************************************************************
 *   Copyright (C) 2008-2013 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2014 by Ahmed Charles - acharles@outlook.com            *
 *   Copyright (C) 2019, 2022-2024, 2026 by Stephen Lyons                  *
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


#include "TimerUnit.h"


#include "Host.h"
#include "Tree.h"
#include "mudlet.h"
#include "TTimer.h"
#include "utils.h"

#include <QLatin1String>
#include <QMutableSetIterator>
#include <QSetIterator>
#include <QStringList>
#include <QTimer>
#include <QVariant>

#include <functional>
#include <utility>

/* We need an explicit constructor in this file as the Host class is forward
 * declared in the header file and it is problematic to define any dereferencing
 * of it there:*/
TimerUnit::TimerUnit(Host* pHost)
: mpHost(pHost)
{
}

TimerUnit::~TimerUnit()
{
    // Set mpHost to null on all timers (including children) to prevent them from trying to
    // unregister themselves during destruction (which would modify the list
    // we're iterating over and cause iterator invalidation)
    for (auto timer : mTimerRootNodeList) {
        timer->mpHost = nullptr;
        // Also set mpHost to null on all children recursively
        std::function<void(TTimer*)> nullifyChildren = [&nullifyChildren](TTimer* t) {
            for (auto* childNode : *t->mpMyChildrenList) {
                auto* child = static_cast<TTimer*>(childNode);
                child->mpHost = nullptr;
                nullifyChildren(child);
            }
        };
        nullifyChildren(timer);
    }
    // Delete all TTimer objects - each TTimer destructor will handle its own QTimer
    for (auto timer : mTimerRootNodeList) {
        delete timer;
    }
}

void TimerUnit::resetStats()
{
    statsItemsTotal = 0;
    statsTempItems = 0;
    statsActiveItems = 0;
}

void TimerUnit::_uninstall(TTimer* pChild, const QString& packageName)
{
    std::list<Tree<TTimer>*>* childrenList = pChild->mpMyChildrenList;
    for (auto* timerNode : *childrenList) {
        auto* timer = static_cast<TTimer*>(timerNode);
        _uninstall(timer, packageName);
        uninstallList.append(timer);
    }
}


void TimerUnit::uninstall(const QString& packageName)
{
    for (auto rootTimer : mTimerRootNodeList) {
        if (rootTimer->mPackageName == packageName) {
            _uninstall(rootTimer, packageName);
            uninstallList.append(rootTimer);
        }
    }
    // Re-entrant uninstall (#9337): a timer's own script (e.g. a package
    // auto-updater calling uninstallPackage()) is removing its package while
    // TTimer::execute() is still on the call stack for that timer. Deleting now
    // would be a use-after-free, so defer to doCleanup() at depth 0.
    if (mProcessingDepth > 0) {
        for (auto timer : uninstallList) {
            timer->setIsActive(false);
            mCleanupSet.remove(timer); // keep the two deferred-delete paths disjoint
        }
        return;
    }
    for (auto& timer : uninstallList) {
        // in case the timer was also queued for the markCleanup()/doCleanup()
        // path - deleting it here would otherwise leave a dangling pointer there:
        mCleanupSet.remove(timer);
        delete timer;
    }
    uninstallList.clear();
}

void TimerUnit::stopAllTriggers()
{
    for (auto timer : mTimerRootNodeList) {
        timer->disableTimer(timer->getID());
    }
}

void TimerUnit::compileAll()
{
    for (auto timer : mTimerRootNodeList) {
        if (timer->isActive()) {
            timer->compileAll();
        }
    }
}

void TimerUnit::reenableAllTriggers()
{
    for (auto timer : mTimerRootNodeList) {
        timer->enableTimer(timer->getID());
    }
}


void TimerUnit::addTimerRootNode(TTimer* pT, int parentPosition, int childPosition)
{
    if (!pT) {
        return;
    }
    if (!pT->getID()) {
        pT->setID(getNewID());
    }

    if ((parentPosition == -1) || (childPosition >= static_cast<int>(mTimerRootNodeList.size()))) {
        mTimerRootNodeList.push_back(pT);
    } else {
        // insert item at proper position
        int cnt = 0;
        for (auto it = mTimerRootNodeList.begin(); it != mTimerRootNodeList.end(); it++) {
            if (cnt >= childPosition) {
                mTimerRootNodeList.insert(it, pT);
                break;
            }
            cnt++;
        }
    }

    mTimerMap.insert(pT->getID(), pT);
    // no lookup table entry - see addTimer()
}

void TimerUnit::reParentTimer(int childID, int oldParentID, int newParentID, int parentPosition, int childPosition)
{
    TTimer* pOldParent = getTimerPrivate(oldParentID);
    TTimer* pNewParent = getTimerPrivate(newParentID);
    TTimer* pChild = getTimerPrivate(childID);
    if (!pChild) {
        return;
    }

    pChild->disableTimer(childID);

    if (pOldParent) {
        pOldParent->popChild(pChild);
    }
    if (!pOldParent) {
        mTimerRootNodeList.remove(pChild);
    }
    if (pNewParent) {
        pNewParent->addChild(pChild, parentPosition, childPosition);
        pChild->setParent(pNewParent);
    } else {
        pChild->Tree<TTimer>::setParent(nullptr);
        addTimerRootNode(pChild, parentPosition, childPosition);
    }

    pChild->enableTimer(childID);
}

void TimerUnit::reParentTimer(int childID, int oldParentID, int newParentID, TreeItemInsertMode mode, int position)
{
    if (mode == TreeItemInsertMode::Append) {
        reParentTimer(childID, oldParentID, newParentID, -1, -1);
    } else {
        // AtPosition mode - use 0 for parentPosition to enable position-based insertion
        reParentTimer(childID, oldParentID, newParentID, 0, position);
    }
}

void TimerUnit::removeAllTempTimers()
{
    mCleanupSet.clear();
    for (auto timer : mTimerRootNodeList) {
        if (timer->isTemporary()) {
            timer->killTimer();
            timer->mOK_code = false; //important to not crash on stale Lua function args
            markCleanup(timer);
        }
    }
}

void TimerUnit::_removeTimerRootNode(TTimer* pT)
{
    if (!pT) {
        return;
    }
    // Names are not unique - the lookup table is a QMultiMap - so drop this one
    // timer's entry rather than every entry filed under the name. The
    // single-argument remove() used to be taken for temporary timers on the
    // grounds that their name is their id, but a permanent timer named after
    // that id was evicted with it and left unreachable by name for the rest of
    // the session
    mLookupTable.remove(pT->getName(), pT);
    mTimerMap.remove(pT->getID());
    mTimerRootNodeList.remove(pT);
}

TTimer* TimerUnit::getTimer(int id)
{
    return mTimerMap.value(id);
}

TTimer* TimerUnit::getTimerPrivate(int id)
{
    return mTimerMap.value(id);
}

bool TimerUnit::registerTimer(TTimer* pT)
{
    if (!pT) {
        return false;
    }

    if (pT->getParent()) {
        // This allocates the ID number
        addTimer(pT);
    } else {
        // This allocates the ID number
        addTimerRootNode(pT);
    }

    pT->setIsActive(false);
    if (pT->isTemporary()) {
        // Insert the ID number as the name:
        pT->setName(QString::number(pT->mID));
    }

    // This has some side effects, including stopping the timer...
    pT->setTime(pT->getTime());
    QTimer::connect(pT->getQTimer(), &QTimer::timeout, mudlet::self(), &mudlet::slot_timerFires, Qt::UniqueConnection);
    return true;
}

void TimerUnit::unregisterTimer(TTimer* pT)
{
    if (!pT) {
        return;
    }
    // Stop the QTimer ASAP:
    pT->stop();
    pT->deactivate();
    QTimer::disconnect(pT->getQTimer(), &QTimer::timeout, mudlet::self(), &mudlet::slot_timerFires);
    if (pT->getParent()) {
        _removeTimer(pT);
        return;
    }

    _removeTimerRootNode(pT);
}


void TimerUnit::addTimer(TTimer* pT)
{
    if (!pT) {
        return;
    }

    if (!pT->getID()) {
        pT->setID(getNewID());
    }

    mTimerMap.insert(pT->getID(), pT);
    // DE: in den lookup table wird der timer erst dann eingetragen, wenn er auch einen namen hat -> setName()
    // EN: in the lookup table, the timer is not entered until it has a name -> setName ()
}

void TimerUnit::_removeTimer(TTimer* pT)
{
    if (!pT) {
        return;
    }

    // see _removeTimerRootNode(): one entry, not every same-named one
    mLookupTable.remove(pT->getName(), pT);
    mTimerMap.remove(pT->getID());
}


bool TimerUnit::enableTimer(const QString& name)
{
    bool found = false;
    // equal_range visits every same-named timer; constFind() + (++it) can start
    // mid-run and skip duplicates on some QMultiMap implementations
    const auto [begin, end] = mLookupTable.equal_range(name);
    for (auto it = begin; it != end; ++it) {
        TTimer* pT = it.value();
        // A timer queued for deletion stays in the lookup table until
        // doCleanup() frees it - re-activating one restarts the QTimer that
        // killTimer() stopped, that a spent one-shot stopped itself (see
        // TTimer::execute(), which markCleanup()s without deactivating, so that
        // corpse is still isActive()), or that an uninstall is waiting to free.
        if (mCleanupSet.contains(pT) || uninstallList.contains(pT)) {
            continue;
        }

        if (!pT->isOffsetTimer()) {
            pT->setIsActive(true);
        } else {
            pT->setShouldBeActive(true);
        }


        if (pT->isFolder()) {
            // disable or enable all timers in the respective branch
            // irrespective of the user defined state - and without re-checking
            // the skip above. That is only safe while no child timer is ever
            // queued for deletion under a live parent: only temporary root
            // timers are ever queued (doCleanup() relies on the same thing) and
            // _uninstall() queues whole subtrees.
            if (pT->shouldBeActive()) {
                pT->enableTimer();
            } else {
                pT->disableTimer();
            }
        } else {
            if (pT->isOffsetTimer()) {
                // state of offset timers is managed by the trigger engine
                if (pT->shouldBeActive()) {
                    pT->enableTimer();
                } else {
                    pT->disableTimer();
                }
            }
        }

        found = true;
    }
    return found;
}

bool TimerUnit::disableTimer(const QString& name)
{
    bool found = false;
    // equal_range visits every same-named timer; constFind() + (++it) can start
    // mid-run and skip duplicates on some QMultiMap implementations
    const auto [begin, end] = mLookupTable.equal_range(name);
    for (auto it = begin; it != end; ++it) {
        TTimer* pT = it.value();
        if (pT->isOffsetTimer()) {
            pT->setShouldBeActive(false);
        } else {
            pT->setIsActive(false);
        }

        pT->disableTimer();
        found = true;
    }
    return found;
}

// This is currently only used during the lua scripted creation of a new
// permTime to find a parent with the given name:
TTimer* TimerUnit::findFirstTimer(const QString& name) const
{
    return mLookupTable.value(name);
}

std::vector<int> TimerUnit::findItems(const QString& name, const bool exactMatch, const bool caseSensitive)
{
    std::vector<int> ids;
    const auto searchCaseSensitivity = caseSensitive ? Qt::CaseSensitive : Qt::CaseInsensitive;
    if (exactMatch) {
        for (auto& item : std::as_const(mTimerMap)) {
            if (!item->getName().compare(name, searchCaseSensitivity)) {
                ids.push_back(item->getID());
            }
        }
    } else {
        for (auto& item : std::as_const(mTimerMap)) {
            if (item->getName().contains(name, searchCaseSensitivity)) {
                ids.push_back(item->getID());
            }
        }
    }
    return ids;
}

bool TimerUnit::killTimer(const QString& name)
{
    for (auto timer : mTimerRootNodeList) {
        if (timer->getName() != name) {
            continue;
        }
        // Names are not unique, so keep looking rather than give up on the first
        // same-named timer that cannot be killed - a permanent timer loaded from
        // the profile precedes this session's temporaries in this list, and
        // reporting a failure over it would strand a killable timer
        if (!timer->isTemporary()) {
            // only temporary timers can be killed
            continue;
        }
        // An already killed timer is only unlinked from this list once doCleanup()
        // gets to free it, which cannot happen while a timer script is on the call
        // stack - so until then it is still findable by name. Killing it a second
        // time achieves nothing:
        if (mCleanupSet.contains(timer)) {
            continue;
        }
        timer->killTimer();
        markCleanup(timer);
        return true;
    }
    return false;
}

int TimerUnit::remainingTime(const QString& name) const
{
    auto pTimer = findFirstTimer(name);
    if (pTimer) {
        return pTimer->remainingTime();
    }

    return -2;
}

int TimerUnit::remainingTime(const int id) const
{
    auto timer = mTimerMap.value(id);
    if (timer) {
        return timer->remainingTime();
    }

    return -2;
}

int TimerUnit::getNewID()
{
    return ++mMaxID;
}

void TimerUnit::doCleanup()
{
    if (mProcessingDepth > 0) {
        return;
    }

    QSet<TTimer*> deletedTimers;
    QMutableSetIterator<TTimer*> itTimer(mCleanupSet);
    while (itTimer.hasNext()) {
        auto pTimer = itTimer.next();
        // It is important to take the item OUT of the set before you delete
        // (and thus invalidate this pointer to) it...!
        itTimer.remove();
        deletedTimers.insert(pTimer);
        delete pTimer;
    }
    // Flush the deletes uninstall() deferred (#9337). uninstallList is ordered
    // children-before-parents and each ~Tree unlinks from its parent, so deleting
    // children first empties the parent's child list (no double free); the seen
    // set guards a node queued twice by re-entrant uninstalls and is shared with
    // the mCleanupSet loop above so an object that ended up in both containers is
    // freed once. It matches on pointer identity only: a node freed indirectly, as
    // a child of a queued parent, is not in the set (not reachable today - only
    // temporary root nodes are ever queued, and those have no children).
    for (auto timer : uninstallList) {
        if (!deletedTimers.contains(timer)) {
            deletedTimers.insert(timer);
            delete timer;
        }
    }
    uninstallList.clear();
}

void TimerUnit::markCleanup(TTimer* pT)
{
    mCleanupSet.insert(pT);
}

void TimerUnit::assembleReport(TTimer* pItem)
{
    std::list<Tree<TTimer>*>* childrenList = pItem->mpMyChildrenList;
    for (auto* pChildNode : *childrenList) {
        auto* pChild = static_cast<TTimer*>(pChildNode);
        ++statsItemsTotal;
        if (pChild->isOffsetTimer() ? pChild->shouldBeActive() : pChild->isActive()) {
            ++statsActiveItems;
        }
        if (pChild->isTemporary()) {
            ++statsTempItems;
        }
        assembleReport(pChild);
    }
}

std::tuple<QString, int, int, int> TimerUnit::assembleReport()
{
    resetStats();
    for (auto pItem : mTimerRootNodeList) {
        ++statsItemsTotal;
        if (pItem->isOffsetTimer() ? pItem->shouldBeActive() : pItem->isActive()) {
            ++statsActiveItems;
        }
        if (pItem->isTemporary()) {
            ++statsTempItems;
        }
        assembleReport(pItem);
    }
    QStringList msg;
    msg << QLatin1String("Timers current total: ") << QString::number(statsItemsTotal) << QLatin1String("\n") << QLatin1String("tempTimers current total: ") << QString::number(statsTempItems)
        << QLatin1String("\n") << QLatin1String("active Timers: ") << QString::number(statsActiveItems) << QLatin1String("\n");

    return {msg.join(QString()), statsItemsTotal, statsTempItems, statsActiveItems};
}

void TimerUnit::changeHostName(const QString& newName)
{
    QSetIterator<QTimer*> itQTimerPtr(mQTimerSet);
    while (itQTimerPtr.hasNext()) {
        itQTimerPtr.next()->setProperty(TTimer::scmProperty_HostName, newName);
    }
}
