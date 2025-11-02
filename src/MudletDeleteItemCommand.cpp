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

#include "MudletDeleteItemCommand.h"

#include "EditorItemXMLHelpers.h"
#include "Host.h"
#include "TAction.h"
#include "TAlias.h"
#include "TKey.h"
#include "TScript.h"
#include "TTimer.h"
#include "TTrigger.h"

#include <algorithm>

MudletDeleteItemCommand::MudletDeleteItemCommand(EditorViewType viewType, const QList<DeletedItemInfo>& deletedItems, Host* host)
: MudletEditorCommand(generateText(viewType, deletedItems.size(), deletedItems.isEmpty() ? QString() : deletedItems.first().itemName), host)
, mViewType(viewType)
, mDeletedItems(deletedItems)
{
}

void MudletDeleteItemCommand::undo() {
#if defined(DEBUG_UNDO_REDO)
    qDebug() << "MudletDeleteItemCommand::undo() - Restoring" << mDeletedItems.size() << "deleted items";
#endif
    // Clear ID changes from any previous undo
    mIDChanges.clear();

    // Track if any items are valid (not invalidated by Lua API changes)
    mLastOperationWasValid = false;

    // Restore all deleted items from their XML snapshots
    // Sort items to restore in correct order:
    // 1. Parents before children (items whose parent is not in deleted list come first)
    // 2. Siblings in position order (lower position first)
    QList<DeletedItemInfo> sortedItems = mDeletedItems;
    std::sort(sortedItems.begin(), sortedItems.end(), [&sortedItems](const DeletedItemInfo& a, const DeletedItemInfo& b) {
        // Check if parent is in the deleted list
        bool aParentDeleted = std::any_of(sortedItems.begin(), sortedItems.end(),
                                          [&a](const DeletedItemInfo& item) { return item.itemID == a.parentID; });
        bool bParentDeleted = std::any_of(sortedItems.begin(), sortedItems.end(),
                                          [&b](const DeletedItemInfo& item) { return item.itemID == b.parentID; });

        // Items whose parent is not deleted come first
        if (aParentDeleted != bParentDeleted) {
            return !aParentDeleted; // a comes first if its parent is not deleted
        }

        // Within same parent, sort by position (ascending)
        if (a.parentID == b.parentID) {
            return a.positionInParent < b.positionInParent;
        }

        // Different parents, maintain original order
        return false;
    });

    for (int i = 0; i < sortedItems.size(); ++i) {
        auto& info = sortedItems[i];

        // Skip items whose parent was also deleted - they'll be restored from parent's XML
        bool skipRestore = false;
        if (info.parentID != -1) {
            bool parentWasDeleted = std::any_of(mDeletedItems.begin(), mDeletedItems.end(),
                                                [&info](const DeletedItemInfo& item) {
                                                    return item.itemID == info.parentID;
                                                });
            if (parentWasDeleted) {
                skipRestore = true;
            }
        }

        // Find the corresponding item in mDeletedItems to update the ID
        auto it = std::find_if(mDeletedItems.begin(), mDeletedItems.end(),
                               [&info](const DeletedItemInfo& item) {
                                   return item.itemName == info.itemName && item.parentID == info.parentID;
                               });
        if (it == mDeletedItems.end()) {
            qWarning() << "MudletDeleteItemCommand::undo() - Could not find item in original list:" << info.itemName;
            continue;
        }
        auto& originalInfo = *it;

        if (skipRestore) {
            // Item was restored from parent's XML, but we still need to find its new ID for redo
            // This will be done after parent is restored
            continue;
        }

        switch (mViewType) {
        case EditorViewType::cmTriggerView: {
            // Get parent trigger
            TTrigger* pParent = nullptr;
            if (info.parentID != -1) {
                pParent = mpHost->getTriggerUnit()->getTrigger(info.parentID);
                if (!pParent) {
                    qWarning() << "MudletDeleteItemCommand::undo() - Parent trigger not found for" << info.itemName
                               << "parentID=" << info.parentID;
                }
            }

            // Restore the trigger from XML snapshot at its original position
            TTrigger* pRestoredTrigger = importTriggerFromXML(info.xmlSnapshot, pParent, mpHost, info.positionInParent);
            if (!pRestoredTrigger) {
                qWarning() << "MudletDeleteItemCommand::undo() - Failed to restore trigger" << info.itemName;
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
                            auto childIt = std::find_if(mDeletedItems.begin(), mDeletedItems.end(),
                                                        [&sortedItems, j, oldID](const DeletedItemInfo& item) {
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
                        auto childIt = std::find_if(mDeletedItems.begin(), mDeletedItems.end(),
                                                     [pChild, parentID](const DeletedItemInfo& item) {
                                                         return item.itemName == pChild->getName() && item.parentID == parentID;
                                                     });
                        if (childIt != mDeletedItems.end()) {
                            int childOldID = childIt->itemID;
                            int childNewID = pChild->getID();
                            if (childOldID != childNewID) {
                                childIt->itemID = childNewID;
                                // Recursively update grandchildren
                                updateChildIDs(pChild, childNewID);
                            }
                        }
                    }
                };
                updateChildIDs(pRestoredTrigger, oldID);
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
                qWarning() << "MudletDeleteItemCommand::undo() - Failed to restore alias" << info.itemName;
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
                            auto childIt = std::find_if(mDeletedItems.begin(), mDeletedItems.end(),
                                                        [&sortedItems, j, oldID](const DeletedItemInfo& item) {
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
                        auto childIt = std::find_if(mDeletedItems.begin(), mDeletedItems.end(),
                                                     [pChild, parentID](const DeletedItemInfo& item) {
                                                         return item.itemName == pChild->getName() && item.parentID == parentID;
                                                     });
                        if (childIt != mDeletedItems.end()) {
                            int childOldID = childIt->itemID;
                            int childNewID = pChild->getID();
                            if (childOldID != childNewID) {
                                childIt->itemID = childNewID;
                                updateChildIDs(pChild, childNewID);
                            }
                        }
                    }
                };
                updateChildIDs(pRestoredAlias, oldID);
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
                qWarning() << "MudletDeleteItemCommand::undo() - Failed to restore timer" << info.itemName;
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
                            auto childIt = std::find_if(mDeletedItems.begin(), mDeletedItems.end(),
                                                        [&sortedItems, j, oldID](const DeletedItemInfo& item) {
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
                        auto childIt = std::find_if(mDeletedItems.begin(), mDeletedItems.end(),
                                                     [pChild, parentID](const DeletedItemInfo& item) {
                                                         return item.itemName == pChild->getName() && item.parentID == parentID;
                                                     });
                        if (childIt != mDeletedItems.end()) {
                            int childOldID = childIt->itemID;
                            int childNewID = pChild->getID();
                            if (childOldID != childNewID) {
                                childIt->itemID = childNewID;
                                updateChildIDs(pChild, childNewID);
                            }
                        }
                    }
                };
                updateChildIDs(pRestoredTimer, oldID);
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
                qWarning() << "MudletDeleteItemCommand::undo() - Failed to restore script" << info.itemName;
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
                            auto childIt = std::find_if(mDeletedItems.begin(), mDeletedItems.end(),
                                                        [&sortedItems, j, oldID](const DeletedItemInfo& item) {
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
                        auto childIt = std::find_if(mDeletedItems.begin(), mDeletedItems.end(),
                                                     [pChild, parentID](const DeletedItemInfo& item) {
                                                         return item.itemName == pChild->getName() && item.parentID == parentID;
                                                     });
                        if (childIt != mDeletedItems.end()) {
                            int childOldID = childIt->itemID;
                            int childNewID = pChild->getID();
                            if (childOldID != childNewID) {
                                childIt->itemID = childNewID;
                                updateChildIDs(pChild, childNewID);
                            }
                        }
                    }
                };
                updateChildIDs(pRestoredScript, oldID);
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
                qWarning() << "MudletDeleteItemCommand::undo() - Failed to restore key" << info.itemName;
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
                            auto childIt = std::find_if(mDeletedItems.begin(), mDeletedItems.end(),
                                                        [&sortedItems, j, oldID](const DeletedItemInfo& item) {
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
                        auto childIt = std::find_if(mDeletedItems.begin(), mDeletedItems.end(),
                                                     [pChild, parentID](const DeletedItemInfo& item) {
                                                         return item.itemName == pChild->getName() && item.parentID == parentID;
                                                     });
                        if (childIt != mDeletedItems.end()) {
                            int childOldID = childIt->itemID;
                            int childNewID = pChild->getID();
                            if (childOldID != childNewID) {
                                childIt->itemID = childNewID;
                                updateChildIDs(pChild, childNewID);
                            }
                        }
                    }
                };
                updateChildIDs(pRestoredKey, oldID);
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
                qWarning() << "MudletDeleteItemCommand::undo() - Failed to restore action" << info.itemName;
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
                            auto childIt = std::find_if(mDeletedItems.begin(), mDeletedItems.end(),
                                                        [&sortedItems, j, oldID](const DeletedItemInfo& item) {
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
                        auto childIt = std::find_if(mDeletedItems.begin(), mDeletedItems.end(),
                                                     [pChild, parentID](const DeletedItemInfo& item) {
                                                         return item.itemName == pChild->getName() && item.parentID == parentID;
                                                     });
                        if (childIt != mDeletedItems.end()) {
                            int childOldID = childIt->itemID;
                            int childNewID = pChild->getID();
                            if (childOldID != childNewID) {
                                childIt->itemID = childNewID;
                                updateChildIDs(pChild, childNewID);
                            }
                        }
                    }
                };
                updateChildIDs(pRestoredAction, oldID);
            }
            break;
        }
        default:
            qWarning() << "MudletDeleteItemCommand::undo() - Unknown item type";
            break;
        }
    }
}

