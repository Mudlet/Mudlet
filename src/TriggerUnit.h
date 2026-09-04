#ifndef MUDLET_TRIGGERUNIT_H
#define MUDLET_TRIGGERUNIT_H

/***************************************************************************
 *   Copyright (C) 2008-2013 by Heiko Koehn - KoehnHeiko@googlemail.com    *
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


#include "TTriggerPrescan.h"
#include "utils.h"

#include <QByteArray>
#include <QCoreApplication>
#include <QElapsedTimer>
#include <QHash>
#include <QMultiMap>
#include <QPointer>
#include <QSet>
#include <QString>

#include <list>
#include <memory>
#include <vector>

class Host;
class TTrigger;

class TriggerUnit
{
    Q_DECLARE_TR_FUNCTIONS(TriggerUnit) // Needed so we can use tr() even though TriggerUnit is NOT derived from QObject
    friend class XMLexport;
    friend class XMLimport;

public:
    explicit TriggerUnit(Host*);
    ~TriggerUnit();

    std::list<TTrigger*> getTriggerRootNodeList() { return mTriggerRootNodeList; }

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
    // Enum-based API for clear insertion mode specification
    void reParentTrigger(int childID, int oldParentID, int newParentID, TreeItemInsertMode mode, int position = 0);
    // Legacy integer-based position API - delegates to enum-based version
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
    // Called by anything that changes whether a trigger can be ruled out of a
    // line by its text alone.
    void markPrescanStale() { mRootNodeSnapshotStale = true; }
    // As above, but for the changes that make a trigger fire without matching
    // text. Those have to reach the line already being processed, whose
    // candidate list was settled before the change - see processDataStream().
    void markRootUnfilterable()
    {
        ++mUnfilterableEpoch;
        mRootNodeSnapshotStale = true;
    }
    void doCleanup();
    void uninstall(const QString&);
    void _uninstall(TTrigger* pChild, const QString& packageName);

    int processingDepth() const { return mProcessingDepth; }
    // Raw pointer is safe: a trigger outlives its own execute() frame, as deletion
    // is deferred to doCleanup() once mProcessingDepth returns to 0.
    const QString* currentExecutingTriggerName() const { return mpCurrentExecutingTriggerName; }
    void setCurrentExecutingTriggerName(const QString* pName) { mpCurrentExecutingTriggerName = pName; }
    // The same-line creation lineage of the root of the trigger whose script is
    // running, so a trigger it creates joins that lineage rather than starting
    // one - see registerTrigger(). Zero while no trigger script is running (an
    // alias or a timer counts as none), or while the running one predates the
    // line being processed.
    int currentSameLineChainId() const { return mCurrentSameLineChainId; }
    int currentSameLineGeneration() const { return mCurrentSameLineGeneration; }
    void setCurrentSameLineChain(const int chainId, const int generation)
    {
        mCurrentSameLineChainId = chainId;
        mCurrentSameLineGeneration = generation;
    }
    // Turns an endless self-feeding-trigger loop into a catchable Lua error before
    // it overflows the stack. Sized for the smallest platform stack (~1MB on
    // Windows, where the original crash hit before Lua's own 200-C-call guard):
    // a few times any legitimate nesting, comfortably below the native limit.
    inline static const int scmMaxProcessingDepth = 50;
    // How many creations deep one lineage of same-line creations may go while a
    // single line is processed. Separate from the depth above, which measures the
    // C stack: nothing recurses here, it is the list processDataStream() walks
    // that grows. Generations rather than a head count is what separates the two
    // shapes: a script arming a batch produces one generation however big the
    // batch, while a trigger that re-creates itself adds a generation per round
    // and is the only thing that can go on forever. 1000 is far past any chain a
    // real script builds.
    inline static const int scmMaxSameLineGenerations = 1000;
    // Generations alone do not bound what one line costs: a lineage that widens
    // as it deepens multiplies. Past this many creations new triggers stop being
    // offered the line, and since matching is what makes them create more, that
    // ends the growth. Nothing is stopped or disowned here - all of them are
    // still armed for the lines that follow - so it can sit well clear of any
    // legitimate batch.
    inline static const qsizetype scmMaxSameLineCreationsPerLine = 20000;

    QList<TTrigger*> uninstallList;
    bool hasPendingDeletes() const { return !mCleanupSet.isEmpty() || !uninstallList.isEmpty(); }

private:
    TriggerUnit() = default;
    void assembleReport(TTrigger*);
    TTrigger* getTriggerPrivate(int id);
    void addTriggerRootNode(TTrigger* pT, int parentPosition = -1, int childPosition = -1, bool moveTrigger = false);
    void addTrigger(TTrigger* pT);
    void removeTriggerRootNode(TTrigger* pT);
    void removeTrigger(TTrigger*);
    void startOrExtendSameLineChain(TTrigger* pT);
    void stopSameLineCreationLoop(const int chainId);

    QPointer<Host> mpHost;
    // Storage processDataStream() lends out for the UTF-8 form of the line it is
    // matching, kept between lines for its capacity alone - it holds nothing
    // meaningful outside that call. Past this size the capacity is dropped
    // instead of kept, so one outsized line cannot hold its allocation for the
    // rest of the session; the bound is three bytes per QChar of a line longer
    // than any game sends.
    static constexpr qsizetype scmMaxRetainedUtf8Scratch = 3 * 8192;
    QByteArray mUtf8Scratch;
    QMap<int, TTrigger*> mTriggerMap;
    std::list<TTrigger*> mTriggerRootNodeList;
    // What processDataStream() iterates instead of mTriggerRootNodeList itself -
    // see the note there. Shared rather than rebuilt per line: a pass pins the
    // snapshot that was current when it started, so mutating the root list
    // mid-pass leaves that one alone and only the next pass sees the rebuilt
    // one. Every mutation of mTriggerRootNodeList must set the flag below, or a
    // pass would go on walking triggers that have since been freed.
    // The prescan files triggers by their position in the snapshot, so the two
    // are rebuilt and pinned together.
    struct RootNodeSnapshot
    {
        std::vector<TTrigger*> mNodes;
        TTriggerPrescan mPrescan;
    };
    std::shared_ptr<RootNodeSnapshot> mpRootNodeSnapshot;
    bool mRootNodeSnapshotStale = true;
    std::vector<int> mCandidateScratch;
    std::vector<int> mCandidates;
    quint32 mUnfilterableEpoch = 0;
    int mMaxID;
    bool mModuleMember;
    int statsItemsTotal = 0;
    int statsTempItems = 0;
    int statsActiveItems = 0;
    int statsPatternsTotal = 0;
    int statsPatternsActive = 0;
    // Counter for nested processing; cleanup deferred until 0
    int mProcessingDepth = 0;
    const QString* mpCurrentExecutingTriggerName = nullptr;
    // Root triggers registered while processDataStream() is running, so each
    // pass can match the ones created during it against the line being
    // processed - see processDataStream(). Cleared once the outermost pass ends.
    QList<TTrigger*> mRootNodesAddedWhileProcessing;
    // The name of the trigger whose script started each same-line creation
    // lineage, for the message when one runs away. Keyed by chain id, so a
    // trigger dying mid-line cannot leave a stale pointer behind. Cleared once
    // the outermost pass ends.
    QHash<int, QString> mSameLineChainStarters;
    int mCurrentSameLineChainId = 0;
    int mCurrentSameLineGeneration = 0;
    // Handed out monotonically and never deliberately recycled: an id that
    // outlived the pass it was given out in would otherwise be misfiled under a
    // later lineage. Zero means "none".
    int mLastSameLineChainId = 0;
    QElapsedTimer mSameLineLoopReportTimer;
};

#endif // MUDLET_TRIGGERUNIT_H
