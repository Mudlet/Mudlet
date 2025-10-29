/***************************************************************************
 *   Copyright (C) 2025 by Vadim Peretokin - vadim.peretokin@mudlet.org   *
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
    : MudletCommand(generateText(viewType, itemName, newState), host)
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
    QString typeName;
    switch (viewType) {
    case EditorViewType::cmTriggerView:
        //: Undo/redo menu text for activating/deactivating a trigger
        typeName = QObject::tr("trigger");
        break;
    case EditorViewType::cmAliasView:
        //: Undo/redo menu text for activating/deactivating an alias
        typeName = QObject::tr("alias");
        break;
    case EditorViewType::cmTimerView:
        //: Undo/redo menu text for activating/deactivating a timer
        typeName = QObject::tr("timer");
        break;
    case EditorViewType::cmScriptView:
        //: Undo/redo menu text for activating/deactivating a script
        typeName = QObject::tr("script");
        break;
    case EditorViewType::cmKeysView:
        //: Undo/redo menu text for activating/deactivating a key binding
        typeName = QObject::tr("key");
        break;
    case EditorViewType::cmActionView:
        //: Undo/redo menu text for activating/deactivating a button
        typeName = QObject::tr("button");
        break;
    default:
        //: Undo/redo menu text for activating/deactivating an unknown item
        typeName = QObject::tr("item");
        break;
    }

    if (newState) {
        //: Undo/redo menu text for activating an item. %1 = item type (e.g. "trigger"), %2 = item name
        return QObject::tr("Activate %1 \"%2\"").arg(typeName, itemName);
    } else {
        //: Undo/redo menu text for deactivating an item. %1 = item type (e.g. "trigger"), %2 = item name
        return QObject::tr("Deactivate %1 \"%2\"").arg(typeName, itemName);
    }
}