void MudletDeleteItemCommand::redo() {
#if defined(DEBUG_UNDO_REDO)
    qDebug() << "MudletDeleteItemCommand::redo() - Deleting" << mDeletedItems.size() << "items again";
#endif
    // Delete items again
    // Note: When the command is first created, items are already deleted,
    // but we never call redo() at that point. The first time redo() is called is after
    // undo() has restored the items, so we need to delete them again.

    // Track if any items are valid (not invalidated by Lua API changes)
    mLastOperationWasValid = false;

    // Important: Set mpHost to null before deleting to prevent items from trying to
    // unregister themselves during destruction (which could cause iterator invalidation
    // or access to partially-destroyed parent items)
    for (const auto& info : mDeletedItems) {
        switch (mViewType) {
        case EditorViewType::cmTriggerView: {
            TTrigger* trigger = mpHost->getTriggerUnit()->getTrigger(info.itemID);
            // Validate: item exists and has expected name (prevents deleting wrong item if ID reused)
            if (!trigger) {
                qWarning() << "MudletDeleteItemCommand::redo() - Trigger" << info.itemName
                          << "ID:" << info.itemID << "no longer exists, skipping";
                continue;
            }
            if (trigger->getName() != info.itemName) {
                qWarning() << "MudletDeleteItemCommand::redo() - Trigger ID" << info.itemID
                          << "expected name" << info.itemName << "but found" << trigger->getName() << ", skipping";
                continue;
            }
            // Valid item found
            mLastOperationWasValid = true;
            if (trigger) {
                // Nullify mpHost on trigger and all children recursively
                trigger->mpHost = nullptr;
                std::function<void(TTrigger*)> nullifyChildren = [&nullifyChildren](TTrigger* t) {
                    for (auto child : *t->mpMyChildrenList) {
                        child->mpHost = nullptr;
                        nullifyChildren(child);
                    }
                };
                nullifyChildren(trigger);
            }
            break;
        }
        case EditorViewType::cmAliasView: {
            TAlias* alias = mpHost->getAliasUnit()->getAlias(info.itemID);
            // Validate: item exists and has expected name
            if (!alias) {
                qWarning() << "MudletDeleteItemCommand::redo() - Alias" << info.itemName
                          << "ID:" << info.itemID << "no longer exists, skipping";
                continue;
            }
            if (alias->getName() != info.itemName) {
                qWarning() << "MudletDeleteItemCommand::redo() - Alias ID" << info.itemID
                          << "expected name" << info.itemName << "but found" << alias->getName() << ", skipping";
                continue;
            }
            // Valid item found
            mLastOperationWasValid = true;
            if (alias) {
                // Nullify mpHost on alias and all children recursively
                alias->mpHost = nullptr;
                std::function<void(TAlias*)> nullifyChildren = [&nullifyChildren](TAlias* a) {
                    for (auto child : *a->mpMyChildrenList) {
                        child->mpHost = nullptr;
                        nullifyChildren(child);
                    }
                };
                nullifyChildren(alias);
            }
            break;
        }
        case EditorViewType::cmTimerView: {
            TTimer* timer = mpHost->getTimerUnit()->getTimer(info.itemID);
            // Validate: item exists and has expected name
            if (!timer) {
                qWarning() << "MudletDeleteItemCommand::redo() - Timer" << info.itemName
                          << "ID:" << info.itemID << "no longer exists, skipping";
                continue;
            }
            if (timer->getName() != info.itemName) {
                qWarning() << "MudletDeleteItemCommand::redo() - Timer ID" << info.itemID
                          << "expected name" << info.itemName << "but found" << timer->getName() << ", skipping";
                continue;
            }
            // Valid item found
            mLastOperationWasValid = true;
            if (timer) {
                // Nullify mpHost on timer and all children recursively
                timer->mpHost = nullptr;
                std::function<void(TTimer*)> nullifyChildren = [&nullifyChildren](TTimer* t) {
                    for (auto child : *t->mpMyChildrenList) {
                        child->mpHost = nullptr;
                        nullifyChildren(child);
                    }
                };
                nullifyChildren(timer);
            }
            break;
        }
        case EditorViewType::cmScriptView: {
            TScript* script = mpHost->getScriptUnit()->getScript(info.itemID);
            // Validate: item exists and has expected name
            if (!script) {
                qWarning() << "MudletDeleteItemCommand::redo() - Script" << info.itemName
                          << "ID:" << info.itemID << "no longer exists, skipping";
                continue;
            }
            if (script->getName() != info.itemName) {
                qWarning() << "MudletDeleteItemCommand::redo() - Script ID" << info.itemID
                          << "expected name" << info.itemName << "but found" << script->getName() << ", skipping";
                continue;
            }
            // Valid item found
            mLastOperationWasValid = true;
            if (script) {
                // Nullify mpHost on script and all children recursively
                script->mpHost = nullptr;
                std::function<void(TScript*)> nullifyChildren = [&nullifyChildren](TScript* s) {
                    for (auto child : *s->mpMyChildrenList) {
                        child->mpHost = nullptr;
                        nullifyChildren(child);
                    }
                };
                nullifyChildren(script);
            }
            break;
        }
        case EditorViewType::cmKeysView: {
            TKey* key = mpHost->getKeyUnit()->getKey(info.itemID);
            // Validate: item exists and has expected name
            if (!key) {
                qWarning() << "MudletDeleteItemCommand::redo() - Key" << info.itemName
                          << "ID:" << info.itemID << "no longer exists, skipping";
                continue;
            }
            if (key->getName() != info.itemName) {
                qWarning() << "MudletDeleteItemCommand::redo() - Key ID" << info.itemID
                          << "expected name" << info.itemName << "but found" << key->getName() << ", skipping";
                continue;
            }
            // Valid item found
            mLastOperationWasValid = true;
            if (key) {
                // Nullify mpHost on key and all children recursively
                key->mpHost = nullptr;
                std::function<void(TKey*)> nullifyChildren = [&nullifyChildren](TKey* k) {
                    for (auto child : *k->mpMyChildrenList) {
                        child->mpHost = nullptr;
                        nullifyChildren(child);
                    }
                };
                nullifyChildren(key);
            }
            break;
        }
        case EditorViewType::cmActionView: {
            TAction* action = mpHost->getActionUnit()->getAction(info.itemID);
            // Validate: item exists and has expected name
            if (!action) {
                qWarning() << "MudletDeleteItemCommand::redo() - Action" << info.itemName
                          << "ID:" << info.itemID << "no longer exists, skipping";
                continue;
            }
            if (action->getName() != info.itemName) {
                qWarning() << "MudletDeleteItemCommand::redo() - Action ID" << info.itemID
                          << "expected name" << info.itemName << "but found" << action->getName() << ", skipping";
                continue;
            }
            // Valid item found
            mLastOperationWasValid = true;
            if (action) {
                // Nullify mpHost on action and all children recursively
                action->mpHost = nullptr;
                std::function<void(TAction*)> nullifyChildren = [&nullifyChildren](TAction* a) {
                    for (auto child : *a->mpMyChildrenList) {
                        child->mpHost = nullptr;
                        nullifyChildren(child);
                    }
                };
                nullifyChildren(action);
            }
            break;
        }
        default:
            break;
        }
    }

    // Now manually unregister and delete all items
    // Since mpHost is null, destructors won't try to unregister, so we must do it manually
    for (const auto& info : mDeletedItems) {
        switch (mViewType) {
        case EditorViewType::cmTriggerView: {
            TTrigger* trigger = mpHost->getTriggerUnit()->getTrigger(info.itemID);
            if (trigger) {
                mpHost->getTriggerUnit()->unregisterTrigger(trigger);
                delete trigger;
            }
            break;
        }
        case EditorViewType::cmAliasView: {
            TAlias* alias = mpHost->getAliasUnit()->getAlias(info.itemID);
            if (alias) {
                mpHost->getAliasUnit()->unregisterAlias(alias);
                delete alias;
            }
            break;
        }
        case EditorViewType::cmTimerView: {
            TTimer* timer = mpHost->getTimerUnit()->getTimer(info.itemID);
            if (timer) {
                mpHost->getTimerUnit()->unregisterTimer(timer);
                delete timer;
            }
            break;
        }
        case EditorViewType::cmScriptView: {
            TScript* script = mpHost->getScriptUnit()->getScript(info.itemID);
            if (script) {
                mpHost->getScriptUnit()->unregisterScript(script);
                delete script;
            }
            break;
        }
        case EditorViewType::cmKeysView: {
            TKey* key = mpHost->getKeyUnit()->getKey(info.itemID);
            if (key) {
                mpHost->getKeyUnit()->unregisterKey(key);
                delete key;
            }
            break;
        }
        case EditorViewType::cmActionView: {
            TAction* action = mpHost->getActionUnit()->getAction(info.itemID);
            if (action) {
                mpHost->getActionUnit()->unregisterAction(action);
                delete action;
            }
            break;
        }
        default:
            break;
        }
    }
}
QString MudletDeleteItemCommand::generateText(EditorViewType viewType, int itemCount, const QString& firstName)
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

QList<int> MudletDeleteItemCommand::affectedItemIDs() const
{
    QList<int> ids;
    for (const auto& item : mDeletedItems) {
        ids.append(item.itemID);
    }
    return ids;
}

// Updates stored IDs when items are deleted and recreated (e.g., during undo/redo)
void MudletDeleteItemCommand::remapItemID(int oldID, int newID)
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
