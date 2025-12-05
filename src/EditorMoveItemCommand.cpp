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

#include "EditorMoveItemCommand.h"

#include "ActionUnit.h"
#include "AliasUnit.h"
#include "Host.h"
#include "KeyUnit.h"
#include "ScriptUnit.h"
#include "TimerUnit.h"
#include "TriggerUnit.h"

#include <QDebug>

EditorMoveItemCommand::EditorMoveItemCommand(EditorViewType viewType, int itemID, const QString& itemName, int oldParentID, int oldPosition, int newParentID, int newPosition, Host* host)
: EditorCommand(generateText(viewType, itemName), host), mViewType(viewType), mItemID(itemID), mItemName(itemName), mOldParentID(oldParentID), mOldPosition(oldPosition), mNewParentID(newParentID), mNewPosition(newPosition)
{
}

void EditorMoveItemCommand::undo()
{
#if defined(DEBUG_UNDO_REDO)
    qDebug() << "EditorMoveItemCommand::undo() - Moving" << mItemName << "ID:" << mItemID << "back to parent:" << mOldParentID << "pos:" << mOldPosition;
#endif
    // Move from new location back to old location
    // current parent is mNewParentID, target is mOldParentID
    moveItem(mItemID, mOldParentID, mOldPosition);
}

void EditorMoveItemCommand::redo()
{
#if defined(DEBUG_UNDO_REDO)
    qDebug() << "EditorMoveItemCommand::redo() - Moving" << mItemName << "ID:" << mItemID << "to parent:" << mNewParentID << "pos:" << mNewPosition;
#endif
    // Move from old location to new location
    // current parent is mOldParentID, target is mNewParentID
    moveItem(mItemID, mNewParentID, mNewPosition);
}

void EditorMoveItemCommand::moveItem(int itemID, int targetParentID, int targetPosition)
{
    // Determine the "current" parent ID based on whether we are undoing or redoing
    // If we are moving TO targetParentID, we must be coming FROM the other one.
    int currentParentID = (targetParentID == mNewParentID) ? mOldParentID : mNewParentID;

    switch (mViewType) {
    case EditorViewType::cmTriggerView:
        mpHost->getTriggerUnit()->reParentTrigger(itemID, currentParentID, targetParentID, TreeItemInsertMode::AtPosition, targetPosition);
        break;
    case EditorViewType::cmAliasView:
        mpHost->getAliasUnit()->reParentAlias(itemID, currentParentID, targetParentID, TreeItemInsertMode::AtPosition, targetPosition);
        break;
    case EditorViewType::cmTimerView:
        mpHost->getTimerUnit()->reParentTimer(itemID, currentParentID, targetParentID, TreeItemInsertMode::AtPosition, targetPosition);
        break;
    case EditorViewType::cmScriptView:
        mpHost->getScriptUnit()->reParentScript(itemID, currentParentID, targetParentID, TreeItemInsertMode::AtPosition, targetPosition);
        break;
    case EditorViewType::cmKeysView:
        mpHost->getKeyUnit()->reParentKey(itemID, currentParentID, targetParentID, TreeItemInsertMode::AtPosition, targetPosition);
        break;
    case EditorViewType::cmActionView:
        mpHost->getActionUnit()->reParentAction(itemID, currentParentID, targetParentID, TreeItemInsertMode::AtPosition, targetPosition);
        break;
    default:
        break;
    }
}

QList<int> EditorMoveItemCommand::affectedItemIDs() const
{
    return {mItemID};
}

void EditorMoveItemCommand::remapItemID(int oldID, int newID)
{
    if (mItemID == oldID) {
        mItemID = newID;
    }
    if (mOldParentID == oldID) {
        mOldParentID = newID;
    }
    if (mNewParentID == oldID) {
        mNewParentID = newID;
    }
}

QString EditorMoveItemCommand::generateText(EditorViewType viewType, const QString& itemName)
{
    switch (viewType) {
    case EditorViewType::cmTriggerView:
        //: Undo/redo menu text for moving a trigger
        return QObject::tr("move trigger \"%1\"").arg(itemName);
    case EditorViewType::cmAliasView:
        //: Undo/redo menu text for moving an alias
        return QObject::tr("move alias \"%1\"").arg(itemName);
    case EditorViewType::cmTimerView:
        //: Undo/redo menu text for moving a timer
        return QObject::tr("move timer \"%1\"").arg(itemName);
    case EditorViewType::cmScriptView:
        //: Undo/redo menu text for moving a script
        return QObject::tr("move script \"%1\"").arg(itemName);
    case EditorViewType::cmKeysView:
        //: Undo/redo menu text for moving a key binding
        return QObject::tr("move key \"%1\"").arg(itemName);
    case EditorViewType::cmActionView:
        //: Undo/redo menu text for moving a button
        return QObject::tr("move button \"%1\"").arg(itemName);
    default:
        //: Undo/redo menu text for moving an unknown item
        return QObject::tr("move item \"%1\"").arg(itemName);
    }
}
