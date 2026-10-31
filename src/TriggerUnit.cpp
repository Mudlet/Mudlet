/***************************************************************************
 *   Copyright (C) 2008-2013 by Heiko Koehn - KoehnHeiko@googlemail.com    *
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


#include "TriggerUnit.h"


#include "Host.h"
#include "TConsoleModel.h"
#include "TTrigger.h"
#include "TriggerMatchPool.h"

#include <QScopeGuard>

#include <algorithm>
#include <chrono>
#include <functional>
#include <iterator>
#include <limits>
#include <vector>

/* We need an explicit constructor in this file as the Host class is forward
 * declared in the header file and it is problematic to define any dereferencing
 * of it there:*/
TriggerUnit::TriggerUnit(Host* pHost)
: mpHost(pHost)
, mMaxID(0)
, mModuleMember()
{
}

TriggerUnit::~TriggerUnit()
{
    // Set mpHost to null on all triggers (including children) to prevent them from trying to
    // unregister themselves during destruction (which would modify the list
    // we're iterating over and cause iterator invalidation)
    for (auto trigger : mTriggerRootNodeList) {
        trigger->mpHost = nullptr;
        // Also set mpHost to null on all children recursively
        std::function<void(TTrigger*)> nullifyChildren = [&nullifyChildren](TTrigger* t) {
            for (auto* childNode : *t->mpMyChildrenList) {
                auto* child = static_cast<TTrigger*>(childNode);
                child->mpHost = nullptr;
                nullifyChildren(child);
            }
        };
        nullifyChildren(trigger);
    }
    for (auto trigger : mTriggerRootNodeList) {
        delete trigger;
    }
}

void TriggerUnit::resetStats()
{
    statsItemsTotal = 0;
    statsTempItems = 0;
    statsActiveItems = 0;
    statsPatternsTotal = 0;
    statsPatternsActive = 0;
}

void TriggerUnit::_uninstall(TTrigger* pChild, const QString& packageName)
{
    std::list<Tree<TTrigger>*>* childrenList = pChild->mpMyChildrenList;
    for (auto* triggerNode : *childrenList) {
        auto* trigger = static_cast<TTrigger*>(triggerNode);
        _uninstall(trigger, packageName);
        uninstallList.append(trigger);
    }
}


void TriggerUnit::uninstall(const QString& packageName)
{
    for (auto rootTrigger : mTriggerRootNodeList) {
        if (rootTrigger->mPackageName == packageName) {
            _uninstall(rootTrigger, packageName);
            uninstallList.append(rootTrigger);
        }
    }
    // Re-entrant uninstall (#9337): a trigger's own script (e.g. uninstallPackage())
    // is removing its package while match()/processDataStream() are still on the
    // stack for that trigger. Deleting now would be a use-after-free, so defer to
    // doCleanup() at depth 0. Deactivating is enough to stop them firing for the
    // rest of this pass: processDataStream()'s loop skips deactivated triggers
    // and match() runs its whole body inside if (isActive()) for those reached
    // via a parent chain or filter.
    if (mProcessingDepth > 0) {
        for (auto trigger : uninstallList) {
            trigger->setIsActive(false);
            mCleanupSet.remove(trigger); // keep the two deferred-delete paths disjoint
        }
        return;
    }
    for (auto& trigger : uninstallList) {
        // in case the trigger was also queued for the markCleanup()/doCleanup()
        // path - deleting it here would otherwise leave a dangling pointer there:
        mCleanupSet.remove(trigger);
        delete trigger;
    }
    uninstallList.clear();
}

void TriggerUnit::removeAllTempTriggers()
{
    for (auto trigger : mTriggerRootNodeList) {
        if (trigger->isTemporary()) {
            trigger->setIsActive(false);
            markCleanup(trigger);
        }
    }
}

void TriggerUnit::addTriggerRootNode(TTrigger* pT, int parentPosition, int childPosition, bool moveTrigger)
{
    if (!pT) {
        return;
    }
    if (!pT->getID()) {
        pT->setID(getNewID());
    }
    if ((parentPosition == -1) || (childPosition >= static_cast<int>(mTriggerRootNodeList.size()))) {
        if (listRootNode(pT, mTriggerRootNodeList.end())) {
            markRootNodeAppended(pT);
        }
    } else {
        markRootNodeListReordered();
        // insert item at proper position
        int cnt = 0;
        for (auto it = mTriggerRootNodeList.begin(); it != mTriggerRootNodeList.end(); it++) {
            if (cnt >= childPosition) {
                listRootNode(pT, it);
                break;
            }
            cnt++;
        }
    }

    if (!moveTrigger) {
        mTriggerMap.insert(pT->getID(), pT);
    }
}

// Enum-based reParentTrigger implementation
void TriggerUnit::reParentTrigger(int childID, int oldParentID, int newParentID, TreeItemInsertMode mode, int position)
{
    TTrigger* pOldParent = getTriggerPrivate(oldParentID);
    TTrigger* pNewParent = getTriggerPrivate(newParentID);
    TTrigger* pChild = getTriggerPrivate(childID);

    if (!pChild) {
        return;
    }

    // Changes the prescan list: a trigger under a chain matches its parent's capture, not the line
    TTrigger::bumpStructureGeneration();
    unfileTreeNode(pChild);

    if (pOldParent) {
        pOldParent->popChild(pChild);
    } else {
        markRootNodeRemoved(pChild);
        unlistRootNode(pChild);
    }

    // Convert enum mode to the internal flags
    int parentPosition = (mode == TreeItemInsertMode::AtPosition) ? 0 : -1;
    int childPosition = (mode == TreeItemInsertMode::AtPosition) ? position : -1;

    if (pNewParent) {
        pNewParent->addChild(pChild, parentPosition, childPosition);
        pChild->setParent(pNewParent);
        invalidateChildWalks(pNewParent);
    } else {
        pChild->Tree<TTrigger>::setParent(nullptr);
        addTriggerRootNode(pChild, parentPosition, childPosition, true);
    }
}

// Legacy integer-based reParentTrigger - delegates to enum-based version
void TriggerUnit::reParentTrigger(int childID, int oldParentID, int newParentID, int parentPosition, int childPosition)
{
    if (parentPosition == -1 || childPosition == -1) {
        reParentTrigger(childID, oldParentID, newParentID, TreeItemInsertMode::Append, 0);
    } else {
        reParentTrigger(childID, oldParentID, newParentID, TreeItemInsertMode::AtPosition, childPosition);
    }
}

void TriggerUnit::removeTriggerRootNode(TTrigger* pT)
{
    if (!pT) {
        return;
    }
    // Names are not unique - the lookup table is a QMultiMap - so drop this one
    // trigger's entry rather than every entry filed under the name. The
    // single-argument remove() used to be taken for temporary triggers, which
    // evicted live same-named triggers and left them unreachable by name for the
    // rest of the session (tempComplexRegexTrigger() takes a user-supplied name,
    // so a collision needs no coincidence)
    mLookupTable.remove(pT->getName(), pT);
    mTriggerMap.remove(pT->getID());
    markRootNodeRemoved(pT);
    unlistRootNode(pT);
}

