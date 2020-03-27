/***************************************************************************
 *   Copyright (C) 2008-2012 by Heiko Koehn - KoehnHeiko@googlemail.com    *
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


#include "ScriptUnit.h"


#include "Host.h"
#include "TScript.h"

void ScriptUnit::_uninstall(TScript* pChild, const QString& packageName)
{
    std::list<TScript*>* childrenList = pChild->mpMyChildrenList;
    for (auto script : *childrenList) {
        _uninstall(script, packageName);
        uninstallList.append(script);
    }
}


void ScriptUnit::uninstall(const QString& packageName)
{
    for (auto rootScript : mScriptRootNodeList) {
        if (rootScript->mPackageName == packageName) {
            _uninstall(rootScript, packageName);
            uninstallList.append(rootScript);
        }
    }
    for (auto& script : uninstallList) {
        unregisterScript(script);
    }
    uninstallList.clear();
}

void ScriptUnit::stopAllTriggers()
{
    for (auto script : mScriptRootNodeList) {
        script->setIsActive(false);
    }
}

void ScriptUnit::addScriptRootNode(TScript* pT, int parentPosition, int childPosition)
{
    if (!pT) {
        return;
    }
    if (!pT->getID()) {
        pT->setID(getNewID());
    }

    if ((parentPosition == -1) || (childPosition >= static_cast<int>(mScriptRootNodeList.size()))) {
        mScriptRootNodeList.push_back(pT);
    } else {
        // insert item at proper position
        int cnt = 0;
        for (auto it = mScriptRootNodeList.begin(); it != mScriptRootNodeList.end(); it++) {
            if (cnt >= childPosition) {
                mScriptRootNodeList.insert(it, pT);
                break;
            }
            cnt++;
        }
    }

    mScriptMap.insert(pT->getID(), pT);
}

void ScriptUnit::reParentScript(int childID, int oldParentID, int newParentID, int parentPosition, int childPosition)
{
    TScript* pOldParent = getScriptPrivate(oldParentID);
    TScript* pNewParent = getScriptPrivate(newParentID);
    TScript* pChild = getScriptPrivate(childID);
    if (!pChild) {
        return;
    }
    if (pOldParent) {
        pOldParent->popChild(pChild);
    }
    if (!pOldParent) {
        removeScriptRootNode(pChild);
    }
    if (pNewParent) {
        pNewParent->addChild(pChild, parentPosition, childPosition);
        pChild->setParent(pNewParent);
        //cout << "dumping family of newParent:"<<endl;
        //pNewParent->Dump();
    } else {
        pChild->Tree<TScript>::setParent(nullptr);
        addScriptRootNode(pChild, parentPosition, childPosition);
    }
}

void ScriptUnit::removeScriptRootNode(TScript* pT)
{
    if (!pT) {
        return;
    }
    mScriptRootNodeList.remove(pT);
}

TScript* ScriptUnit::getScript(int id)
{
    QMutexLocker locker(&mScriptUnitLock);
    if (mScriptMap.find(id) != mScriptMap.end()) {
        return mScriptMap.value(id);
    } else {
        return nullptr;
    }
}

TScript* ScriptUnit::getScriptPrivate(int id)
{
    if (mScriptMap.find(id) != mScriptMap.end()) {
        return mScriptMap.value(id);
    } else {
        return nullptr;
    }
}

bool ScriptUnit::registerScript(TScript* pT)
{
    if (!pT) {
        return false;
    }

    if (pT->getParent()) {
        addScript(pT);
        return true;
    } else {
        addScriptRootNode(pT);
        return true;
    }
}

void ScriptUnit::unregisterScript(TScript* pT)
{
    if (!pT) {
        return;
    }
    if (pT->getParent()) {
        removeScript(pT);
        return;
    } else {
        removeScript(pT);
        removeScriptRootNode(pT);
        return;
    }
}


void ScriptUnit::addScript(TScript* pT)
{
    if (!pT) {
        return;
    }

    QMutexLocker locker(&mScriptUnitLock);

    if (!pT->getID()) {
        pT->setID(getNewID());
    }

    mScriptMap.insert(pT->getID(), pT);
}

void ScriptUnit::removeScript(TScript* pT)
{
    if (!pT) {
        return;
    }
    QMapIterator<QString, QList<TScript*>> it(mpHost->mEventHandlerMap);
    while (it.hasNext()) {
        it.next();
        mpHost->mEventHandlerMap[it.key()].removeAll(pT);
    }
    mScriptMap.remove(pT->getID());
}


int ScriptUnit::getNewID()
{
    return ++mMaxID;
}

void ScriptUnit::compileAll()
{
    for (auto script : mScriptRootNodeList) {
        if (script->isActive()) {
            script->compileAll();
        }
    }
}

QVector<int> ScriptUnit::findScriptId(const QString& name) const
{
    QVector<int> Ids;
    for (auto script : mScriptMap) {
        if (script->getName() == name) {
            Ids.append(script->getID());
        }
    }
    return Ids;
}
