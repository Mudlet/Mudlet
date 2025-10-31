/***************************************************************************
 *   Copyright (C) 2008-2013 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2014 by Ahmed Charles - acharles@outlook.com            *
 *   Copyright (C) 2019, 2022-2024 by Stephen Lyons                        *
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


#include "mudlet.h"
#include "TTimer.h"

#include <functional>

TimerUnit::~TimerUnit()
{
    // Set mpHost to null on all timers (including children) to prevent them from trying to
    // unregister themselves during destruction (which would modify the list
    // we're iterating over and cause iterator invalidation)
    for (auto timer : mTimerRootNodeList) {
        timer->mpHost = nullptr;
        // Also set mpHost to null on all children recursively
        std::function<void(TTimer*)> nullifyChildren = [&nullifyChildren](TTimer* t) {
            for (auto child : *t->mpMyChildrenList) {
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
    std::list<TTimer*>* childrenList = pChild->mpMyChildrenList;
    for (auto timer : *childrenList) {
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
    for (auto& timer : uninstallList) {
        unregisterTimer(timer);
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
    // temp timers do not need to check for names referring to multiple different
    // objects as names=ID -> much faster tempTimer creation
    if (!pT->isTemporary()) {
        mLookupTable.remove(pT->mName, pT);
    } else {
        mLookupTable.remove(pT->getName());
    }
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

    // temp timers do not need to check for names referring to multiple different
    // objects as names=ID -> much faster tempTimer creation
    if (!pT->isTemporary()) {
        mLookupTable.remove(pT->mName, pT);
    } else {
        mLookupTable.remove(pT->getName());
    }
    mTimerMap.remove(pT->getID());
}


bool TimerUnit::enableTimer(const QString& name)
{
    bool found = false;
    auto it = mLookupTable.constFind(name);
    while (it != mLookupTable.cend() && it.key() == name) {
        TTimer* pT = it.value();

        if (!pT->isOffsetTimer()) {
            pT->setIsActive(true);
        } else {
            pT->setShouldBeActive(true);
        }


        if (pT->isFolder()) {
            // disable or enable all timers in the respective branch
            // irrespective of the user defined state.
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

        ++it;
        found = true;
    }
    return found;
}

bool TimerUnit::disableTimer(const QString& name)
{
    bool found = false;
    auto it = mLookupTable.constFind(name);
    while (it != mLookupTable.cend() && it.key() == name) {
        TTimer* pT = it.value();
        if (pT->isOffsetTimer()) {
            pT->setShouldBeActive(false);
        } else {
            pT->setIsActive(false);
        }

        pT->disableTimer();
        ++it;
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
        if (timer->getName() == name) {
            // only temporary timers can be killed
            if (!timer->isTemporary()) {
                return false;
            }
            timer->killTimer();
            markCleanup(timer);
            return true;
        }
    }
    return false;
}

int TimerUnit::remainingTime(const QString& name) const
{
    auto pTimer = findFirstTimer(name);
    if (pTimer){
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
    QMutableSetIterator<TTimer*> itTimer(mCleanupSet);
    while (itTimer.hasNext()) {
        auto pTimer = itTimer.next();
        // It is important to take the item OUT of the set before you delete
        // (and thus invalidate this pointer to) it...!
        itTimer.remove();
        delete pTimer;
    }
}

void TimerUnit::markCleanup(TTimer* pT)
{
    mCleanupSet.insert(pT);
}

void TimerUnit::assembleReport(TTimer* pItem)
{
    std::list<TTimer*>* childrenList = pItem->mpMyChildrenList;
    for (auto pChild : *childrenList) {
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
    msg << QLatin1String("Timers current total: ") << QString::number(statsItemsTotal) << QLatin1String("\n")
        << QLatin1String("tempTimers current total: ") << QString::number(statsTempItems) << QLatin1String("\n")
        << QLatin1String("active Timers: ") << QString::number(statsActiveItems) << QLatin1String("\n");

    return {
        msg.join(QString()),
        statsItemsTotal,
        statsTempItems,
        statsActiveItems
    };
}

void TimerUnit::changeHostName(const QString& newName)
{
    QSetIterator<QTimer*> itQTimerPtr(mQTimerSet);
    while (itQTimerPtr.hasNext()) {
        itQTimerPtr.next()->setProperty(TTimer::scmProperty_HostName, newName);
    }
}
