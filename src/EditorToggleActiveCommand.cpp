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

#include "EditorToggleActiveCommand.h"

#include "Host.h"
#include "TAction.h"
#include "TAlias.h"
#include "TKey.h"
#include "TScript.h"
#include "TTimer.h"
#include "TTrigger.h"

EditorToggleActiveCommand::EditorToggleActiveCommand(EditorViewType viewType, int itemID, bool oldState, bool newState, const QString& itemName, Host* host)
: EditorCommand(generateText(viewType, itemName, newState), host), mViewType(viewType), mItemID(itemID), mOldActiveState(oldState), mNewActiveState(newState), mItemName(itemName)
{
}

void EditorToggleActiveCommand::undo()
{
#if defined(DEBUG_UNDO_REDO)
    qDebug() << "EditorToggleActiveCommand::undo() - Setting" << mItemName << "(ID:" << mItemID << ")" << "active state to" << mOldActiveState;
#endif
    setItemActiveState(mItemID, mOldActiveState);
}

void EditorToggleActiveCommand::redo()
{
#if defined(DEBUG_UNDO_REDO)
    qDebug() << "EditorToggleActiveCommand::redo() - Setting" << mItemName << "(ID:" << mItemID << ")" << "active state to" << mNewActiveState;
#endif
    // Skip the first redo() which is automatically called by QUndoStack::push()
    // The state change has already been performed before pushing to the stack
    if (mSkipFirstRedo) {
#if defined(DEBUG_UNDO_REDO)
        qDebug() << "EditorToggleActiveCommand::redo() - Skipping first redo (state already changed)";
#endif
        mSkipFirstRedo = false;
        return;
    }

    setItemActiveState(mItemID, mNewActiveState);
}

// Updates stored IDs when items are deleted and recreated (e.g., during undo/redo)
void EditorToggleActiveCommand::remapItemID(int oldID, int newID)
{
    if (mItemID == oldID) {
        mItemID = newID;
    }
}

void EditorToggleActiveCommand::setItemActiveState(int itemID, bool active)
{
    switch (mViewType) {
    case EditorViewType::cmTriggerView: {
        TTrigger* pT = mpHost->getTriggerUnit()->getTrigger(itemID);
        if (pT) {
            pT->setIsActive(active);
        } else {
            qWarning() << "EditorToggleActiveCommand::setItemActiveState() - Trigger" << itemID << "not found";
        }
        break;
    }
    case EditorViewType::cmAliasView: {
        TAlias* pA = mpHost->getAliasUnit()->getAlias(itemID);
        if (pA) {
            pA->setIsActive(active);
        } else {
            qWarning() << "EditorToggleActiveCommand::setItemActiveState() - Alias" << itemID << "not found";
        }
        break;
    }
    case EditorViewType::cmTimerView: {
        TTimer* pT = mpHost->getTimerUnit()->getTimer(itemID);
        if (pT) {
            pT->setIsActive(active);
        } else {
            qWarning() << "EditorToggleActiveCommand::setItemActiveState() - Timer" << itemID << "not found";
        }
        break;
    }
    case EditorViewType::cmScriptView: {
        TScript* pS = mpHost->getScriptUnit()->getScript(itemID);
        if (pS) {
            pS->setIsActive(active);
        } else {
            qWarning() << "EditorToggleActiveCommand::setItemActiveState() - Script" << itemID << "not found";
        }
        break;
    }
    case EditorViewType::cmKeysView: {
        TKey* pK = mpHost->getKeyUnit()->getKey(itemID);
        if (pK) {
            pK->setIsActive(active);
        } else {
            qWarning() << "EditorToggleActiveCommand::setItemActiveState() - Key" << itemID << "not found";
        }
        break;
    }
    case EditorViewType::cmActionView: {
        TAction* pA = mpHost->getActionUnit()->getAction(itemID);
        if (pA) {
            pA->setIsActive(active);
        } else {
            qWarning() << "EditorToggleActiveCommand::setItemActiveState() - Action" << itemID << "not found";
        }
        break;
    }
    default:
        qWarning() << "EditorToggleActiveCommand::setItemActiveState() - Unknown view type";
        break;
    }
}

QString EditorToggleActiveCommand::generateText(EditorViewType viewType, const QString& itemName, bool newState)
{
    if (newState) {
        // Activating an item
        switch (viewType) {
        case EditorViewType::cmTriggerView:
            //: Undo/redo menu text for activating a trigger
            return QObject::tr("activate trigger \"%1\"").arg(itemName);
        case EditorViewType::cmAliasView:
            //: Undo/redo menu text for activating an alias
            return QObject::tr("activate alias \"%1\"").arg(itemName);
        case EditorViewType::cmTimerView:
            //: Undo/redo menu text for activating a timer
            return QObject::tr("activate timer \"%1\"").arg(itemName);
        case EditorViewType::cmScriptView:
            //: Undo/redo menu text for activating a script
            return QObject::tr("activate script \"%1\"").arg(itemName);
        case EditorViewType::cmKeysView:
            //: Undo/redo menu text for activating a key binding
            return QObject::tr("activate key \"%1\"").arg(itemName);
        case EditorViewType::cmActionView:
            //: Undo/redo menu text for activating a button
            return QObject::tr("activate button \"%1\"").arg(itemName);
        default:
            //: Undo/redo menu text for activating an unknown item
            return QObject::tr("activate item \"%1\"").arg(itemName);
        }
    } else {
        // Deactivating an item
        switch (viewType) {
        case EditorViewType::cmTriggerView:
            //: Undo/redo menu text for deactivating a trigger
            return QObject::tr("deactivate trigger \"%1\"").arg(itemName);
        case EditorViewType::cmAliasView:
            //: Undo/redo menu text for deactivating an alias
            return QObject::tr("deactivate alias \"%1\"").arg(itemName);
        case EditorViewType::cmTimerView:
            //: Undo/redo menu text for deactivating a timer
            return QObject::tr("deactivate timer \"%1\"").arg(itemName);
        case EditorViewType::cmScriptView:
            //: Undo/redo menu text for deactivating a script
            return QObject::tr("deactivate script \"%1\"").arg(itemName);
        case EditorViewType::cmKeysView:
            //: Undo/redo menu text for deactivating a key binding
            return QObject::tr("deactivate key \"%1\"").arg(itemName);
        case EditorViewType::cmActionView:
            //: Undo/redo menu text for deactivating a button
            return QObject::tr("deactivate button \"%1\"").arg(itemName);
        default:
            //: Undo/redo menu text for deactivating an unknown item
            return QObject::tr("deactivate item \"%1\"").arg(itemName);
        }
    }
}