TTrigger* TriggerUnit::getTrigger(int id)
{
    if (mTriggerMap.find(id) != mTriggerMap.end()) {
        return mTriggerMap.value(id);
    }
    return nullptr;
}

TTrigger* TriggerUnit::getTriggerPrivate(int id)
{
    if (mTriggerMap.find(id) != mTriggerMap.end()) {
        return mTriggerMap.value(id);
    }
    return nullptr;
}

bool TriggerUnit::registerTrigger(TTrigger* pT)
{
    if (!pT) {
        return false;
    }

    if (pT->getParent()) {
        addTrigger(pT);
        return true;
    }
    addTriggerRootNode(pT);
    if (mProcessingDepth > 0) {
        mRootNodesAddedWhileProcessing.append(pT);
        startOrExtendSameLineChain(pT);
    }
    return true;
}

// A trigger created by a trigger that was itself created while this line was
// being processed joins that trigger's lineage, one generation further down;
// anything created from a script that predates the line starts a lineage of its
// own at generation one. So a script arming a batch produces a generation of
// one-deep lineages however big the batch, while a trigger that re-creates
// itself keeps adding generations to a single lineage.
void TriggerUnit::startOrExtendSameLineChain(TTrigger* pT)
{
    int chainId = mCurrentSameLineChainId;
    if (!chainId) {
        if (mLastSameLineChainId == std::numeric_limits<int>::max()) {
            mLastSameLineChainId = 0;
        }
        chainId = ++mLastSameLineChainId;
        mSameLineChainStarters.insert(chainId, mpCurrentExecutingTriggerName ? *mpCurrentExecutingTriggerName : QString());
    }
    pT->setSameLineChain(chainId, mCurrentSameLineGeneration + 1);
}

void TriggerUnit::unregisterTrigger(TTrigger* pT)
{
    if (!pT) {
        return;
    }
    // A node can be removed and deleted mid-pass without going through the
    // deferred-cleanup paths (e.g. XMLimport discarding its placeholder trigger
    // when installPackage() runs from a trigger script), so it must not linger in
    // the same-line match list. Done here rather than in removeTriggerRootNode()
    // because a trigger that was a root node when it was added to that list can
    // have been reparented since, which routes it to removeTrigger() instead.
    // Null the slot instead of compacting: processDataStream() may be walking the
    // list by index right now, and shifting entries under it would skip a
    // trigger's same-line match. Nulling it also takes the trigger out of reach
    // of the end-of-pass reset, so drop its lineage here instead.
    std::replace(mRootNodesAddedWhileProcessing.begin(), mRootNodesAddedWhileProcessing.end(), pT, static_cast<TTrigger*>(nullptr));
    pT->setSameLineChain(0, 0);
    // Here rather than in ~TTrigger(): the editor deletes a trigger with its mpHost nulled
    forgetOpenTreeNode(pT);
    if (pT->mTreeIndexPosition >= 0) {
        unfileTreeNode(pT);
    }
    if (pT->getParent()) {
        invalidateChildWalks(pT->getParent());
        removeTrigger(pT);
        return;
    }
    removeTriggerRootNode(pT);
}


void TriggerUnit::addTrigger(TTrigger* pT)
{
    if (!pT) {
        return;
    }

    if (!pT->getID()) {
        pT->setID(getNewID());
    }

    mTriggerMap.insert(pT->getID(), pT);
    mTreeIndexStale = true;
}

void TriggerUnit::removeTrigger(TTrigger* pT)
{
    if (!pT) {
        return;
    }
    // see removeTriggerRootNode(): one entry, not every same-named one
    mLookupTable.remove(pT->getName(), pT);

    mTriggerMap.remove(pT->getID());
}

// trigger matching order is permanent trigger objects first, temporary objects second
// after package import or module sync this order needs to be reset
void TriggerUnit::reorderTriggersAfterPackageImport()
{
    QList<TTrigger*> tempList;
    for (auto trigger : mTriggerRootNodeList) {
        if (trigger->isTemporary()) {
            tempList.push_back(trigger);
        }
    }
    markRootNodeListReordered();
    for (auto& trigger : tempList) {
        unlistRootNode(trigger);
    }
    for (auto& trigger : tempList) {
        listRootNode(trigger, mTriggerRootNodeList.end());
    }
}

bool TriggerUnit::listRootNode(TTrigger* pT, std::list<TTrigger*>::iterator before)
{
    if (mRootNodePositions.contains(pT)) {
        return false;
    }
    mRootNodePositions.insert(pT, mTriggerRootNodeList.insert(before, pT));
    return true;
}

void TriggerUnit::unlistRootNode(TTrigger* pT)
{
    const auto position = mRootNodePositions.constFind(pT);
    if (position != mRootNodePositions.cend()) {
        mTriggerRootNodeList.erase(position.value());
        mRootNodePositions.erase(position);
    }
}

int TriggerUnit::getNewID()
{
    return ++mMaxID;
}

// Stopping the pass is not enough: what the lineage created is still live and
// still matching, so the next line would start with a budget's worth of them and
// each would spawn a budget's worth again, costing a multiple of the line before
// it. Only the runaway lineage is disowned - a capture trigger an unrelated
// script armed on the same line belongs to a lineage of its own and is left
// alone. The whole list is scanned rather than the tail of this pass: a lineage
// started in an outer pass can go on growing inside a nested feedTriggers() pass,
// and when that nested pass is the one to trip, the earlier members sit below its
// first-node index. Permanent triggers get deactivate() and not setIsActive(false),
// which would clear the user-active state XMLexport saves and leave them switched
// off after a restart.
void TriggerUnit::stopSameLineCreationLoop(const int chainId)
{
    int killedCount = 0;
    int deactivatedCount = 0;
    for (auto trigger : std::as_const(mRootNodesAddedWhileProcessing)) {
        if (!trigger || trigger->sameLineChainId() != chainId) {
            continue;
        }
        if (trigger->isTemporary()) {
            trigger->setIsActive(false);
            markCleanup(trigger);
            ++killedCount;
        } else {
            trigger->deactivate();
            ++deactivatedCount;
        }
    }
    const QString triggerName = mSameLineChainStarters.value(chainId);

    qWarning().nospace() << "TriggerUnit::processDataStream(...) aborting: one lineage of triggers created while processing a line reached " << scmMaxSameLineGenerations
                         << " generations - probably a trigger that re-creates itself. Profile: " << (mpHost ? mpHost->getName() : QString()) << ", triggers removed: " << killedCount
                         << ", deactivated: " << deactivatedCount << ", lineage started by: " << triggerName;
    if (!mpHost) {
        return;
    }
    // A runaway whose creator outlives the line trips on every matching line and
    // would bury the game text; the qWarning() above is not throttled.
    constexpr qint64 reportIntervalMs = 10000;
    if (mSameLineLoopReportTimer.isValid() && mSameLineLoopReportTimer.elapsed() < reportIntervalMs) {
        return;
    }
    mSameLineLoopReportTimer.start();

    //: %n is a count of triggers. Shown in the game window when a trigger keeps creating new triggers that match the same line, which would otherwise never end
    const QString created = tr("%n trigger(s) created while processing this line have been stopped: temporary ones removed, permanent ones switched off until the profile is reloaded.",
                               nullptr,
                               killedCount + deactivatedCount);
    if (triggerName.isEmpty()) {
        //: %1 is the sentence above, about the triggers that were stopped
        mpHost->postMessage(tr("[ ERROR ] - Trigger processing stopped to prevent a freeze: a trigger (or another trigger it creates) keeps creating new triggers that match the line being "
                               "processed, so that line never finishes. %1 Create the trigger once, outside its own script, or give it a pattern that does not match the line it is created on.")
                                    .arg(created));
        return;
    }
    //: %1 is the name of a trigger - the name of a trigger made by tempTrigger() and friends is its id number - and %2 is the sentence above, about the triggers that were stopped
    mpHost->postMessage(tr("[ ERROR ] - Trigger processing stopped to prevent a freeze: trigger '%1' (or another trigger it creates) keeps creating new triggers that match the line being "
                           "processed, so that line never finishes. %2 Create the trigger once, outside its own script, or give it a pattern that does not match the line it is created on.")
                                .arg(triggerName, created));
}

