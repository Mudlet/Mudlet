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

#include "MudletAddItemCommand.h"

#include "EditorItemXMLHelpers.h"
#include "Host.h"
#include "TAction.h"
#include "TAlias.h"
#include "TKey.h"
#include "TScript.h"
#include "TTimer.h"
#include "TTrigger.h"

MudletAddItemCommand::MudletAddItemCommand(EditorViewType viewType, int itemID, int parentID,
                                           int positionInParent, bool isFolder, const QString& itemName, Host* host)
    : MudletEditorCommand(generateText(viewType, itemName, isFolder), host)
    , mViewType(viewType)
    , mItemID(itemID)
    , mParentID(parentID)
    , mPositionInParent(positionInParent)
    , mIsFolder(isFolder)
    , mItemName(itemName)
{
}

void MudletAddItemCommand::undo()
{
    // Export the item to XML before deleting (for redo)
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

    // Delete the item (unregister first to match MudletDeleteItemCommand pattern)
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

void MudletAddItemCommand::redo()
{
    // Skip the first redo() which is automatically called by QUndoStack::push()
    // The item has already been added by the user action
    if (mSkipFirstRedo) {
        mSkipFirstRedo = false;
        return;
    }

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
            } else {
                qWarning() << "MudletAddItemCommand::redo() - Failed to recreate trigger from snapshot";
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
            } else {
                qWarning() << "MudletAddItemCommand::redo() - Failed to recreate alias from snapshot";
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
            } else {
                qWarning() << "MudletAddItemCommand::redo() - Failed to recreate timer from snapshot";
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
            } else {
                qWarning() << "MudletAddItemCommand::redo() - Failed to recreate script from snapshot";
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
            } else {
                qWarning() << "MudletAddItemCommand::redo() - Failed to recreate key from snapshot";
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
            } else {
                qWarning() << "MudletAddItemCommand::redo() - Failed to recreate action from snapshot";
            }
            break;
        }
        default:
            break;
        }
    }
}

// Updates stored IDs when items are deleted and recreated (e.g., during undo/redo)
void MudletAddItemCommand::remapItemID(int oldID, int newID)
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

QString MudletAddItemCommand::generateText(EditorViewType viewType, const QString& itemName, bool isFolder)
{
    switch (viewType) {
    case EditorViewType::cmTriggerView:
        if (isFolder) {
            //: Undo/redo menu text for adding a trigger folder
            return QObject::tr("Add trigger group \"%1\"").arg(itemName);
        } else {
            //: Undo/redo menu text for adding a trigger
            return QObject::tr("Add trigger \"%1\"").arg(itemName);
        }
    case EditorViewType::cmAliasView:
        if (isFolder) {
            //: Undo/redo menu text for adding an alias folder
            return QObject::tr("Add alias group \"%1\"").arg(itemName);
        } else {
            //: Undo/redo menu text for adding an alias
            return QObject::tr("Add alias \"%1\"").arg(itemName);
        }
    case EditorViewType::cmTimerView:
        if (isFolder) {
            //: Undo/redo menu text for adding a timer folder
            return QObject::tr("Add timer group \"%1\"").arg(itemName);
        } else {
            //: Undo/redo menu text for adding a timer
            return QObject::tr("Add timer \"%1\"").arg(itemName);
        }
    case EditorViewType::cmScriptView:
        if (isFolder) {
            //: Undo/redo menu text for adding a script folder
            return QObject::tr("Add script group \"%1\"").arg(itemName);
        } else {
            //: Undo/redo menu text for adding a script
            return QObject::tr("Add script \"%1\"").arg(itemName);
        }
    case EditorViewType::cmKeysView:
        if (isFolder) {
            //: Undo/redo menu text for adding a key folder
            return QObject::tr("Add key group \"%1\"").arg(itemName);
        } else {
            //: Undo/redo menu text for adding a key binding
            return QObject::tr("Add key \"%1\"").arg(itemName);
        }
    case EditorViewType::cmActionView:
        if (isFolder) {
            //: Undo/redo menu text for adding a button toolbar
            return QObject::tr("Add button group \"%1\"").arg(itemName);
        } else {
            //: Undo/redo menu text for adding a button
            return QObject::tr("Add button \"%1\"").arg(itemName);
        }
    default:
        if (isFolder) {
            //: Undo/redo menu text for adding an unknown folder type
            return QObject::tr("Add group \"%1\"").arg(itemName);
        } else {
            //: Undo/redo menu text for adding an unknown item type
            return QObject::tr("Add item \"%1\"").arg(itemName);
        }
    }
}
