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

#include "EditorMoveItemCommand.h"

#include "Host.h"
#include "TAction.h"
#include "TAlias.h"
#include "TKey.h"
#include "TScript.h"
#include "TTimer.h"
#include "TTrigger.h"
#include "utils.h"

EditorMoveItemCommand::EditorMoveItemCommand(EditorViewType viewType, int itemID, int oldParentID, int newParentID, int oldPosition, int newPosition, const QString& itemName, Host* host)
: EditorCommand(generateText(viewType, itemName), host)
, mViewType(viewType)
, mItemID(itemID)
, mOldParentID(oldParentID)
, mNewParentID(newParentID)
, mOldPosition(oldPosition)
, mNewPosition(newPosition)
, mItemName(itemName)
{
}

void EditorMoveItemCommand::undo()
{
#if defined(DEBUG_UNDO_REDO)
    qDebug() << "EditorMoveItemCommand::undo() - Moving" << mItemName << "(ID:" << mItemID << ")" << "from parent" << mNewParentID << "to parent" << mOldParentID << "at position" << mOldPosition;
#endif
    moveItem(mNewParentID, mOldParentID, mOldPosition);
}

void EditorMoveItemCommand::redo()
{
#if defined(DEBUG_UNDO_REDO)
    qDebug() << "EditorMoveItemCommand::redo() - Moving" << mItemName << "(ID:" << mItemID << ")" << "from parent" << mOldParentID << "to parent" << mNewParentID << "at position" << mNewPosition;
#endif
    // Skip the first redo() which is automatically called by QUndoStack::push()
    // The move has already been performed by TTreeWidget::rowsInserted()
    if (mSkipFirstRedo) {
#if defined(DEBUG_UNDO_REDO)
        qDebug() << "EditorMoveItemCommand::redo() - Skipping first redo (move already performed)";
#endif
        mSkipFirstRedo = false;
        return;
    }

    moveItem(mOldParentID, mNewParentID, mNewPosition);
}

// Updates stored IDs when items are deleted and recreated (e.g., during undo/redo)
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

void EditorMoveItemCommand::moveItem(int fromParentID, int toParentID, int position)
{
    switch (mViewType) {
    case EditorViewType::cmTriggerView: {
        mpHost->getTriggerUnit()->reParentTrigger(mItemID, fromParentID, toParentID, TreeItemInsertMode::AtPosition, position);
        break;
    }
    case EditorViewType::cmAliasView: {
        mpHost->getAliasUnit()->reParentAlias(mItemID, fromParentID, toParentID, TreeItemInsertMode::AtPosition, position);
        break;
    }
    case EditorViewType::cmTimerView: {
        mpHost->getTimerUnit()->reParentTimer(mItemID, fromParentID, toParentID, TreeItemInsertMode::AtPosition, position);
        break;
    }
    case EditorViewType::cmScriptView: {
        mpHost->getScriptUnit()->reParentScript(mItemID, fromParentID, toParentID, TreeItemInsertMode::AtPosition, position);
        break;
    }
    case EditorViewType::cmKeysView: {
        mpHost->getKeyUnit()->reParentKey(mItemID, fromParentID, toParentID, TreeItemInsertMode::AtPosition, position);
        break;
    }
    case EditorViewType::cmActionView: {
        mpHost->getActionUnit()->reParentAction(mItemID, fromParentID, toParentID, TreeItemInsertMode::AtPosition, position);
        break;
    }
    default:
        qWarning() << "EditorMoveItemCommand::moveItem() - Unknown view type";
        break;
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
