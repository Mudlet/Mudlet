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


#include "TriggerUnit.h"


#include "Host.h"
#include "TConsole.h"
#include "TTrigger.h"


using namespace std;

void TriggerUnit::initStats()
{
    statsTriggerTotal = 0;
    statsTempTriggers = 0;
    statsActiveTriggers = 0;
    statsActiveTriggersMax = 0;
    statsActiveTriggersMin = 0;
    statsActiveTriggersAverage = 0;
    statsTempTriggersCreated = 0;
    statsTempTriggersKilled = 0;
    statsAverageLineProcessingTime = 0;
    statsMaxLineProcessingTime = 0;
    statsMinLineProcessingTime = 0;
    statsRegexTriggers = 0;
}

void TriggerUnit::_uninstall(TTrigger* pChild, const QString& packageName)
{
    list<TTrigger*>* childrenList = pChild->mpMyChildrenList;
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
    QMutexLocker locker(&mTriggerUnitLock);
    if (mTriggerMap.find(id) != mTriggerMap.end()) {
        return mTriggerMap.value(id);
    } else {
        return nullptr;
    }
}

TTrigger* TriggerUnit::getTriggerPrivate(int id)
{
    if (mTriggerMap.find(id) != mTriggerMap.end()) {
        return mTriggerMap.value(id);
    } else {
        return nullptr;
    }
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
    if (!data.isEmpty()) {
#if defined(Q_OS_WIN32)
        // strndup(3) - a safe strdup(3) does not seem to be available on mingw32 with GCC-4.9.2
        char* subject = (char*)malloc(strlen(data.toUtf8().data()) + 1);
        strcpy(subject, data.toUtf8().data());
#else
        char* subject = strndup(data.toUtf8().constData(), strlen(data.toUtf8().constData()));
#endif
        for (auto trigger : mTriggerRootNodeList) {
            trigger->match(subject, data, line);
        }
        free(subject);

        for (auto& trigger : mCleanupList) {
            delete trigger;
        }
        mCleanupList.clear();
    }
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
    QMap<QString, TTrigger*>::const_iterator it = mLookupTable.constFind(name);
    while (it != mLookupTable.cend() && it.key() == name) {
        TTrigger* pT = it.value();
        return pT;
    }
    return nullptr;
}

bool TriggerUnit::enableTrigger(const QString& name)
{
    bool found = false;
    QMap<QString, TTrigger*>::const_iterator it = mLookupTable.constFind(name);
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
    QMap<QString, TTrigger*>::const_iterator it = mLookupTable.constFind(name);
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
    QMap<QString, TTrigger*>::const_iterator it = mLookupTable.constFind(name);
    while (it != mLookupTable.cend() && it.key() == name) {
        TTrigger* pT = it.value();
        pT->mKeepFiring = lines;
        ++it;
    }
}

bool TriggerUnit::killTrigger(const QString& name)
{
    QMap<QString, TTrigger*>::const_iterator it = mLookupTable.constFind(name);
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

void TriggerUnit::_assembleReport(TTrigger* pChild)
{
    list<TTrigger*>* childrenList = pChild->mpMyChildrenList;
    for (auto trigger : *childrenList) {
        _assembleReport(trigger);
        if (trigger->isActive()) {
            statsActiveTriggers++;
        }
        if (trigger->isTemporary()) {
            statsTempTriggers++;
        }
        statsPatterns += trigger->mRegexCodeList.size();
        statsTriggerTotal++;
    }
}

QString TriggerUnit::assembleReport()
{
    statsActiveTriggers = 0;
    statsTriggerTotal = 0;
    statsTempTriggers = 0;
    statsPatterns = 0;
    for (auto rootTrigger : mTriggerRootNodeList) {
        if (rootTrigger->isActive()) {
            statsActiveTriggers++;
        }
        if (rootTrigger->isTemporary()) {
            statsTempTriggers++;
        }
        statsPatterns += rootTrigger->mRegexCodeList.size();
        statsTriggerTotal++;
        list<TTrigger*>* childrenList = rootTrigger->mpMyChildrenList;
        for (auto childTrigger : *childrenList) {
            _assembleReport(childTrigger);
            if (childTrigger->isActive()) {
                statsActiveTriggers++;
            }
            if (childTrigger->isTemporary()) {
                statsTempTriggers++;
            }
            statsPatterns += childTrigger->mRegexCodeList.size();
            statsTriggerTotal++;
        }
    }
    QStringList msg;
    msg << "triggers current total: " << QString::number(statsTriggerTotal) << "\n"
        << "trigger patterns total: " << QString::number(statsPatterns) << "\n"
        << "tempTriggers current total: " << QString::number(statsTempTriggers) << "\n"
        << "active triggers: " << QString::number(statsActiveTriggers) << "\n";
    return msg.join("");
}

void TriggerUnit::doCleanup()
{
    for (auto trigger : mCleanupList) {
        delete trigger;
    }
    mCleanupList.clear();
}

void TriggerUnit::markCleanup(TTrigger* pT)
{
    for (auto trigger : mCleanupList) {
        if (trigger == pT) {
            return;
        }
    }
    mCleanupList.push_back(pT);
}
