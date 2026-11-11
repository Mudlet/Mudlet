/***************************************************************************
 *   Copyright (C) 2025 by Vadim Peretokin - vadim.peretokin@mudlet.org    *
 *   Copyright (C) 2026 by Stephen Lyons - slysven@virginmedia.com         *
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

#include <QSet>

EditorDeleteItemCommand::EditorDeleteItemCommand(EditorViewType viewType, const QList<DeletedItemInfo>& deletedItems, Host* host)
: EditorCommand(generateText(viewType, deletedItems.size(), deletedItems.isEmpty() ? QString() : deletedItems.first().itemName), host)
, mViewType(viewType)
, mDeletedItems(deletedItems)
{
}

void EditorDeleteItemCommand::undo()
{
#if defined(DEBUG_UNDO_REDO)
    qDebug() << "EditorDeleteItemCommand::undo() - Restoring" << mDeletedItems.size() << "deleted items";
#endif
    mIDChanges.clear();
    mLastOperationWasValid = false;

    QSet<int> deletedIDs;
    deletedIDs.reserve(mDeletedItems.size());
    QHash<int, QList<int>> childEntries;
    for (int i = 0; i < mDeletedItems.size(); ++i) {
        deletedIDs.insert(mDeletedItems.at(i).itemID);
        childEntries[mDeletedItems.at(i).parentID].append(i);
    }
    // Chosen up front as restoring rewrites parent IDs; the rest come back inside their parent's snapshot
    QList<int> topmost;
    for (int i = 0; i < mDeletedItems.size(); ++i) {
        const int parentID = mDeletedItems.at(i).parentID;
        if (parentID == -1 || !deletedIDs.contains(parentID)) {
            topmost.append(i);
        }
    }

    for (auto it = topmost.crbegin(); it != topmost.crend(); ++it) {
        auto& info = mDeletedItems[*it];
        const int newID = restoreItem(info);
        if (newID == -1) {
#if defined(DEBUG_UNDO_REDO)
            qWarning() << "EditorDeleteItemCommand::undo() - Failed to restore" << info.itemName;
#endif
            continue;
        }
        mLastOperationWasValid = true;
        const int oldID = info.itemID;
        info.itemID = newID;
        if (newID != oldID) {
            mIDChanges.append(qMakePair(oldID, newID));
        }
        adoptRestoredChildren(oldID, newID, childEntries);
    }
}

// Returns the restored item's ID, or -1 if it could not be restored
int EditorDeleteItemCommand::restoreItem(const DeletedItemInfo& info)
{
    const bool hasParent = info.parentID != -1;
    switch (mViewType) {
    case EditorViewType::cmTriggerView: {
        TTrigger* pParent = hasParent ? mpHost->getTriggerUnit()->getTrigger(info.parentID) : nullptr;
        TTrigger* pRestored = importTriggerFromXML(info.xmlSnapshot, pParent, mpHost, info.positionInParent);
        return pRestored ? pRestored->getID() : -1;
    }
    case EditorViewType::cmAliasView: {
        TAlias* pParent = hasParent ? mpHost->getAliasUnit()->getAlias(info.parentID) : nullptr;
        TAlias* pRestored = importAliasFromXML(info.xmlSnapshot, pParent, mpHost, info.positionInParent);
        return pRestored ? pRestored->getID() : -1;
    }
    case EditorViewType::cmTimerView: {
        TTimer* pParent = hasParent ? mpHost->getTimerUnit()->getTimer(info.parentID) : nullptr;
        TTimer* pRestored = importTimerFromXML(info.xmlSnapshot, pParent, mpHost, info.positionInParent);
        return pRestored ? pRestored->getID() : -1;
    }
    case EditorViewType::cmScriptView: {
        TScript* pParent = hasParent ? mpHost->getScriptUnit()->getScript(info.parentID) : nullptr;
        TScript* pRestored = importScriptFromXML(info.xmlSnapshot, pParent, mpHost, info.positionInParent);
        return pRestored ? pRestored->getID() : -1;
    }
    case EditorViewType::cmKeysView: {
        TKey* pParent = hasParent ? mpHost->getKeyUnit()->getKey(info.parentID) : nullptr;
        TKey* pRestored = importKeyFromXML(info.xmlSnapshot, pParent, mpHost, info.positionInParent);
        return pRestored ? pRestored->getID() : -1;
    }
    case EditorViewType::cmActionView: {
        TAction* pParent = hasParent ? mpHost->getActionUnit()->getAction(info.parentID) : nullptr;
        TAction* pRestored = importActionFromXML(info.xmlSnapshot, pParent, mpHost, info.positionInParent);
        return pRestored ? pRestored->getID() : -1;
    }
    default:
        return -1;
    }
}

QList<QPair<QString, int>> EditorDeleteItemCommand::restoredChildren(int itemID) const
{
    QList<QPair<QString, int>> children;
    switch (mViewType) {
    case EditorViewType::cmTriggerView:
        if (TTrigger* pItem = mpHost->getTriggerUnit()->getTrigger(itemID)) {
            for (auto* pChild : *pItem->getChildrenList()) {
                children.append(qMakePair(static_cast<TTrigger*>(pChild)->getName(), pChild->getID()));
            }
        }
        break;
    case EditorViewType::cmAliasView:
        if (TAlias* pItem = mpHost->getAliasUnit()->getAlias(itemID)) {
            for (auto* pChild : *pItem->getChildrenList()) {
                children.append(qMakePair(static_cast<TAlias*>(pChild)->getName(), pChild->getID()));
            }
        }
        break;
    case EditorViewType::cmTimerView:
        if (TTimer* pItem = mpHost->getTimerUnit()->getTimer(itemID)) {
            for (auto* pChild : *pItem->getChildrenList()) {
                children.append(qMakePair(static_cast<TTimer*>(pChild)->getName(), pChild->getID()));
            }
        }
        break;
    case EditorViewType::cmScriptView:
        if (TScript* pItem = mpHost->getScriptUnit()->getScript(itemID)) {
            for (auto* pChild : *pItem->getChildrenList()) {
                children.append(qMakePair(static_cast<TScript*>(pChild)->getName(), pChild->getID()));
            }
        }
        break;
    case EditorViewType::cmKeysView:
        if (TKey* pItem = mpHost->getKeyUnit()->getKey(itemID)) {
            for (auto* pChild : *pItem->getChildrenList()) {
                children.append(qMakePair(static_cast<TKey*>(pChild)->getName(), pChild->getID()));
            }
        }
        break;
    case EditorViewType::cmActionView:
        if (TAction* pItem = mpHost->getActionUnit()->getAction(itemID)) {
            for (auto* pChild : *pItem->getChildrenList()) {
                children.append(qMakePair(static_cast<TAction*>(pChild)->getName(), pChild->getID()));
            }
        }
        break;
    default:
        break;
    }
    return children;
}

// A snapshot holds every child in order, so each restored child sits at its entry's recorded position
void EditorDeleteItemCommand::adoptRestoredChildren(int oldParentID, int newParentID, QHash<int, QList<int>>& childEntries)
{
    // Taken, as some views record an item more than once and its children need adopting only once
    const QList<int> entries = childEntries.take(oldParentID);
    if (entries.isEmpty()) {
        return;
    }

    const QList<QPair<QString, int>> children = restoredChildren(newParentID);
    for (const int entry : entries) {
        auto& info = mDeletedItems[entry];
        info.parentID = newParentID;
        const int position = info.positionInParent;
        if (position < 0 || position >= children.size() || children.at(position).first != info.itemName) {
            continue;
        }
        const int oldChildID = info.itemID;
        info.itemID = children.at(position).second;
        adoptRestoredChildren(oldChildID, info.itemID, childEntries);
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
    for (const auto& info : std::as_const(mDeletedItems)) {
        switch (mViewType) {
        case EditorViewType::cmTriggerView: {
            TTrigger* trigger = mpHost->getTriggerUnit()->getTrigger(info.itemID);
            // Validate: item exists and has expected name (prevents deleting wrong item if ID reused)
            if (!trigger) {
#if defined(DEBUG_UNDO_REDO)
                qWarning() << "EditorDeleteItemCommand::redo() - Trigger" << info.itemName << "ID:" << info.itemID << "no longer exists, skipping";
#endif
                continue;
            }
            if (trigger->getName() != info.itemName) {
#if defined(DEBUG_UNDO_REDO)
                qWarning() << "EditorDeleteItemCommand::redo() - Trigger ID" << info.itemID << "expected name" << info.itemName << "but found" << trigger->getName() << ", skipping";
#endif
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
#if defined(DEBUG_UNDO_REDO)
                qWarning() << "EditorDeleteItemCommand::redo() - Alias" << info.itemName << "ID:" << info.itemID << "no longer exists, skipping";
#endif
                continue;
            }
            if (alias->getName() != info.itemName) {
#if defined(DEBUG_UNDO_REDO)
                qWarning() << "EditorDeleteItemCommand::redo() - Alias ID" << info.itemID << "expected name" << info.itemName << "but found" << alias->getName() << ", skipping";
#endif
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
#if defined(DEBUG_UNDO_REDO)
                qWarning() << "EditorDeleteItemCommand::redo() - Timer" << info.itemName << "ID:" << info.itemID << "no longer exists, skipping";
#endif
                continue;
            }
            if (timer->getName() != info.itemName) {
#if defined(DEBUG_UNDO_REDO)
                qWarning() << "EditorDeleteItemCommand::redo() - Timer ID" << info.itemID << "expected name" << info.itemName << "but found" << timer->getName() << ", skipping";
#endif
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
#if defined(DEBUG_UNDO_REDO)
                qWarning() << "EditorDeleteItemCommand::redo() - Script" << info.itemName << "ID:" << info.itemID << "no longer exists, skipping";
#endif
                continue;
            }
            if (script->getName() != info.itemName) {
#if defined(DEBUG_UNDO_REDO)
                qWarning() << "EditorDeleteItemCommand::redo() - Script ID" << info.itemID << "expected name" << info.itemName << "but found" << script->getName() << ", skipping";
#endif
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
#if defined(DEBUG_UNDO_REDO)
                qWarning() << "EditorDeleteItemCommand::redo() - Key" << info.itemName << "ID:" << info.itemID << "no longer exists, skipping";
#endif
                continue;
            }
            if (key->getName() != info.itemName) {
#if defined(DEBUG_UNDO_REDO)
                qWarning() << "EditorDeleteItemCommand::redo() - Key ID" << info.itemID << "expected name" << info.itemName << "but found" << key->getName() << ", skipping";
#endif
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
#if defined(DEBUG_UNDO_REDO)
                qWarning() << "EditorDeleteItemCommand::redo() - Action" << info.itemName << "ID:" << info.itemID << "no longer exists, skipping";
#endif
                continue;
            }
            if (action->getName() != info.itemName) {
#if defined(DEBUG_UNDO_REDO)
                qWarning() << "EditorDeleteItemCommand::redo() - Action ID" << info.itemID << "expected name" << info.itemName << "but found" << action->getName() << ", skipping";
#endif
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
    QSet<int> deletedIDs;
    deletedIDs.reserve(mDeletedItems.size());
    for (const auto& info : std::as_const(mDeletedItems)) {
        deletedIDs.insert(info.itemID);
    }
    for (const auto& info : std::as_const(mDeletedItems)) {
        // Skip items whose parent is also in the deletion list
        // (they will be automatically deleted when the parent is deleted)
        if (info.parentID != -1 && deletedIDs.contains(info.parentID)) {
            continue;
        }

        switch (mViewType) {
        case EditorViewType::cmTriggerView: {
            TTrigger* trigger = mpHost->getTriggerUnit()->getTrigger(info.itemID);
            if (!trigger) {
                break;
            }
            if (trigger->getName() != info.itemName) {
#if defined(DEBUG_UNDO_REDO)
                qWarning() << "EditorDeleteItemCommand::redo() - Trigger ID" << info.itemID << "name changed from" << info.itemName << "to" << trigger->getName() << "- not deleting";
#endif
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
#if defined(DEBUG_UNDO_REDO)
                qWarning() << "EditorDeleteItemCommand::redo() - Alias ID" << info.itemID << "name changed from" << info.itemName << "to" << alias->getName() << "- not deleting";
#endif
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
#if defined(DEBUG_UNDO_REDO)
                qWarning() << "EditorDeleteItemCommand::redo() - Timer ID" << info.itemID << "name changed from" << info.itemName << "to" << timer->getName() << "- not deleting";
#endif
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
#if defined(DEBUG_UNDO_REDO)
                qWarning() << "EditorDeleteItemCommand::redo() - Script ID" << info.itemID << "name changed from" << info.itemName << "to" << script->getName() << "- not deleting";
#endif
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
#if defined(DEBUG_UNDO_REDO)
                qWarning() << "EditorDeleteItemCommand::redo() - Key ID" << info.itemID << "name changed from" << info.itemName << "to" << key->getName() << "- not deleting";
#endif
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
#if defined(DEBUG_UNDO_REDO)
                qWarning() << "EditorDeleteItemCommand::redo() - Action ID" << info.itemID << "name changed from" << info.itemName << "to" << action->getName() << "- not deleting";
#endif
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
