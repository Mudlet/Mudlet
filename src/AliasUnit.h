#ifndef MUDLET_ALIASUNIT_H
#define MUDLET_ALIASUNIT_H

/***************************************************************************
 *   Copyright (C) 2008-2011 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2014 by Ahmed Charles - acharles@outlook.com            *
 *   Copyright (C) 2022-2023 by Stephen Lyons - slysven@virginmedia.com    *
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


#include <QMultiMap>
#include <QPointer>
#include <QSet>
#include <QString>

#include <list>

class Host;
class TAlias;


class AliasUnit
{
    friend class XMLexport;
    friend class XMLimport;

public:
    explicit AliasUnit(Host* pHost)
    : mpHost(pHost)
    {}
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
    bool processDataStream(const QString&);
    void stopAllTriggers();
    void reenableAllTriggers();
    std::tuple<QString, int, int, int> assembleReport();
    int getNewID();
    void markCleanup(TAlias* pT);
    void doCleanup();

    QMultiMap<QString, TAlias*> mLookupTable;
    QSet<TAlias*> mCleanupSet;
    QList<TAlias*> uninstallList;


private:
    AliasUnit() = default;

    void resetStats();
    void assembleReport(TAlias*);
    TAlias* getAliasPrivate(int id);
    void addAliasRootNode(TAlias* pT, int parentPosition = -1, int childPosition = -1, bool moveAlias = false);
    void addAlias(TAlias* pT);
    void removeAliasRootNode(TAlias* pT);
    void removeAlias(TAlias*);

    QPointer<Host> mpHost;
    QMap<int, TAlias*> mAliasMap;
    std::list<TAlias*> mAliasRootNodeList;
    int mMaxID = 0;
    bool mModuleMember = false;
    int statsItemsTotal = 0;
    int statsTempItems = 0;
    int statsActiveItems = 0;
};

#endif // MUDLET_ALIASUNIT_H
