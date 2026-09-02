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
#include "TTrigger.h"

#include <QScopeGuard>

#include <algorithm>
#include <functional>
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
        mTriggerRootNodeList.push_back(pT);
    } else {
        // insert item at proper position
        int cnt = 0;
        for (auto it = mTriggerRootNodeList.begin(); it != mTriggerRootNodeList.end(); it++) {
            if (cnt >= childPosition) {
                mTriggerRootNodeList.insert(it, pT);
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

    if (pOldParent) {
        pOldParent->popChild(pChild);
    } else {
        mTriggerRootNodeList.remove(pChild);
    }

    // Convert enum mode to the internal flags
    int parentPosition = (mode == TreeItemInsertMode::AtPosition) ? 0 : -1;
    int childPosition = (mode == TreeItemInsertMode::AtPosition) ? position : -1;

    if (pNewParent) {
        pNewParent->addChild(pChild, parentPosition, childPosition);
        pChild->setParent(pNewParent);
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
    mTriggerRootNodeList.remove(pT);
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
    if (pT->getParent()) {
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
    for (auto& trigger : tempList) {
        mTriggerRootNodeList.remove(trigger);
    }
    for (auto& trigger : tempList) {
        mTriggerRootNodeList.push_back(trigger);
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

void TriggerUnit::processDataStream(const QString& data, int line)
{
    if (data.isEmpty()) {
        return;
    }

    // subject points into utf8Data, so utf8Data has to outlive every match()
    // call below. Perl patterns see the line only as far as its first NUL
    // byte, so this is qstrnlen() rather than the byte count.
    const QByteArray utf8Data = data.toUtf8();
    const char* subject = utf8Data.constData();
    const int subjectLength = static_cast<int>(qstrnlen(subject, utf8Data.size()));

    mProcessingDepth++;
    const auto processingGuard = qScopeGuard([this] {
        mProcessingDepth--;
        Q_ASSERT(mProcessingDepth >= 0);
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
    std::vector<TTrigger*> copyOfNodeList(mTriggerRootNodeList.cbegin(), mTriggerRootNodeList.cend());
    // Triggers registered by a script during this pass (tempTrigger() & Co.)
    // are missing from the snapshot but must still match the current line:
    // before the snapshot the loop walked the live std::list, which a push_back
    // extends in front of end(), so a trigger created mid-pass was reached in
    // the same iteration - long-standing behaviour capture scripts depend on.
    // Entries below this index were added by outer (nested-feedTriggers) passes
    // and are already part of this pass's snapshot.
    const qsizetype firstNodeAddedThisPass = mRootNodesAddedWhileProcessing.size();
    for (auto trigger : copyOfNodeList) {
        if (!trigger->isActive()) {
            continue;
        }
        trigger->match(subject, subjectLength, data, line);
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
        trigger->match(subject, subjectLength, data, line);
    }
}

void TriggerUnit::compileAll()
{
    for (auto trigger : mTriggerRootNodeList) {
        if (trigger->isActive()) {
            trigger->compileAll();
        }
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
    }
    return found;
}

void TriggerUnit::setTriggerStayOpen(const QString& name, int lines)
{
    // equal_range visits every same-named trigger; constFind() + (++it) can
    // start mid-run and skip duplicates on some QMultiMap implementations
    const auto [begin, end] = mLookupTable.equal_range(name);
    for (auto it = begin; it != end; ++it) {
        it.value()->mKeepFiring = lines;
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
        // rather than report a kill that would achieve nothing.
        if (mCleanupSet.contains(pT)) {
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

    QSet<TTrigger*> deletedTriggers;
    QMutableSetIterator<TTrigger*> itTrigger(mCleanupSet);
    while (itTrigger.hasNext()) {
        auto pTrigger = itTrigger.next();
        itTrigger.remove();
        deletedTriggers.insert(pTrigger);
        delete pTrigger;
    }
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
