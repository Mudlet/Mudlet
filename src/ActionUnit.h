#ifndef MUDLET_ACTIONUNIT_H
#define MUDLET_ACTIONUNIT_H

/***************************************************************************
 *   Copyright (C) 2008-2011 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2014 by Ahmed Charles - acharles@outlook.com            *
 *   Copyright (C) 2017, 2022-2023, 2026 by Stephen Lyons                  *
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
#include <QPointer>
#include <QString>
#include <list>
#include <utility>
#include <vector>

class Host;
class mudlet;
class TAction;
class TEasyButtonBar;
class TToolBar;

class ActionUnit
{
    friend class XMLexport;
    friend class XMLimport;

public:
    explicit ActionUnit(Host*);
    ~ActionUnit();

    std::list<TAction*> getActionRootNodeList() { return mActionRootNodeList; }

    QMap<int, TAction*> getActionList() { return mActionMap; }

    TAction* getAction(int id);
    TAction* findAction(const QString&);
    std::vector<int> findItems(const QString& name, const bool exactMatch = true, const bool caseSensitive = true);
    void compileAll();
    bool registerAction(TAction* pT);
    void unregisterAction(TAction* pT);
    void reParentAction(int childID, int oldParentID, int newParentID, int parentPostion = -1, int childPosition = -1);
    void reParentAction(int childID, int oldParentID, int newParentID, TreeItemInsertMode mode, int position = 0);
    int getNewID();
    void uninstall(const QString&);
    void _uninstall(TAction* pChild, const QString& packageName);
    void doCleanup();
    void beginProcessing() { ++mProcessingDepth; }
    // Only decrements the depth - deliberately no doCleanup() here: that would
    // delete `this` (and other deferred actions) while a caller of
    // TAction::execute() may still hold the pointer. Deferred deletes are
    // flushed once no button script is executing - by the dispatchers right
    // after execute() returns and by Host's catch-all doCleanup() calls.
    void endProcessing();
    int processingDepth() const { return mProcessingDepth; }
    void updateAllToolbars();
    std::list<QPointer<TToolBar>> getToolBarList() { return mToolBarList; }
    TAction* getHeadAction(TToolBar*);
    TAction* getHeadAction(TEasyButtonBar*);
    void regenerateToolBars();
    void regenerateEasyButtonBars();
    void constructToolbar(TAction*, TToolBar* pToolBar);
    void constructToolbar(TAction*, TEasyButtonBar* pTB);
    std::pair<bool, QString> showToolBar(const QString&);
    std::pair<bool, QString> hideToolBar(const QString&);

    QList<TAction*> uninstallList;

private:
    ActionUnit() = default;

    TAction* getActionPrivate(int id);
    TAction* findEasyButtonBarAction(const QString& name);
    bool namesAFloatingToolBar(const QString& name);
    std::pair<bool, QString> setToolBarActive(const QString& name, const bool active);
    void addActionRootNode(TAction* pT, int parentPosition = -1, int childPosition = -1);
    void addAction(TAction* pT);
    void removeActionRootNode(TAction* pT);
    void removeAction(TAction*);
    QPointer<Host> mpHost;
    QMap<int, TAction*> mActionMap;
    std::list<TAction*> mActionRootNodeList;
    int mMaxID = 0;
    // > 0 whilst a TAction::execute() is on the call stack; uninstall() and
    // doCleanup() must not delete actions then - see ActionUnit::uninstall():
    int mProcessingDepth = 0;
    bool mModuleMember = false;
    std::list<QPointer<TToolBar>> mToolBarList;
    std::list<QPointer<TEasyButtonBar>> mEasyButtonBarList;
};

#endif // MUDLET_ACTIONUNIT_H