// Only Perl regex triggers are worth sending to the pool: the bigram filter dismisses a substring
// pattern in a few instructions and a begin-of-line or exact pattern takes one comparison, cheaper
// than the cross-thread traffic. The walk stops at a filter chain: ruling it out rules out its
// children, which match against its captures, so a verdict against the line would not apply to them.
void TriggerUnit::collectPrescanTasks(TTrigger* pT)
{
    if (pT->getRegexCodePropertyList().contains(REGEX_PERL)) {
        pT->createPrescanMatchData();
        mPrescanTasks.push_back(pT);
    }
    if (pT->isFilterChain()) {
        return;
    }
    for (auto* childNode : *pT->mpMyChildrenList) {
        collectPrescanTasks(static_cast<TTrigger*>(childNode));
    }
}

// Must rebuild when the tree changed shape or a pattern was recompiled, or an entry could name a freed trigger
void TriggerUnit::rebuildPrescanTasksIfStale()
{
    const quint64 generation = TTrigger::structureGeneration();
    if (generation == mPrescanTasksGeneration) {
        return;
    }
    mPrescanTasks.clear();
    for (auto trigger : mTriggerRootNodeList) {
        collectPrescanTasks(trigger);
    }
    mPrescanTasksGeneration = generation;
}

TriggerUnit::TreeIndexMode TriggerUnit::smTreeIndexMode = TriggerUnit::readTreeIndexMode();

TriggerUnit::TreeIndexMode TriggerUnit::readTreeIndexMode()
{
    const QString value = qEnvironmentVariable("MUDLET_TREE_INDEX");
    if (value == QLatin1String("0")) {
        return TreeIndexMode::Off;
    }
    if (value == QLatin1String("verify")) {
        return TreeIndexMode::Verify;
    }
    return TreeIndexMode::On;
}

// True when none of pT's children has to be entered on every line
bool TriggerUnit::fileTreeNodes(TTrigger* pT, std::vector<quint64>& grams)
{
    const auto& children = *pT->mpMyChildrenList;
    if (children.empty()) {
        pT->mpChildWalk.reset();
        return true;
    }
    if (!pT->mpChildWalk) {
        pT->mpChildWalk = std::make_unique<TChildWalk>();
    }
    TChildWalk& walk = *pT->mpChildWalk;
    const size_t words = (children.size() + 63) / 64;
    walk.mChildren.clear();
    walk.mAlways.assign(words, 0);
    walk.mMarked.assign(words, 0);
    walk.mMarkedStamp = 0;
    bool quiet = true;
    for (auto* childNode : children) {
        auto* child = static_cast<TTrigger*>(childNode);
        const int position = static_cast<int>(walk.mChildren.size());
        child->mChildWalkPosition = position;
        walk.mChildren.push_back(child);
        // A root is not noted when it opens, so one the editor moved below another trigger is noted here
        if (child->actsWithoutMatching() && !child->mInOpenTreeNodes) {
            child->mInOpenTreeNodes = true;
            mOpenTreeNodes.push_back(child);
        }
        const bool filed = child->treeIndexGrams(grams);
        if (filed) {
            child->mTreeIndexPosition = fileChildSlot(child, grams);
            mChildSlots.push_back(child->mTreeIndexPosition);
        }
        const bool childrenQuiet = fileTreeNodes(child, grams);
        // Entering a folder only walks its children; one held open is marked through mOpenTreeNodes
        const bool folder = child->mPatterns.isEmpty() && !child->isLineTrigger() && !child->isMultiline();
        if (!filed && !(folder && childrenQuiet)) {
            walk.mAlways[position / 64] |= quint64(1) << (position % 64);
            quiet = false;
        }
    }
    walk.mValid = true;
    return quiet;
}

void TriggerUnit::dropChildWalks(TTrigger* pT)
{
    pT->mpChildWalk.reset();
    for (auto* child : *pT->mpMyChildrenList) {
        dropChildWalks(static_cast<TTrigger*>(child));
    }
}

void TriggerUnit::invalidateChildWalks(TTrigger* pFrom)
{
    for (TTrigger* node = pFrom; node; node = node->getParent()) {
        if (node->mpChildWalk) {
            node->mpChildWalk->mValid = false;
        }
    }
    mTreeIndexStale = true;
}

// Marks pT for its parent's walk on this line, and each trigger above it for its own parent's
void TriggerUnit::markTreeNode(TTrigger* pT, const quint32 stamp)
{
    for (TTrigger* parent = pT->getParent(); parent; pT = parent, parent = parent->getParent()) {
        TChildWalk* walk = parent->mpChildWalk.get();
        const int position = pT->mChildWalkPosition;
        if (!walk || position < 0 || position >= static_cast<int>(walk->mChildren.size()) || walk->mChildren[position] != pT) {
            continue;
        }
        if (!walk->mark(position, stamp)) {
            // Every marking goes all the way up, so the rest of the way is marked already
            return;
        }
    }
}

void TriggerUnit::noteOpenTreeNode(TTrigger* pT)
{
    if (!pT->getParent()) {
        return;
    }
    if (!pT->mInOpenTreeNodes) {
        pT->mInOpenTreeNodes = true;
        mOpenTreeNodes.push_back(pT);
    }
    // Mark the line in flight too: its walk may not have reached pT yet, and pT now acts on it
    markTreeNode(pT, TTrigger::lineStamp());
}

