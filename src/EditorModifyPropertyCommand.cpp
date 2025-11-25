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

#include "EditorModifyPropertyCommand.h"

#include "EditorItemXMLHelpers.h"
#include "Host.h"
#include "TAction.h"
#include "TAlias.h"
#include "TKey.h"
#include "TScript.h"
#include "TTimer.h"
#include "TTrigger.h"

EditorModifyPropertyCommand::EditorModifyPropertyCommand(EditorViewType viewType, int itemID, const QString& itemName, const QString& oldStateXML, const QString& newStateXML, Host* host)
: EditorCommand(generateText(viewType, itemName), host), mViewType(viewType), mItemID(itemID), mItemName(itemName), mOldStateXML(oldStateXML), mNewStateXML(newStateXML)
{
    mCreationTime.start();
}

void EditorModifyPropertyCommand::undo()
{
#if defined(DEBUG_UNDO_REDO)
    qDebug() << "EditorModifyPropertyCommand::undo() - Restoring" << mItemName << "(ID:" << mItemID << ") to old state";
#endif
    // Restore item to old state from XML
    switch (mViewType) {
    case EditorViewType::cmTriggerView: {
        TTrigger* pTrigger = mpHost->getTriggerUnit()->getTrigger(mItemID);
        if (pTrigger) {
            if (!updateTriggerFromXML(pTrigger, mOldStateXML)) {
                qWarning() << "EditorModifyPropertyCommand::undo() - Failed to restore trigger" << mItemName << "to old state";
            }
        } else {
            qWarning() << "EditorModifyPropertyCommand::undo() - Trigger" << mItemName << "not found";
        }
        break;
    }
    case EditorViewType::cmAliasView: {
        TAlias* pAlias = mpHost->getAliasUnit()->getAlias(mItemID);
        if (pAlias) {
            if (!updateAliasFromXML(pAlias, mOldStateXML)) {
                qWarning() << "EditorModifyPropertyCommand::undo() - Failed to restore alias" << mItemName << "to old state";
            }
        } else {
            qWarning() << "EditorModifyPropertyCommand::undo() - Alias" << mItemName << "not found";
        }
        break;
    }
    case EditorViewType::cmTimerView: {
        TTimer* pTimer = mpHost->getTimerUnit()->getTimer(mItemID);
        if (pTimer) {
            if (!updateTimerFromXML(pTimer, mOldStateXML)) {
                qWarning() << "EditorModifyPropertyCommand::undo() - Failed to restore timer" << mItemName << "to old state";
            }
        } else {
            qWarning() << "EditorModifyPropertyCommand::undo() - Timer" << mItemName << "not found";
        }
        break;
    }
    case EditorViewType::cmScriptView: {
        TScript* pScript = mpHost->getScriptUnit()->getScript(mItemID);
        if (pScript) {
            if (!updateScriptFromXML(pScript, mOldStateXML)) {
                qWarning() << "EditorModifyPropertyCommand::undo() - Failed to restore script" << mItemName << "to old state";
            }
        } else {
            qWarning() << "EditorModifyPropertyCommand::undo() - Script" << mItemName << "not found";
        }
        break;
    }
    case EditorViewType::cmKeysView: {
        TKey* pKey = mpHost->getKeyUnit()->getKey(mItemID);
        if (pKey) {
            if (!updateKeyFromXML(pKey, mOldStateXML)) {
                qWarning() << "EditorModifyPropertyCommand::undo() - Failed to restore key" << mItemName << "to old state";
            }
        } else {
            qWarning() << "EditorModifyPropertyCommand::undo() - Key" << mItemName << "not found";
        }
        break;
    }
    case EditorViewType::cmActionView: {
        TAction* pAction = mpHost->getActionUnit()->getAction(mItemID);
        if (pAction) {
            if (!updateActionFromXML(pAction, mOldStateXML)) {
                qWarning() << "EditorModifyPropertyCommand::undo() - Failed to restore action" << mItemName << "to old state";
            }
        } else {
            qWarning() << "EditorModifyPropertyCommand::undo() - Action" << mItemName << "not found";
        }
        break;
    }
    default:
        break;
    }
}

