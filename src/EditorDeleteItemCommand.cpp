/***************************************************************************
 *   Copyright (C) 2025 by Vadim Peretokin - vadim.peretokin@mudlet.org    *
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

#include "EditorDeleteItemCommand.h"

#include "EditorItemXMLHelpers.h"
#include "Host.h"
#include "TAction.h"
#include "TAlias.h"
#include "TKey.h"
#include "TScript.h"
#include "TTimer.h"
#include "TTrigger.h"

#include <algorithm>

EditorDeleteItemCommand::EditorDeleteItemCommand(EditorViewType viewType, const QList<DeletedItemInfo>& deletedItems, Host* host)
: EditorCommand(generateText(viewType, deletedItems.size(), deletedItems.isEmpty() ? QString() : deletedItems.first().itemName), host), mViewType(viewType), mDeletedItems(deletedItems)
{
}

void EditorDeleteItemCommand::undo()
{
#if defined(DEBUG_UNDO_REDO)
    qDebug() << "EditorDeleteItemCommand::undo() - Restoring" << mDeletedItems.size() << "deleted items";
#endif
    // Clear ID changes from any previous undo
    mIDChanges.clear();

    // Track if any items are valid (not invalidated by Lua API changes)
    mLastOperationWasValid = false;

    // Restore all deleted items from their XML snapshots
    // Use topological sort to ensure parents are ALWAYS restored before children (any depth)
    QList<DeletedItemInfo> sortedItems;
    QSet<int> processedIDs; // Track which items have been added to sortedItems
    QList<DeletedItemInfo> remainingItems = mDeletedItems;

    // Keep processing until all items are sorted
    while (!remainingItems.isEmpty()) {
        bool madeProgress = false;

        // Process items from end to allow safe removal during iteration
        for (int i = remainingItems.size() - 1; i >= 0; --i) {
            const auto& item = remainingItems[i];

            // Determine if this item can be restored now
            bool canRestore = false;
            if (item.parentID == -1) {
                // Root item (no parent) - can always restore
                canRestore = true;
            } else {
                // Check if parent was also deleted
                bool parentWasDeleted = std::any_of(mDeletedItems.begin(), mDeletedItems.end(), [&item](const DeletedItemInfo& info) { return info.itemID == item.parentID; });
                if (parentWasDeleted) {
                    // Parent was deleted - check if it's already been processed
                    if (processedIDs.contains(item.parentID)) {
                        canRestore = true;
                    }
                } else {
                    // Parent wasn't deleted, so it still exists in the tree - can restore
                    canRestore = true;
                }
            }

            if (canRestore) {
                sortedItems.append(item);
                processedIDs.insert(item.itemID);
                remainingItems.removeAt(i);
                madeProgress = true;
            }
        }

        // Safety check for circular dependencies or broken references
        if (!madeProgress && !remainingItems.isEmpty()) {
            qWarning() << "EditorDeleteItemCommand::undo() - Could not resolve parent-child dependencies for" << remainingItems.size() << "items, adding them anyway";
            sortedItems.append(remainingItems);
            break;
        }
    }

    // Build a set of original deleted item IDs for fast skipRestore lookup
    // Also store original parent IDs since sortedItems will be modified during iteration
    QSet<int> originalDeletedIDs;
    QMap<int, int> originalParentIDs; // itemID -> original parentID
    for (const auto& item : sortedItems) {
        originalDeletedIDs.insert(item.itemID);
        originalParentIDs[item.itemID] = item.parentID;
    }

    for (int i = 0; i < sortedItems.size(); ++i) {
        auto& info = sortedItems[i];

        // Skip items whose parent was also deleted - they'll be restored from parent's XML
        // Use the ORIGINAL parentID (before any updates during iteration)
        bool skipRestore = false;
        int origParentID = originalParentIDs[info.itemID];
        if (origParentID != -1) {
            bool parentWasDeleted = originalDeletedIDs.contains(origParentID);
            if (parentWasDeleted) {
                skipRestore = true;
#if defined(DEBUG_UNDO_REDO)
                qDebug() << "EditorDeleteItemCommand::undo() - Skipping" << info.itemName << "(ID:" << info.itemID << ") - parent (ID:" << info.parentID
                         << ") was also deleted, will be restored from parent's XML";
#endif
            }
        }

        if (skipRestore) {
            // Item was restored from parent's XML - no need to restore individually
            continue;
        }

        // Find the corresponding item in mDeletedItems to update the ID
        auto it = std::find_if(mDeletedItems.begin(), mDeletedItems.end(), [&info](const DeletedItemInfo& item) { return item.itemName == info.itemName && item.parentID == info.parentID; });
        if (it == mDeletedItems.end()) {
            qWarning() << "EditorDeleteItemCommand::undo() - Could not find item in original list:" << info.itemName << "with parentID=" << info.parentID;
            continue;
        }
        auto& originalInfo = *it;

#if defined(DEBUG_UNDO_REDO)
        qDebug() << "EditorDeleteItemCommand::undo() - Restoring" << info.itemName << "(ID:" << info.itemID << ", parentID:" << info.parentID << ") individually";
#endif

        switch (mViewType) {
        case EditorViewType::cmTriggerView: {
            // Get parent trigger
            TTrigger* pParent = nullptr;
            if (info.parentID != -1) {
                pParent = mpHost->getTriggerUnit()->getTrigger(info.parentID);
                if (!pParent) {
                    qWarning() << "EditorDeleteItemCommand::undo() - Parent trigger not found for" << info.itemName << "parentID=" << info.parentID;
                }
            }

            // Restore the trigger from XML snapshot at its original position
            TTrigger* pRestoredTrigger = importTriggerFromXML(info.xmlSnapshot, pParent, mpHost, info.positionInParent);
            if (!pRestoredTrigger) {
                qWarning() << "EditorDeleteItemCommand::undo() - Failed to restore trigger" << info.itemName;
            } else {
                mLastOperationWasValid = true;
                int newID = pRestoredTrigger->getID();
                int oldID = info.itemID;

                // Update the stored ID in original list so redo can find it
                originalInfo.itemID = newID;

                // Track ID change for stack remapping
                if (newID != oldID) {
                    mIDChanges.append(qMakePair(oldID, newID));
                }

                // If ID changed, update all remaining items that reference this as their parent
                if (newID != oldID) {
                    for (int j = i + 1; j < sortedItems.size(); ++j) {
                        if (sortedItems[j].parentID == oldID) {
                            sortedItems[j].parentID = newID;

                            // Also update in mDeletedItems so we can find it later
                            auto childIt = std::find_if(mDeletedItems.begin(), mDeletedItems.end(), [&sortedItems, j, oldID](const DeletedItemInfo& item) {
                                return item.itemName == sortedItems[j].itemName && item.parentID == oldID;
                            });
                            if (childIt != mDeletedItems.end()) {
                                childIt->parentID = newID;
                            }
                        }
                    }
                }

                // Walk the restored trigger's children and update their IDs in mDeletedItems
                // (children were restored from XML, not individually)
                std::function<void(TTrigger*, int)> updateChildIDs = [&](TTrigger* pT, int parentID) {
                    if (!pT || !pT->mpMyChildrenList) {
                        return;
                    }
                    for (auto* pChild : *pT->mpMyChildrenList) {
                        // Find this child in mDeletedItems by name and parent ID
                        auto childIt = std::find_if(mDeletedItems.begin(), mDeletedItems.end(), [pChild, parentID](const DeletedItemInfo& item) {
                            return item.itemName == pChild->getName() && item.parentID == parentID;
                        });
                        if (childIt != mDeletedItems.end()) {
                            int childOldID = childIt->itemID;
                            int childNewID = pChild->getID();
                            if (childOldID != childNewID) {
                                childIt->itemID = childNewID;
                                // Update grandchildren's parentID references in mDeletedItems
                                for (auto& item : mDeletedItems) {
                                    if (item.parentID == childOldID) {
                                        item.parentID = childNewID;
                                    }
                                }
                                // Recursively update grandchildren
                                updateChildIDs(pChild, childNewID);
                            }
                        }
                    }
                };
                updateChildIDs(pRestoredTrigger, newID);
            }
            break;
        }
        case EditorViewType::cmAliasView: {
            TAlias* pParent = nullptr;
            if (info.parentID != -1) {
                pParent = mpHost->getAliasUnit()->getAlias(info.parentID);
            }

            TAlias* pRestoredAlias = importAliasFromXML(info.xmlSnapshot, pParent, mpHost, info.positionInParent);
            if (!pRestoredAlias) {
                qWarning() << "EditorDeleteItemCommand::undo() - Failed to restore alias" << info.itemName;
            } else {
                mLastOperationWasValid = true;
                int newID = pRestoredAlias->getID();
                int oldID = info.itemID;

                originalInfo.itemID = newID;

                // Track ID change for stack remapping
                if (newID != oldID) {
                    mIDChanges.append(qMakePair(oldID, newID));
                }

                // If ID changed, update all remaining items that reference this as their parent
                if (newID != oldID) {
                    for (int j = i + 1; j < sortedItems.size(); ++j) {
                        if (sortedItems[j].parentID == oldID) {
                            sortedItems[j].parentID = newID;

                            // Also update in mDeletedItems so we can find it later
                            auto childIt = std::find_if(mDeletedItems.begin(), mDeletedItems.end(), [&sortedItems, j, oldID](const DeletedItemInfo& item) {
                                return item.itemName == sortedItems[j].itemName && item.parentID == oldID;
                            });
                            if (childIt != mDeletedItems.end()) {
                                childIt->parentID = newID;
                            }
                        }
                    }
                }

                // Walk the restored alias's children and update their IDs in mDeletedItems
                std::function<void(TAlias*, int)> updateChildIDs = [&](TAlias* pA, int parentID) {
                    if (!pA || !pA->mpMyChildrenList) {
                        return;
                    }
                    for (auto* pChild : *pA->mpMyChildrenList) {
                        auto childIt = std::find_if(mDeletedItems.begin(), mDeletedItems.end(), [pChild, parentID](const DeletedItemInfo& item) {
                            return item.itemName == pChild->getName() && item.parentID == parentID;
                        });
                        if (childIt != mDeletedItems.end()) {
                            int childOldID = childIt->itemID;
                            int childNewID = pChild->getID();
                            if (childOldID != childNewID) {
                                childIt->itemID = childNewID;
                                // Update grandchildren's parentID references in mDeletedItems
                                for (auto& item : mDeletedItems) {
                                    if (item.parentID == childOldID) {
                                        item.parentID = childNewID;
                                    }
                                }
                                updateChildIDs(pChild, childNewID);
                            }
                        }
                    }
                };
                updateChildIDs(pRestoredAlias, newID);
            }
            break;
        }
        case EditorViewType::cmTimerView: {
            TTimer* pParent = nullptr;
            if (info.parentID != -1) {
                pParent = mpHost->getTimerUnit()->getTimer(info.parentID);
            }

            TTimer* pRestoredTimer = importTimerFromXML(info.xmlSnapshot, pParent, mpHost, info.positionInParent);
            if (!pRestoredTimer) {
                qWarning() << "EditorDeleteItemCommand::undo() - Failed to restore timer" << info.itemName;
            } else {
                mLastOperationWasValid = true;
                int newID = pRestoredTimer->getID();
                int oldID = info.itemID;

                originalInfo.itemID = newID;

                // Track ID change for stack remapping
                if (newID != oldID) {
                    mIDChanges.append(qMakePair(oldID, newID));
                }

                // If ID changed, update all remaining items that reference this as their parent
                if (newID != oldID) {
                    for (int j = i + 1; j < sortedItems.size(); ++j) {
                        if (sortedItems[j].parentID == oldID) {
                            sortedItems[j].parentID = newID;

                            // Also update in mDeletedItems so we can find it later
                            auto childIt = std::find_if(mDeletedItems.begin(), mDeletedItems.end(), [&sortedItems, j, oldID](const DeletedItemInfo& item) {
                                return item.itemName == sortedItems[j].itemName && item.parentID == oldID;
                            });
                            if (childIt != mDeletedItems.end()) {
                                childIt->parentID = newID;
                            }
                        }
                    }
                }

                // Walk the restored timer's children and update their IDs in mDeletedItems
                std::function<void(TTimer*, int)> updateChildIDs = [&](TTimer* pT, int parentID) {
                    if (!pT || !pT->mpMyChildrenList) {
                        return;
                    }
                    for (auto* pChild : *pT->mpMyChildrenList) {
                        auto childIt = std::find_if(mDeletedItems.begin(), mDeletedItems.end(), [pChild, parentID](const DeletedItemInfo& item) {
                            return item.itemName == pChild->getName() && item.parentID == parentID;
                        });
                        if (childIt != mDeletedItems.end()) {
                            int childOldID = childIt->itemID;
                            int childNewID = pChild->getID();
                            if (childOldID != childNewID) {
                                childIt->itemID = childNewID;
                                // Update grandchildren's parentID references in mDeletedItems
                                for (auto& item : mDeletedItems) {
                                    if (item.parentID == childOldID) {
                                        item.parentID = childNewID;
                                    }
                                }
                                updateChildIDs(pChild, childNewID);
                            }
                        }
                    }
                };
                updateChildIDs(pRestoredTimer, newID);
            }
            break;
        }
        case EditorViewType::cmScriptView: {
            TScript* pParent = nullptr;
            if (info.parentID != -1) {
                pParent = mpHost->getScriptUnit()->getScript(info.parentID);
            }

            TScript* pRestoredScript = importScriptFromXML(info.xmlSnapshot, pParent, mpHost, info.positionInParent);
            if (!pRestoredScript) {
                qWarning() << "EditorDeleteItemCommand::undo() - Failed to restore script" << info.itemName;
            } else {
                mLastOperationWasValid = true;
                int newID = pRestoredScript->getID();
                int oldID = info.itemID;

                originalInfo.itemID = newID;

                // Track ID change for stack remapping
                if (newID != oldID) {
                    mIDChanges.append(qMakePair(oldID, newID));
                }

                // If ID changed, update all remaining items that reference this as their parent
                if (newID != oldID) {
                    for (int j = i + 1; j < sortedItems.size(); ++j) {
                        if (sortedItems[j].parentID == oldID) {
                            sortedItems[j].parentID = newID;

                            // Also update in mDeletedItems so we can find it later
                            auto childIt = std::find_if(mDeletedItems.begin(), mDeletedItems.end(), [&sortedItems, j, oldID](const DeletedItemInfo& item) {
                                return item.itemName == sortedItems[j].itemName && item.parentID == oldID;
                            });
                            if (childIt != mDeletedItems.end()) {
                                childIt->parentID = newID;
                            }
                        }
                    }
                }

                // Walk the restored script's children and update their IDs in mDeletedItems
                std::function<void(TScript*, int)> updateChildIDs = [&](TScript* pS, int parentID) {
                    if (!pS || !pS->mpMyChildrenList) {
                        return;
                    }
                    for (auto* pChild : *pS->mpMyChildrenList) {
                        auto childIt = std::find_if(mDeletedItems.begin(), mDeletedItems.end(), [pChild, parentID](const DeletedItemInfo& item) {
                            return item.itemName == pChild->getName() && item.parentID == parentID;
                        });
                        if (childIt != mDeletedItems.end()) {
                            int childOldID = childIt->itemID;
                            int childNewID = pChild->getID();
                            if (childOldID != childNewID) {
                                childIt->itemID = childNewID;
                                // Update grandchildren's parentID references in mDeletedItems
                                for (auto& item : mDeletedItems) {
                                    if (item.parentID == childOldID) {
                                        item.parentID = childNewID;
                                    }
                                }
                                updateChildIDs(pChild, childNewID);
                            }
                        }
                    }
                };
                updateChildIDs(pRestoredScript, newID);
            }
            break;
        }
        case EditorViewType::cmKeysView: {
            TKey* pParent = nullptr;
            if (info.parentID != -1) {
                pParent = mpHost->getKeyUnit()->getKey(info.parentID);
            }

            TKey* pRestoredKey = importKeyFromXML(info.xmlSnapshot, pParent, mpHost, info.positionInParent);
            if (!pRestoredKey) {
                qWarning() << "EditorDeleteItemCommand::undo() - Failed to restore key" << info.itemName;
            } else {
                mLastOperationWasValid = true;
                int newID = pRestoredKey->getID();
                int oldID = info.itemID;

                originalInfo.itemID = newID;

                // Track ID change for stack remapping
                if (newID != oldID) {
                    mIDChanges.append(qMakePair(oldID, newID));
                }

                // If ID changed, update all remaining items that reference this as their parent
                if (newID != oldID) {
                    for (int j = i + 1; j < sortedItems.size(); ++j) {
                        if (sortedItems[j].parentID == oldID) {
                            sortedItems[j].parentID = newID;

                            // Also update in mDeletedItems so we can find it later
                            auto childIt = std::find_if(mDeletedItems.begin(), mDeletedItems.end(), [&sortedItems, j, oldID](const DeletedItemInfo& item) {
                                return item.itemName == sortedItems[j].itemName && item.parentID == oldID;
                            });
                            if (childIt != mDeletedItems.end()) {
                                childIt->parentID = newID;
                            }
                        }
                    }
                }

                // Walk the restored key's children and update their IDs in mDeletedItems
                std::function<void(TKey*, int)> updateChildIDs = [&](TKey* pK, int parentID) {
                    if (!pK || !pK->mpMyChildrenList) {
                        return;
                    }
                    for (auto* pChild : *pK->mpMyChildrenList) {
                        auto childIt = std::find_if(mDeletedItems.begin(), mDeletedItems.end(), [pChild, parentID](const DeletedItemInfo& item) {
                            return item.itemName == pChild->getName() && item.parentID == parentID;
                        });
                        if (childIt != mDeletedItems.end()) {
                            int childOldID = childIt->itemID;
                            int childNewID = pChild->getID();
                            if (childOldID != childNewID) {
                                childIt->itemID = childNewID;
                                // Update grandchildren's parentID references in mDeletedItems
                                for (auto& item : mDeletedItems) {
                                    if (item.parentID == childOldID) {
                                        item.parentID = childNewID;
                                    }
                                }
                                updateChildIDs(pChild, childNewID);
                            }
                        }
                    }
                };
                updateChildIDs(pRestoredKey, newID);
            }
            break;
        }
        case EditorViewType::cmActionView: {
            TAction* pParent = nullptr;
            if (info.parentID != -1) {
                pParent = mpHost->getActionUnit()->getAction(info.parentID);
            }

            TAction* pRestoredAction = importActionFromXML(info.xmlSnapshot, pParent, mpHost, info.positionInParent);
            if (!pRestoredAction) {
                qWarning() << "EditorDeleteItemCommand::undo() - Failed to restore action" << info.itemName;
            } else {
                mLastOperationWasValid = true;
                int newID = pRestoredAction->getID();
                int oldID = info.itemID;

                originalInfo.itemID = newID;

                // Track ID change for stack remapping
                if (newID != oldID) {
                    mIDChanges.append(qMakePair(oldID, newID));
                }

                // If ID changed, update all remaining items that reference this as their parent
                if (newID != oldID) {
                    for (int j = i + 1; j < sortedItems.size(); ++j) {
                        if (sortedItems[j].parentID == oldID) {
                            sortedItems[j].parentID = newID;

                            // Also update in mDeletedItems so we can find it later
                            auto childIt = std::find_if(mDeletedItems.begin(), mDeletedItems.end(), [&sortedItems, j, oldID](const DeletedItemInfo& item) {
                                return item.itemName == sortedItems[j].itemName && item.parentID == oldID;
                            });
                            if (childIt != mDeletedItems.end()) {
                                childIt->parentID = newID;
                            }
                        }
                    }
                }

                // Walk the restored action's children and update their IDs in mDeletedItems
                std::function<void(TAction*, int)> updateChildIDs = [&](TAction* pA, int parentID) {
                    if (!pA || !pA->mpMyChildrenList) {
                        return;
                    }
                    for (auto* pChild : *pA->mpMyChildrenList) {
                        auto childIt = std::find_if(mDeletedItems.begin(), mDeletedItems.end(), [pChild, parentID](const DeletedItemInfo& item) {
                            return item.itemName == pChild->getName() && item.parentID == parentID;
                        });
                        if (childIt != mDeletedItems.end()) {
                            int childOldID = childIt->itemID;
                            int childNewID = pChild->getID();
                            if (childOldID != childNewID) {
                                childIt->itemID = childNewID;
                                // Update grandchildren's parentID references in mDeletedItems
                                for (auto& item : mDeletedItems) {
                                    if (item.parentID == childOldID) {
                                        item.parentID = childNewID;
                                    }
                                }
                                updateChildIDs(pChild, childNewID);
                            }
                        }
                    }
                };
                updateChildIDs(pRestoredAction, newID);
            }
            break;
        }
        default:
            qWarning() << "EditorDeleteItemCommand::undo() - Unknown item type";
            break;
        }
    }
}

void EditorDeleteItemCommand::redo()
{
#if defined(DEBUG_UNDO_REDO)
    qDebug() << "EditorDeleteItemCommand::redo() - Deleting" << mDeletedItems.size() << "items again";
#endif
    // Delete items again
    // Note: When the command is first created, items are already deleted,
    // but we never call redo() at that point. The first time redo() is called is after
    // undo() has restored the items, so we need to delete them again.

    // Track if any items are valid (not invalidated by Lua API changes)
    mLastOperationWasValid = false;

    // First pass: validate that items still exist and have the expected names
    for (const auto& info : mDeletedItems) {
        switch (mViewType) {
        case EditorViewType::cmTriggerView: {
            TTrigger* trigger = mpHost->getTriggerUnit()->getTrigger(info.itemID);
            // Validate: item exists and has expected name (prevents deleting wrong item if ID reused)
            if (!trigger) {
                qWarning() << "EditorDeleteItemCommand::redo() - Trigger" << info.itemName << "ID:" << info.itemID << "no longer exists, skipping";
                continue;
            }
            if (trigger->getName() != info.itemName) {
                qWarning() << "EditorDeleteItemCommand::redo() - Trigger ID" << info.itemID << "expected name" << info.itemName << "but found" << trigger->getName() << ", skipping";
                continue;
            }
            // Valid item found
            mLastOperationWasValid = true;
            break;
        }
        case EditorViewType::cmAliasView: {
            TAlias* alias = mpHost->getAliasUnit()->getAlias(info.itemID);
            // Validate: item exists and has expected name
            if (!alias) {
                qWarning() << "EditorDeleteItemCommand::redo() - Alias" << info.itemName << "ID:" << info.itemID << "no longer exists, skipping";
                continue;
            }
            if (alias->getName() != info.itemName) {
                qWarning() << "EditorDeleteItemCommand::redo() - Alias ID" << info.itemID << "expected name" << info.itemName << "but found" << alias->getName() << ", skipping";
                continue;
            }
            // Valid item found
            mLastOperationWasValid = true;
            break;
        }
        case EditorViewType::cmTimerView: {
            TTimer* timer = mpHost->getTimerUnit()->getTimer(info.itemID);
            // Validate: item exists and has expected name
            if (!timer) {
                qWarning() << "EditorDeleteItemCommand::redo() - Timer" << info.itemName << "ID:" << info.itemID << "no longer exists, skipping";
                continue;
            }
            if (timer->getName() != info.itemName) {
                qWarning() << "EditorDeleteItemCommand::redo() - Timer ID" << info.itemID << "expected name" << info.itemName << "but found" << timer->getName() << ", skipping";
                continue;
            }
            // Valid item found
            mLastOperationWasValid = true;
            break;
        }
        case EditorViewType::cmScriptView: {
            TScript* script = mpHost->getScriptUnit()->getScript(info.itemID);
            // Validate: item exists and has expected name
            if (!script) {
                qWarning() << "EditorDeleteItemCommand::redo() - Script" << info.itemName << "ID:" << info.itemID << "no longer exists, skipping";
                continue;
            }
            if (script->getName() != info.itemName) {
                qWarning() << "EditorDeleteItemCommand::redo() - Script ID" << info.itemID << "expected name" << info.itemName << "but found" << script->getName() << ", skipping";
                continue;
            }
            // Valid item found
            mLastOperationWasValid = true;
            break;
        }
        case EditorViewType::cmKeysView: {
            TKey* key = mpHost->getKeyUnit()->getKey(info.itemID);
            // Validate: item exists and has expected name
            if (!key) {
                qWarning() << "EditorDeleteItemCommand::redo() - Key" << info.itemName << "ID:" << info.itemID << "no longer exists, skipping";
                continue;
            }
            if (key->getName() != info.itemName) {
                qWarning() << "EditorDeleteItemCommand::redo() - Key ID" << info.itemID << "expected name" << info.itemName << "but found" << key->getName() << ", skipping";
                continue;
            }
            // Valid item found
            mLastOperationWasValid = true;
            break;
        }
        case EditorViewType::cmActionView: {
            TAction* action = mpHost->getActionUnit()->getAction(info.itemID);
            // Validate: item exists and has expected name
            if (!action) {
                qWarning() << "EditorDeleteItemCommand::redo() - Action" << info.itemName << "ID:" << info.itemID << "no longer exists, skipping";
                continue;
            }
            if (action->getName() != info.itemName) {
                qWarning() << "EditorDeleteItemCommand::redo() - Action ID" << info.itemID << "expected name" << info.itemName << "but found" << action->getName() << ", skipping";
                continue;
            }
            // Valid item found
            mLastOperationWasValid = true;
            break;
        }
        default:
            break;
        }
    }

    // Now manually unregister and delete all items.
    // We nullify mpHost right before deleting each item to prevent destructors from
    // trying to unregister. Only delete items whose parent is not also being deleted
    // (parent deletion handles children), and only if they still match the recorded
    // name to avoid deleting unrelated items.
    for (const auto& info : mDeletedItems) {
        // Skip items whose parent is also in the deletion list
        // (they will be automatically deleted when the parent is deleted)
        if (info.parentID != -1) {
            bool parentBeingDeleted = std::any_of(mDeletedItems.begin(), mDeletedItems.end(), [&info](const DeletedItemInfo& item) { return item.itemID == info.parentID; });
            if (parentBeingDeleted) {
                continue; // Skip this item - it will be deleted by its parent
            }
        }

        switch (mViewType) {
        case EditorViewType::cmTriggerView: {
            TTrigger* trigger = mpHost->getTriggerUnit()->getTrigger(info.itemID);
            if (!trigger) {
                break;
            }
            if (trigger->getName() != info.itemName) {
                qWarning() << "EditorDeleteItemCommand::redo() - Trigger ID" << info.itemID
                           << "name changed from" << info.itemName << "to" << trigger->getName()
                           << "- not deleting";
                break;
            }
            mpHost->getTriggerUnit()->unregisterTrigger(trigger);
            trigger->mpHost = nullptr;
            delete trigger;
            break;
        }
        case EditorViewType::cmAliasView: {
            TAlias* alias = mpHost->getAliasUnit()->getAlias(info.itemID);
            if (!alias) {
                break;
            }
            if (alias->getName() != info.itemName) {
                qWarning() << "EditorDeleteItemCommand::redo() - Alias ID" << info.itemID
                           << "name changed from" << info.itemName << "to" << alias->getName()
                           << "- not deleting";
                break;
            }
            mpHost->getAliasUnit()->unregisterAlias(alias);
            alias->mpHost = nullptr;
            delete alias;
            break;
        }
        case EditorViewType::cmTimerView: {
            TTimer* timer = mpHost->getTimerUnit()->getTimer(info.itemID);
            if (!timer) {
                break;
            }
            if (timer->getName() != info.itemName) {
                qWarning() << "EditorDeleteItemCommand::redo() - Timer ID" << info.itemID
                           << "name changed from" << info.itemName << "to" << timer->getName()
                           << "- not deleting";
                break;
            }
            mpHost->getTimerUnit()->unregisterTimer(timer);
            timer->mpHost = nullptr;
            delete timer;
            break;
        }
        case EditorViewType::cmScriptView: {
            TScript* script = mpHost->getScriptUnit()->getScript(info.itemID);
            if (!script) {
                break;
            }
            if (script->getName() != info.itemName) {
                qWarning() << "EditorDeleteItemCommand::redo() - Script ID" << info.itemID
                           << "name changed from" << info.itemName << "to" << script->getName()
                           << "- not deleting";
                break;
            }
            mpHost->getScriptUnit()->unregisterScript(script);
            script->mpHost = nullptr;
            delete script;
            break;
        }
        case EditorViewType::cmKeysView: {
            TKey* key = mpHost->getKeyUnit()->getKey(info.itemID);
            if (!key) {
                break;
            }
            if (key->getName() != info.itemName) {
                qWarning() << "EditorDeleteItemCommand::redo() - Key ID" << info.itemID
                           << "name changed from" << info.itemName << "to" << key->getName()
                           << "- not deleting";
                break;
            }
            mpHost->getKeyUnit()->unregisterKey(key);
            key->mpHost = nullptr;
            delete key;
            break;
        }
        case EditorViewType::cmActionView: {
            TAction* action = mpHost->getActionUnit()->getAction(info.itemID);
            if (!action) {
                break;
            }
            if (action->getName() != info.itemName) {
                qWarning() << "EditorDeleteItemCommand::redo() - Action ID" << info.itemID
                           << "name changed from" << info.itemName << "to" << action->getName()
                           << "- not deleting";
                break;
            }
            mpHost->getActionUnit()->unregisterAction(action);
            action->mpHost = nullptr;
            delete action;
            break;
        }
        default:
            break;
        }
    }
}
QString EditorDeleteItemCommand::generateText(EditorViewType viewType, int itemCount, const QString& firstName)
{
    if (itemCount == 1) {
        // Single item deletion - use item name
        switch (viewType) {
        case EditorViewType::cmTriggerView:
            //: Undo/redo menu text for deleting a single trigger
            return QObject::tr("delete trigger \"%1\"").arg(firstName);
        case EditorViewType::cmAliasView:
            //: Undo/redo menu text for deleting a single alias
            return QObject::tr("delete alias \"%1\"").arg(firstName);
        case EditorViewType::cmTimerView:
            //: Undo/redo menu text for deleting a single timer
            return QObject::tr("delete timer \"%1\"").arg(firstName);
        case EditorViewType::cmScriptView:
            //: Undo/redo menu text for deleting a single script
            return QObject::tr("delete script \"%1\"").arg(firstName);
        case EditorViewType::cmKeysView:
            //: Undo/redo menu text for deleting a single key binding
            return QObject::tr("delete key \"%1\"").arg(firstName);
        case EditorViewType::cmActionView:
            //: Undo/redo menu text for deleting a single button
            return QObject::tr("delete button \"%1\"").arg(firstName);
        default:
            //: Undo/redo menu text for deleting a single unknown item
            return QObject::tr("delete item \"%1\"").arg(firstName);
        }
    } else {
        // Multiple items deletion - use count
        switch (viewType) {
        case EditorViewType::cmTriggerView:
            //: Undo/redo menu text for deleting multiple triggers. %1 = count
            return QObject::tr("delete %1 triggers").arg(itemCount);
        case EditorViewType::cmAliasView:
            //: Undo/redo menu text for deleting multiple aliases. %1 = count
            return QObject::tr("delete %1 aliases").arg(itemCount);
        case EditorViewType::cmTimerView:
            //: Undo/redo menu text for deleting multiple timers. %1 = count
            return QObject::tr("delete %1 timers").arg(itemCount);
        case EditorViewType::cmScriptView:
            //: Undo/redo menu text for deleting multiple scripts. %1 = count
            return QObject::tr("delete %1 scripts").arg(itemCount);
        case EditorViewType::cmKeysView:
            //: Undo/redo menu text for deleting multiple key bindings. %1 = count
            return QObject::tr("delete %1 keys").arg(itemCount);
        case EditorViewType::cmActionView:
            //: Undo/redo menu text for deleting multiple buttons. %1 = count
            return QObject::tr("delete %1 buttons").arg(itemCount);
        default:
            //: Undo/redo menu text for deleting multiple unknown items. %1 = count
            return QObject::tr("delete %1 items").arg(itemCount);
        }
    }
}

QList<int> EditorDeleteItemCommand::affectedItemIDs() const
{
    QList<int> ids;
    for (const auto& item : mDeletedItems) {
        ids.append(item.itemID);
    }
    return ids;
}

// Updates stored IDs when items are deleted and recreated (e.g., during undo/redo)
void EditorDeleteItemCommand::remapItemID(int oldID, int newID)
{
    for (auto& item : mDeletedItems) {
        if (item.itemID == oldID) {
            item.itemID = newID;
        }
        if (item.parentID == oldID) {
            item.parentID = newID;
        }
    }
}

const EditorDeleteItemCommand::DeletedItemInfo* EditorDeleteItemCommand::getDeletedItemInfo(int itemID) const
{
    for (const auto& item : mDeletedItems) {
        if (item.itemID == itemID) {
            return &item;
        }
    }
    return nullptr;
}