void TriggerUnit::forgetOpenTreeNode(TTrigger* pT)
{
    if (pT->mInOpenTreeNodes) {
        pT->mInOpenTreeNodes = false;
        std::erase(mOpenTreeNodes, pT);
    }
}

void TriggerUnit::freeChildSlots(const std::vector<int>& freed)
{
    if (freed.empty()) {
        return;
    }
    for (const int slot : freed) {
        if (TTrigger* node = mIndexSlots[slot].mpTrigger) {
            node->mTreeIndexPosition = -1;
        }
        mIndexSlots[slot] = IndexSlot{};
    }
    mTreeIndex.removeSlots(freed);
}

void TriggerUnit::clearTreeNodes()
{
    freeChildSlots(mChildSlots);
    mChildSlots.clear();
}

void TriggerUnit::rebuildTreeIndex()
{
    std::vector<int> previous;
    previous.swap(mChildSlots);
    std::vector<quint64> grams;
    for (auto* root : mTriggerRootNodeList) {
        fileTreeNodes(root, grams);
    }
    std::sort(previous.begin(), previous.end());
    std::sort(mChildSlots.begin(), mChildSlots.end());
    std::vector<int> unused;
    std::set_difference(previous.cbegin(), previous.cend(), mChildSlots.cbegin(), mChildSlots.cend(), std::back_inserter(unused));
    freeChildSlots(unused);
    mTreeIndexActive = static_cast<int>(mChildSlots.size()) >= scmMinimumIndexedTriggers;
    if (!mTreeIndexActive) {
        // Nothing stamps, so nothing may be skipped
        clearTreeNodes();
        for (auto* root : mTriggerRootNodeList) {
            dropChildWalks(root);
        }
    }
    mTreeIndexStale = false;
}

// A child keeps its slot while it stays filed, so refiling a tree whose patterns did not change costs the
// index nothing
int TriggerUnit::fileChildSlot(TTrigger* pT, const std::vector<quint64>& grams)
{
    const int slot = pT->mTreeIndexPosition;
    if (slot >= 0 && slot < static_cast<int>(mIndexSlots.size()) && mIndexSlots[slot].mpTrigger == pT && !mIndexSlots[slot].mRoot) {
        mTreeIndex.refileSlot(slot, grams);
        return slot;
    }
    return fileIndexSlot(pT, false, grams);
}

int TriggerUnit::fileIndexSlot(TTrigger* pT, const bool root, const std::vector<quint64>& grams)
{
    const int slot = mTreeIndex.fileSlot(grams);
    if (slot >= static_cast<int>(mIndexSlots.size())) {
        mIndexSlots.resize(slot + 1);
    }
    mIndexSlots[slot] = IndexSlot{pT, root};
    return slot;
}

bool TriggerUnit::fileRoot(TTrigger* pT, std::vector<quint64>& grams)
{
    if (smTreeIndexMode == TreeIndexMode::Off || !pT->prescanGrams(grams)) {
        unfileRoot(pT);
        return false;
    }
    if (pT->mRootIndexSlot >= 0) {
        mTreeIndex.refileSlot(pT->mRootIndexSlot, grams);
    } else {
        pT->mRootIndexSlot = fileIndexSlot(pT, true, grams);
        ++mFiledRoots;
    }
    return true;
}

void TriggerUnit::unfileRoot(TTrigger* pT)
{
    const int slot = pT->mRootIndexSlot;
    if (slot < 0) {
        return;
    }
    mTreeIndex.removeSlot(slot);
    mIndexSlots[slot] = IndexSlot{};
    pT->mRootIndexSlot = -1;
    --mFiledRoots;
}

void TriggerUnit::unfileTreeNode(TTrigger* pT)
{
    const int slot = pT->mTreeIndexPosition;
    if (slot >= 0 && slot < static_cast<int>(mIndexSlots.size()) && mIndexSlots[slot].mpTrigger == pT && !mIndexSlots[slot].mRoot) {
        mIndexSlots[slot].mpTrigger = nullptr;
    }
    pT->mTreeIndexPosition = -1;
    invalidateChildWalks(pT->getParent());
    mTreeIndexStale = true;
}

// Rebuilt only by this unit's outermost pass: a nested rebuild would file a trigger the outer line's
// stamping never saw, and the outer walk would then skip it on a line it matches. Until then a
// changed or new trigger is unfiled, so it is visited. A root is filed as its snapshot is refreshed,
// so by the time a pass stamps, the snapshot it pinned holds every root the index has.
bool TriggerUnit::stampTreeCandidates(const QString& line, RootNodeSnapshot& snapshot)
{
    if (mTreeIndexStale && mProcessingDepth == 1) {
        rebuildTreeIndex();
    }
    bool rootsWalkable = snapshot.mIndexed;
    if (!rootsWalkable && !mTreeIndexActive) {
        return false;
    }
    mTreeIndex.hits(line, mTreeHits);
    const quint32 stamp = TTrigger::lineStamp();
    TChildWalk& roots = snapshot.mWalk;
    const int rootCount = static_cast<int>(roots.mChildren.size());
    for (const int slot : mTreeHits) {
        const IndexSlot entry = mIndexSlots[slot];
        TTrigger* const node = entry.mpTrigger;
        if (!node) {
            continue;
        }
        if (!entry.mRoot) {
            node->mLineStamp = stamp;
            markTreeNode(node, stamp);
            continue;
        }
        if (!rootsWalkable) {
            continue;
        }
        const int position = node->rootSnapshotPosition();
        if (position < 0 || position >= rootCount || roots.mChildren[position] != node) {
            if (Q_UNLIKELY(smTreeIndexMode == TreeIndexMode::Verify)) {
                qWarning().nospace() << "TriggerUnit tree index MISMATCH: root " << node->getName() << " is filed at position " << position << " of a root list it is not at";
            }
            rootsWalkable = false;
            continue;
        }
        roots.mark(position, stamp);
    }
    if (!mTreeIndexActive) {
        return rootsWalkable;
    }
    for (size_t i = 0; i < mOpenTreeNodes.size();) {
        TTrigger* node = mOpenTreeNodes[i];
        if (node->actsWithoutMatching()) {
            markTreeNode(node, stamp);
            ++i;
            continue;
        }
        node->mInOpenTreeNodes = false;
        mOpenTreeNodes[i] = mOpenTreeNodes.back();
        mOpenTreeNodes.pop_back();
    }
    return rootsWalkable;
}

