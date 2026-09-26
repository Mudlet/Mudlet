/***************************************************************************
 *   Copyright (C) 2008-2013 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2014 by Ahmed Charles - acharles@outlook.com            *
 *   Copyright (C) 2017, 2021, 2023-2024, 2026 by Stephen Lyons            *
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


#include "ActionUnit.h"


#include "Host.h"
#include "TAction.h"
#include "TEasyButtonBar.h"
#include "TToolBar.h"
#include "Tree.h"
#include "mudlet.h"
#include "TMainConsole.h"
#include "utils.h"

#include <QDebug>
#include <QMapIterator>
#include <QSet>

#include <functional>

/* We need an explicit constructor in this file as the Host class is forward
 * declared in the header file and it is problematic to define any dereferencing
 * of it there:*/
ActionUnit::ActionUnit(Host* pHost)
: mpHost(pHost)
{
}

ActionUnit::~ActionUnit()
{
    for (auto action : mActionRootNodeList) {
        action->mpHost = nullptr;
        std::function<void(TAction*)> nullifyChildren = [&nullifyChildren](TAction* a) {
            for (auto* childNode : *a->mpMyChildrenList) {
                auto* child = static_cast<TAction*>(childNode);
                child->mpHost = nullptr;
                nullifyChildren(child);
            }
        };
        nullifyChildren(action);
    }
    for (auto action : mActionRootNodeList) {
        delete action;
    }
}

void ActionUnit::_uninstall(TAction* pChild, const QString& packageName)
{
    std::list<Tree<TAction>*>* childrenList = pChild->mpMyChildrenList;
    for (auto* actionNode : *childrenList) {
        auto* action = static_cast<TAction*>(actionNode);
        _uninstall(action, packageName);
        uninstallList.append(action);
    }
}


void ActionUnit::uninstall(const QString& packageName)
{
    for (auto rootAction : mActionRootNodeList) {
        if (rootAction->mPackageName == packageName) {
            _uninstall(rootAction, packageName);
            uninstallList.append(rootAction);
        }
    }
    // Re-entrant uninstall (#9337): a button's own script (e.g. a package
    // auto-updater calling uninstallPackage()) is removing its package while
    // TAction::execute() is still on the call stack for that button. Deleting
    // now would be a use-after-free, so defer to doCleanup() at depth 0.
    // Deactivating stops the buttons from firing again in the meantime.
    if (mProcessingDepth > 0) {
        for (auto action : uninstallList) {
            action->setIsActive(false);
        }
        return;
    }
    // Not inside a button script - delete now. Route through doCleanup() rather
    // than an inline loop so the same seen-set guards against a double free if a
    // re-entrant uninstall of the same package queued any action twice.
    doCleanup();
}

void ActionUnit::doCleanup()
{
    if (mProcessingDepth > 0) {
        return;
    }

    // Runs per unit on every line of game text and next to never has work queued.
    if (!hasPendingDeletes()) {
        return;
    }

    // Flush the deletes uninstall() deferred (#9337). uninstallList is ordered
    // children-before-parents and each ~Tree unlinks from its parent, so deleting
    // children first empties the parent's child list (no double free); the seen
    // set guards a node queued twice by re-entrant uninstalls.
    QSet<TAction*> deletedActions;
    for (auto action : uninstallList) {
        if (!deletedActions.contains(action)) {
            deletedActions.insert(action);
            delete action;
        }
    }
    uninstallList.clear();
}

void ActionUnit::endProcessing()
{
    --mProcessingDepth;
    Q_ASSERT(mProcessingDepth >= 0);
}

void ActionUnit::compileAll()
{
    for (auto action : mActionRootNodeList) {
        if (action->isActive()) {
            action->compileAll();
        }
    }
}

TAction* ActionUnit::findAction(const QString& name)
{
    QMapIterator<int, TAction*> it(mActionMap);
    while (it.hasNext()) {
        it.next();
        if (it.value()->getName() == name) {
            // qDebug().nospace().noquote() << "ActionUnit::findAction(const QString&) INFO - found: \"" << it.value()->getName() << "\".";
            TAction* pT = it.value();
            return pT;
        }
    }
    return nullptr;
}

