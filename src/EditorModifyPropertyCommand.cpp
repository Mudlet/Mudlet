/***************************************************************************
 *   Copyright (C) 2025 by Excellencedev - ademiluyisuccessandexcellence@gmail.com *
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

EditorModifyPropertyCommand::EditorModifyPropertyCommand(EditorViewType viewType, int itemID, const QString& itemName, const QString& oldXml, const QString& newXml, Host* host)
: EditorCommand(generateText(viewType, itemName), host), mViewType(viewType), mItemID(itemID), mItemName(itemName), mOldXmlSnapshot(oldXml), mNewXmlSnapshot(newXml)
{
}

void EditorModifyPropertyCommand::undo()
{
#if defined(DEBUG_UNDO_REDO)
    qDebug() << "EditorModifyPropertyCommand::undo() - Reverting properties for" << mItemName << "ID:" << mItemID;
#endif
    bool success = false;
    switch (mViewType) {
    case EditorViewType::cmTriggerView: {
        TTrigger* pT = mpHost->getTriggerUnit()->getTrigger(mItemID);
        if (pT) {
            success = updateTriggerFromXML(pT, mOldXmlSnapshot);
        }
        break;
    }
    case EditorViewType::cmAliasView: {
        TAlias* pA = mpHost->getAliasUnit()->getAlias(mItemID);
        if (pA) {
            success = updateAliasFromXML(pA, mOldXmlSnapshot);
        }
        break;
    }
    case EditorViewType::cmTimerView: {
        TTimer* pT = mpHost->getTimerUnit()->getTimer(mItemID);
        if (pT) {
            success = updateTimerFromXML(pT, mOldXmlSnapshot);
        }
        break;
    }
    case EditorViewType::cmScriptView: {
        TScript* pS = mpHost->getScriptUnit()->getScript(mItemID);
        if (pS) {
            success = updateScriptFromXML(pS, mOldXmlSnapshot);
        }
        break;
    }
    case EditorViewType::cmKeysView: {
        TKey* pK = mpHost->getKeyUnit()->getKey(mItemID);
        if (pK) {
            success = updateKeyFromXML(pK, mOldXmlSnapshot);
        }
        break;
    }
    case EditorViewType::cmActionView: {
        TAction* pA = mpHost->getActionUnit()->getAction(mItemID);
        if (pA) {
            success = updateActionFromXML(pA, mOldXmlSnapshot);
        }
        break;
    }
    default:
        break;
    }

    if (!success) {
        qWarning() << "EditorModifyPropertyCommand::undo() - Failed to revert properties for" << mItemName;
    }
}

void EditorModifyPropertyCommand::redo()
{
#if defined(DEBUG_UNDO_REDO)
    qDebug() << "EditorModifyPropertyCommand::redo() - Applying properties for" << mItemName << "ID:" << mItemID;
#endif
    bool success = false;
    switch (mViewType) {
    case EditorViewType::cmTriggerView: {
        TTrigger* pT = mpHost->getTriggerUnit()->getTrigger(mItemID);
        if (pT) {
            success = updateTriggerFromXML(pT, mNewXmlSnapshot);
        }
        break;
    }
    case EditorViewType::cmAliasView: {
        TAlias* pA = mpHost->getAliasUnit()->getAlias(mItemID);
        if (pA) {
            success = updateAliasFromXML(pA, mNewXmlSnapshot);
        }
        break;
    }
    case EditorViewType::cmTimerView: {
        TTimer* pT = mpHost->getTimerUnit()->getTimer(mItemID);
        if (pT) {
            success = updateTimerFromXML(pT, mNewXmlSnapshot);
        }
        break;
    }
    case EditorViewType::cmScriptView: {
        TScript* pS = mpHost->getScriptUnit()->getScript(mItemID);
        if (pS) {
            success = updateScriptFromXML(pS, mNewXmlSnapshot);
        }
        break;
    }
    case EditorViewType::cmKeysView: {
        TKey* pK = mpHost->getKeyUnit()->getKey(mItemID);
        if (pK) {
            success = updateKeyFromXML(pK, mNewXmlSnapshot);
        }
        break;
    }
    case EditorViewType::cmActionView: {
        TAction* pA = mpHost->getActionUnit()->getAction(mItemID);
        if (pA) {
            success = updateActionFromXML(pA, mNewXmlSnapshot);
        }
        break;
    }
    default:
        break;
    }

    if (!success) {
        qWarning() << "EditorModifyPropertyCommand::redo() - Failed to apply properties for" << mItemName;
    }
}

int EditorModifyPropertyCommand::id() const
{
    // Unique ID for this command type to allow merging
    return 1001;
}

bool EditorModifyPropertyCommand::mergeWith(const QUndoCommand* other)
{
    if (other->id() != id()) {
        return false;
    }

    const EditorModifyPropertyCommand* otherCmd = static_cast<const EditorModifyPropertyCommand*>(other);

    // Only merge if it's the same item and same view type
    if (mItemID != otherCmd->mItemID || mViewType != otherCmd->mViewType) {
        return false;
    }

    // Update the new state to the other command's new state
    mNewXmlSnapshot = otherCmd->mNewXmlSnapshot;
    return true;
}

QList<int> EditorModifyPropertyCommand::affectedItemIDs() const
{
    return {mItemID};
}

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
        //: Undo/redo menu text for modifying a trigger
        return QObject::tr("modify trigger \"%1\"").arg(itemName);
    case EditorViewType::cmAliasView:
        //: Undo/redo menu text for modifying an alias
        return QObject::tr("modify alias \"%1\"").arg(itemName);
    case EditorViewType::cmTimerView:
        //: Undo/redo menu text for modifying a timer
        return QObject::tr("modify timer \"%1\"").arg(itemName);
    case EditorViewType::cmScriptView:
        //: Undo/redo menu text for modifying a script
        return QObject::tr("modify script \"%1\"").arg(itemName);
    case EditorViewType::cmKeysView:
        //: Undo/redo menu text for modifying a key binding
        return QObject::tr("modify key \"%1\"").arg(itemName);
    case EditorViewType::cmActionView:
        //: Undo/redo menu text for modifying a button
        return QObject::tr("modify button \"%1\"").arg(itemName);
    default:
        //: Undo/redo menu text for modifying an unknown item
        return QObject::tr("modify item \"%1\"").arg(itemName);
    }
}