void TriggerUnit::refile(TTrigger* pT, const TriggerRefile change)
{
    // The stay-open count is read live by the tree index, so only a change to the grams unfiles a trigger
    bool unfile = false;
    // The line's root walk was marked before any script ran, so the rest of it goes unfiltered
    bool actsOnLineInFlight = false;
    switch (change) {
    case TriggerRefile::Patterns:
        unfile = true;
        break;
    case TriggerRefile::SeesEveryLine:
        unfile = true;
        actsOnLineInFlight = true;
        break;
    case TriggerRefile::OpenedByScript:
        actsOnLineInFlight = true;
        break;
    case TriggerRefile::OpenedByMatch:
        actsOnLineInFlight = mProcessingDepth > 1;
        break;
    case TriggerRefile::Shut:
        break;
    }
    // A root is refiled as its snapshot entry is, so the temporary triggers scripts arm every line cost the tree no refiling
    if (unfile && (pT->mTreeIndexPosition >= 0 || pT->getParent())) {
        unfileTreeNode(pT);
    }
    // The epochs stop the rest of a line trusting what a pass pinned, so only a trigger a pinned snapshot
    // could hold moves them. rootFilter() and prescanGrams() read only the trigger's own state, so one no
    // snapshot has filed - a trigger only ever a child, or one a script creates mid-line - is in none. One
    // that has left the root list may still be pinned, and the filter check runs ahead of isActive().
    const int position = pT->rootSnapshotPosition();
    if (position == TTrigger::scmNeverSnapshotted) {
        return;
    }
    ++mRootFilterEpoch;
    if (actsOnLineInFlight) {
        ++mUnfilterableEpoch;
    }
    // A dropped trigger has no slot to refile; if it is a root again, appending files it from its current state
    if (position >= 0) {
        mRootNodeSnapshotStale = true;
        if (!mRootNodeSnapshotNeedsRebuild) {
            mRootNodesRefiled.push_back(position);
        }
    }
}

void TriggerUnit::markRootNodeAppended(TTrigger* pT)
{
    mRootNodeSnapshotStale = true;
    if (!mRootNodeSnapshotNeedsRebuild) {
        mRootNodesAppended.push_back(pT);
    }
}

void TriggerUnit::markRootNodeRemoved(TTrigger* pT)
{
    mRootNodeSnapshotStale = true;
    // Unfiled at once: the trigger may be freed before the next line, and the line in flight has stamped already
    unfileRoot(pT);
    const int position = pT->rootSnapshotPosition();
    // Even when rebuilding: a stale position on a trigger registered again later would make its next
    // removal empty another trigger's slot. The dropped sentinel, not the never-filed one, is how
    // refile() tells a pinned copy of it from no copy at all.
    pT->setRootSnapshotPosition(TTrigger::scmSnapshotPositionDropped);
    if (mRootNodeSnapshotNeedsRebuild) {
        return;
    }
    if (position >= 0) {
        mRootNodesRemoved.push_back(position);
        return;
    }
    // Not filed yet, so at most queued for appending; if not queued either, it was never a root node
    const auto queued = std::find(mRootNodesAppended.begin(), mRootNodesAppended.end(), pT);
    if (queued != mRootNodesAppended.end()) {
        mRootNodesAppended.erase(queued);
    }
}

// Arming or killing a trigger only appends or empties a position, so is patched in place; a rebuild
// is for changes that move existing triggers, and to reclaim the holes removals leave.
void TriggerUnit::refreshRootNodeSnapshot()
{
    const bool canPatch = mpRootNodeSnapshot && mpRootNodeSnapshot.use_count() == 1 && !mRootNodeSnapshotNeedsRebuild
                          && mpRootNodeSnapshot->mHoles < std::max(scmMinimumIndexedTriggers, static_cast<int>(mTriggerRootNodeList.size()));
    std::vector<quint64> grams;
    if (canPatch) {
        RootNodeSnapshot& snapshot = *mpRootNodeSnapshot;
        TChildWalk& walk = snapshot.mWalk;
        // Removals first, so a refile queued for a since-freed trigger finds an empty slot and is skipped
        // rather than dereferencing it
        for (const int position : mRootNodesRemoved) {
            walk.mChildren[position] = nullptr;
            walk.setAlways(position, false);
            snapshot.mFilters[position] = TRootTriggerFilter();
            ++snapshot.mHoles;
        }
        if (mRootNodesRefiled.size() > 1) {
            std::sort(mRootNodesRefiled.begin(), mRootNodesRefiled.end());
            mRootNodesRefiled.erase(std::unique(mRootNodesRefiled.begin(), mRootNodesRefiled.end()), mRootNodesRefiled.end());
        }
        for (const int position : mRootNodesRefiled) {
            if (TTrigger* pT = walk.mChildren[position]) {
                snapshot.mFilters[position] = pT->rootFilter();
                walk.setAlways(position, !fileRoot(pT, grams));
            }
        }
        for (TTrigger* pT : mRootNodesAppended) {
            const int position = static_cast<int>(walk.mChildren.size());
            pT->setRootSnapshotPosition(position);
            walk.mChildren.push_back(pT);
            snapshot.mFilters.push_back(pT->rootFilter());
            walk.setAlways(position, !fileRoot(pT, grams));
        }
    } else {
        // Replace only a snapshot an outer pass has pinned; refilling an unpinned one avoids allocating
        if (mpRootNodeSnapshot.use_count() != 1) {
            mpRootNodeSnapshot = std::make_shared<RootNodeSnapshot>();
        }
        RootNodeSnapshot& snapshot = *mpRootNodeSnapshot;
        TChildWalk& walk = snapshot.mWalk;
        walk.mChildren.assign(mTriggerRootNodeList.cbegin(), mTriggerRootNodeList.cend());
        const int rootCount = static_cast<int>(walk.mChildren.size());
        const size_t words = (rootCount + 63) / 64;
        walk.mAlways.assign(words, 0);
        walk.mMarked.assign(words, 0);
        walk.mMarkedStamp = 0;
        snapshot.mFilters.resize(rootCount);
        snapshot.mHoles = 0;
        for (int position = 0; position < rootCount; ++position) {
            TTrigger* const pT = walk.mChildren[position];
            pT->setRootSnapshotPosition(position);
            snapshot.mFilters[position] = pT->rootFilter();
            if (!fileRoot(pT, grams)) {
                walk.mAlways[position / 64] |= quint64(1) << (position % 64);
            }
        }
        ++mPrescanRebuilds;
    }
    mpRootNodeSnapshot->mIndexed = mFiledRoots >= scmMinimumIndexedTriggers;
    mRootNodesAppended.clear();
    mRootNodesRemoved.clear();
    mRootNodesRefiled.clear();
    mRootNodeSnapshotNeedsRebuild = false;
    mRootNodeSnapshotStale = false;
}