std::vector<int> ActionUnit::findItems(const QString& name, const bool exactMatch, const bool caseSensitive)
{
    std::vector<int> ids;
    const auto searchCaseSensitivity = caseSensitive ? Qt::CaseSensitive : Qt::CaseInsensitive;
    if (exactMatch) {
        for (auto& item : std::as_const(mActionMap)) {
            if (!item->getName().compare(name, searchCaseSensitivity)) {
                ids.push_back(item->getID());
            }
        }
    } else {
        for (auto& item : std::as_const(mActionMap)) {
            if (item->getName().contains(name, searchCaseSensitivity)) {
                ids.push_back(item->getID());
            }
        }
    }
    return ids;
}

void ActionUnit::addActionRootNode(TAction* pT, int parentPosition, int childPosition)
{
    if (!pT) {
        return;
    }
    if (!pT->getID()) {
        pT->setID(getNewID());
    }
    if ((parentPosition == -1) || (childPosition >= static_cast<int>(mActionRootNodeList.size()))) {
        mActionRootNodeList.push_back(pT);
    } else {
        // insert item at proper position
        int cnt = 0;
        for (auto it = mActionRootNodeList.begin(); it != mActionRootNodeList.end(); it++) {
            if (cnt >= childPosition) {
                mActionRootNodeList.insert(it, pT);
                break;
            }
            cnt++;
        }
    }

    mActionMap.insert(pT->getID(), pT);
}

void ActionUnit::reParentAction(int childID, int oldParentID, int newParentID, int parentPosition, int childPosition)
{
    TAction* pOldParent = getActionPrivate(oldParentID);
    TAction* pNewParent = getActionPrivate(newParentID);
    TAction* pChild = getActionPrivate(childID);
    if (!pChild) {
        return;
    }

    if (pOldParent) {
        pChild->setDataChanged();
        pOldParent->popChild(pChild);
        pOldParent->setDataChanged();

        // clear references to old parent toolbars and buttonbars.
        if (pOldParent->mpToolBar == pChild->mpToolBar) {
            pChild->mpToolBar = nullptr;
        }
        if (pOldParent->mpEasyButtonBar == pChild->mpEasyButtonBar) {
            pChild->mpEasyButtonBar = nullptr;
        }
    }
    if (!pOldParent) {
        removeActionRootNode(pChild);
        pChild->setDataChanged();
    }

    if (pNewParent) {
        pNewParent->Tree<TAction>::addChild(pChild, parentPosition, childPosition);
        pChild->Tree<TAction>::setParent(pNewParent);
        pChild->setDataChanged();
        pNewParent->setDataChanged();
        //cout << "dumping family of newParent:"<<endl;
        //pNewParent->Dump();
    } else {
        pChild->Tree<TAction>::setParent(nullptr);
        addActionRootNode(pChild, parentPosition, childPosition);
    }

    pChild->setDataChanged();

    if ((!pOldParent) && (pNewParent)) {
        // A profile with no view has no console, so no bars to take down
        if (mpHost->mpConsole) {
            mpHost->mpConsole->detachActionBars(pChild);
        }
    }
}

void ActionUnit::reParentAction(int childID, int oldParentID, int newParentID, TreeItemInsertMode mode, int position)
{
    if (mode == TreeItemInsertMode::Append) {
        reParentAction(childID, oldParentID, newParentID, -1, -1);
    } else {
        // AtPosition mode - use 0 for parentPosition to enable position-based insertion
        reParentAction(childID, oldParentID, newParentID, 0, position);
    }
}

void ActionUnit::removeActionRootNode(TAction* pT)
{
    if (!pT) {
        return;
    }
    mActionRootNodeList.remove(pT);
}

TAction* ActionUnit::getAction(int id)
{
    if (mActionMap.contains(id)) {
        return mActionMap.value(id);
    }
    return nullptr;
}

TAction* ActionUnit::getActionPrivate(int id)
{
    if (mActionMap.find(id) != mActionMap.end()) {
        return mActionMap.value(id);
    }
    return nullptr;
}

bool ActionUnit::registerAction(TAction* pT)
{
    if (!pT) {
        return false;
    }

    if (pT->getParent()) {
        addAction(pT);
        return true;
    }
    addActionRootNode(pT);
    return true;
}

