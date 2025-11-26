/***************************************************************************
 *   Copyright (C) 2008-2013 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2014 by Ahmed Charles - acharles@outlook.com            *
 *   Copyright (C) 2022-2024 by Stephen Lyons - slysven@virginmedia.com    *
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


#include "TriggerUnit.h"


#include "Host.h"
#include "TConsole.h"
#include "TTrigger.h"

#include <functional>

TriggerUnit::~TriggerUnit()
{
    // Set mpHost to null on all triggers (including children) to prevent them from trying to
    // unregister themselves during destruction (which would modify the list
    // we're iterating over and cause iterator invalidation)
    for (auto trigger : mTriggerRootNodeList) {
        trigger->mpHost = nullptr;
        // Also set mpHost to null on all children recursively
        std::function<void(TTrigger*)> nullifyChildren = [&nullifyChildren](TTrigger* t) {
            for (auto child : *t->mpMyChildrenList) {
                child->mpHost = nullptr;
                nullifyChildren(child);
            }
        };
        nullifyChildren(trigger);
    }
    for (auto trigger : mTriggerRootNodeList) {
        delete trigger;
    }
}

void TriggerUnit::resetStats()
{
    statsItemsTotal = 0;
    statsTempItems = 0;
    statsActiveItems = 0;
    statsPatternsTotal = 0;
    statsPatternsActive = 0;
}

void TriggerUnit::_uninstall(TTrigger* pChild, const QString& packageName)
{
    std::list<TTrigger*>* childrenList = pChild->mpMyChildrenList;
    for (auto trigger : *childrenList) {
        _uninstall(trigger, packageName);
        uninstallList.append(trigger);
    }
}


void TriggerUnit::uninstall(const QString& packageName)
{
    for (auto rootTrigger : mTriggerRootNodeList) {
        if (rootTrigger->mPackageName == packageName) {
            _uninstall(rootTrigger, packageName);
            uninstallList.append(rootTrigger);
        }
    }
    for (auto& trigger : uninstallList) {
        unregisterTrigger(trigger);
    }
    uninstallList.clear();
}

void TriggerUnit::removeAllTempTriggers()
{
    for (auto trigger : mTriggerRootNodeList) {
        if (trigger->isTemporary()) {
            trigger->setIsActive(false);
            markCleanup(trigger);
        }
    }
}

void TriggerUnit::addTriggerRootNode(TTrigger* pT, int parentPosition, int childPosition, bool moveTrigger)
{
    if (!pT) {
        return;
    }
    if (!pT->getID()) {
        pT->setID(getNewID());
    }
    if ((parentPosition == -1) || (childPosition >= static_cast<int>(mTriggerRootNodeList.size()))) {
        mTriggerRootNodeList.push_back(pT);
    } else {
        // insert item at proper position
        int cnt = 0;
        for (auto it = mTriggerRootNodeList.begin(); it != mTriggerRootNodeList.end(); it++) {
            if (cnt >= childPosition) {
                mTriggerRootNodeList.insert(it, pT);
                break;
            }
            cnt++;
        }
    }

    if (!moveTrigger) {
        mTriggerMap.insert(pT->getID(), pT);
    }
}

void TriggerUnit::reParentTrigger(int childID, int oldParentID, int newParentID, int parentPosition, int childPosition)
{
    TTrigger* pOldParent = getTriggerPrivate(oldParentID);
    TTrigger* pNewParent = getTriggerPrivate(newParentID);
    TTrigger* pChild = getTriggerPrivate(childID);
    if (!pChild) {
        return;
    }
    if (pOldParent) {
        pOldParent->popChild(pChild);
    } else {
        mTriggerRootNodeList.remove(pChild);
    }
    if (pNewParent) {
        pNewParent->addChild(pChild, parentPosition, childPosition);
        pChild->setParent(pNewParent);
    } else {
        pChild->Tree<TTrigger>::setParent(nullptr);
        addTriggerRootNode(pChild, parentPosition, childPosition, true);
    }
}

void TriggerUnit::removeTriggerRootNode(TTrigger* pT)
{
    if (!pT) {
        return;
    }
    if (!pT->isTemporary()) {
        mLookupTable.remove(pT->mName, pT);
    } else {
        mLookupTable.remove(pT->getName());
    }
    mTriggerMap.remove(pT->getID());
    mTriggerRootNodeList.remove(pT);
}

TTrigger* TriggerUnit::getTrigger(int id)
{
    if (mTriggerMap.find(id) != mTriggerMap.end()) {
        return mTriggerMap.value(id);
    }
    return nullptr;
}

TTrigger* TriggerUnit::getTriggerPrivate(int id)
{
    if (mTriggerMap.find(id) != mTriggerMap.end()) {
        return mTriggerMap.value(id);
    }
    return nullptr;
}

bool TriggerUnit::registerTrigger(TTrigger* pT)
{
    if (!pT) {
        return false;
    }

    if (pT->getParent()) {
        addTrigger(pT);
        return true;
    } else {
        addTriggerRootNode(pT);
        return true;
    }
}

void TriggerUnit::unregisterTrigger(TTrigger* pT)
{
    if (!pT) {
        return;
    }
    if (pT->getParent()) {
        removeTrigger(pT);
        return;
    } else {
        removeTriggerRootNode(pT);
        return;
    }
}


void TriggerUnit::addTrigger(TTrigger* pT)
{
    if (!pT) {
        return;
    }

    if (!pT->getID()) {
        pT->setID(getNewID());
    }

    mTriggerMap.insert(pT->getID(), pT);
}

void TriggerUnit::removeTrigger(TTrigger* pT)
{
    if (!pT) {
        return;
    }
    if (!pT->isTemporary()) {
        mLookupTable.remove(pT->mName, pT);
    } else {
        mLookupTable.remove(pT->getName());
    }

    mTriggerMap.remove(pT->getID());
}

// trigger matching order is permanent trigger objects first, temporary objects second
// after package import or module sync this order needs to be reset
void TriggerUnit::reorderTriggersAfterPackageImport()
{
    QList<TTrigger*> tempList;
    for (auto trigger : mTriggerRootNodeList) {
        if (trigger->isTemporary()) {
            tempList.push_back(trigger);
        }
    }
    for (auto& trigger : tempList) {
        mTriggerRootNodeList.remove(trigger);
    }
    for (auto& trigger : tempList) {
        mTriggerRootNodeList.push_back(trigger);
    }
}

int TriggerUnit::getNewID()
{
    return ++mMaxID;
}

void TriggerUnit::processDataStream(const QString& data, int line)
{
    if (data.isEmpty()) {
        return;
    }

    const QByteArray utf8Data = data.toUtf8();
    const char* utf8Ptr = utf8Data.constData();
    const int utf8Length = utf8Data.size();

#if defined(Q_OS_WINDOWS)
    // strndup(3) - a safe strdup(3) does not seem to be available in the
    // original mingw or the replacement mingw-w64 enmvironment we use:
    char* subject = static_cast<char*>(malloc(utf8Length + 1));
    strcpy(subject, utf8Ptr);
#else
    char* subject = strndup(utf8Ptr, utf8Length);
#endif

    // Set processing flag to prevent re-entrant cleanup during trigger execution
    mIsProcessing = true;

    for (auto trigger : mTriggerRootNodeList) {
        trigger->match(subject, data, line);
    }
    free(subject);

    // Clear processing flag and perform any deferred cleanup
    mIsProcessing = false;
    doCleanup();
}

void TriggerUnit::compileAll()
{
    for (auto trigger : mTriggerRootNodeList) {
        if (trigger->isActive()) {
            trigger->compileAll();
        }
    }
}

void TriggerUnit::stopAllTriggers()
{
    for (auto trigger : mTriggerRootNodeList) {
        trigger->disableFamily();
    }
}

void TriggerUnit::reenableAllTriggers()
{
    for (auto trigger : mTriggerRootNodeList) {
        trigger->enableFamily();
    }
}

TTrigger* TriggerUnit::findTrigger(const QString& name)
{
    auto it = mLookupTable.constFind(name);
    while (it != mLookupTable.cend() && it.key() == name) {
        TTrigger* pT = it.value();
        return pT;
    }
    return nullptr;
}

std::vector<int> TriggerUnit::findItems(const QString& name, const bool exactMatch, const bool caseSensitive)
{
    std::vector<int> ids;
    const auto searchCaseSensitivity = caseSensitive ? Qt::CaseSensitive : Qt::CaseInsensitive;
    if (exactMatch) {
        for (auto& item : std::as_const(mTriggerMap)) {
            if (!item->getName().compare(name, searchCaseSensitivity)) {
                ids.push_back(item->getID());
            }
        }
    } else {
        for (auto& item : std::as_const(mTriggerMap)) {
            if (item->getName().contains(name, searchCaseSensitivity)) {
                ids.push_back(item->getID());
            }
        }
    }
    return ids;
}

bool TriggerUnit::enableTrigger(const QString& name)
{
    bool found = false;
    auto it = mLookupTable.constFind(name);
    while (it != mLookupTable.cend() && it.key() == name) {
        TTrigger* pT = it.value();
        pT->setIsActive(true);
        ++it;
        found = true;
    }
    return found;
}

bool TriggerUnit::disableTrigger(const QString& name)
{
    bool found = false;
    auto it = mLookupTable.constFind(name);
    while (it != mLookupTable.cend() && it.key() == name) {
        TTrigger* pT = it.value();
        pT->setIsActive(false);
        ++it;
        found = true;
    }
    return found;
}

void TriggerUnit::setTriggerStayOpen(const QString& name, int lines)
{
    auto it = mLookupTable.constFind(name);
    while (it != mLookupTable.cend() && it.key() == name) {
        TTrigger* pT = it.value();
        pT->mKeepFiring = lines;
        ++it;
    }
}

bool TriggerUnit::killTrigger(const QString& name)
{
    auto it = mLookupTable.constFind(name);
    while (it != mLookupTable.cend() && it.key() == name) {
        TTrigger* pT = it.value();
        if (pT->isTemporary()) //this function is only defined for tempTriggers, permanent objects cannot be removed
        {
            // there can only be a single tempTrigger by this name and this function ignores non-tempTriggers by definition
            markCleanup(pT);
            return true;
        }
        it++;
    }
    return false;
}

void TriggerUnit::assembleReport(TTrigger* pItem)
{
    std::list<TTrigger*>* childrenList = pItem->mpMyChildrenList;
    for (auto pChild : *childrenList) {
        ++statsItemsTotal;
        if (pChild->isActive()) {
            ++statsActiveItems;
            statsPatternsActive += pChild->mPatterns.size();
        }
        if (pChild->isTemporary()) {
            ++statsTempItems;
        }
        statsPatternsTotal += pChild->mPatterns.size();
        assembleReport(pChild);
    }
}

std::tuple<QString, int, int, int, int, int> TriggerUnit::assembleReport()
{
    resetStats();
    for (auto pItem : mTriggerRootNodeList) {
        ++statsItemsTotal;
        if (pItem->isActive()) {
            ++statsActiveItems;
            statsPatternsActive += pItem->mPatterns.size();
        }
        if (pItem->isTemporary()) {
            ++statsTempItems;
        }
        statsPatternsTotal += pItem->mPatterns.size();
        assembleReport(pItem);
    }
    QStringList msg;
    msg << QLatin1String("triggers current total: ") << QString::number(statsItemsTotal) << QLatin1String("\n")
        << QLatin1String("tempTriggers current total: ") << QString::number(statsTempItems) << QLatin1String("\n")
        << QLatin1String("active triggers: ") << QString::number(statsActiveItems) << QLatin1String("\n")
        << QLatin1String("trigger patterns total: ") << QString::number(statsPatternsTotal) << QLatin1String("\n")
        << QLatin1String("active patterns total: ") << QString::number(statsPatternsActive) << QLatin1String("\n");
    return {
        msg.join(QString()),
        statsItemsTotal,
        statsPatternsTotal,
        statsTempItems,
        statsActiveItems,
        statsPatternsActive
    };
}

void TriggerUnit::doCleanup()
{
    // Skip cleanup if we're currently processing triggers to prevent iterator invalidation
    // Cleanup will be performed when processDataStream() completes
    if (mIsProcessing) {
        return;
    }

    QMutableSetIterator<TTrigger*> itTrigger(mCleanupSet);
    while (itTrigger.hasNext()) {
        auto pTrigger = itTrigger.next();
        itTrigger.remove();
        delete pTrigger;
    }
}

void TriggerUnit::markCleanup(TTrigger* pT)
{
    mCleanupSet.insert(pT);
}
