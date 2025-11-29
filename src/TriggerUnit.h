#ifndef MUDLET_TRIGGERUNIT_H
#define MUDLET_TRIGGERUNIT_H

/***************************************************************************
 *   Copyright (C) 2008-2013 by Heiko Koehn - KoehnHeiko@googlemail.com    *
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
class TTrigger;


class TriggerUnit
{
    friend class XMLexport;
    friend class XMLimport;

public:
    explicit TriggerUnit(Host* pHost)
    : mpHost(pHost)
    , mMaxID(0)
    , mModuleMember()
    {}
    ~TriggerUnit();

    std::list<TTrigger*> getTriggerRootNodeList()
    {
        return mTriggerRootNodeList;
    }

    void resetStats();
    TTrigger* getTrigger(int id);
    void removeAllTempTriggers();
    void reorderTriggersAfterPackageImport();
    TTrigger* findTrigger(const QString&);
    std::vector<int> findItems(const QString& name, const bool exactMatch = true, const bool caseSensitive = true);
    bool enableTrigger(const QString&);
    bool disableTrigger(const QString&);
    bool killTrigger(const QString& name);
    bool registerTrigger(TTrigger* pT);
    void unregisterTrigger(TTrigger* pT);
    void reParentTrigger(int childID, int oldParentID, int newParentID, int parentPosition = -1, int childPosition = -1);
    void processDataStream(const QString&, int);
    void compileAll();
    void setTriggerStayOpen(const QString&, int);
    void stopAllTriggers();
    void reenableAllTriggers();
    std::tuple<QString, int, int, int, int, int> assembleReport();
    QSet<TTrigger*> mCleanupSet;
    int getNewID();
    QMultiMap<QString, TTrigger*> mLookupTable;
    void markCleanup(TTrigger* pT);
    void doCleanup();
    void uninstall(const QString&);
    void _uninstall(TTrigger* pChild, const QString& packageName);

    QList<TTrigger*> uninstallList;

private:
    TriggerUnit() = default;
    void assembleReport(TTrigger*);
    TTrigger* getTriggerPrivate(int id);
    void addTriggerRootNode(TTrigger* pT, int parentPosition = -1, int childPosition = -1, bool moveTrigger = false);
    void addTrigger(TTrigger* pT);
    void removeTriggerRootNode(TTrigger* pT);
    void removeTrigger(TTrigger*);

    QPointer<Host> mpHost;
    QMap<int, TTrigger*> mTriggerMap;
    std::list<TTrigger*> mTriggerRootNodeList;
    int mMaxID;
    bool mModuleMember;
    int statsItemsTotal = 0;
    int statsTempItems = 0;
    int statsActiveItems = 0;
    int statsPatternsTotal = 0;
    int statsPatternsActive = 0;
    bool mIsProcessing = false;
};

#endif // MUDLET_TRIGGERUNIT_H