void EditorModifyPropertyCommand::redo()
{
    // Skip the first redo() which is automatically called by QUndoStack::push()
    // The property change has already been applied before creating this command
    if (mSkipFirstRedo) {
#if defined(DEBUG_UNDO_REDO)
        qDebug() << "EditorModifyPropertyCommand::redo() - Skipping first redo (change already applied)";
#endif
        mSkipFirstRedo = false;
        return;
    }

#if defined(DEBUG_UNDO_REDO)
    qDebug() << "EditorModifyPropertyCommand::redo() - Applying new state to" << mItemName << "(ID:" << mItemID << ")";
#endif
    // Apply the new state
    switch (mViewType) {
    case EditorViewType::cmTriggerView: {
        TTrigger* pTrigger = mpHost->getTriggerUnit()->getTrigger(mItemID);
        if (pTrigger) {
            if (!updateTriggerFromXML(pTrigger, mNewStateXML)) {
                qWarning() << "EditorModifyPropertyCommand::redo() - Failed to apply new state to trigger" << mItemName;
            }
        } else {
            qWarning() << "EditorModifyPropertyCommand::redo() - Trigger" << mItemName << "not found";
        }
        break;
    }
    case EditorViewType::cmAliasView: {
        TAlias* pAlias = mpHost->getAliasUnit()->getAlias(mItemID);
        if (pAlias) {
            if (!updateAliasFromXML(pAlias, mNewStateXML)) {
                qWarning() << "EditorModifyPropertyCommand::redo() - Failed to apply new state to alias" << mItemName;
            }
        } else {
            qWarning() << "EditorModifyPropertyCommand::redo() - Alias" << mItemName << "not found";
        }
        break;
    }
    case EditorViewType::cmTimerView: {
        TTimer* pTimer = mpHost->getTimerUnit()->getTimer(mItemID);
        if (pTimer) {
            if (!updateTimerFromXML(pTimer, mNewStateXML)) {
                qWarning() << "EditorModifyPropertyCommand::redo() - Failed to apply new state to timer" << mItemName;
            }
        } else {
            qWarning() << "EditorModifyPropertyCommand::redo() - Timer" << mItemName << "not found";
        }
        break;
    }
    case EditorViewType::cmScriptView: {
        TScript* pScript = mpHost->getScriptUnit()->getScript(mItemID);
        if (pScript) {
            if (!updateScriptFromXML(pScript, mNewStateXML)) {
                qWarning() << "EditorModifyPropertyCommand::redo() - Failed to apply new state to script" << mItemName;
            }
        } else {
            qWarning() << "EditorModifyPropertyCommand::redo() - Script" << mItemName << "not found";
        }
        break;
    }
    case EditorViewType::cmKeysView: {
        TKey* pKey = mpHost->getKeyUnit()->getKey(mItemID);
        if (pKey) {
            if (!updateKeyFromXML(pKey, mNewStateXML)) {
                qWarning() << "EditorModifyPropertyCommand::redo() - Failed to apply new state to key" << mItemName;
            }
        } else {
            qWarning() << "EditorModifyPropertyCommand::redo() - Key" << mItemName << "not found";
        }
        break;
    }
    case EditorViewType::cmActionView: {
        TAction* pAction = mpHost->getActionUnit()->getAction(mItemID);
        if (pAction) {
            if (!updateActionFromXML(pAction, mNewStateXML)) {
                qWarning() << "EditorModifyPropertyCommand::redo() - Failed to apply new state to action" << mItemName;
            }
        } else {
            qWarning() << "EditorModifyPropertyCommand::redo() - Action" << mItemName << "not found";
        }
        break;
    }
    default:
        break;
    }
}

// Updates stored IDs when items are deleted and recreated (e.g., during undo/redo)
void EditorModifyPropertyCommand::remapItemID(int oldID, int newID)
{
    if (mItemID == oldID) {
        mItemID = newID;
    }
}

QString EditorModifyPropertyCommand::generateText(EditorViewType viewType, const QString& itemName)
{
    switch (viewType) {
    case EditorViewType::cmTriggerView:
        //: Undo/redo menu text for modifying a trigger's properties
        return QObject::tr("modify trigger \"%1\"").arg(itemName);
    case EditorViewType::cmAliasView:
        //: Undo/redo menu text for modifying an alias's properties
        return QObject::tr("modify alias \"%1\"").arg(itemName);
    case EditorViewType::cmTimerView:
        //: Undo/redo menu text for modifying a timer's properties
        return QObject::tr("modify timer \"%1\"").arg(itemName);
    case EditorViewType::cmScriptView:
        //: Undo/redo menu text for modifying a script's properties
        return QObject::tr("modify script \"%1\"").arg(itemName);
    case EditorViewType::cmKeysView:
        //: Undo/redo menu text for modifying a key binding's properties
        return QObject::tr("modify key \"%1\"").arg(itemName);
    case EditorViewType::cmActionView:
        //: Undo/redo menu text for modifying a button's properties
        return QObject::tr("modify button \"%1\"").arg(itemName);
    default:
        //: Undo/redo menu text for modifying an unknown item's properties
        return QObject::tr("modify item \"%1\"").arg(itemName);
    }
}

int EditorModifyPropertyCommand::id() const
{
    return CommandId;
}

bool EditorModifyPropertyCommand::mergeWith(const QUndoCommand* other)
{
    // Only merge if this command has a property ID set (for per-property immediate saves)
    if (mPropertyId.isEmpty()) {
        return false;
    }

    const auto* otherCmd = dynamic_cast<const EditorModifyPropertyCommand*>(other);
    if (!otherCmd) {
        return false;
    }

    // Only merge if same property on same item
    if (mPropertyId != otherCmd->mPropertyId) {
        return false;
    }

    // Only merge if within time window (check if the OTHER command was created within the window)
    // Note: mCreationTime tracks when THIS command was created
    if (mCreationTime.elapsed() > MERGE_TIMEOUT_MS) {
        return false;
    }

#if defined(DEBUG_UNDO_REDO)
    qDebug() << "EditorModifyPropertyCommand::mergeWith() - Merging" << mPropertyId << "commands within" << mCreationTime.elapsed() << "ms";
#endif

    // Merge: keep our old state, take their new state
    mNewStateXML = otherCmd->mNewStateXML;
    // Reset the timer so we can continue merging rapid edits
    mCreationTime.restart();
    return true;
}

void EditorModifyPropertyCommand::setPropertyId(const QString& propertyId)
{
    mPropertyId = propertyId;
}
