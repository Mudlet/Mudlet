#ifndef MUDLET_ACTIONUNIT_H
#define MUDLET_ACTIONUNIT_H

/***************************************************************************
 *   Copyright (C) 2008-2011 by Heiko Koehn - KoehnHeiko@googlemail.com    *
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


#include "pre_guard.h"
#include <QMap>
#include <QMutex>
#include <QPointer>
#include <QString>
#include "post_guard.h"

#include <list>

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
    ActionUnit(Host* pHost) : mpHost(pHost), mMaxID(0), mpToolBar(), mpEasyButtonBar(), mModuleMember() {}

    std::list<TAction*> getActionRootNodeList()
    {
        QMutexLocker locker(&mActionUnitLock);
        return mActionRootNodeList;
    }

    TAction* getAction(int id);
    TAction* findAction(const QString&);
    std::vector<TAction*> findActionsByName(const QString&);
    void compileAll();
    bool registerAction(TAction* pT);
    void unregisterAction(TAction* pT);
    void reParentAction(int childID, int oldParentID, int newParentID, int parentPostion = -1, int childPosition = -1);
    int getNewID();
    void uninstall(const QString&);
    void _uninstall(TAction* pChild, const QString& packageName);
    void updateToolbar();
    std::list<QPointer<TToolBar>> getToolBarList();
    std::list<QPointer<TEasyButtonBar>> getEasyButtonBarList();
    TAction* getHeadAction(TToolBar*);
    TAction* getHeadAction(TEasyButtonBar*);
    void constructToolbar(TAction*, TToolBar* pTB);
    void constructToolbar(TAction*, TEasyButtonBar* pTB);
    void showToolBar(const QString&);
    void hideToolBar(const QString&);

    QMutex mActionUnitLock;
    QList<TAction*> uninstallList;

private:
    ActionUnit() {}
    TAction* getActionPrivate(int id);
    void addActionRootNode(TAction* pT, int parentPosition = -1, int childPosition = -1);
    void addAction(TAction* pT);
    void removeActionRootNode(TAction* pT);
    void removeAction(TAction*);
    QPointer<Host> mpHost;
    QMap<int, TAction*> mActionMap;
    std::list<TAction*> mActionRootNodeList;
    int mMaxID;
    QPointer<TToolBar> mpToolBar;
    QPointer<TEasyButtonBar> mpEasyButtonBar;
    bool mModuleMember;
    std::list<QPointer<TToolBar>> mToolBarList;
    std::list<QPointer<TEasyButtonBar>> mEasyButtonBarList;
};

#endif // MUDLET_ACTIONUNIT_H