void TriggerUnit::processDataStream(const QString& data, int line)
{
    if (data.isEmpty()) {
        return;
    }

    if (mpHost->getLuaInterpreter()->buildingCaptureTables()) {
        qWarning() << "TriggerUnit::processDataStream(...) WARNING - not running triggers on a line a garbage collection finaliser fed while the capture tables were being built.";
        return;
    }

    // Encoded, when a perl pattern asks, into storage borrowed from the unit so only a line longer than
    // any before allocates. Moved out rather than lent, so a nested pass finds the member empty and
    // cannot resize the outer pass's buffer.
    TUtf8Subject subject(data, std::move(mUtf8Scratch));
    const auto utf8Guard = qScopeGuard([this, &subject] {
        QByteArray scratch = subject.takeScratch();
        if (scratch.capacity() > scmMaxRetainedUtf8Scratch) {
            scratch = QByteArray();
        }
        mUtf8Scratch = std::move(scratch);
    });

    mProcessingDepth++;
    const auto processingGuard = qScopeGuard([this] {
        mProcessingDepth--;
        Q_ASSERT(mProcessingDepth >= 0);
        if (mProcessingDepth <= 1) {
            mRunawayFeedStopped = false;
        }
        if (mProcessingDepth == 0) {
            // Deletion is deferred while any pass runs, so these pointers stayed
            // valid; drop them before doCleanup() frees the underlying triggers.
            // A trigger that outlives the line it was created on stops being part
            // of a lineage, so its own creations start counting afresh.
            for (auto trigger : std::as_const(mRootNodesAddedWhileProcessing)) {
                if (trigger) {
                    trigger->setSameLineChain(0, 0);
                }
            }
            mRootNodesAddedWhileProcessing.clear();
            mSameLineChainStarters.clear();
            doCleanup();
        }
    });

    // Iterate a snapshot of the root list: a trigger's Lua script can call
    // uninstallPackage()/installPackage() and mutate mTriggerRootNodeList
    // mid-iteration (the underlying std::list::remove frees the iterator's
    // current node → use-after-free on the next ++). AliasUnit dodges the
    // same hazard for the same reason — see Mudlet issue #4297.
    // Pinned, not copied: a mutation replaces the shared snapshot rather than editing the pinned one.
    if (mRootNodeSnapshotStale) {
        refreshRootNodeSnapshot();
    }
    const auto pinnedSnapshot = mpRootNodeSnapshot;
    const std::vector<TTrigger*>& pinnedNodeList = pinnedSnapshot->mWalk.mChildren;
    TTrigger::enterPass();
    const auto passGuard = qScopeGuard([] {
        TTrigger::leavePass();
    });
    bool rootsWalkable = false;
    if (smTreeIndexMode != TreeIndexMode::Off) {
        // After enterPass(): the stamps have to carry this line's stamp
        rootsWalkable = stampTreeCandidates(data, *pinnedSnapshot);
    }
    // Triggers registered by a script during this pass (tempTrigger() & Co.)
    // are missing from the snapshot but must still match the current line:
    // before the snapshot the loop walked the live std::list, which a push_back
    // extends in front of end(), so a trigger created mid-pass was reached in
    // the same iteration - long-standing behaviour capture scripts depend on.
    // Entries below this index were added by outer (nested-feedTriggers) passes
    // and are already part of this pass's snapshot.
    const qsizetype firstNodeAddedThisPass = mRootNodesAddedWhileProcessing.size();
    const TBigramFilter lineBigrams(data, mSubstringQuestionsOnTheLastLine);
    // Helper threads rule triggers out and search ahead for regex matches; every match is still run and
    // ordered by the loop below, on this thread.
    const quint32 previousPrescanPassId = TTrigger::prescanPassId();
    const char* const previousPrescanSubject = TTrigger::prescanSubject();
    const int previousPrescanSubjectLength = TTrigger::prescanSubjectLength();
    TTrigger::setPrescanPass(0, nullptr, 0);
    const auto prescanGuard = qScopeGuard([previousPrescanPassId, previousPrescanSubject, previousPrescanSubjectLength] {
        TTrigger::setPrescanPass(previousPrescanPassId, previousPrescanSubject, previousPrescanSubjectLength);
    });
    // Only while behind, i.e. a chunk carries many lines: a wake-up is repaid only when the next line is
    // already waiting, so at normal game speed it would spend CPU to save nothing perceptible.
    TriggerMatchPool& pool = TriggerMatchPool::instance();
    const bool inFlood = pool.workerCount() > 0 && mpHost && mpHost->mainConsoleModelOrNull() && mpHost->mainConsoleModel().buffer.pendingChunkLines() >= pool.floodChunkLines();
    const quint64 regexSearchesBefore = TTrigger::regexSearches();
    const quint64 regexMatchesBefore = TTrigger::regexMatches();
    int prescanRegexSearches = 0;
    int prescanRegexMatches = 0;
    // Lines the pool takes are timed, and a few it would have taken are matched without it for comparison,
    // so the statistics can say whether it pays on this profile. Top-level lines only: a nested pass's time
    // is already inside the line that fed it.
    const bool poolWouldTake = inFlood && !mSamplingLine && prescanPays();
    const bool timed = poolWouldTake && mProcessingDepth == 1;
    const bool sampled = timed && pool.takeSample();
    const auto lineStart = timed ? std::chrono::steady_clock::now() : std::chrono::steady_clock::time_point{};
    bool pooled = false;
    mSamplingLine = mSamplingLine || sampled;
    const auto timingGuard = qScopeGuard([this, &pool, timed, sampled, lineStart, &pooled] {
        if (sampled) {
            mSamplingLine = false;
        }
        if (timed && (pooled || sampled)) {
            pool.recordLine(pooled, std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now() - lineStart).count());
        }
    });
    if (poolWouldTake && !sampled) {
        rebuildPrescanTasksIfStale();
        const quint32 passId = TTrigger::nextPrescanPassId();
        // The helper threads run perl patterns of their own, so the line is
        // encoded here, on this thread, before any of them can ask for it -
        // TUtf8Subject encodes on first use, which is not a helper's to do.
        if (pool.prescan(mPrescanTasks.data(), static_cast<int>(mPrescanTasks.size()), passId, subject.data(), subject.length(), data, lineBigrams, subject.dropsText())) {
            TTrigger::setPrescanPass(passId, subject.data(), subject.length());
            prescanRegexSearches = pool.regexSearchesInLastBatch();
            prescanRegexMatches = pool.regexMatchesInLastBatch();
            pooled = true;
        }
    }

    {
        const std::vector<TRootTriggerFilter>& pinnedFilters = pinnedSnapshot->mFilters;
        const TChildWalk& rootWalk = pinnedSnapshot->mWalk;
        const quint32 stamp = TTrigger::lineStamp();
        const bool verifying = treeIndexVerifying();
        // A script can make a later trigger act on a line it does not match after the walk was settled -
        // setTriggerStayOpen(), or a line it feeds opening it; from then on the rest of the line goes to
        // every remaining trigger. See refile().
        const quint32 unfilterableEpochAtStart = mUnfilterableEpoch;
        // The pinned filters are copies, and a firing script can change what they were copied from (a
        // pattern, a stay-open count, a trigger made multiline); from then on the line asks the triggers.
        const quint32 filterEpochAtStart = mRootFilterEpoch;
        // Asked for only once a color trigger wants it, and again after any script has run, which can
        // recolor, edit, delete or feed lines - too much to second-guess what match_color_pattern() reads.
        bool lineColorsKnown = false;
        bool lineColorsUniform = false;
        QRgb lineForeground = 0;
        QRgb lineBackground = 0;
        const int rootCount = static_cast<int>(pinnedNodeList.size());
        for (int position = 0; position < rootCount; ++position) {
            bool passedOver = false;
            if (rootsWalkable && mUnfilterableEpoch == unfilterableEpochAtStart) {
                const int next = rootWalk.nextToEnter(position, stamp);
                if (Q_UNLIKELY(verifying)) {
                    // Enters every root, and reports any the walk would have passed over that acted
                    passedOver = next != position;
                } else {
                    if (next >= rootCount) {
                        break;
                    }
                    position = next;
                }
            }
            // Read before the trigger is, as most lines are over for most
            // triggers right here and the trigger's own memory is never touched
            bool textDecided = false;
            if (mRootFilterEpoch == filterEpochAtStart) {
                const TRootTriggerFilter& filter = pinnedFilters[position];
                if (filter.mKind == TRootTriggerFilter::Kind::Text) {
                    if (!lineBigrams.couldContain(data, filter.mText) && !subject.dropsText()) {
                        continue;
                    }
                    textDecided = true;
                } else if (filter.mKind == TRootTriggerFilter::Kind::Color) {
                    if (!lineColorsKnown) {
                        lineColorsUniform = TTrigger::uniformLineColors(mpHost, line, lineForeground, lineBackground);
                        lineColorsKnown = true;
                    }
                    if (lineColorsUniform && filter.lacksColors(lineForeground, lineBackground)) {
                        continue;
                    }
                }
            }
            // A hole is a trigger the snapshot has outlived - see
            // refreshRootNodeSnapshot()
            TTrigger* trigger = pinnedNodeList[position];
            if (!trigger || !trigger->isActive() || (!textDecided && trigger->cannotMatch(lineBigrams, data) && !subject.dropsText())) {
                continue;
            }
            if (Q_UNLIKELY(passedOver)) {
                trigger->verifySkippable(subject, data, line, 0, &lineBigrams);
            } else {
                trigger->match(subject, data, line, 0, &lineBigrams);
            }
            lineColorsKnown = false;
        }
    }
    // A match here can register more triggers, which also get a shot at the
    // current line - so the list grows in front of the loop, and a trigger that
    // re-creates itself never lets the line finish. Nothing else catches that: no
    // C++ frame recurses, so mProcessingDepth stays put and the feedTriggers()
    // depth guard never sees it. Only the lineage that is extending itself gets
    // stopped; every other lineage the line started carries on matching, which is
    // the difference between a runaway and a script arming a batch of triggers.
    for (qsizetype i = firstNodeAddedThisPass; i < mRootNodesAddedWhileProcessing.size(); ++i) {
        if (i - firstNodeAddedThisPass >= scmMaxSameLineCreationsPerLine) {
            qWarning().nospace() << "TriggerUnit::processDataStream(...) stopping: more than " << scmMaxSameLineCreationsPerLine
                                 << " triggers were created while processing one line, so the rest are not being offered it. Profile: " << (mpHost ? mpHost->getName() : QString());
            break;
        }
        auto trigger = mRootNodesAddedWhileProcessing.at(i);
        if (!trigger || !trigger->isActive()) {
            continue;
        }
        // stopSameLineCreationLoop() deactivates the whole lineage, so the check
        // above skips its remaining members and this loop reaches a lineage once
        if (trigger->sameLineGeneration() > scmMaxSameLineGenerations) {
            stopSameLineCreationLoop(trigger->sameLineChainId());
            continue;
        }
        if (trigger->cannotMatch(lineBigrams, data) && !subject.dropsText()) {
            continue;
        }
        trigger->match(subject, data, line, 0, &lineBigrams);
    }
    mSubstringQuestionsOnTheLastLine = lineBigrams.questionsAsked();
    // Includes nested passes' searches, which is fine: the count only steers the next line
    mRegexSearchesOnTheLastLine = prescanRegexSearches + static_cast<int>(TTrigger::regexSearches() - regexSearchesBefore);
    mRegexMatchesOnTheLastLine = prescanRegexMatches + static_cast<int>(TTrigger::regexMatches() - regexMatchesBefore);
}

