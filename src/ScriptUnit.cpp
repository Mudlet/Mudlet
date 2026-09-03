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
#include "Tree.h"
#include "dlgTriggerEditor.h"
#include "utils.h"

#include <QLatin1String>
#include <QMapIterator>
#include <QSet>
#include <QStringList>

#include <functional>
#include <utility>

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
            for (auto* childNode : *s->mpMyChildrenList) {
                auto* child = static_cast<TScript*>(childNode);
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
    std::list<Tree<TScript>*>* childrenList = pChild->mpMyChildrenList;
    for (auto* scriptNode : *childrenList) {
        auto* script = static_cast<TScript*>(scriptNode);
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
    // Re-entrant uninstall (#9337): a package's own script (e.g. a package
    // auto-updater calling uninstallPackage()) is removing its package while one
    // of that package's scripts is still on the call stack - either an event
    // handler Host::raiseEvent() is dispatching to, or a top-level body
    // TScript::compileScript() is compiling. Deleting now would be a use-after-free
    // (of the script still executing, and of the other TScript pointers raiseEvent()
    // or ScriptUnit::compileAll() is still iterating), so defer to doCleanup() at
    // depth 0. Deactivating is enough to stop the handlers firing for the rest of
    // the dispatch: TScript::callEventHandler() checks isActive().
    if (mProcessingDepth > 0) {
        for (auto script : uninstallList) {
            script->setIsActive(false);
        }
        return;
    }
    // At depth 0 delete straight away, but go through doCleanup() rather than a bare
    // loop: uninstallList is a member that a prior deferred uninstall may have left
    // populated, so a second uninstall of the same still-registered package can queue
    // the same pointers twice - doCleanup()'s seen set stops that double-freeing.
    doCleanup();
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
    // Iterate a snapshot of the root list: a script's top-level body, run by
    // compile() below, can uninstall its own package (a package auto-updater
    // pattern). uninstall() defers the actual delete whilst compileScript() is on
    // the stack, so no node is unlinked mid-loop, but taking a copy keeps the
    // iteration safe even against a body that adds or removes root scripts:
    const std::vector<TScript*> rootNodes(mScriptRootNodeList.begin(), mScriptRootNodeList.end());
    for (auto script : rootNodes) {
        if (script->isActive()) {
            script->compileAll(saveLoadingError);
        }
    }
    // The loop is now done with the (possibly self-uninstalled) scripts, so flush
    // the deletes uninstall() deferred - before the editor tree is rebuilt below and
    // before returning to the event loop, where the 0ms save Host::uninstallPackage()
    // queues would otherwise serialize the still-live "uninstalled" scripts back in:
    doCleanup();
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
    std::list<Tree<TScript>*>* childrenList = pItem->mpMyChildrenList;
    for (auto* pChildNode : *childrenList) {
        auto* pChild = static_cast<TScript*>(pChildNode);
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