void ActionUnit::unregisterAction(TAction* pT)
{
    if (!pT) {
        return;
    }
    if (pT->getParent() && pT->getParent()->mPackageName.isEmpty()) {
        removeAction(pT);
        updateAllToolbars();
        return;
    }
    if (mpHost->mpConsole && pT->mpEasyButtonBar && pT->mPackageName.isEmpty()) {
        mpHost->mpConsole->detachActionBars(pT);
    }
    removeAction(pT);
    if (!pT->getParent()) {
        removeActionRootNode(pT);
    }
    updateAllToolbars();
}


void ActionUnit::addAction(TAction* pT)
{
    if (!pT) {
        return;
    }

    if (!pT->getID()) {
        pT->setID(getNewID());
    }

    mActionMap.insert(pT->getID(), pT);
}

void ActionUnit::removeAction(TAction* pT)
{
    if (!pT) {
        return;
    }

    mActionMap.remove(pT->getID());
}

int ActionUnit::getNewID()
{
    return ++mMaxID;
}

// A root action that is a package or module container is not a toolbar itself -
// its children are. Location 4 is the floating setting, which is not one of the
// TEasyButtonBars that live in the profile's window.
TAction* ActionUnit::findEasyButtonBarAction(const QString& name)
{
    for (auto& rootAction : mActionRootNodeList) {
        if (rootAction->mLocation == 4) {
            continue;
        }
        if (!rootAction->mPackageName.isEmpty()) {
            for (auto* childActionNode : *rootAction->mpMyChildrenList) {
                auto* childAction = static_cast<TAction*>(childActionNode);
                if (childAction->mLocation != 4 && childAction->getName() == name) {
                    return childAction;
                }
            }
            continue;
        }
        if (rootAction->getName() == name) {
            return rootAction;
        }
    }
    return nullptr;
}

// showToolBar() and hideToolBar() only reach the button bars in the profile's
// window, so a toolbar set to float is worth telling apart from a typo.
bool ActionUnit::namesAFloatingToolBar(const QString& name)
{
    for (auto& rootAction : mActionRootNodeList) {
        if (rootAction->mLocation == 4 && rootAction->getName() == name) {
            return true;
        }
        if (rootAction->mPackageName.isEmpty()) {
            continue;
        }
        for (auto* childActionNode : *rootAction->mpMyChildrenList) {
            auto* childAction = static_cast<TAction*>(childActionNode);
            if (childAction->mLocation == 4 && childAction->getName() == name) {
                return true;
            }
        }
    }
    return false;
}

std::pair<bool, QString> ActionUnit::setToolBarActive(const QString& name, const bool active)
{
    bool found = false;
    if (auto* pAction = findEasyButtonBarAction(name)) {
        pAction->setIsActive(active);
        found = true;
    } else {
        // the name of a package is accepted as well, and covers every toolbar
        // that came in it
        for (auto& rootAction : mActionRootNodeList) {
            if (rootAction->mLocation == 4 || rootAction->mPackageName.isEmpty() || rootAction->getName() != name) {
                continue;
            }
            for (auto* childActionNode : *rootAction->mpMyChildrenList) {
                auto* childAction = static_cast<TAction*>(childActionNode);
                if (childAction->mLocation != 4) {
                    childAction->setIsActive(active);
                    found = true;
                }
            }
        }
    }

    if (found) {
        updateAllToolbars();
    }
    mudlet::self()->processEventLoopHack();
    if (found) {
        return {true, QString()};
    }
    if (namesAFloatingToolBar(name)) {
        return {false, qsl("toolbar '%1' is set to float, which showToolBar() and hideToolBar() do not move").arg(name)};
    }
    return {false, qsl("toolbar '%1' not found").arg(name)};
}

std::pair<bool, QString> ActionUnit::showToolBar(const QString& name)
{
    return setToolBarActive(name, true);
}

std::pair<bool, QString> ActionUnit::hideToolBar(const QString& name)
{
    return setToolBarActive(name, false);
}

void ActionUnit::updateAllToolbars()
{
    // The bars are the console's widgets, so a profile with no view has nothing
    // to build
    if (!mpHost->mpConsole) {
        return;
    }
    mpHost->mpConsole->regenerateToolBars(mActionRootNodeList, mToolBarList);
    mpHost->mpConsole->regenerateEasyButtonBars(mActionRootNodeList, mEasyButtonBarList);
}
