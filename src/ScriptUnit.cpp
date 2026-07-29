/***************************************************************************
 *   Copyright (C) 2008-2012 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2014 by Ahmed Charles - acharles@outlook.com            *
 *   Copyright (C) 2022-2024, 2026 by Stephen Lyons                        *
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


#include "ScriptUnit.h"


#include "Host.h"
#include "TScript.h"

#include <functional>

/* We need an explicit constructor in this file as the Host class is forward
 * declared in the header file and it is problematic to define any dereferencing
 * of it there:*/
ScriptUnit::ScriptUnit(Host* pHost)
: mpHost(pHost)
{
}

ScriptUnit::~ScriptUnit()
{
    for (auto script : mScriptRootNodeList) {
        script->mpHost = nullptr;
        std::function<void(TScript*)> nullifyChildren = [&nullifyChildren](TScript* s) {
            for (auto child : *s->mpMyChildrenList) {
                child->mpHost = nullptr;
                nullifyChildren(child);
            }
        };
        nullifyChildren(script);
    }
    for (auto script : mScriptRootNodeList) {
        delete script;
    }
}

void ScriptUnit::resetStats()
{
    statsItemsTotal = 0;
    statsTempItems = 0;
    statsActiveItems = 0;
}

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
    // Re-entrant uninstall (#9337): a script's own event handler (e.g. a package
    // auto-updater calling uninstallPackage()) is removing its package while
    // Host::raiseEvent() is still dispatching to that script. Deleting now would
    // be a use-after-free - both of the handler still executing and of the other
    // TScript pointers in raiseEvent()'s copied handler list - so defer to
    // doCleanup() at depth 0. Deactivating is enough to stop the handlers firing
    // for the rest of the dispatch: TScript::callEventHandler() checks isActive().
    if (mProcessingDepth > 0) {
        for (auto script : uninstallList) {
            script->setIsActive(false);
        }
        return;
    }
    for (auto& script : uninstallList) {
        delete script;
    }
    uninstallList.clear();
}

// Flush the deletes uninstall() deferred (#9337). uninstallList is ordered
// children-before-parents and each ~Tree unlinks from its parent, so deleting
// children first empties the parent's child list (no double free); the seen
// set guards a node queued twice by re-entrant uninstalls.
void ScriptUnit::doCleanup()
{
    if (mProcessingDepth > 0) {
        return;
    }

    QSet<TScript*> deletedScripts;
    for (auto script : uninstallList) {
        if (!deletedScripts.contains(script)) {
            deletedScripts.insert(script);
            delete script;
        }
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

void ScriptUnit::reParentScript(int childID, int oldParentID, int newParentID, TreeItemInsertMode mode, int position)
{
    if (mode == TreeItemInsertMode::Append) {
        reParentScript(childID, oldParentID, newParentID, -1, -1);
    } else {
        // AtPosition mode - use 0 for parentPosition to enable position-based insertion
        reParentScript(childID, oldParentID, newParentID, 0, position);
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
    if (mScriptMap.find(id) != mScriptMap.end()) {
        return mScriptMap.value(id);
    }
    return nullptr;
}

TScript* ScriptUnit::getScriptPrivate(int id)
{
    if (mScriptMap.find(id) != mScriptMap.end()) {
        return mScriptMap.value(id);
    }
    return nullptr;
}

bool ScriptUnit::registerScript(TScript* pT)
{
    if (!pT) {
        return false;
    }

    if (pT->getParent()) {
        addScript(pT);
    } else {
        addScriptRootNode(pT);
    }
    return true;
}

void ScriptUnit::unregisterScript(TScript* pT)
{
    if (!pT) {
        return;
    }
    removeScript(pT);
    if (!pT->getParent()) {
        removeScriptRootNode(pT);
    }
}


void ScriptUnit::addScript(TScript* pT)
{
    if (!pT) {
        return;
    }

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

void ScriptUnit::compileAll(bool saveLoadingError)
{
    for (auto script : mScriptRootNodeList) {
        if (script->isActive()) {
            script->compileAll(saveLoadingError);
        }
    }
    if (mpHost->mpEditorDialog) {
        mpHost->mpEditorDialog->doCleanReset();
    }
}

std::vector<int> ScriptUnit::findItems(const QString& name, const bool exactMatch, const bool caseSensitive)
{
    std::vector<int> ids;
    const auto searchCaseSensitivity = caseSensitive ? Qt::CaseSensitive : Qt::CaseInsensitive;
    if (exactMatch) {
        for (auto& item : std::as_const(mScriptMap)) {
            if (!item->getName().compare(name, searchCaseSensitivity)) {
                ids.push_back(item->getID());
            }
        }
    } else {
        for (auto& item : std::as_const(mScriptMap)) {
            if (item->getName().contains(name, searchCaseSensitivity)) {
                ids.push_back(item->getID());
            }
        }
    }
    return ids;
}

void ScriptUnit::assembleReport(TScript* pItem)
{
    std::list<TScript*>* childrenList = pItem->mpMyChildrenList;
    for (auto pChild : *childrenList) {
        ++statsItemsTotal;
        if (pChild->isActive()) {
            ++statsActiveItems;
        }
        if (pChild->isTemporary()) {
            ++statsTempItems;
        }
        assembleReport(pChild);
    }
}

std::tuple<QString, int, int, int> ScriptUnit::assembleReport()
{
    resetStats();
    for (auto pItem : mScriptRootNodeList) {
        ++statsItemsTotal;
        if (pItem->isActive()) {
            ++statsActiveItems;
        }
        if (pItem->isTemporary()) {
            ++statsTempItems;
        }
        assembleReport(pItem);
    }
    QStringList msg;
    msg << QLatin1String("Scripts current total: ") << QString::number(statsItemsTotal) << QLatin1String("\n") << QLatin1String("tempScripts current total: ") << QString::number(statsTempItems)
        << QLatin1String("\n") << QLatin1String("active Scripts: ") << QString::number(statsActiveItems) << QLatin1String("\n");
    return {msg.join(QString()), statsItemsTotal, statsTempItems, statsActiveItems};
}
