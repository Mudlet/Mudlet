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

#include "MudletToggleActiveCommand.h"

#include "Host.h"
#include "TAction.h"
#include "TAlias.h"
#include "TKey.h"
#include "TScript.h"
#include "TTimer.h"
#include "TTrigger.h"

MudletToggleActiveCommand::MudletToggleActiveCommand(EditorViewType viewType, int itemID,
                                                     bool oldState, bool newState,
                                                     const QString& itemName, Host* host)
    : MudletEditorCommand(generateText(viewType, itemName, newState), host)
    , mViewType(viewType)
    , mItemID(itemID)
    , mOldActiveState(oldState)
    , mNewActiveState(newState)
    , mItemName(itemName)
{
}

void MudletToggleActiveCommand::undo()
{
    setItemActiveState(mItemID, mOldActiveState);
}

void MudletToggleActiveCommand::redo()
{
    // Skip the first redo() which is automatically called by QUndoStack::push()
    // The state change has already been performed before pushing to the stack
    if (mSkipFirstRedo) {
        mSkipFirstRedo = false;
        return;
    }

    setItemActiveState(mItemID, mNewActiveState);
}

// Updates stored IDs when items are deleted and recreated (e.g., during undo/redo)
void MudletToggleActiveCommand::remapItemID(int oldID, int newID)
{
    if (mItemID == oldID) {
        mItemID = newID;
    }
}

void MudletToggleActiveCommand::setItemActiveState(int itemID, bool active)
{
    switch (mViewType) {
    case EditorViewType::cmTriggerView: {
        TTrigger* pT = mpHost->getTriggerUnit()->getTrigger(itemID);
        if (pT) {
            pT->setIsActive(active);
        } else {
            qWarning() << "MudletToggleActiveCommand::setItemActiveState() - Trigger" << itemID << "not found";
        }
        break;
    }
    case EditorViewType::cmAliasView: {
        TAlias* pA = mpHost->getAliasUnit()->getAlias(itemID);
        if (pA) {
            pA->setIsActive(active);
        } else {
            qWarning() << "MudletToggleActiveCommand::setItemActiveState() - Alias" << itemID << "not found";
        }
        break;
    }
    case EditorViewType::cmTimerView: {
        TTimer* pT = mpHost->getTimerUnit()->getTimer(itemID);
        if (pT) {
            pT->setIsActive(active);
        } else {
            qWarning() << "MudletToggleActiveCommand::setItemActiveState() - Timer" << itemID << "not found";
        }
        break;
    }
    case EditorViewType::cmScriptView: {
        TScript* pS = mpHost->getScriptUnit()->getScript(itemID);
        if (pS) {
            pS->setIsActive(active);
        } else {
            qWarning() << "MudletToggleActiveCommand::setItemActiveState() - Script" << itemID << "not found";
        }
        break;
    }
    case EditorViewType::cmKeysView: {
        TKey* pK = mpHost->getKeyUnit()->getKey(itemID);
        if (pK) {
            pK->setIsActive(active);
        } else {
            qWarning() << "MudletToggleActiveCommand::setItemActiveState() - Key" << itemID << "not found";
        }
        break;
    }
    case EditorViewType::cmActionView: {
        TAction* pA = mpHost->getActionUnit()->getAction(itemID);
        if (pA) {
            pA->setIsActive(active);
        } else {
            qWarning() << "MudletToggleActiveCommand::setItemActiveState() - Action" << itemID << "not found";
        }
        break;
    }
    default:
        qWarning() << "MudletToggleActiveCommand::setItemActiveState() - Unknown view type";
        break;
    }
}

QString MudletToggleActiveCommand::generateText(EditorViewType viewType,
                                                const QString& itemName,
                                                bool newState)
{
    if (newState) {
        // Activating an item
        switch (viewType) {
        case EditorViewType::cmTriggerView:
            //: Undo/redo menu text for activating a trigger
            return QObject::tr("Activate trigger \"%1\"").arg(itemName);
        case EditorViewType::cmAliasView:
            //: Undo/redo menu text for activating an alias
            return QObject::tr("Activate alias \"%1\"").arg(itemName);
        case EditorViewType::cmTimerView:
            //: Undo/redo menu text for activating a timer
            return QObject::tr("Activate timer \"%1\"").arg(itemName);
        case EditorViewType::cmScriptView:
            //: Undo/redo menu text for activating a script
            return QObject::tr("Activate script \"%1\"").arg(itemName);
        case EditorViewType::cmKeysView:
            //: Undo/redo menu text for activating a key binding
            return QObject::tr("Activate key \"%1\"").arg(itemName);
        case EditorViewType::cmActionView:
            //: Undo/redo menu text for activating a button
            return QObject::tr("Activate button \"%1\"").arg(itemName);
        default:
            //: Undo/redo menu text for activating an unknown item
            return QObject::tr("Activate item \"%1\"").arg(itemName);
        }
    } else {
        // Deactivating an item
        switch (viewType) {
        case EditorViewType::cmTriggerView:
            //: Undo/redo menu text for deactivating a trigger
            return QObject::tr("Deactivate trigger \"%1\"").arg(itemName);
        case EditorViewType::cmAliasView:
            //: Undo/redo menu text for deactivating an alias
            return QObject::tr("Deactivate alias \"%1\"").arg(itemName);
        case EditorViewType::cmTimerView:
            //: Undo/redo menu text for deactivating a timer
            return QObject::tr("Deactivate timer \"%1\"").arg(itemName);
        case EditorViewType::cmScriptView:
            //: Undo/redo menu text for deactivating a script
            return QObject::tr("Deactivate script \"%1\"").arg(itemName);
        case EditorViewType::cmKeysView:
            //: Undo/redo menu text for deactivating a key binding
            return QObject::tr("Deactivate key \"%1\"").arg(itemName);
        case EditorViewType::cmActionView:
            //: Undo/redo menu text for deactivating a button
            return QObject::tr("Deactivate button \"%1\"").arg(itemName);
        default:
            //: Undo/redo menu text for deactivating an unknown item
            return QObject::tr("Deactivate item \"%1\"").arg(itemName);
        }
    }
}
