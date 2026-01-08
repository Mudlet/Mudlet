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

#include "EditorAddItemCommand.h"

#include "EditorItemXMLHelpers.h"
#include "Host.h"
#include "TAction.h"
#include "TAlias.h"
#include "TKey.h"
#include "TScript.h"
#include "TTimer.h"
#include "TTrigger.h"

EditorAddItemCommand::EditorAddItemCommand(EditorViewType viewType, int itemID, int parentID, int positionInParent, bool isFolder, const QString& itemName, Host* host)
: EditorCommand(generateText(viewType, itemName, isFolder), host)
, mViewType(viewType)
, mItemID(itemID)
, mParentID(parentID)
, mPositionInParent(positionInParent)
, mIsFolder(isFolder)
, mItemName(itemName)
{
}

void EditorAddItemCommand::undo()
{
#if defined(DEBUG_UNDO_REDO)
    qDebug() << "EditorAddItemCommand::undo() - Undoing add for item" << mItemName << "(ID:" << mItemID << ")";
#endif

    // Clear old descendant IDs from any previous undo
    mOldDescendantIDs.clear();

    // ALWAYS collect descendant IDs before deletion (needed for remapping on redo)
    // This must happen every time because IDs change on each recreation
    switch (mViewType) {
    case EditorViewType::cmTriggerView: {
        TTrigger* trigger = mpHost->getTriggerUnit()->getTrigger(mItemID);
        if (trigger) {
            std::function<void(TTrigger*)> collectIDs = [&](TTrigger* t) {
                if (!t) {
                    return;
                }
                mOldDescendantIDs.append(t->getID());
                auto* children = t->getChildrenList();
                if (children) {
                    for (auto* child : *children) {
                        collectIDs(child);
                    }
                }
            };
            collectIDs(trigger);
#if defined(DEBUG_UNDO_REDO)
            qDebug() << "EditorAddItemCommand::undo() - Deleting trigger with IDs:" << mOldDescendantIDs;
#endif
        }
        break;
    }
    case EditorViewType::cmAliasView: {
        TAlias* alias = mpHost->getAliasUnit()->getAlias(mItemID);
        if (alias) {
            std::function<void(TAlias*)> collectIDs = [&](TAlias* a) {
                if (!a) {
                    return;
                }
                mOldDescendantIDs.append(a->getID());
                auto* children = a->getChildrenList();
                if (children) {
                    for (auto* child : *children) {
                        collectIDs(child);
                    }
                }
            };
            collectIDs(alias);
#if defined(DEBUG_UNDO_REDO)
            qDebug() << "EditorAddItemCommand::undo() - Deleting alias with IDs:" << mOldDescendantIDs;
#endif
        }
        break;
    }
    case EditorViewType::cmTimerView: {
        TTimer* timer = mpHost->getTimerUnit()->getTimer(mItemID);
        if (timer) {
            std::function<void(TTimer*)> collectIDs = [&](TTimer* t) {
                if (!t) {
                    return;
                }
                mOldDescendantIDs.append(t->getID());
                auto* children = t->getChildrenList();
                if (children) {
                    for (auto* child : *children) {
                        collectIDs(child);
                    }
                }
            };
            collectIDs(timer);
#if defined(DEBUG_UNDO_REDO)
            qDebug() << "EditorAddItemCommand::undo() - Deleting timer with IDs:" << mOldDescendantIDs;
#endif
        }
        break;
    }
    case EditorViewType::cmScriptView: {
        TScript* script = mpHost->getScriptUnit()->getScript(mItemID);
        if (script) {
            std::function<void(TScript*)> collectIDs = [&](TScript* s) {
                if (!s) {
                    return;
                }
                mOldDescendantIDs.append(s->getID());
                auto* children = s->getChildrenList();
                if (children) {
                    for (auto* child : *children) {
                        collectIDs(child);
                    }
                }
            };
            collectIDs(script);
#if defined(DEBUG_UNDO_REDO)
            qDebug() << "EditorAddItemCommand::undo() - Deleting script with IDs:" << mOldDescendantIDs;
#endif
        }
        break;
    }
    case EditorViewType::cmKeysView: {
        TKey* key = mpHost->getKeyUnit()->getKey(mItemID);
        if (key) {
            std::function<void(TKey*)> collectIDs = [&](TKey* k) {
                if (!k) {
                    return;
                }
                mOldDescendantIDs.append(k->getID());
                auto* children = k->getChildrenList();
                if (children) {
                    for (auto* child : *children) {
                        collectIDs(child);
                    }
                }
            };
            collectIDs(key);
#if defined(DEBUG_UNDO_REDO)
            qDebug() << "EditorAddItemCommand::undo() - Deleting key with IDs:" << mOldDescendantIDs;
#endif
        }
        break;
    }
    case EditorViewType::cmActionView: {
        TAction* action = mpHost->getActionUnit()->getAction(mItemID);
        if (action) {
            std::function<void(TAction*)> collectIDs = [&](TAction* a) {
                if (!a) {
                    return;
                }
                mOldDescendantIDs.append(a->getID());
                auto* children = a->getChildrenList();
                if (children) {
                    for (auto* child : *children) {
                        collectIDs(child);
                    }
                }
            };
            collectIDs(action);
#if defined(DEBUG_UNDO_REDO)
            qDebug() << "EditorAddItemCommand::undo() - Deleting action with IDs:" << mOldDescendantIDs;
#endif
        }
        break;
    }
    default:
        break;
    }

    // Export the item to XML before deleting (for redo) - only needed once
    if (mItemSnapshot.isEmpty()) {
        switch (mViewType) {
        case EditorViewType::cmTriggerView: {
            TTrigger* trigger = mpHost->getTriggerUnit()->getTrigger(mItemID);
            if (trigger) {
                mItemSnapshot = exportTriggerToXML(trigger);
            }
            break;
        }
        case EditorViewType::cmAliasView: {
            TAlias* alias = mpHost->getAliasUnit()->getAlias(mItemID);
            if (alias) {
                mItemSnapshot = exportAliasToXML(alias);
            }
            break;
        }
        case EditorViewType::cmTimerView: {
            TTimer* timer = mpHost->getTimerUnit()->getTimer(mItemID);
            if (timer) {
                mItemSnapshot = exportTimerToXML(timer);
            }
            break;
        }
        case EditorViewType::cmScriptView: {
            TScript* script = mpHost->getScriptUnit()->getScript(mItemID);
            if (script) {
                mItemSnapshot = exportScriptToXML(script);
            }
            break;
        }
        case EditorViewType::cmKeysView: {
            TKey* key = mpHost->getKeyUnit()->getKey(mItemID);
            if (key) {
                mItemSnapshot = exportKeyToXML(key);
            }
            break;
        }
        case EditorViewType::cmActionView: {
            TAction* action = mpHost->getActionUnit()->getAction(mItemID);
            if (action) {
                mItemSnapshot = exportActionToXML(action);
            }
            break;
        }
        default:
            break;
        }
    }

    // Delete the item (unregister first to match EditorDeleteItemCommand pattern)
    switch (mViewType) {
    case EditorViewType::cmTriggerView: {
        TTrigger* trigger = mpHost->getTriggerUnit()->getTrigger(mItemID);
        if (trigger) {
            mpHost->getTriggerUnit()->unregisterTrigger(trigger);
            delete trigger;
        }
        break;
    }
    case EditorViewType::cmAliasView: {
        TAlias* alias = mpHost->getAliasUnit()->getAlias(mItemID);
        if (alias) {
            mpHost->getAliasUnit()->unregisterAlias(alias);
            delete alias;
        }
        break;
    }
    case EditorViewType::cmTimerView: {
        TTimer* timer = mpHost->getTimerUnit()->getTimer(mItemID);
        if (timer) {
            mpHost->getTimerUnit()->unregisterTimer(timer);
            delete timer;
        }
        break;
    }
    case EditorViewType::cmScriptView: {
        TScript* script = mpHost->getScriptUnit()->getScript(mItemID);
        if (script) {
            mpHost->getScriptUnit()->unregisterScript(script);
            delete script;
        }
        break;
    }
    case EditorViewType::cmKeysView: {
        TKey* key = mpHost->getKeyUnit()->getKey(mItemID);
        if (key) {
            mpHost->getKeyUnit()->unregisterKey(key);
            delete key;
        }
        break;
    }
    case EditorViewType::cmActionView: {
        TAction* action = mpHost->getActionUnit()->getAction(mItemID);
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

void EditorAddItemCommand::redo()
{
#if defined(DEBUG_UNDO_REDO)
    qDebug() << "EditorAddItemCommand::redo() - Redoing add for item" << mItemName << "(ID:" << mItemID << ")";
#endif
    // Skip the first redo() which is automatically called by QUndoStack::push()
    // The item has already been added by the user action
    if (mSkipFirstRedo) {
#if defined(DEBUG_UNDO_REDO)
        qDebug() << "EditorAddItemCommand::redo() - Skipping first redo (item already added)";
#endif
        mSkipFirstRedo = false;
        return;
    }

    // Clear ID changes from any previous redo
    mIDChanges.clear();

    // Recreate the item from XML snapshot
    // Note: The first time redo() is actually executed (after undo), we need to recreate the item
    if (!mItemSnapshot.isEmpty()) {
        // Track old ID for remapping purposes
        mOldItemID = mItemID;

        // Recreate based on view type
        switch (mViewType) {
        case EditorViewType::cmTriggerView: {
            TTrigger* pParent = nullptr;
            if (mParentID != -1) {
                pParent = mpHost->getTriggerUnit()->getTrigger(mParentID);
            }
            TTrigger* pNewTrigger = importTriggerFromXML(mItemSnapshot, pParent, mpHost, mPositionInParent);
            if (pNewTrigger) {
                mItemID = pNewTrigger->getID();

                // Collect all new descendant IDs and map to old IDs
                QList<int> newDescendantIDs;
                std::function<void(TTrigger*)> collectIDs = [&](TTrigger* t) {
                    if (!t)
                        return;
                    newDescendantIDs.append(t->getID());
                    auto* children = t->getChildrenList();
                    if (children) {
                        for (auto* child : *children) {
                            collectIDs(child);
                        }
                    }
                };
                collectIDs(pNewTrigger);

                if (mOldDescendantIDs.size() == newDescendantIDs.size()) {
                    for (int i = 0; i < mOldDescendantIDs.size(); ++i) {
                        if (mOldDescendantIDs[i] != newDescendantIDs[i]) {
                            mIDChanges.append(qMakePair(mOldDescendantIDs[i], newDescendantIDs[i]));
                        }
                    }
                }
            } else {
#if defined(DEBUG_UNDO_REDO)
                qWarning() << "EditorAddItemCommand::redo() - Failed to recreate trigger from snapshot";
#endif
            }
            break;
        }
        case EditorViewType::cmAliasView: {
            TAlias* pAliasParent = nullptr;
            if (mParentID != -1) {
                pAliasParent = mpHost->getAliasUnit()->getAlias(mParentID);
            }
            TAlias* pNewAlias = importAliasFromXML(mItemSnapshot, pAliasParent, mpHost, mPositionInParent);
            if (pNewAlias) {
                mItemID = pNewAlias->getID();

                // Collect all new descendant IDs and map to old IDs
                QList<int> newDescendantIDs;
                std::function<void(TAlias*)> collectIDs = [&](TAlias* a) {
                    if (!a)
                        return;
                    newDescendantIDs.append(a->getID());
                    auto* children = a->getChildrenList();
                    if (children) {
                        for (auto* child : *children) {
                            collectIDs(child);
                        }
                    }
                };
                collectIDs(pNewAlias);

                if (mOldDescendantIDs.size() == newDescendantIDs.size()) {
                    for (int i = 0; i < mOldDescendantIDs.size(); ++i) {
                        if (mOldDescendantIDs[i] != newDescendantIDs[i]) {
                            mIDChanges.append(qMakePair(mOldDescendantIDs[i], newDescendantIDs[i]));
                        }
                    }
                }
            } else {
#if defined(DEBUG_UNDO_REDO)
                qWarning() << "EditorAddItemCommand::redo() - Failed to recreate alias from snapshot";
#endif
            }
            break;
        }
        case EditorViewType::cmTimerView: {
            TTimer* pTimerParent = nullptr;
            if (mParentID != -1) {
                pTimerParent = mpHost->getTimerUnit()->getTimer(mParentID);
            }
            TTimer* pNewTimer = importTimerFromXML(mItemSnapshot, pTimerParent, mpHost, mPositionInParent);
            if (pNewTimer) {
                mItemID = pNewTimer->getID();

                // Collect all new descendant IDs and map to old IDs
                QList<int> newDescendantIDs;
                std::function<void(TTimer*)> collectIDs = [&](TTimer* t) {
                    if (!t)
                        return;
                    newDescendantIDs.append(t->getID());
                    auto* children = t->getChildrenList();
                    if (children) {
                        for (auto* child : *children) {
                            collectIDs(child);
                        }
                    }
                };
                collectIDs(pNewTimer);

                if (mOldDescendantIDs.size() == newDescendantIDs.size()) {
                    for (int i = 0; i < mOldDescendantIDs.size(); ++i) {
                        if (mOldDescendantIDs[i] != newDescendantIDs[i]) {
                            mIDChanges.append(qMakePair(mOldDescendantIDs[i], newDescendantIDs[i]));
                        }
                    }
                }
            } else {
#if defined(DEBUG_UNDO_REDO)
                qWarning() << "EditorAddItemCommand::redo() - Failed to recreate timer from snapshot";
#endif
            }
            break;
        }
        case EditorViewType::cmScriptView: {
            TScript* pScriptParent = nullptr;
            if (mParentID != -1) {
                pScriptParent = mpHost->getScriptUnit()->getScript(mParentID);
            }
            TScript* pNewScript = importScriptFromXML(mItemSnapshot, pScriptParent, mpHost, mPositionInParent);
            if (pNewScript) {
                mItemID = pNewScript->getID();

                // Collect all new descendant IDs after recreation
                QList<int> newDescendantIDs;
                std::function<void(TScript*)> collectIDs = [&](TScript* s) {
                    if (!s)
                        return;
                    newDescendantIDs.append(s->getID());
                    auto* children = s->getChildrenList();
                    if (children) {
                        for (auto* child : *children) {
                            collectIDs(child);
                        }
                    }
                };
                collectIDs(pNewScript);

#if defined(DEBUG_UNDO_REDO)
                qDebug() << "EditorAddItemCommand::redo() - Old IDs:" << mOldDescendantIDs;
                qDebug() << "EditorAddItemCommand::redo() - New IDs:" << newDescendantIDs;
#endif

                // Map old IDs to new IDs (they're in same traversal order)
                if (mOldDescendantIDs.size() == newDescendantIDs.size()) {
                    for (int i = 0; i < mOldDescendantIDs.size(); ++i) {
                        int oldID = mOldDescendantIDs[i];
                        int newID = newDescendantIDs[i];
                        if (oldID != newID) {
                            mIDChanges.append(qMakePair(oldID, newID));
#if defined(DEBUG_UNDO_REDO)
                            qDebug() << "EditorAddItemCommand::redo() - ID mapping:" << oldID << "->" << newID;
#endif
                        }
                    }
                } else {
#if defined(DEBUG_UNDO_REDO)
                    qWarning() << "EditorAddItemCommand::redo() - ID count mismatch! Old:" << mOldDescendantIDs.size() << "New:" << newDescendantIDs.size();
#endif
                }
            } else {
#if defined(DEBUG_UNDO_REDO)
                qWarning() << "EditorAddItemCommand::redo() - Failed to recreate script from snapshot";
#endif
            }
            break;
        }
        case EditorViewType::cmKeysView: {
            TKey* pKeyParent = nullptr;
            if (mParentID != -1) {
                pKeyParent = mpHost->getKeyUnit()->getKey(mParentID);
            }
            TKey* pNewKey = importKeyFromXML(mItemSnapshot, pKeyParent, mpHost, mPositionInParent);
            if (pNewKey) {
                mItemID = pNewKey->getID();

                // Collect all new descendant IDs and map to old IDs
                QList<int> newDescendantIDs;
                std::function<void(TKey*)> collectIDs = [&](TKey* k) {
                    if (!k)
                        return;
                    newDescendantIDs.append(k->getID());
                    auto* children = k->getChildrenList();
                    if (children) {
                        for (auto* child : *children) {
                            collectIDs(child);
                        }
                    }
                };
                collectIDs(pNewKey);

                if (mOldDescendantIDs.size() == newDescendantIDs.size()) {
                    for (int i = 0; i < mOldDescendantIDs.size(); ++i) {
                        if (mOldDescendantIDs[i] != newDescendantIDs[i]) {
                            mIDChanges.append(qMakePair(mOldDescendantIDs[i], newDescendantIDs[i]));
                        }
                    }
                }
            } else {
#if defined(DEBUG_UNDO_REDO)
                qWarning() << "EditorAddItemCommand::redo() - Failed to recreate key from snapshot";
#endif
            }
            break;
        }
        case EditorViewType::cmActionView: {
            TAction* pActionParent = nullptr;
            if (mParentID != -1) {
                pActionParent = mpHost->getActionUnit()->getAction(mParentID);
            }
            TAction* pNewAction = importActionFromXML(mItemSnapshot, pActionParent, mpHost, mPositionInParent);
            if (pNewAction) {
                mItemID = pNewAction->getID();

                // Collect all new descendant IDs and map to old IDs
                QList<int> newDescendantIDs;
                std::function<void(TAction*)> collectIDs = [&](TAction* a) {
                    if (!a)
                        return;
                    newDescendantIDs.append(a->getID());
                    auto* children = a->getChildrenList();
                    if (children) {
                        for (auto* child : *children) {
                            collectIDs(child);
                        }
                    }
                };
                collectIDs(pNewAction);

                if (mOldDescendantIDs.size() == newDescendantIDs.size()) {
                    for (int i = 0; i < mOldDescendantIDs.size(); ++i) {
                        if (mOldDescendantIDs[i] != newDescendantIDs[i]) {
                            mIDChanges.append(qMakePair(mOldDescendantIDs[i], newDescendantIDs[i]));
                        }
                    }
                }
            } else {
#if defined(DEBUG_UNDO_REDO)
                qWarning() << "EditorAddItemCommand::redo() - Failed to recreate action from snapshot";
#endif
            }
            break;
        }
        default:
            break;
        }
    }
}

// Updates stored IDs when items are deleted and recreated (e.g., during undo/redo)
void EditorAddItemCommand::remapItemID(int oldID, int newID)
{
    if (mItemID == oldID) {
        mItemID = newID;
    }
    if (mParentID == oldID) {
        mParentID = newID;
    }
    if (mOldItemID == oldID) {
        mOldItemID = newID;
    }
}

QString EditorAddItemCommand::generateText(EditorViewType viewType, const QString& itemName, bool isFolder)
{
    switch (viewType) {
    case EditorViewType::cmTriggerView:
        if (isFolder) {
            //: Undo/redo menu text for adding a trigger folder
            return QObject::tr("add trigger group \"%1\"").arg(itemName);
        } else {
            //: Undo/redo menu text for adding a trigger
            return QObject::tr("add trigger \"%1\"").arg(itemName);
        }
    case EditorViewType::cmAliasView:
        if (isFolder) {
            //: Undo/redo menu text for adding an alias folder
            return QObject::tr("add alias group \"%1\"").arg(itemName);
        } else {
            //: Undo/redo menu text for adding an alias
            return QObject::tr("add alias \"%1\"").arg(itemName);
        }
    case EditorViewType::cmTimerView:
        if (isFolder) {
            //: Undo/redo menu text for adding a timer folder
            return QObject::tr("add timer group \"%1\"").arg(itemName);
        } else {
            //: Undo/redo menu text for adding a timer
            return QObject::tr("add timer \"%1\"").arg(itemName);
        }
    case EditorViewType::cmScriptView:
        if (isFolder) {
            //: Undo/redo menu text for adding a script folder
            return QObject::tr("add script group \"%1\"").arg(itemName);
        } else {
            //: Undo/redo menu text for adding a script
            return QObject::tr("add script \"%1\"").arg(itemName);
        }
    case EditorViewType::cmKeysView:
        if (isFolder) {
            //: Undo/redo menu text for adding a key folder
            return QObject::tr("add key group \"%1\"").arg(itemName);
        } else {
            //: Undo/redo menu text for adding a key binding
            return QObject::tr("add key \"%1\"").arg(itemName);
        }
    case EditorViewType::cmActionView:
        if (isFolder) {
            //: Undo/redo menu text for adding a button toolbar
            return QObject::tr("add button group \"%1\"").arg(itemName);
        } else {
            //: Undo/redo menu text for adding a button
            return QObject::tr("add button \"%1\"").arg(itemName);
        }
    default:
        if (isFolder) {
            //: Undo/redo menu text for adding an unknown folder type
            return QObject::tr("add group \"%1\"").arg(itemName);
        } else {
            //: Undo/redo menu text for adding an unknown item type
            return QObject::tr("add item \"%1\"").arg(itemName);
        }
    }
}