// A search the pool rules out saves the main thread its cost. One that matches is reused, but still a net
// loss - the trigger is visited twice and its state crosses between cores - so a line where many match is
// quicker left to the main thread alone.
bool TriggerUnit::prescanPays() const
{
    const TriggerMatchPool& pool = TriggerMatchPool::instance();
    if (pool.missesPerMatch() == 0) {
        return mRegexSearchesOnTheLastLine >= pool.threshold();
    }
    const int misses = mRegexSearchesOnTheLastLine - mRegexMatchesOnTheLastLine;
    return misses >= pool.threshold() && misses >= static_cast<qint64>(pool.missesPerMatch()) * mRegexMatchesOnTheLastLine;
}

void TriggerUnit::compileAll()
{
    // Switched off ones as well: a reset has just closed the Lua state their
    // compiled functions lived in, and switching one back on later does
    // not compile it again
    for (auto trigger : mTriggerRootNodeList) {
        trigger->compileAll();
    }
}

void TriggerUnit::stopAllTriggers()
{
    for (auto trigger : mTriggerRootNodeList) {
        trigger->disableFamily();
    }
}

void TriggerUnit::reenableAllTriggers()
{
    for (auto trigger : mTriggerRootNodeList) {
        trigger->enableFamily();
    }
}

TTrigger* TriggerUnit::findTrigger(const QString& name)
{
    auto it = mLookupTable.constFind(name);
    while (it != mLookupTable.cend() && it.key() == name) {
        TTrigger* pT = it.value();
        return pT;
    }
    return nullptr;
}

std::vector<int> TriggerUnit::findItems(const QString& name, const bool exactMatch, const bool caseSensitive)
{
    std::vector<int> ids;
    const auto searchCaseSensitivity = caseSensitive ? Qt::CaseSensitive : Qt::CaseInsensitive;
    if (exactMatch) {
        for (auto& item : std::as_const(mTriggerMap)) {
            if (!item->getName().compare(name, searchCaseSensitivity)) {
                ids.push_back(item->getID());
            }
        }
    } else {
        for (auto& item : std::as_const(mTriggerMap)) {
            if (item->getName().contains(name, searchCaseSensitivity)) {
                ids.push_back(item->getID());
            }
        }
    }
    return ids;
}

bool TriggerUnit::pendingDeletion(TTrigger* pItem) const
{
    if (!hasPendingDeletes()) {
        return false;
    }
    for (auto* pAncestor = pItem; pAncestor; pAncestor = pAncestor->getParent()) {
        if (mCleanupSet.contains(pAncestor) || uninstallList.contains(pAncestor)) {
            return true;
        }
    }
    return false;
}

