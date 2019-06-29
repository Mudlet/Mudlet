/***************************************************************************
 *   Copyright (C) 2008-2013 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2014 by Ahmed Charles - acharles@outlook.com            *
 *   Copyright (C) 2017 by Stephen Lyons - slysven@virginmedia.com         *
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


#include "TAction.h"
#include "TCommandLine.h"
#include "TConsole.h"
#include "TEasyButtonBar.h"
#include "TToolBar.h"
#include "mudlet.h"


using namespace std;

void ActionUnit::_uninstall(TAction* pChild, const QString& packageName)
{
    list<TAction*>* childrenList = pChild->mpMyChildrenList;
    for (auto action : *childrenList) {
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
    for (auto& action : uninstallList) {
        delete action;
    }
    uninstallList.clear();
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
    //QMap<int, TAction *>  mActionMap;

    QMapIterator<int, TAction*> it(mActionMap);
    while (it.hasNext()) {
        it.next();
        if (it.value()->getName() == name) {
            qDebug() << it.value()->getName();
            TAction* pT = it.value();
            return pT;
        }
    }
    return nullptr;
}

std::vector<TAction*> ActionUnit::findActionsByName(const QString& name)
{
    std::vector<TAction*> actions;
    for (auto action : qAsConst(mActionMap)) {
        if (action->getName() == name) {
            actions.push_back(action);
        }
    }
    return actions;
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
        if (pChild->mpEasyButtonBar) {
            if (pChild->mLocation == 0) {
                mpHost->mpConsole->mpTopToolBar->layout()->removeWidget(pChild->mpEasyButtonBar);
            }
            if (pChild->mLocation == 2) {
                mpHost->mpConsole->mpLeftToolBar->layout()->removeWidget(pChild->mpEasyButtonBar);
            }
            if (pChild->mLocation == 3) {
                mpHost->mpConsole->mpRightToolBar->layout()->removeWidget(pChild->mpEasyButtonBar);
            }
            if (pChild->mLocation == 4) {
                if (pChild->mpToolBar) {
                    pChild->mpToolBar->setFloating(false);
                    mudlet::self()->removeDockWidget(pChild->mpToolBar);
                }
            }
        }
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
    } else {
        return nullptr;
    }
}

TAction* ActionUnit::getActionPrivate(int id)
{
    if (mActionMap.find(id) != mActionMap.end()) {
        return mActionMap.value(id);
    } else {
        return nullptr;
    }
}

bool ActionUnit::registerAction(TAction* pT)
{
    if (!pT) {
        return false;
    }

    if (pT->getParent()) {
        addAction(pT);
        return true;
    } else {
        addActionRootNode(pT);
        return true;
    }
}

void ActionUnit::unregisterAction(TAction* pT)
{
    if (!pT) {
        return;
    }
    if (pT->getParent() && pT->getParent()->mPackageName.isEmpty()) {
        removeAction(pT);
        updateToolbar();
        return;
    } else {
        if (pT->mpEasyButtonBar && pT->mPackageName.isEmpty()) {
            if (pT->mLocation == 0) {
                mpHost->mpConsole->mpTopToolBar->layout()->removeWidget(pT->mpEasyButtonBar);
            }
            if (pT->mLocation == 2) {
                mpHost->mpConsole->mpLeftToolBar->layout()->removeWidget(pT->mpEasyButtonBar);
            }
            if (pT->mLocation == 3) {
                mpHost->mpConsole->mpRightToolBar->layout()->removeWidget(pT->mpEasyButtonBar);
            }
            if (pT->mLocation == 4) {
                if (pT->mpToolBar) {
                    pT->mpToolBar->setFloating(false);
                    mudlet::self()->removeDockWidget(pT->mpToolBar);
                }
            }
        }
        if (!pT->getParent()) {
            removeActionRootNode(pT);
        } else {
            removeAction(pT);
        }
        updateToolbar();
        return;
    }
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

std::list<QPointer<TToolBar>> ActionUnit::getToolBarList()
{
    for (auto& action : mActionRootNodeList) {
        if (action->mLocation != 4) {
            continue; // skip over any root action node that is NOT going to be a TToolBar.
        }
        if (action->mPackageName.size() > 0) {
            for (auto& childAction : *action->mpMyChildrenList) {
                bool found = false;
                QPointer<TToolBar> pTB = nullptr;
                for (auto& toolBar : mToolBarList) {
                    if (toolBar == childAction->mpToolBar) {
                        found = true;
                        pTB = toolBar;
                    }
                }
                if (!found) {
                    pTB = new TToolBar(childAction, childAction->getName(), mudlet::self());
                    mToolBarList.push_back(pTB);
                }
                if (childAction->mOrientation == 1) {
                    pTB->setVerticalOrientation();
                } else {
                    pTB->setHorizontalOrientation();
                }
                constructToolbar(childAction, pTB);
                childAction->mpToolBar = pTB;
                pTB->setStyleSheet(pTB->mpTAction->css);
            }
            continue; //action package
        }
        bool found = false;
        QPointer<TToolBar> pTB = nullptr;
        for (auto& toolBar : mToolBarList) {
            if (toolBar == action->mpToolBar) {
                found = true;
                pTB = toolBar;
            }
        }
        if (!found) {
            pTB = new TToolBar(action, action->getName(), mudlet::self());
            mToolBarList.push_back(pTB);
        }
        if (action->mOrientation == 1) {
            pTB->setVerticalOrientation();
        } else {
            pTB->setHorizontalOrientation();
        }
        constructToolbar(action, pTB);
        action->mpToolBar = pTB;
        pTB->setStyleSheet(pTB->mpTAction->css);
    }

    return mToolBarList;
}

std::list<QPointer<TEasyButtonBar>> ActionUnit::getEasyButtonBarList()
{
    for (auto& rootAction : mActionRootNodeList) {
        if (rootAction->mLocation == 4) {
            continue; // skip over any root action node that IS going to be a TToolBar.
        }
        if (rootAction->mPackageName.size() > 0) {
            for (auto childActionIterator = rootAction->mpMyChildrenList->begin(); childActionIterator != rootAction->mpMyChildrenList->end(); childActionIterator++) {
                bool found = false;
                TEasyButtonBar* pTB = nullptr;
                for (auto& easyButtonBar : mEasyButtonBarList) {
                    if (easyButtonBar == (*childActionIterator)->mpEasyButtonBar) {
                        found = true;
                        pTB = easyButtonBar;
                    }
                }
                if (!found) {
                    pTB = new TEasyButtonBar(rootAction, (*childActionIterator)->getName(), mpHost->mpConsole->mpTopToolBar);
                    mpHost->mpConsole->mpTopToolBar->layout()->addWidget(pTB);
                    mEasyButtonBarList.emplace_back(pTB);
                    (*childActionIterator)->mpEasyButtonBar = pTB; // wird fuer drag&drop gebraucht
                }
                if ((*childActionIterator)->mOrientation == 1) {
                    pTB->setVerticalOrientation();
                } else {
                    pTB->setHorizontalOrientation();
                }
                constructToolbar(*childActionIterator, pTB);
                (*childActionIterator)->mpEasyButtonBar = pTB;
                pTB->setStyleSheet(pTB->mpTAction->css);
            }
            continue; //rootAction package
        }
        bool found = false;
        TEasyButtonBar* pTB = nullptr;
        for (auto& easyButtonBar : mEasyButtonBarList) {
            if (easyButtonBar == rootAction->mpEasyButtonBar) {
                found = true;
                pTB = easyButtonBar;
            }
        }
        if (!found) {
            pTB = new TEasyButtonBar(rootAction, rootAction->getName(), mpHost->mpConsole->mpTopToolBar);
            mpHost->mpConsole->mpTopToolBar->layout()->addWidget(pTB);
            mEasyButtonBarList.emplace_back(pTB);
            rootAction->mpEasyButtonBar = pTB; // wird fuer drag&drop gebraucht
        }
        if (rootAction->mOrientation == 1) {
            pTB->setVerticalOrientation();
        } else {
            pTB->setHorizontalOrientation();
        }
        constructToolbar(rootAction, pTB);
        rootAction->mpEasyButtonBar = pTB;
        pTB->setStyleSheet(pTB->mpTAction->css);
    }

    return mEasyButtonBarList;
}

TAction* ActionUnit::getHeadAction(TToolBar* pT)
{
    for (auto& action : mActionRootNodeList) {
        for (auto it2 = mToolBarList.begin(); it2 != mToolBarList.end(); it2++) {
            if (pT == action->mpToolBar) {
                return action;
            }
        }
    }
    return nullptr;
}

void ActionUnit::showToolBar(const QString& name)
{
    for (auto& easyButtonBar : mEasyButtonBarList) {
        if (easyButtonBar->mpTAction->mName == name) {
            easyButtonBar->mpTAction->setIsActive(true);
            updateToolbar();
        }
    }
    mudlet::self()->processEventLoopHack();
    mpHost->mpConsole->mpCommandLine->setFocus();
}

void ActionUnit::hideToolBar(const QString& name)
{
    for (auto& easyButtonBar : mEasyButtonBarList) {
        if (easyButtonBar->mpTAction->mName == name) {
            easyButtonBar->mpTAction->setIsActive(false);
            updateToolbar();
        }
    }
    mudlet::self()->processEventLoopHack();
}

void ActionUnit::constructToolbar(TAction* pA, TToolBar* pTB)
{
    if (!pA->isDataChanged()) {
        return;
    }

    pTB->clear();
    if ((pA->mLocation != 4) || (!pA->isActive())) {
        pTB->setFloating(false);
        mudlet::self()->removeDockWidget(pTB);
        return;
    }

    if (pA->mLocation == 4) {
        pA->expandToolbar(pTB);
        pTB->setTitleBarWidget(nullptr);
    }

    pTB->finalize();

    if (pA->mOrientation == 0) {
        pTB->setHorizontalOrientation();
    } else {
        pTB->setVerticalOrientation();
    }

    pTB->setTitleBarWidget(nullptr);
    pTB->setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);
    if (pA->mLocation == 4) {
        mudlet::self()->addDockWidget(pA->mToolbarLastDockArea, pTB);
        if (pA->mToolbarLastFloatingState) {
            pTB->setFloating(true);
            QPoint pos = QPoint(pA->mPosX, pA->mPosY);
            pTB->show();
            pTB->move(pos);
        } else {
            pTB->setFloating(false);
            pTB->show();
        }
        pTB->mpTAction = pA;
        pTB->recordMove();
    } else {
        pTB->show();
    }

    pTB->setStyleSheet(pTB->mpTAction->css);
    pA->setDataSaved();
}

TAction* ActionUnit::getHeadAction(TEasyButtonBar* pT)
{
    for (auto& action : mActionRootNodeList) {
        for (auto it2 = mEasyButtonBarList.begin(); it2 != mEasyButtonBarList.end(); it2++) {
            if (pT == action->mpEasyButtonBar) {
                return action;
            }
        }
    }
    return nullptr;
}

void ActionUnit::constructToolbar(TAction* pA, TEasyButtonBar* pTB)
{
    pTB->clear();
    if (pA->mLocation == 4) {
        //floating toolbars are handled differently
        return;
    }
    if (!pA->isActive()) {
        pTB->hide();
        return;
    }

    pA->expandToolbar(pTB);
    pTB->finalize();
    if (pA->mOrientation == 0) {
        pTB->setHorizontalOrientation();
    } else {
        pTB->setVerticalOrientation();
    }
    switch (pA->mLocation) {
    case 0:
        mpHost->mpConsole->mpTopToolBar->layout()->addWidget(pTB);
        break;
    //case 1:
    //mpHost->mpConsole->mpTopToolBar->layout()->addWidget( pTB );
    //break;
    case 2:
        mpHost->mpConsole->mpLeftToolBar->layout()->addWidget(pTB);
        break;
    case 3:
        mpHost->mpConsole->mpRightToolBar->layout()->addWidget(pTB);
        break;
    }

    pTB->setStyleSheet(pTB->mpTAction->css);
    pTB->show();
}


void ActionUnit::updateToolbar()
{
    getToolBarList();
    getEasyButtonBarList();
}
