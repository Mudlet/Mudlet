#ifndef MUDLET_ALIASUNIT_H
#define MUDLET_ALIASUNIT_H

/***************************************************************************
 *   Copyright (C) 2008-2011 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2014 by Ahmed Charles - acharles@outlook.com            *
 *   Copyright (C) 2022-2023, 2026 by Stephen Lyons                        *
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

#include <QCoreApplication>
#include <QHash>
#include <QList>
#include <QMap>
#include <QMultiMap>
#include <QPointer>
#include <QSet>
#include <QString>

#include <list>
#include <tuple>
#include <vector>

class Host;
class TAlias;

class AliasUnit
{
    Q_DECLARE_TR_FUNCTIONS(AliasUnit)

    friend class XMLexport;
    friend class XMLimport;

public:
    explicit AliasUnit(Host*);
    ~AliasUnit();

    std::list<TAlias*> getAliasRootNodeList() { return mAliasRootNodeList; }
    TAlias* getAlias(int id);
    void compileAll();
    TAlias* findFirstAlias(const QString& name);
    std::vector<int> findItems(const QString& name, const bool exactMatch, const bool caseSensitive);
    bool enableAlias(const QString&);
    bool disableAlias(const QString&);
    bool killAlias(const QString& name);
    void removeAllTempAliases();
    bool registerAlias(TAlias* pT);
    void unregisterAlias(TAlias* pT);
    void uninstall(const QString&);
    void _uninstall(TAlias* pChild, const QString& packageName);
    void reParentAlias(int childID, int oldParentID, int newParentID, int parentPosition = -1, int childPosition = -1);
    void reParentAlias(int childID, int oldParentID, int newParentID, TreeItemInsertMode mode, int position = 0);
    bool processDataStream(const QString&);
    void stopAllTriggers();
    void reenableAllTriggers();
    std::tuple<QString, int, int, int> assembleReport();
    int getNewID();
    void markCleanup(TAlias* pT);
    void doCleanup();
    int processingDepth() const { return mProcessingDepth; }
    // Each nested alias expansion is a C++ stack frame; past this depth the
    // command goes to the game unexpanded.
    inline static const int scmMaxProcessingDepth = 50;

    QMultiMap<QString, TAlias*> mLookupTable;
    QSet<TAlias*> mCleanupSet;
    QList<TAlias*> uninstallList;
    bool hasPendingDeletes() const { return !mCleanupSet.isEmpty() || !uninstallList.isEmpty(); }
    // Killed or uninstalled, itself or through an ancestor, but still in the lookup tables: doCleanup() runs only
    // on the next line, timer flush or temp purge, so on an idle profile that can be a minute away
    bool pendingDeletion(TAlias* pItem) const;


private:
    AliasUnit() = default;

    void resetStats();
    void assembleReport(TAlias*);
    TAlias* getAliasPrivate(int id);
    void addAliasRootNode(TAlias* pT, int parentPosition = -1, int childPosition = -1, bool moveAlias = false);
    void addAlias(TAlias* pT);
    void removeAliasRootNode(TAlias* pT);
    void listRootNode(TAlias* pT, std::list<TAlias*>::iterator before);
    void unlistRootNode(TAlias* pT);
    void removeAlias(TAlias*);

    QPointer<Host> mpHost;
    QMap<int, TAlias*> mAliasMap;
    std::list<TAlias*> mAliasRootNodeList;
    // Where each root node sits in mAliasRootNodeList: std::list::remove() walks the whole list,
    // which made freeing a batch of temporary aliases quadratic
    QHash<TAlias*, std::list<TAlias*>::iterator> mRootNodePositions;
    int mMaxID = 0;
    bool mModuleMember = false;
    int statsItemsTotal = 0;
    int statsTempItems = 0;
    int statsActiveItems = 0;
    // Counter for nested processing; cleanup deferred until 0
    int mProcessingDepth = 0;
    // Set once scmMaxProcessingDepth is reached, cleared when the expansion that started the
    // chain returns to the outermost pass, so other aliases on that command can still expand.
    // An alias that expands into itself twice branches at every level, so refusing only the
    // expansions at the limit still leaves 2^50 of them to run below it.
    bool mRunawayExpansionStopped = false;
};

#endif // MUDLET_ALIASUNIT_H