bool TriggerUnit::enableTrigger(const QString& name)
{
    bool found = false;
    // equal_range visits every same-named trigger; constFind() + (++it) can
    // start mid-run and skip duplicates on some QMultiMap implementations
    const auto [begin, end] = mLookupTable.equal_range(name);
    for (auto it = begin; it != end; ++it) {
        // A trigger queued for deletion stays in the lookup table until
        // doCleanup() frees it, which cannot run mid-pass - re-activating one
        // resurrects a spent one-shot, a killTrigger()ed trigger, or a trigger
        // whose package was uninstalled mid-pass.
        if (mCleanupSet.contains(it.value()) || uninstallList.contains(it.value())) {
            continue;
        }
        it.value()->setIsActive(true);
        found = true;
        if (!it.value()->isTemporary() || it.value()->getParent()) {
            emit mpHost->signal_triggerToggled(it.value()->getID());
        }
    }
    return found;
}

bool TriggerUnit::disableTrigger(const QString& name)
{
    bool found = false;
    // equal_range visits every same-named trigger; constFind() + (++it) can
    // start mid-run and skip duplicates on some QMultiMap implementations
    const auto [begin, end] = mLookupTable.equal_range(name);
    for (auto it = begin; it != end; ++it) {
        it.value()->setIsActive(false);
        found = true;
        if (!it.value()->isTemporary() || it.value()->getParent()) {
            emit mpHost->signal_triggerToggled(it.value()->getID());
        }
    }
    return found;
}

void TriggerUnit::setTriggerStayOpen(const QString& name, int lines)
{
    // equal_range visits every same-named trigger; constFind() + (++it) can
    // start mid-run and skip duplicates on some QMultiMap implementations
    const auto [begin, end] = mLookupTable.equal_range(name);
    for (auto it = begin; it != end; ++it) {
        TTrigger* pT = it.value();
        ++pT->mStayOpenSetsByScript;
        const bool wasOpen = pT->mKeepFiring > 0;
        pT->mKeepFiring = lines;
        const bool nowOpen = pT->mKeepFiring > 0;
        if (wasOpen == nowOpen) {
            // Staying open or shut changes nothing the trigger is filtered by, and scripts often set the
            // same count every line: refiling would cost the rest of each such line its filters. Not keyed
            // on grams: a regex or colour trigger has none, yet can be ruled out of a line while shut.
            continue;
        }
        if (nowOpen) {
            noteOpenTreeNode(pT);
            refile(pT, TriggerRefile::OpenedByScript);
        } else {
            refile(pT, TriggerRefile::Shut);
        }
    }
}

bool TriggerUnit::killTrigger(const QString& name)
{
    // equal_range visits every same-named trigger; constFind() + (++it) can
    // start mid-run and skip duplicates on some QMultiMap implementations
    const auto [begin, end] = mLookupTable.equal_range(name);
    for (auto it = begin; it != end; ++it) {
        TTrigger* pT = it.value();
        if (!pT->isTemporary()) {
            // this function is only defined for tempTriggers, permanent objects cannot be removed
            continue;
        }
        // An already killed trigger is only unlinked from the lookup table once
        // doCleanup() gets to free it, which cannot happen while a trigger script
        // is on the call stack - so until then it is still findable by name.
        // tempComplexRegexTrigger() replaces a temporary trigger under the name it
        // was given, so a corpse and a live trigger can share one: keep looking
        // rather than report a kill that would achieve nothing. One a mid-pass
        // uninstallPackage() queued must stay out of mCleanupSet too: doCleanup()
        // would free its children twice.
        if (pendingDeletion(pT)) {
            continue;
        }
        // Deactivating matters as much as queueing the delete: the trigger stays
        // in the list processDataStream() is walking until that deferred cleanup,
        // and a killed trigger must no more fire on the rest of the line than a
        // disabled one does
        pT->setIsActive(false);
        markCleanup(pT);
        return true;
    }
    return false;
}

void TriggerUnit::assembleReport(TTrigger* pItem)
{
    std::list<Tree<TTrigger>*>* childrenList = pItem->mpMyChildrenList;
    for (auto* pChildNode : *childrenList) {
        auto* pChild = static_cast<TTrigger*>(pChildNode);
        ++statsItemsTotal;
        if (pChild->isActive()) {
            ++statsActiveItems;
            statsPatternsActive += pChild->mPatterns.size();
        }
        if (pChild->isTemporary()) {
            ++statsTempItems;
        }
        statsPatternsTotal += pChild->mPatterns.size();
        assembleReport(pChild);
    }
}

std::tuple<QString, int, int, int, int, int> TriggerUnit::assembleReport()
{
    resetStats();
    for (auto pItem : mTriggerRootNodeList) {
        ++statsItemsTotal;
        if (pItem->isActive()) {
            ++statsActiveItems;
            statsPatternsActive += pItem->mPatterns.size();
        }
        if (pItem->isTemporary()) {
            ++statsTempItems;
        }
        statsPatternsTotal += pItem->mPatterns.size();
        assembleReport(pItem);
    }
    QStringList msg;
    msg << QLatin1String("triggers current total: ") << QString::number(statsItemsTotal) << QLatin1String("\n") << QLatin1String("tempTriggers current total: ") << QString::number(statsTempItems)
        << QLatin1String("\n") << QLatin1String("active triggers: ") << QString::number(statsActiveItems) << QLatin1String("\n") << QLatin1String("trigger patterns total: ")
        << QString::number(statsPatternsTotal) << QLatin1String("\n") << QLatin1String("active patterns total: ") << QString::number(statsPatternsActive) << QLatin1String("\n");
    return {msg.join(QString()), statsItemsTotal, statsPatternsTotal, statsTempItems, statsActiveItems, statsPatternsActive};
}

void TriggerUnit::doCleanup()
{
    if (mProcessingDepth > 0) {
        return;
    }

    if (!hasPendingDeletes()) {
        return;
    }

    QSet<TTrigger*> deletedTriggers;
    QMutableSetIterator<TTrigger*> itTrigger(mCleanupSet);
    while (itTrigger.hasNext()) {
        auto pTrigger = itTrigger.next();
        itTrigger.remove();
        deletedTriggers.insert(pTrigger);
        delete pTrigger;
    }
    // The drain frees no buckets, so later flushes would re-scan an array sized for the largest batch
    // ever held. squeeze(), not clear(), keeps anything the drain left behind.
    mCleanupSet.squeeze();
    // Flush the deletes uninstall() deferred (#9337). uninstallList is ordered
    // children-before-parents and each ~Tree unlinks from its parent, so deleting
    // children first empties the parent's child list (no double free); the seen
    // set guards a node queued twice by re-entrant uninstalls and is shared with
    // the mCleanupSet loop above so an object that ended up in both containers is
    // freed once. It matches on pointer identity only: a node freed indirectly, as
    // a child of a queued parent, is not in the set (not reachable today - only
    // temporary root nodes are ever queued, and those have no children).
    for (auto trigger : uninstallList) {
        if (!deletedTriggers.contains(trigger)) {
            deletedTriggers.insert(trigger);
            delete trigger;
        }
    }
    uninstallList.clear();
}

void TriggerUnit::markCleanup(TTrigger* pT)
{
    mCleanupSet.insert(pT);
}
